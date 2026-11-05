/*
 * HIDL graphics allocator 4.0 on the same allocation backends as the AIDL
 * stub: virtio-gpu resources for gfxstream, Mesa GBM BOs for VirGL, DRM dumb
 * buffers, and ashmem as the last resort.
 *
 * The descriptor wire format is AOSP gralloctypes':
 *   int64 nameLength, char name[nameLength],
 *   uint32 width, uint32 height, uint32 layerCount, int32 format,
 *   uint64 usage, uint64 reservedSize
 */

#define LOG_TAG "tebox-allocator"

#include <android/hardware/graphics/allocator/4.0/IAllocator.h>
#include <android/hardware/graphics/mapper/4.0/IMapper.h>
#include <cutils/native_handle.h>

#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/ashmem.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

#include <log/log.h>

#include "buffer_format.h"
#include "buffer_handle.h"
#include "gpu_buffer.h"
#include "rgb_storage_layout.h"

namespace android {
namespace hardware {
namespace graphics {
namespace allocator {
namespace V4_0 {

namespace {

using mapper::V4_0::Error;

constexpr int32_t kImplementationDefined = 0x22;
constexpr int32_t kRgba8888 = 1;
constexpr int32_t kYcbcr420 = 35;
constexpr int32_t kBlob = 0x21;
constexpr uint32_t kMaxBuffers = 1024;

struct DecodedDescriptor {
    std::string name;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t layerCount = 0;
    int32_t format = 0;
    uint64_t usage = 0;
    uint64_t reservedSize = 0;
};

bool decodeDescriptor(const hidl_vec<uint8_t>& bytes, DecodedDescriptor* out) {
    size_t off = 0;
    auto read = [&](void* dst, size_t n) {
        if (n > bytes.size() - off) return false;
        memcpy(dst, bytes.data() + off, n);
        off += n;
        return true;
    };
    int64_t nameLength = 0;
    if (!read(&nameLength, sizeof(nameLength))) return false;
    if (nameLength < 0 || uint64_t(nameLength) > bytes.size() - off) return false;
    out->name.assign(reinterpret_cast<const char*>(bytes.data() + off), size_t(nameLength));
    off += size_t(nameLength);
    return read(&out->width, sizeof(out->width)) && read(&out->height, sizeof(out->height)) &&
           read(&out->layerCount, sizeof(out->layerCount)) &&
           read(&out->format, sizeof(out->format)) && read(&out->usage, sizeof(out->usage)) &&
           read(&out->reservedSize, sizeof(out->reservedSize));
}

bool isSupportedDescriptor(const DecodedDescriptor& d) {
    const bool fmt_ok = bufferFormat(d.format).bytes != 0 || d.format == kImplementationDefined;
    // BLOB buffers are width bytes x 1 row (camera JPEG: several MB).
    const bool blob = d.format == kBlob;
    const uint32_t maxWidth = blob ? (64u << 20) : 16384u;
    return fmt_ok && d.width > 0 && d.height > 0 && d.width <= maxWidth &&
           d.height <= (blob ? 1u : 16384u) && d.layerCount == 1 &&
           !(d.usage & (1ull << 14)) && d.reservedSize == 0;
}

int ashmemCreate(size_t size) {
    int fd = open("/dev/ashmem", O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        // Android 12 kernels usually have no /dev/ashmem; a scratch file on
        // tmpfs backs the same mmap contract for the fallback path.
        char name[] = "/dev/ashmem-stub-XXXXXX";
        fd = mkstemp(name);
        if (fd < 0) return -1;
        unlink(name);
        if (ftruncate(fd, static_cast<off_t>(size)) != 0) {
            close(fd);
            return -1;
        }
        return fd;
    }
    char name[ASHMEM_NAME_LEN] = "graphics-stub";
    ioctl(fd, ASHMEM_SET_NAME, name);
    if (ioctl(fd, ASHMEM_SET_SIZE, size) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

// Prefer virtio-gpu dumb buffers (dma-buf) so Mesa/EGL can import as textures.
// Falls back to -1 if DRM is unavailable (caller uses ashmem).
int drmDumbCreate(uint32_t width, uint32_t height, uint32_t bpp_bits, uint32_t* out_stride_px,
                  uint32_t* out_size) {
    static int drm_fd = -2;  // -2 = uninit, -1 = failed
    if (drm_fd == -2) {
        drm_fd = -1;
        for (const char* path : {"/dev/dri/card0", "/dev/dri/renderD128"}) {
            int fd = open(path, O_RDWR | O_CLOEXEC);
            if (fd < 0) continue;
            drm_mode_create_dumb probe = {};
            probe.width = 64;
            probe.height = 64;
            probe.bpp = 32;
            if (ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &probe) != 0) {
                ALOGI("DRM %s: CREATE_DUMB unsupported (%d)", path, errno);
                close(fd);
                continue;
            }
            drm_mode_destroy_dumb destroy = {};
            destroy.handle = probe.handle;
            ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
            drm_fd = fd;
            ALOGI("DRM dumb buffers via %s", path);
            break;
        }
        if (drm_fd < 0) ALOGI("DRM dumb buffers unavailable; using ashmem");
    }
    if (drm_fd < 0) return -1;

    drm_mode_create_dumb creq = {};
    // virtio-gpu accepts 32-bit dumb buffers; allocate enough linear storage
    // for the requested format, including FP16, with an integral pixel stride.
    const uint32_t bytes = bpp_bits / 8;
    const uint32_t alignment = bytes == 8 ? 8 : 12;
    creq.width = ((width * bytes + alignment - 1) / alignment) * alignment / 4;
    creq.height = height;
    creq.bpp = 32;
    if (ioctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq) != 0) {
        ALOGE("CREATE_DUMB %ux%u bpp=%u failed errno=%d", width, height, bpp_bits, errno);
        return -1;
    }

    drm_prime_handle prime = {};
    prime.handle = creq.handle;
    prime.flags = DRM_CLOEXEC | DRM_RDWR;
    if (ioctl(drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime) != 0) {
        ALOGE("PRIME_HANDLE_TO_FD failed errno=%d", errno);
        drm_mode_destroy_dumb destroy = {};
        destroy.handle = creq.handle;
        ioctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
        return -1;
    }

    // Drop GEM handle; dma-buf fd keeps the backing store alive.
    drm_mode_destroy_dumb destroy = {};
    destroy.handle = creq.handle;
    ioctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);

