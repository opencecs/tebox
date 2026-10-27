/*
 * HIDL graphics mapper 4.0, passthrough. Same handle layout and metadata
 * encoding as the AIDL AIMAPPER5 stub, so libui's Gralloc4 client sees the
 * buffers the allocator hands out.
 *
 * The passthrough loader dlopens this library from /vendor/lib64/hw and calls
 * HIDL_FETCH_IMapper().
 */

#define LOG_TAG "tebox-mapper"

#include <android/hardware/graphics/mapper/4.0/IMapper.h>
#include <cutils/native_handle.h>

#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <linux/dma-buf.h>

#include <string>
#include <vector>

#include <log/log.h>

#include "buffer_format.h"
#include "buffer_handle.h"
#include "gpu_buffer.h"

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V4_0 {
namespace implementation {

using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

namespace {

enum {
    META_BUFFER_ID = 1,
    META_NAME = 2,
    META_WIDTH = 3,
    META_HEIGHT = 4,
    META_LAYER_COUNT = 5,
    META_PIXEL_FORMAT_REQUESTED = 6,
    META_PIXEL_FORMAT_FOURCC = 7,
    META_PIXEL_FORMAT_MODIFIER = 8,
    META_USAGE = 9,
    META_ALLOCATION_SIZE = 10,
    META_PROTECTED_CONTENT = 11,
    META_COMPRESSION = 12,
    META_INTERLACED = 13,
    META_CHROMA_SITING = 14,
    META_PLANE_LAYOUTS = 15,
    META_CROP = 16,
    META_DATASPACE = 17,
    META_BLEND_MODE = 18,
    META_STRIDE = 23,
};

const char kStdMetaName[] = "android.hardware.graphics.common.StandardMetadataType";

struct BufSlot {
    const native_handle_t* handle = nullptr;
    int32_t width = 0, height = 0, stride = 0, format = 0;
    uint64_t usage = 0, id = 0, size = 0;
    void* mapped = nullptr;
    int lock_count = 0;
    int used = 0;
    // VirGL/GBM BOs are often not CPU-mappable; lock() may use anonymous pages.
    int mapped_anonymous = 0;
    // 0 when the dma-buf must not be mmap'd (fstat failed, or st_size is a
    // positive value smaller than the logical image).
    int cpu_mappable = 0;
    // gfxstream: a virtio-gpu resource whose pixels live in the host
    // ColorBuffer; CPU locks transfer them in and (for writers) back out.
    int gfxstream = 0;
    int lock_write = 0;
};

constexpr int kMaxBuffers = 512;
BufSlot g_bufs[kMaxBuffers];
pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;

BufSlot* find_buf(const void* buffer) {
    for (int i = 0; i < kMaxBuffers; i++)
        if (g_bufs[i].used && g_bufs[i].handle == buffer) return &g_bufs[i];
    return nullptr;
}

BufSlot* alloc_slot() {
    for (int i = 0; i < kMaxBuffers; i++)
        if (!g_bufs[i].used) {
            g_bufs[i] = BufSlot{};
            g_bufs[i].used = 1;
            return &g_bufs[i];
        }
    return nullptr;
}

bool parse_handle(const native_handle_t* h, BufSlot* info) {
    struct TeboxBufferInfo tb;
    if (teboxBufferInfo(h, &tb) && tb.gfxstream) {
        int bpp = bufferFormat(tb.format).bytes;
        if (!bpp || tb.width <= 0 || tb.height <= 0 || tb.stride < tb.width) return false;
        info->width = tb.width;
        info->height = tb.height;
        info->stride = tb.stride;
        info->format = tb.format;
        info->usage = tb.usage;
        info->size = uint64_t(tb.stride) * uint64_t(tb.height) * uint64_t(bpp);
        info->cpu_mappable = 1;
        info->gfxstream = 1;
        struct stat st;
        info->id = fstat(h->data[0], &st) == 0 ? uint64_t(st.st_ino) : 0;
        return true;
    }
    info->gfxstream = 0;
    if (!h || h->version != int(sizeof(native_handle_t)) || h->numFds < 1 || h->numInts < 6)
        return false;
    const int* ints = &h->data[h->numFds];
    info->width = ints[0];
    info->height = ints[1];
    info->stride = ints[2];
    info->format = ints[3];
    info->usage = (uint64_t(uint32_t(ints[5])) << 32) | uint32_t(ints[4]);
    int bpp = bufferFormat(info->format).bytes;
    if (!bpp || info->width <= 0 || info->height <= 0 || info->stride < info->width) return false;
    info->size = uint64_t(info->stride) * uint64_t(info->height) * uint64_t(bpp);
    info->cpu_mappable = 0;
    struct stat st;
    if (fstat(h->data[0], &st) != 0) {
        // Still import: ImageReader must be able to lock. CPU reads stay anonymous.
        ALOGW("import fstat failed w=%d h=%d stride=%d format=%d errno=%d", info->width,
              info->height, info->stride, info->format, errno);
        info->id = 0;
        return true;
    }
    // dma-buf inode identity remains stable across processes and re-imports.
    info->id = uint64_t(st.st_ino);
    // st_size == 0 is normal for virtio-gpu. A smaller positive size is not safe to mmap.
    if (st.st_size == 0 || uint64_t(st.st_size) >= info->size) {
        info->cpu_mappable = 1;
    } else {
        ALOGW("import undersized dma-buf w=%d h=%d stride=%d format=%d logical=%llu st_size=%lld",
              info->width, info->height, info->stride, info->format,
              (unsigned long long)info->size, (long long)st.st_size);
    }
    return true;
}

void log_import_reject(const native_handle_t* handle) {
    int numFds = handle ? handle->numFds : -1;
    int numInts = handle ? handle->numInts : -1;
    int w = 0, h = 0, stride = 0, format = 0;
    long long st_size = -1;
    int err = 0;
    if (handle && numFds >= 1 && numInts >= 4) {
        const int* ints = &handle->data[numFds];
        w = ints[0];
        h = ints[1];
        stride = ints[2];
        format = ints[3];
        struct stat st;
        if (fstat(handle->data[0], &st) == 0)
            st_size = (long long)st.st_size;
        else
            err = errno;
    }
    ALOGE("importBuffer reject fds=%d ints=%d w=%d h=%d stride=%d format=%d st_size=%lld errno=%d",
          numFds, numInts, w, h, stride, format, st_size, err);
}

// Virtio-GPU sync fds often poll as POLLERR/POLLNVAL immediately. A failed wait
// must not fail the lock: ImageReader turns that into an uncaught exception
// that kills system_server, and init then SIGKILLs zygote.
bool fence_is_signaled(int fd) {
    if (fd < 0) return true;
    struct pollfd fence = {fd, POLLIN, 0};
    int ret;
    int err = 0;
    do {
        ret = poll(&fence, 1, 150);
        err = errno;
    } while (ret < 0 && err == EINTR);
    int revents = fence.revents;
    close(fd);
    if (ret > 0 && (revents & POLLIN) && !(revents & (POLLERR | POLLNVAL | POLLHUP))) return true;
    ALOGW("acquire fence not signaled ret=%d revents=0x%x errno=%d", ret, revents,
          ret < 0 ? err : 0);
    return false;
}

void* map_anonymous(uint64_t size) {
    void* p = mmap(nullptr, size_t(size), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1,
                   0);
    if (p != MAP_FAILED) memset(p, 0, size_t(size));
    return p;
}

// VirGL images need a TRANSFER_FROM_HOST operation before a CPU reader can see
// GPU pixels. mmap of their PRIME fd is neither a readback nor necessarily
// supported. Return a detached snapshot: unlock must never upload this copy
// over the GPU's scanout image.
void* read_gpu_snapshot(const BufSlot* slot, int fd) {
    // mediaswcodec's seccomp policy forbids mkdirat. Readback needs no shader
    // disk cache; disable it before loading the Mesa driver in this process.
    setenv("MESA_SHADER_CACHE_DISABLE", "true", 1);
    // Mesa 24.3's database mode cleans up old directories before checking
    // CACHE_DISABLE. Single-file mode bypasses that cleanup as well.
    setenv("MESA_DISK_CACHE_SINGLE_FILE", "true", 1);
    setenv("LIBGL_DRIVERS_PATH", "/vendor/lib64/dri", 0);
    gbm_device* device = qemuGbmDevice();
    if (!device) return MAP_FAILED;
    BufferFormat format = bufferFormat(slot->format);
    gbm_import_fd_data data = {};
    data.fd = fd;
    data.width = slot->width;
    data.height = slot->height;
    data.stride = slot->stride * format.bytes;
    data.format = format.fourcc;
    gbm_bo* bo = gbm_bo_import(device, GBM_BO_IMPORT_FD, &data, GBM_BO_USE_RENDERING);
    if (!bo) {
        ALOGW("GBM CPU readback import failed errno=%d", errno);
        return MAP_FAILED;
    }
    uint32_t stride = 0;
    void* transfer = nullptr;
    void* pixels =
            gbm_bo_map(bo, 0, 0, slot->width, slot->height, GBM_BO_TRANSFER_READ, &stride, &transfer);
    void* snapshot = MAP_FAILED;
    const size_t row = size_t(slot->width) * format.bytes;
    if (pixels && pixels != MAP_FAILED) {
        if (stride >= row) {
            snapshot = map_anonymous(slot->size);
            if (snapshot != MAP_FAILED) {
                for (int y = 0; y < slot->height; ++y)
                    memcpy((uint8_t*)snapshot + size_t(y) * data.stride,
                           (const uint8_t*)pixels + size_t(y) * stride, row);
            }
        }
        gbm_bo_unmap(bo, transfer);  // READ only: no transfer back to the host.
    } else {
        ALOGW("GBM CPU readback map failed errno=%d", errno);
    }
    gbm_bo_destroy(bo);
    return snapshot;
}

// ---- metadata encoding (AOSP gralloctypes wire format) ----

struct MetaWriter {
    std::vector<uint8_t>* out;
};

void mw_bytes(MetaWriter* w, const void* src, size_t n) {
    if (n == 0) return;
    const auto* p = static_cast<const uint8_t*>(src);
    w->out->insert(w->out->end(), p, p + n);
}
void mw_i64(MetaWriter* w, int64_t v) { mw_bytes(w, &v, sizeof(v)); }
void mw_u64(MetaWriter* w, uint64_t v) { mw_bytes(w, &v, sizeof(v)); }
void mw_u32(MetaWriter* w, uint32_t v) { mw_bytes(w, &v, sizeof(v)); }
void mw_i32(MetaWriter* w, int32_t v) { mw_bytes(w, &v, sizeof(v)); }

void mw_header(MetaWriter* w, int64_t type) {
    int64_t nlen = int64_t(strlen(kStdMetaName));
    mw_i64(w, nlen);
    mw_bytes(w, kStdMetaName, size_t(nlen));
    mw_i64(w, type);
}

void mw_str(MetaWriter* w, const char* s) {
    int64_t n = int64_t(strlen(s));
    mw_i64(w, n);
    mw_bytes(w, s, size_t(n));
}

void encode_u64(std::vector<uint8_t>* out, int64_t type, uint64_t v) {
    MetaWriter w{out};
    mw_header(&w, type);
    mw_u64(&w, v);
}

void encode_i32(std::vector<uint8_t>* out, int64_t type, int32_t v) {
    MetaWriter w{out};
    mw_header(&w, type);
    mw_i32(&w, v);
}

void encode_str(std::vector<uint8_t>* out, int64_t type, const char* s) {
    MetaWriter w{out};
    mw_header(&w, type);
    mw_str(&w, s);
}

void encode_ext_none(std::vector<uint8_t>* out, int64_t type) {
    MetaWriter w{out};
    mw_header(&w, type);
    mw_str(&w, type == META_COMPRESSION
                       ? "android.hardware.graphics.common.Compression"
                       : type == META_INTERLACED
                                 ? "android.hardware.graphics.common.Interlaced"
                                 : "android.hardware.graphics.common.ChromaSiting");
    mw_i64(&w, 0);
}

void encode_planes(std::vector<uint8_t>* out, const BufSlot& info) {
    MetaWriter w{out};
    mw_header(&w, META_PLANE_LAYOUTS);
    mw_i64(&w, 1);
    BufferFormat fmt = bufferFormat(info.format);
    mw_i64(&w, fmt.components);
    const char* kComp = "android.hardware.graphics.common.PlaneLayoutComponentType";
    const int64_t comps[] = {1LL << 10, 1LL << 11, 1LL << 12, 1LL << 30};
    for (int i = 0; i < fmt.components; i++) {
        mw_str(&w, kComp);
        mw_i64(&w, comps[i]);
        mw_i64(&w, fmt.offsets[i]);
        mw_i64(&w, fmt.bits[i]);
    }
    int64_t strideBytes = int64_t(info.stride) * fmt.bytes;
    mw_i64(&w, 0);
    mw_i64(&w, fmt.bytes * 8);
    mw_i64(&w, strideBytes);
    mw_i64(&w, info.width);
    mw_i64(&w, info.height);
    mw_i64(&w, strideBytes * info.height);
    mw_i64(&w, 1);
    mw_i64(&w, 1);
}

void encode_crop(std::vector<uint8_t>* out, const BufSlot& info) {
    MetaWriter w{out};
    mw_header(&w, META_CROP);
    mw_i64(&w, 1);
    mw_i32(&w, 0);
    mw_i32(&w, 0);
    mw_i32(&w, info.width);
    mw_i32(&w, info.height);
}

Error encodeStandard(const BufSlot& info, const char* name, int64_t type,
                     std::vector<uint8_t>* out) {
    switch (type) {
        case META_BUFFER_ID:
            encode_u64(out, type, info.id);
            return Error::NONE;
        case META_NAME:
            encode_str(out, type, name);
            return Error::NONE;
        case META_WIDTH:
            encode_u64(out, type, uint64_t(info.width));
            return Error::NONE;
        case META_HEIGHT:
            encode_u64(out, type, uint64_t(info.height));
            return Error::NONE;
        case META_LAYER_COUNT:
            encode_u64(out, type, 1);
            return Error::NONE;
        case META_PIXEL_FORMAT_REQUESTED:
            encode_i32(out, type, info.format);
            return Error::NONE;
        case META_PIXEL_FORMAT_FOURCC: {
            MetaWriter w{out};
            mw_header(&w, type);
            mw_u32(&w, bufferFormat(info.format).fourcc);
            return Error::NONE;
        }
        case META_PIXEL_FORMAT_MODIFIER:
            encode_u64(out, type, 0);
            return Error::NONE;
        case META_USAGE:
            encode_u64(out, type, info.usage);
            return Error::NONE;
        case META_ALLOCATION_SIZE:
            encode_u64(out, type, info.size);
            return Error::NONE;
        case META_PROTECTED_CONTENT:
            encode_u64(out, type, 0);
            return Error::NONE;
        case META_COMPRESSION:
        case META_INTERLACED:
        case META_CHROMA_SITING:
            encode_ext_none(out, type);
            return Error::NONE;
        case META_PLANE_LAYOUTS:
            encode_planes(out, info);
            return Error::NONE;
        case META_CROP:
            encode_crop(out, info);
            return Error::NONE;
        case META_DATASPACE:
        case META_BLEND_MODE:
            encode_i32(out, type, 0);
            return Error::NONE;
        case META_STRIDE:
            encode_i32(out, type, info.stride);
            return Error::NONE;
        default:
            return Error::UNSUPPORTED;
    }
}

bool isSupportedDescriptor(const IMapper::BufferDescriptorInfo& d) {
    const int format = static_cast<int>(d.format);
    const bool fmt_ok = bufferFormat(format).bytes != 0 || format == 0x22;
    const bool blob = format == 0x21;
    const uint32_t maxWidth = blob ? (64u << 20) : 16384u;
    return fmt_ok && d.width > 0 && d.height > 0 && d.width <= maxWidth &&
           d.height <= (blob ? 1u : 16384u) && d.layerCount == 1 && !(d.usage & (1ull << 14)) &&
           d.reservedSize == 0;
}

}  // namespace

class TeBoxMapper : public IMapper {
  public:
    Return<void> createDescriptor(const IMapper::BufferDescriptorInfo& description,
                                  createDescriptor_cb _hidl_cb) override {
        if (description.name.size() > 4096) {
            _hidl_cb(Error::BAD_VALUE, {});
            return Void();
        }
        if (!isSupportedDescriptor(description)) {
            _hidl_cb(Error::UNSUPPORTED, {});
            return Void();
        }
        std::vector<uint8_t> out;
        const int64_t nameLength = int64_t(description.name.size());
        const uint32_t width = description.width;
        const uint32_t height = description.height;
        const uint32_t layerCount = description.layerCount;
        const int32_t format = static_cast<int32_t>(description.format);
        const uint64_t usage = description.usage;
        const uint64_t reservedSize = description.reservedSize;
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&nameLength),
                   reinterpret_cast<const uint8_t*>(&nameLength) + sizeof(nameLength));
        if (description.name.size() > 0) {
            const auto* name = reinterpret_cast<const uint8_t*>(description.name.c_str());
            out.insert(out.end(), name, name + description.name.size());
        }
        auto append = [&out](const void* p, size_t n) {
            out.insert(out.end(), static_cast<const uint8_t*>(p),
                       static_cast<const uint8_t*>(p) + n);
        };
        append(&width, sizeof(width));
        append(&height, sizeof(height));
        append(&layerCount, sizeof(layerCount));
        append(&format, sizeof(format));
        append(&usage, sizeof(usage));
        append(&reservedSize, sizeof(reservedSize));

        hidl_vec<uint8_t> descriptor;
        descriptor.resize(out.size());
        memcpy(descriptor.data(), out.data(), out.size());
        _hidl_cb(Error::NONE, descriptor);
        return Void();
    }

    Return<void> importBuffer(const hidl_handle& rawHandle, importBuffer_cb _hidl_cb) override {
        const native_handle_t* handle = rawHandle.getNativeHandle();
        BufSlot tmp;
        if (!parse_handle(handle, &tmp)) {
            log_import_reject(handle);
            _hidl_cb(Error::BAD_BUFFER, nullptr);
            return Void();
        }
        native_handle_t* clone = native_handle_clone(handle);
        if (!clone) {
            _hidl_cb(Error::NO_RESOURCES, nullptr);
            return Void();
        }
        pthread_mutex_lock(&g_mu);
        BufSlot* slot = alloc_slot();
        if (!slot) {
            pthread_mutex_unlock(&g_mu);
            native_handle_close(clone);
            native_handle_delete(clone);
            _hidl_cb(Error::NO_RESOURCES, nullptr);
            return Void();
        }
        *slot = tmp;
        slot->handle = clone;
        slot->used = 1;
        pthread_mutex_unlock(&g_mu);
        _hidl_cb(Error::NONE, clone);
        return Void();
    }

    Return<Error> freeBuffer(void* buffer) override {
        pthread_mutex_lock(&g_mu);
        BufSlot* slot = find_buf(buffer);
        if (!slot) {
            pthread_mutex_unlock(&g_mu);
            return Error::BAD_BUFFER;
        }
        if (slot->mapped) munmap(slot->mapped, size_t(slot->size));
        slot->used = 0;
        pthread_mutex_unlock(&g_mu);
        native_handle_close(static_cast<native_handle_t*>(buffer));
        native_handle_delete(static_cast<native_handle_t*>(buffer));
        return Error::NONE;
    }

    Return<Error> validateBufferSize(void* buffer, const IMapper::BufferDescriptorInfo& description,
                                     uint32_t stride) override {
        pthread_mutex_lock(&g_mu);
        BufSlot* slot = find_buf(buffer);
        if (!slot) {
            pthread_mutex_unlock(&g_mu);
            return Error::BAD_BUFFER;
        }
        const int requestedFormat = static_cast<int>(description.format);
        // The allocator substitutes IMPLEMENTATION_DEFINED with RGBA_8888 but
        // the producer keeps the requested format in its GraphicBuffer.
        const bool formatOk = requestedFormat == slot->format ||
                              (requestedFormat == 0x22 && slot->format == 1);
        // libui clears GPU_DATA_BUFFER/SENSOR_DIRECT_DATA before allocation.
        const uint64_t usage = description.usage & ~((1ull << 10) | (1ull << 13));
        const bool ok = formatOk && description.width == uint32_t(slot->width) &&
                        description.height == uint32_t(slot->height) &&
                        description.layerCount == 1 && stride == uint32_t(slot->stride) &&
                        usage == slot->usage;
        pthread_mutex_unlock(&g_mu);
        return ok ? Error::NONE : Error::BAD_VALUE;
    }

    Return<void> getTransportSize(void* buffer, getTransportSize_cb _hidl_cb) override {
        pthread_mutex_lock(&g_mu);
        BufSlot* slot = find_buf(buffer);
        if (!slot) {
            pthread_mutex_unlock(&g_mu);
            _hidl_cb(Error::BAD_BUFFER, 0, 0);
            return Void();
        }
        auto* handle = static_cast<const native_handle_t*>(buffer);
        _hidl_cb(Error::NONE, uint32_t(handle->numFds), uint32_t(handle->numInts));
        pthread_mutex_unlock(&g_mu);
        return Void();
    }

    Return<void> lock(void* buffer, uint64_t cpuUsage, const IMapper::Rect& accessRegion,
                      const hidl_handle& acquireFence, lock_cb _hidl_cb) override {
        (void)accessRegion;
        // libui owns and closes the acquire fence after this call; wait on a
        // private duplicate so the two closes cannot collide. A failed dup
        // leaves the fence unwaitable, which must count as unsignaled.
        bool signaled = true;
        const native_handle_t* fenceHandle = acquireFence.getNativeHandle();
        if (fenceHandle && fenceHandle->numFds >= 1) {
            int fenceFd = dup(fenceHandle->data[0]);
            signaled = fenceFd >= 0 && fence_is_signaled(fenceFd);
        }

        pthread_mutex_lock(&g_mu);
        BufSlot* slot = find_buf(buffer);
        if (!slot) {
            pthread_mutex_unlock(&g_mu);
            _hidl_cb(Error::BAD_BUFFER, nullptr);
            return Void();
        }
        auto* handle = static_cast<native_handle_t*>(buffer);
        if (slot->gfxstream) {
            const uint32_t pitch = uint32_t(slot->stride) * uint32_t(bufferFormat(slot->format).bytes);
            // Pull the host ColorBuffer into the guest backing before the CPU
            // reads it (writers get the current pixels too: partial updates).
            if (signaled && !virtgpuTransfer(handle->data[0], false, uint32_t(slot->width),
                                             uint32_t(slot->height), pitch))
                ALOGW("gfxstream transfer from host failed errno=%d", errno);
            if (!slot->mapped) {
                void* p = mmap(nullptr, size_t(slot->size), PROT_READ | PROT_WRITE, MAP_SHARED,
                               handle->data[0], 0);
                if (p == MAP_FAILED) {
                    int err = errno;
                    pthread_mutex_unlock(&g_mu);
                    ALOGE("gfxstream mmap failed size=%llu errno=%d",
                          (unsigned long long)slot->size, err);
                    _hidl_cb(Error::NO_RESOURCES, nullptr);
                    return Void();
                }
                slot->mapped = p;
                slot->mapped_anonymous = 0;
            }
            if (cpuUsage & 0xf0) slot->lock_write = 1;
            slot->lock_count++;
            void* data = slot->mapped;
            pthread_mutex_unlock(&g_mu);
            _hidl_cb(Error::NONE, data);
            return Void();
        }
        if (!slot->mapped) {
            void* p = MAP_FAILED;
            int anonymous = 0;
            // Android CPU_READ_* is in bits 0..3, CPU_WRITE_* in bits 4..7.
            if (signaled && (cpuUsage & 0xf) && !(cpuUsage & 0xf0)) {
                p = read_gpu_snapshot(slot, handle->data[0]);
                if (p != MAP_FAILED) anonymous = 1;
            }
            // Only touch the GBM dma-buf when the GPU is done and the fd covers
            // the image. Otherwise serve zeroed pages so a CPU unlock cannot
            // clobber scanout.
            if (p == MAP_FAILED && signaled && slot->cpu_mappable) {
                p = mmap(nullptr, size_t(slot->size), PROT_READ | PROT_WRITE, MAP_SHARED,
                         handle->data[0], 0);
            }
            if (p == MAP_FAILED) {
                p = map_anonymous(slot->size);
                if (p == MAP_FAILED) {
                    int err = errno;
                    pthread_mutex_unlock(&g_mu);
                    ALOGE("mmap failed size=%llu errno=%d", (unsigned long long)slot->size, err);
                    _hidl_cb(Error::NO_RESOURCES, nullptr);
                    return Void();
                }
                anonymous = 1;
                ALOGI("CPU-lock fallback to anonymous mapping size=%llu signaled=%d mappable=%d",
                      (unsigned long long)slot->size, signaled, slot->cpu_mappable);
            }
            slot->mapped = p;
            slot->mapped_anonymous = anonymous;
        }
        slot->lock_count++;
        if (!slot->mapped_anonymous) {
            struct dma_buf_sync sync = {DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW};
            ioctl(handle->data[0], DMA_BUF_IOCTL_SYNC, &sync);
        }
        void* data = slot->mapped;
        pthread_mutex_unlock(&g_mu);
        _hidl_cb(Error::NONE, data);
        return Void();
    }

    Return<void> unlock(void* buffer, unlock_cb _hidl_cb) override {
        pthread_mutex_lock(&g_mu);
        BufSlot* slot = find_buf(buffer);
        if (!slot) {
            pthread_mutex_unlock(&g_mu);
            _hidl_cb(Error::BAD_BUFFER, hidl_handle());
            return Void();
        }
        auto* handle = static_cast<native_handle_t*>(buffer);
        if (slot->lock_count > 0) slot->lock_count--;
        if (slot->gfxstream) {
            // Upload CPU writes to the host ColorBuffer once the last lock ends.
            if (slot->lock_count == 0 && slot->lock_write) {
                const uint32_t pitch =
                        uint32_t(slot->stride) * uint32_t(bufferFormat(slot->format).bytes);
                if (!virtgpuTransfer(handle->data[0], true, uint32_t(slot->width),
                                     uint32_t(slot->height), pitch))
                    ALOGW("gfxstream transfer to host failed errno=%d", errno);
                slot->lock_write = 0;
            }
            pthread_mutex_unlock(&g_mu);
            _hidl_cb(Error::NONE, hidl_handle());
            return Void();
        }
        if (!slot->mapped_anonymous) {
            struct dma_buf_sync sync = {DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW};
            ioctl(handle->data[0], DMA_BUF_IOCTL_SYNC, &sync);
        } else if (slot->lock_count == 0 && slot->mapped) {
            // Drop the blank mapping so a later lock can retry a real CPU map.
            munmap(slot->mapped, size_t(slot->size));
            slot->mapped = nullptr;
            slot->mapped_anonymous = 0;
        }
        pthread_mutex_unlock(&g_mu);
        _hidl_cb(Error::NONE, hidl_handle());
        return Void();
    }

    Return<void> flushLockedBuffer(void* buffer, flushLockedBuffer_cb _hidl_cb) override {
        pthread_mutex_lock(&g_mu);
        bool ok = find_buf(buffer) != nullptr;
        pthread_mutex_unlock(&g_mu);
        _hidl_cb(ok ? Error::NONE : Error::BAD_BUFFER, hidl_handle());
        return Void();
    }

    Return<Error> rereadLockedBuffer(void* buffer) override {
        pthread_mutex_lock(&g_mu);
        bool ok = find_buf(buffer) != nullptr;
        pthread_mutex_unlock(&g_mu);
        return ok ? Error::NONE : Error::BAD_BUFFER;
    }

    Return<void> isSupported(const IMapper::BufferDescriptorInfo& description,
                             isSupported_cb _hidl_cb) override {
        _hidl_cb(Error::NONE, isSupportedDescriptor(description));
        return Void();
    }

    Return<void> get(void* buffer, const IMapper::MetadataType& metadataType,
                     get_cb _hidl_cb) override {
        if (metadataType.name.size() == 0 ||
            strcmp(metadataType.name.c_str(), kStdMetaName) != 0) {
            _hidl_cb(Error::UNSUPPORTED, {});
            return Void();
        }
        BufSlot info;
        {
            pthread_mutex_lock(&g_mu);
            BufSlot* slot = find_buf(buffer);
            if (!slot) {
                pthread_mutex_unlock(&g_mu);
                _hidl_cb(Error::BAD_BUFFER, {});
                return Void();
            }
            info = *slot;
            pthread_mutex_unlock(&g_mu);
        }
        std::vector<uint8_t> out;
        Error error = encodeStandard(info, "graphics-stub", metadataType.value, &out);
        if (error != Error::NONE) {
            _hidl_cb(error, {});
            return Void();
        }
        hidl_vec<uint8_t> vec;
        vec.resize(out.size());
        memcpy(vec.data(), out.data(), out.size());
        _hidl_cb(Error::NONE, vec);
        return Void();
    }

    Return<Error> set(void* buffer, const IMapper::MetadataType& metadataType,
                      const hidl_vec<uint8_t>& metadata) override {
        (void)buffer;
        (void)metadataType;
        (void)metadata;
        return Error::UNSUPPORTED;
    }

    Return<void> getFromBufferDescriptorInfo(const IMapper::BufferDescriptorInfo& description,
                                             const IMapper::MetadataType& metadataType,
                                             getFromBufferDescriptorInfo_cb _hidl_cb) override {
        if (metadataType.name.size() == 0 ||
            strcmp(metadataType.name.c_str(), kStdMetaName) != 0) {
            _hidl_cb(Error::UNSUPPORTED, {});
            return Void();
        }
        BufSlot info;
        info.width = int32_t(description.width);
        info.height = int32_t(description.height);
        info.stride = int32_t(description.width);
        info.format = static_cast<int32_t>(description.format);
        // Report what an allocation would actually produce.
        if (info.format == 0x22) info.format = 1;
        info.usage = description.usage;
        const int bpp = bufferFormat(info.format).bytes;
        info.size = uint64_t(info.stride) * uint64_t(info.height) * uint64_t(bpp);
        if (info.format == 35) info.size = info.size * 3 / 2;
        std::vector<uint8_t> out;
        Error error = encodeStandard(info, description.name.c_str(), metadataType.value, &out);
        if (error != Error::NONE) {
            _hidl_cb(error, {});
            return Void();
        }
        hidl_vec<uint8_t> vec;
        vec.resize(out.size());
        memcpy(vec.data(), out.data(), out.size());
        _hidl_cb(Error::NONE, vec);
        return Void();
    }

    Return<void> listSupportedMetadataTypes(
            listSupportedMetadataTypes_cb _hidl_cb) override {
        static const int64_t kTypes[] = {
                META_WIDTH, META_HEIGHT, META_STRIDE, META_USAGE, META_BUFFER_ID,
                META_PIXEL_FORMAT_REQUESTED, META_LAYER_COUNT, META_PLANE_LAYOUTS,
                META_ALLOCATION_SIZE,
        };
        hidl_vec<IMapper::MetadataTypeDescription> descriptions;
        descriptions.resize(sizeof(kTypes) / sizeof(kTypes[0]));
        for (size_t i = 0; i < descriptions.size(); ++i) {
            descriptions[i].metadataType.name = kStdMetaName;
            descriptions[i].metadataType.value = kTypes[i];
            descriptions[i].description = "";
            descriptions[i].isGettable = true;
            descriptions[i].isSettable = false;
        }
        _hidl_cb(Error::NONE, descriptions);
        return Void();
    }

    Return<void> dumpBuffer(void* buffer, dumpBuffer_cb _hidl_cb) override {
        pthread_mutex_lock(&g_mu);
        bool ok = find_buf(buffer) != nullptr;
        pthread_mutex_unlock(&g_mu);
        if (!ok) {
            _hidl_cb(Error::BAD_BUFFER, IMapper::BufferDump{});
            return Void();
        }
        _hidl_cb(Error::NONE, IMapper::BufferDump{});
        return Void();
    }

    Return<void> dumpBuffers(dumpBuffers_cb _hidl_cb) override {
        _hidl_cb(Error::NONE, {});
        return Void();
    }

    Return<void> getReservedRegion(void* buffer, getReservedRegion_cb _hidl_cb) override {
        pthread_mutex_lock(&g_mu);
        bool ok = find_buf(buffer) != nullptr;
        pthread_mutex_unlock(&g_mu);
        if (!ok) {
            _hidl_cb(Error::BAD_BUFFER, nullptr, 0);
            return Void();
        }
        _hidl_cb(Error::NONE, nullptr, 0);
        return Void();
    }
};

}  // namespace implementation
}  // namespace V4_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android

extern "C" android::hardware::graphics::mapper::V4_0::IMapper* HIDL_FETCH_IMapper(
        const char* /* name */) {
    return new android::hardware::graphics::mapper::V4_0::implementation::TeBoxMapper();
}