    uint32_t bpp_bytes = bpp_bits / 8;
    if (bpp_bytes == 0) bpp_bytes = 4;
    *out_stride_px = creq.pitch / bpp_bytes;
    *out_size = static_cast<uint32_t>(creq.size);
    return prime.fd;
}

// Preserve the operational allocator's typed GBM/VirGL RGB path. Mesa's VirGL
// GBM export is read-only; CPU writers get a writable descriptor to the same
// typed BO, never anonymous storage.
int gbmCreate(uint32_t width, uint32_t height, int format, bool cpuWrite, uint32_t* stride,
              uint32_t* size) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    pthread_once(&qemu_gbm_once, qemuInitGbm);
    const auto info = bufferFormat(format);
    if (!qemu_gbm_device || !info.bytes || format == kBlob) return -1;
    const int storageWidth = teboxRgbStorageWidth(format, width, teboxRgbRowAlignment());
    if (!storageWidth) return -1;
    gbm_bo* bo = gbm_bo_create(qemu_gbm_device, storageWidth, height, info.fourcc,
                               GBM_BO_USE_RENDERING | GBM_BO_USE_LINEAR);
    if (!bo) return -1;
    uint32_t pitch = gbm_bo_get_stride(bo);
    int fd = -1;
    if (cpuWrite) {
        drm_prime_handle prime{};
        prime.handle = gbm_bo_get_handle(bo).u32;
        prime.flags = DRM_CLOEXEC | DRM_RDWR;
        prime.fd = -1;
        // Export writable on the FIRST export: DMA-BUF caches its file and
        // later exports cannot upgrade a previously read-only file's mode.
        if (!ioctl(gbm_device_get_fd(qemu_gbm_device), DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime))
            fd = prime.fd;
    } else {
        fd = gbm_bo_get_fd(bo);
    }
    uint64_t length = uint64_t(pitch) * height;
    if (fd >= 0 && pitch >= uint32_t(storageWidth) * info.bytes && pitch % info.bytes == 0 &&
        pitch / info.bytes <= 16384 && length <= UINT32_MAX &&
        (teboxRgbRowAlignment() != 64 || pitch == uint32_t(storageWidth) * info.bytes)) {
        *stride = pitch / info.bytes;
        *size = uint32_t(length);
    } else {
        if (fd >= 0) close(fd);
        fd = -1;
    }
    gbm_bo_destroy(bo);
    return fd;
}

// Wraps |fd| in a native_handle_t whose ints carry the tebox layout. On
// success the handle owns the fd; on failure the fd stays with the caller.
native_handle_t* makeTeBoxHandle(int fd, const DecodedDescriptor& d, int32_t stride, size_t size,
                                 bool gfx, bool virgl, int bpp) {
    if (gfx) {
        // minigbm layout: the gfxstream guest Vulkan driver reads it.
        native_handle_t* h = native_handle_create(1, TEBOX_CROS_NUM_INTS);
        if (!h) return nullptr;
        h->data[0] = fd;
        auto* cros = reinterpret_cast<tebox_cros_handle*>(h);
        for (int p = 1; p < TEBOX_CROS_MAX_FDS; ++p) cros->fds[p] = -1;
        cros->strides[0] = static_cast<uint32_t>(stride * bpp);
        cros->sizes[0] = static_cast<uint32_t>(size);
        cros->width = d.width;
        cros->height = d.height;
        cros->format = bufferFormat(d.format).fourcc;
        cros->use_flags = d.usage;
        cros->magic = TEBOX_CROS_MAGIC;
        cros->pixel_stride = static_cast<uint32_t>(stride);
        cros->droid_format = d.format;
        cros->usage = static_cast<int32_t>(d.usage);
        cros->num_planes = 1;
        cros->total_size = size;
        return h;
    }
    const bool padded = virgl && teboxRgbRowAlignment() == 64 && stride > int32_t(d.width);
    native_handle_t* h = native_handle_create(1, padded ? 7 : 6);
    if (!h) return nullptr;
    h->data[0] = fd;
    int* ints = &h->data[1];
    // ints: width, height, stride, format, usage_lo, usage_hi (gralloc-ish)
    ints[0] = int32_t(d.width);
    ints[1] = int32_t(d.height);
    ints[2] = stride;
    ints[3] = d.format;
    ints[4] = int32_t(d.usage & 0xffffffff);
    ints[5] = int32_t((d.usage >> 32) & 0xffffffff);
    if (padded) ints[6] = kTeBoxPaddedRgb;
    return h;
}

class TeBoxAllocator : public IAllocator {
  public:
    Return<void> allocate(const hidl_vec<uint8_t>& descriptor, uint32_t count,
                          allocate_cb _hidl_cb) override {
        DecodedDescriptor d;
        if (!decodeDescriptor(descriptor, &d)) {
            _hidl_cb(Error::BAD_DESCRIPTOR, 0, {});
            return Void();
        }
        if (count == 0 || count > kMaxBuffers) {
            _hidl_cb(Error::BAD_VALUE, 0, {});
            return Void();
        }
        if (d.format == kImplementationDefined) d.format = kRgba8888;
        if (!isSupportedDescriptor(d)) {
            _hidl_cb(Error::UNSUPPORTED, 0, {});
            return Void();
        }

        const int bpp = bufferFormat(d.format).bytes;
        int32_t stride = int32_t(d.width);  // pixels (logical)
        size_t size = size_t(stride) * size_t(d.height) * size_t(bpp);
        if (d.format == kYcbcr420) size = size * 3 / 2;
        if (d.layerCount > 1) size *= d.layerCount;

        const bool cpuOnly = d.format == kYcbcr420 || d.format == kBlob;
        const bool gfx = qemuGpuIsGfxstream() && !cpuOnly && virglFormat(d.format);
        const bool virgl = !gfx && qemuGbmUsesVirgl() && !cpuOnly;

        std::vector<native_handle_t*> raw;
        raw.reserve(count);
        hidl_vec<hidl_handle> handles(count);
        for (uint32_t i = 0; i < count; ++i) {
            int fd = -1;
            uint32_t drm_stride = 0, drm_size = 0;
            if (gfx) {
                fd = virtgpuCreate(d.width, d.height, d.format, bpp, &drm_stride, &drm_size);
                if (fd < 0) {
                    freeHandles(raw);
                    _hidl_cb(Error::NO_RESOURCES, 0, {});
                    return Void();
                }
                drm_stride /= bpp;  // pixels, like the other backends
            } else if (virgl) {
                // CPU writers: CPU_WRITE_* (0xf0) or a camera HAL (CAMERA_OUTPUT).
                fd = gbmCreate(d.width, d.height, d.format,
                               (d.usage & (0xf0 | (1ull << 17))) != 0, &drm_stride, &drm_size);
                if (fd < 0) {
                    freeHandles(raw);
                    _hidl_cb(Error::NO_RESOURCES, 0, {});
                    return Void();
                }
            } else if (!cpuOnly) {
                fd = drmDumbCreate(d.width, d.height, uint32_t(bpp * 8), &drm_stride, &drm_size);
            }
            if (fd >= 0) {
                stride = int32_t(drm_stride);
                size = drm_size;
                if (d.layerCount > 1) size *= d.layerCount;
            } else {
                fd = ashmemCreate(size);
                if (fd < 0) {
                    ALOGE("ashmemCreate failed size=%zu", size);
                    freeHandles(raw);
                    _hidl_cb(Error::NO_RESOURCES, 0, {});
                    return Void();
                }
            }
            native_handle_t* h = makeTeBoxHandle(fd, d, stride, size, gfx, virgl, bpp);
            if (!h) {
                close(fd);
                freeHandles(raw);
                _hidl_cb(Error::NO_RESOURCES, 0, {});
                return Void();
            }
            raw.push_back(h);
            handles[i].setTo(h, false);
        }

        ALOGI("allocate %s %ux%u fmt=%d count=%u stride=%d size=%zu via %s", d.name.c_str(),
              d.width, d.height, d.format, count, stride, size,
              gfx ? "virtgpu-gfxstream" : virgl ? "gbm-virgl" : cpuOnly ? "ashmem" : "drm-dumb");
        _hidl_cb(Error::NONE, uint32_t(stride), handles);
        // The callback duplicated every fd into the reply; the handles stay
        // owned here, exactly like the AOSP default allocator.
        freeHandles(raw);
        return Void();
    }

  private:
    static void freeHandles(std::vector<native_handle_t*>& handles) {
        for (native_handle_t* h : handles) {
            native_handle_close(h);
            native_handle_delete(h);
        }
        handles.clear();
    }
};

}  // namespace

sp<IAllocator> createAllocator() {
    return new TeBoxAllocator();
}

}  // namespace V4_0
}  // namespace allocator
}  // namespace graphics
}  // namespace hardware
}  // namespace android
