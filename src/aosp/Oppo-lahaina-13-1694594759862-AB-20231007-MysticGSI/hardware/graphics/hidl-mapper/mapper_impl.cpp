/*
 * HIDL IMapper@4.0 passthrough implementation for the tebox QEMU bring-up.
 *
 * Android 13 libui (Gralloc4Mapper) resolves IMapper through the passthrough
 * service manager, so this library is dlopen()ed into each client process
 * (SurfaceFlinger, system_server, apps). Buffers come from the AIDL allocator
 * in android.hardware.graphics-service and use the tebox handle layouts from
 * buffer_handle.h.
 *
 * The gralloc4 metadata stream must match frameworks/native/libs/gralloc/
 * types/Gralloc4.cpp exactly; libui decodes it with hasRemainingData()
 * validation, so every encoder here writes the full standard stream.
 */

#include <android/hardware/graphics/mapper/4.0/IMapper.h>
#include <android/log.h>
#include <cutils/native_handle.h>

#include <errno.h>
#include <linux/dma-buf.h>
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

#include "buffer_format.h"
#include "buffer_handle.h"
#include "gpu_buffer.h"

#define LOG_TAG "hidl-mapper"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V4_0 {
namespace {

using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

// StandardMetadataType values from Android 13
// hardware/interfaces/graphics/common/aidl/.../StandardMetadataType.aidl.
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

static const char kStdMetaName[] = "android.hardware.graphics.common.StandardMetadataType";

struct BufSlot {
    const native_handle_t* handle;
    int32_t width, height, stride, format;
    uint64_t usage, id, size;
    void* mapped;
    int lock_count;
    int used;
    // VirGL/GBM BOs are often not CPU-mappable; lock() may use anonymous pages.
    int mapped_anonymous;
    // 0 when the dma-buf must not be mmap'd (fstat failed, or st_size is a
    // positive value smaller than the logical image).
    int cpu_mappable;
    // gfxstream: a virtio-gpu resource whose pixels live in the host
    // ColorBuffer; CPU locks transfer them in and (for writers) back out.
    int gfxstream;
    int lock_write;
};

#define MAX_BUFS 256
static BufSlot g_bufs[MAX_BUFS];
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;

// ---- gralloc4 metadata stream writer ----

struct MetaWriter {
    uint8_t* dest;
    size_t cap;
    int64_t wanted;
};

static void mw_bytes(MetaWriter* w, const void* src, size_t n) {
    if (w->wanted < 0) return;
    if (__builtin_add_overflow(w->wanted, (int64_t)n, &w->wanted)) {
        w->wanted = -static_cast<int64_t>(Error::BAD_VALUE);
        return;
    }
    if (w->dest && n <= w->cap) {
        memcpy(w->dest, src, n);
        w->dest += n;
        w->cap -= n;
    } else {
        w->dest = NULL;
        w->cap = 0;
    }
}

static void mw_i64(MetaWriter* w, int64_t v) { mw_bytes(w, &v, sizeof(v)); }
static void mw_u64(MetaWriter* w, uint64_t v) { mw_bytes(w, &v, sizeof(v)); }
static void mw_u32(MetaWriter* w, uint32_t v) { mw_bytes(w, &v, sizeof(v)); }
static void mw_i32(MetaWriter* w, int32_t v) { mw_bytes(w, &v, sizeof(v)); }

static void mw_header(MetaWriter* w, int64_t type) {
    int64_t nlen = static_cast<int64_t>(strlen(kStdMetaName));
    mw_i64(w, nlen);
    mw_bytes(w, kStdMetaName, static_cast<size_t>(nlen));
    mw_i64(w, type);
}

static void mw_str(MetaWriter* w, const char* s) {
    int64_t n = static_cast<int64_t>(strlen(s));
    mw_i64(w, n);
    mw_bytes(w, s, static_cast<size_t>(n));
}

// Two-pass encoder: pass 1 counts with a null destination, pass 2 fills the
// hidl_vec. Returns false (and leaves |out| empty) on failure.
template <typename EncodeFn>
static bool encodeToVec(const EncodeFn& fn, hidl_vec<uint8_t>* out) {
    MetaWriter probe{NULL, 0, 0};
    fn(&probe);
    if (probe.wanted < 0) return false;
    out->resize(static_cast<size_t>(probe.wanted));
    MetaWriter w{out->data(), out->size(), 0};
    fn(&w);
    return w.wanted == probe.wanted;
}

static void encode_u64_meta(MetaWriter* w, int64_t type, uint64_t v) {
    mw_header(w, type);
    mw_u64(w, v);
}

static void encode_i32_meta(MetaWriter* w, int64_t type, int32_t v) {
    mw_header(w, type);
    mw_i32(w, v);
}

static void encode_str_meta(MetaWriter* w, int64_t type, const char* s) {
    mw_header(w, type);
    mw_str(w, s);
}

// ExtendableType encoding: type name string then int64 value. A value of 0
// with the enum's own name means "none" for these optional enums.
static void encode_ext_none(MetaWriter* w, int64_t type) {
    mw_header(w, type);
    mw_str(w, type == META_COMPRESSION ? "android.hardware.graphics.common.Compression"
            : type == META_INTERLACED  ? "android.hardware.graphics.common.Interlaced"
                                       : "android.hardware.graphics.common.ChromaSiting");
    mw_i64(w, 0);
}

static void encode_planes(MetaWriter* w, const BufSlot& info) {
    mw_header(w, META_PLANE_LAYOUTS);
    mw_i64(w, 1);  // one plane
    BufferFormat fmt = bufferFormat(info.format);
    mw_i64(w, fmt.components);
    const char* kComp = "android.hardware.graphics.common.PlaneLayoutComponentType";
    // R, G, B, A per Android 13 PlaneLayoutComponentType.aidl.
    const int64_t comps[] = {1LL << 10, 1LL << 11, 1LL << 12, 1LL << 30};
    for (int i = 0; i < fmt.components; i++) {
        mw_str(w, kComp);
        mw_i64(w, comps[i]);
        mw_i64(w, fmt.offsets[i]);
        mw_i64(w, fmt.bits[i]);
    }
    int64_t strideBytes = static_cast<int64_t>(info.stride) * fmt.bytes;
    mw_i64(w, 0);                    // offsetInBytes
    mw_i64(w, fmt.bytes * 8);        // sampleIncrementInBits
    mw_i64(w, strideBytes);          // strideInBytes
    mw_i64(w, info.width);           // widthInSamples
    mw_i64(w, info.height);          // heightInSamples
    mw_i64(w, strideBytes * info.height);  // totalSizeInBytes
    mw_i64(w, 1);                    // horizontalSubsampling
    mw_i64(w, 1);                    // verticalSubsampling
}

static void encode_crop(MetaWriter* w, const BufSlot& info) {
    mw_header(w, META_CROP);
    mw_i64(w, 1);
    mw_i32(w, 0);
    mw_i32(w, 0);
    mw_i32(w, info.width);
    mw_i32(w, info.height);
}

// ---- handle bookkeeping ----

static int parse_handle(const native_handle_t* h, BufSlot* info) {
    struct TeboxBufferInfo tb;
    if (!teboxBufferInfo(h, &tb)) return 0;
    if (tb.gfxstream) {
        int bpp = bufferFormat(tb.format).bytes;
        if (!bpp || tb.width <= 0 || tb.height <= 0 || tb.stride < tb.width) return 0;
        info->width = tb.width;
        info->height = tb.height;
        info->stride = tb.stride;
        info->format = tb.format;
        info->usage = tb.usage;
        info->size = static_cast<uint64_t>(tb.stride) * static_cast<uint64_t>(tb.height) *
                     static_cast<uint64_t>(bpp);
        info->cpu_mappable = 1;
        info->gfxstream = 1;
        struct stat st;
        info->id = fstat(h->data[0], &st) == 0 ? static_cast<uint64_t>(st.st_ino) : 0;
        return 1;
    }
    info->gfxstream = 0;
    if (h->numFds < 1 || h->numInts < 6) return 0;
    const int* ints = &h->data[h->numFds];
    info->width = ints[0];
    info->height = ints[1];
    info->stride = ints[2];
    info->format = ints[3];
    info->usage = (static_cast<uint64_t>(static_cast<uint32_t>(ints[5])) << 32) |
                  static_cast<uint32_t>(ints[4]);
    int bpp = bufferFormat(info->format).bytes;
    if (!bpp || info->width <= 0 || info->height <= 0 || info->stride < info->width) return 0;
    info->size = static_cast<uint64_t>(info->stride) * static_cast<uint64_t>(info->height) *
                 static_cast<uint64_t>(bpp);
    info->cpu_mappable = 0;
    struct stat st;
    if (fstat(h->data[0], &st) != 0) {
        // Still import: ImageReader must be able to lock. CPU reads stay anonymous.
        ALOGW("import fstat failed w=%d h=%d stride=%d format=%d errno=%d", info->width,
              info->height, info->stride, info->format, errno);
        info->id = 0;
        return 1;
    }
    info->id = static_cast<uint64_t>(st.st_ino);
    // st_size == 0 is normal for virtio-gpu. A smaller positive size is not safe to mmap.
    if (st.st_size == 0 || static_cast<uint64_t>(st.st_size) >= info->size) {
        info->cpu_mappable = 1;
    } else {
        ALOGW("import undersized dma-buf w=%d h=%d stride=%d format=%d logical=%llu st_size=%lld",
              info->width, info->height, info->stride, info->format,
              static_cast<unsigned long long>(info->size), static_cast<long long>(st.st_size));
    }
    return 1;
}

static BufSlot* find_buf(const native_handle_t* h) {
    for (int i = 0; i < MAX_BUFS; i++)
        if (g_bufs[i].used && g_bufs[i].handle == h) return &g_bufs[i];
    return NULL;
}

static BufSlot* alloc_slot() {
    for (int i = 0; i < MAX_BUFS; i++)
        if (!g_bufs[i].used) {
            memset(&g_bufs[i], 0, sizeof(g_bufs[i]));
            g_bufs[i].used = 1;
            return &g_bufs[i];
        }
    return NULL;
}

// Virtio-GPU sync fds often poll as POLLERR/POLLNVAL immediately. A failed wait
// must not fail the lock: ImageReader turns that into an uncaught exception.
// The caller (Gralloc4Mapper::lock) owns and closes the fence fd, so unlike
// the AIDL mapper this must not close it.
static int fence_is_signaled(int acquireFence) {
    if (acquireFence < 0) return 1;
    struct pollfd fence = {acquireFence, POLLIN, 0};
    int ret;
    int err = 0;
    do {
        ret = poll(&fence, 1, 150);
        err = errno;
    } while (ret < 0 && err == EINTR);
    int revents = fence.revents;
    if (ret > 0 && (revents & POLLIN) && !(revents & (POLLERR | POLLNVAL | POLLHUP))) return 1;
    ALOGW("acquire fence not signaled ret=%d revents=0x%x errno=%d", ret, revents,
          ret < 0 ? err : 0);
    return 0;
}

static void* map_anonymous(uint64_t size) {
    void* p = mmap(NULL, static_cast<size_t>(size), PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p != MAP_FAILED) memset(p, 0, static_cast<size_t>(size));
    return p;
}

// VirGL images need a TRANSFER_FROM_HOST operation before a CPU reader can
// see GPU pixels. Return a detached snapshot; unlock must never upload this
// copy over the GPU's scanout image.
static void* read_gpu_snapshot(const BufSlot* slot, int fd) {
    setenv("MESA_SHADER_CACHE_DISABLE", "true", 1);
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
    void* transfer = NULL;
    void* pixels = gbm_bo_map(bo, 0, 0, slot->width, slot->height, GBM_BO_TRANSFER_READ, &stride,
                              &transfer);
    void* snapshot = MAP_FAILED;
    const size_t row = static_cast<size_t>(slot->width) * format.bytes;
    if (pixels && pixels != MAP_FAILED) {
        if (stride >= row) {
            snapshot = map_anonymous(slot->size);
            if (snapshot != MAP_FAILED) {
                for (int y = 0; y < slot->height; ++y)
                    memcpy(static_cast<uint8_t*>(snapshot) + static_cast<size_t>(y) * data.stride,
                           static_cast<const uint8_t*>(pixels) + static_cast<size_t>(y) * stride,
                           row);
            }
        }
        gbm_bo_unmap(bo, transfer);  // READ only: no transfer back to the host.
    } else {
        ALOGW("GBM CPU readback map failed errno=%d", errno);
    }
    gbm_bo_destroy(bo);
    return snapshot;
}

// Called with g_mu held.
static Error lock_slot(BufSlot* slot, uint64_t cpuUsage, int acquireFence, void** outData) {
    int signaled = fence_is_signaled(acquireFence);
    const native_handle_t* buffer = slot->handle;
    if (slot->gfxstream) {
        const uint32_t pitch = static_cast<uint32_t>(slot->stride) *
                               static_cast<uint32_t>(bufferFormat(slot->format).bytes);
        // Pull the host ColorBuffer into the guest backing before the CPU
        // reads it (writers get the current pixels too: partial updates).
        if (signaled && !virtgpuTransfer(buffer->data[0], false, static_cast<uint32_t>(slot->width),
                                         static_cast<uint32_t>(slot->height), pitch))
            ALOGW("gfxstream transfer from host failed errno=%d", errno);
        if (!slot->mapped) {
            void* p = mmap(NULL, static_cast<size_t>(slot->size), PROT_READ | PROT_WRITE,
                           MAP_SHARED, buffer->data[0], 0);
            if (p == MAP_FAILED) {
                int err = errno;
                ALOGE("gfxstream mmap failed size=%llu errno=%d",
                      static_cast<unsigned long long>(slot->size), err);
                return Error::NO_RESOURCES;
            }
            slot->mapped = p;
            slot->mapped_anonymous = 0;
        }
        if (cpuUsage & 0xf0) slot->lock_write = 1;
        slot->lock_count++;
        *outData = slot->mapped;
        return Error::NONE;
    }
    if (!slot->mapped) {
        void* p = MAP_FAILED;
        int anonymous = 0;
        // Android CPU_READ_* is in bits 0..3, CPU_WRITE_* in bits 4..7.
        if (signaled && (cpuUsage & 0xf) && !(cpuUsage & 0xf0)) {
            p = read_gpu_snapshot(slot, buffer->data[0]);
            if (p != MAP_FAILED) anonymous = 1;
        }
        // Only touch the GBM dma-buf when the GPU is done and the fd covers the image.
        // Otherwise serve zeroed pages so a CPU unlock cannot clobber scanout.
        if (p == MAP_FAILED && signaled && slot->cpu_mappable) {
            p = mmap(NULL, static_cast<size_t>(slot->size), PROT_READ | PROT_WRITE, MAP_SHARED,
                     buffer->data[0], 0);
        }
        if (p == MAP_FAILED) {
            p = map_anonymous(slot->size);
            if (p == MAP_FAILED) {
                int err = errno;
                ALOGE("mmap failed size=%llu errno=%d",
                      static_cast<unsigned long long>(slot->size), err);
                return Error::NO_RESOURCES;
            }
            anonymous = 1;
            ALOGI("CPU-lock fallback to anonymous mapping size=%llu signaled=%d mappable=%d",
                  static_cast<unsigned long long>(slot->size), signaled, slot->cpu_mappable);
        }
        slot->mapped = p;
        slot->mapped_anonymous = anonymous;
    }
    slot->lock_count++;
    if (!slot->mapped_anonymous) {
        struct dma_buf_sync sync = {DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW};
        ioctl(buffer->data[0], DMA_BUF_IOCTL_SYNC, &sync);
    }
    *outData = slot->mapped;
    return Error::NONE;
}

// Called with g_mu held.
static Error unlock_slot(BufSlot* slot) {
    if (slot->lock_count > 0) slot->lock_count--;
    if (slot->gfxstream) {
        // Upload CPU writes to the host ColorBuffer once the last lock ends.
        if (slot->lock_count == 0 && slot->lock_write) {
            const uint32_t pitch = static_cast<uint32_t>(slot->stride) *
                                   static_cast<uint32_t>(bufferFormat(slot->format).bytes);
            if (!virtgpuTransfer(slot->handle->data[0], true,
                                 static_cast<uint32_t>(slot->width),
                                 static_cast<uint32_t>(slot->height), pitch))
                ALOGW("gfxstream transfer to host failed errno=%d", errno);
            slot->lock_write = 0;
        }
        return Error::NONE;
    }
    if (!slot->mapped_anonymous) {
        struct dma_buf_sync sync = {DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW};
        ioctl(slot->handle->data[0], DMA_BUF_IOCTL_SYNC, &sync);
    } else if (slot->lock_count == 0 && slot->mapped) {
        // Drop the blank mapping so a later lock can retry a real CPU map.
        munmap(slot->mapped, static_cast<size_t>(slot->size));
        slot->mapped = NULL;
        slot->mapped_anonymous = 0;
    }
    return Error::NONE;
}

// ---- metadata encoders ----

static int getStandardMetadata(const BufSlot& info, int64_t type, void* dest, size_t destSize) {
    MetaWriter w{static_cast<uint8_t*>(dest), destSize, 0};
    switch (type) {
        case META_BUFFER_ID:
            encode_u64_meta(&w, type, info.id);
            return static_cast<int>(w.wanted);
        case META_NAME:
            encode_str_meta(&w, type, "tebox");
            return static_cast<int>(w.wanted);
        case META_WIDTH:
            encode_u64_meta(&w, type, static_cast<uint64_t>(info.width));
            return static_cast<int>(w.wanted);
        case META_HEIGHT:
            encode_u64_meta(&w, type, static_cast<uint64_t>(info.height));
            return static_cast<int>(w.wanted);
        case META_LAYER_COUNT:
            encode_u64_meta(&w, type, 1);
            return static_cast<int>(w.wanted);
        case META_PIXEL_FORMAT_REQUESTED:
            encode_i32_meta(&w, type, info.format);
            return static_cast<int>(w.wanted);
        case META_PIXEL_FORMAT_FOURCC:
            mw_header(&w, type);
            mw_u32(&w, bufferFormat(info.format).fourcc);
            return static_cast<int>(w.wanted);
        case META_PIXEL_FORMAT_MODIFIER:
            encode_u64_meta(&w, type, 0);
            return static_cast<int>(w.wanted);
        case META_USAGE:
            encode_u64_meta(&w, type, info.usage);
            return static_cast<int>(w.wanted);
        case META_ALLOCATION_SIZE:
            encode_u64_meta(&w, type, info.size);
            return static_cast<int>(w.wanted);
        case META_PROTECTED_CONTENT:
            encode_u64_meta(&w, type, 0);
            return static_cast<int>(w.wanted);
        case META_COMPRESSION:
        case META_INTERLACED:
        case META_CHROMA_SITING:
            encode_ext_none(&w, type);
            return static_cast<int>(w.wanted);
        case META_PLANE_LAYOUTS:
            encode_planes(&w, info);
            return static_cast<int>(w.wanted);
        case META_CROP:
            encode_crop(&w, info);
            return static_cast<int>(w.wanted);
        case META_DATASPACE:
        case META_BLEND_MODE:
            encode_i32_meta(&w, type, 0);
            return static_cast<int>(w.wanted);
        case META_STRIDE:
            encode_i32_meta(&w, type, info.stride);
            return static_cast<int>(w.wanted);
        default:
            return -static_cast<int>(Error::UNSUPPORTED);
    }
}

// Metadata that can be answered from a BufferDescriptorInfo alone.
static int getDescriptorMetadata(const IMapper::BufferDescriptorInfo& d, int64_t type, void* dest,
                                 size_t destSize) {
    BufSlot fake = {};
    fake.width = static_cast<int32_t>(d.width);
    fake.height = static_cast<int32_t>(d.height);
    fake.format = static_cast<int32_t>(d.format);
    fake.usage = d.usage;
    fake.stride = fake.width;
    int bpp = bufferFormat(fake.format).bytes;
    fake.size = static_cast<uint64_t>(fake.width) * static_cast<uint64_t>(fake.height) *
                static_cast<uint64_t>(bpp);
    if (fake.format == 35) fake.size = fake.size * 3 / 2;
    fake.size *= d.layerCount ? d.layerCount : 1;

    switch (type) {
        case META_NAME: {
            MetaWriter w{static_cast<uint8_t*>(dest), destSize, 0};
            mw_header(&w, type);
            mw_str(&w, d.name.c_str());
            return static_cast<int>(w.wanted);
        }
        case META_LAYER_COUNT: {
            MetaWriter w{static_cast<uint8_t*>(dest), destSize, 0};
            encode_u64_meta(&w, type, d.layerCount);
            return static_cast<int>(w.wanted);
        }
        default:
            return getStandardMetadata(fake, type, dest, destSize);
    }
}

static Error metadataError(int64_t wanted) {
    if (wanted >= 0) return Error::NONE;
    int64_t e = -wanted;
    if (e == static_cast<int64_t>(Error::UNSUPPORTED)) return Error::UNSUPPORTED;
    if (e == static_cast<int64_t>(Error::BAD_VALUE)) return Error::BAD_VALUE;
    if (e == static_cast<int64_t>(Error::BAD_BUFFER)) return Error::BAD_BUFFER;
    return Error::BAD_VALUE;
}

class MapperImpl : public IMapper {
  public:
    Return<void> createDescriptor(const BufferDescriptorInfo& description,
                                  createDescriptor_cb _hidl_cb) override {
        MetaWriter w{NULL, 0, 0};
        const char* name = description.name.c_str();
        size_t nlen = strlen(name);
        if (nlen > 127) nlen = 127;  // AIDL BufferDescriptorInfo.name is byte[128]
        mw_i64(&w, static_cast<int64_t>(nlen));
        mw_bytes(&w, name, nlen);
        mw_u32(&w, description.width);
        mw_u32(&w, description.height);
        mw_u32(&w, description.layerCount);
        mw_i32(&w, static_cast<int32_t>(description.format));
        mw_u64(&w, description.usage);
        mw_u64(&w, description.reservedSize);
        if (w.wanted < 0) {
            _hidl_cb(Error::BAD_VALUE, hidl_vec<uint8_t>());
            return Void();
        }
        hidl_vec<uint8_t> out(static_cast<size_t>(w.wanted));
        MetaWriter fill{out.data(), out.size(), 0};
        mw_i64(&fill, static_cast<int64_t>(nlen));
        mw_bytes(&fill, name, nlen);
        mw_u32(&fill, description.width);
        mw_u32(&fill, description.height);
        mw_u32(&fill, description.layerCount);
        mw_i32(&fill, static_cast<int32_t>(description.format));
        mw_u64(&fill, description.usage);
        mw_u64(&fill, description.reservedSize);
        _hidl_cb(Error::NONE, out);
        return Void();
    }

    Return<void> importBuffer(const hidl_handle& rawHandle, importBuffer_cb _hidl_cb) override {
        const native_handle_t* handle = rawHandle.getNativeHandle();
        BufSlot tmp;
        memset(&tmp, 0, sizeof(tmp));
        if (!handle || !parse_handle(handle, &tmp)) {
            ALOGE("importBuffer reject fds=%d ints=%d", handle ? handle->numFds : -1,
                  handle ? handle->numInts : -1);
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
        _hidl_cb(Error::NONE, static_cast<void*>(clone));
        return Void();
    }

    Return<Error> freeBuffer(void* buffer) override {
        native_handle_t* handle = static_cast<native_handle_t*>(buffer);
        pthread_mutex_lock(&g_mu);
        BufSlot* slot = find_buf(handle);
        if (!slot) {
            pthread_mutex_unlock(&g_mu);
            return Error::BAD_BUFFER;
        }
        if (slot->mapped) munmap(slot->mapped, static_cast<size_t>(slot->size));
        slot->used = 0;
        pthread_mutex_unlock(&g_mu);
        native_handle_close(handle);
        native_handle_delete(handle);
        return Error::NONE;
    }

    Return<Error> validateBufferSize(void* buffer, const BufferDescriptorInfo& description,
                                     uint32_t stride) override {
        pthread_mutex_lock(&g_mu);
        BufSlot* slot = find_buf(static_cast<native_handle_t*>(buffer));
        if (!slot) {
            pthread_mutex_unlock(&g_mu);
            return Error::BAD_BUFFER;
        }
        int32_t wantFormat = static_cast<int32_t>(description.format);
        bool formatOk = slot->format == wantFormat ||
                        (wantFormat == 34 /* IMPLEMENTATION_DEFINED */ && slot->format == 1);
        bool ok = formatOk && static_cast<int32_t>(description.width) == slot->width &&
                  static_cast<int32_t>(description.height) == slot->height &&
                  static_cast<int32_t>(description.layerCount) == 1 &&
                  (stride == 0 || static_cast<int32_t>(stride) == slot->stride);
        if (!ok) {
            ALOGW("validateBufferSize mismatch desc=%ux%u fmt=%d stride=%u handle=%dx%d fmt=%d "
                  "stride=%d",
                  description.width, description.height, wantFormat, stride, slot->width,
                  slot->height, slot->format, slot->stride);
        }
        pthread_mutex_unlock(&g_mu);
        return ok ? Error::NONE : Error::BAD_VALUE;
    }

    Return<void> getTransportSize(void* buffer, getTransportSize_cb _hidl_cb) override {
        native_handle_t* handle = static_cast<native_handle_t*>(buffer);
        pthread_mutex_lock(&g_mu);
        BufSlot* slot = find_buf(handle);
        if (!slot) {
            pthread_mutex_unlock(&g_mu);
            _hidl_cb(Error::BAD_BUFFER, 0, 0);
            return Void();
        }
        _hidl_cb(Error::NONE, static_cast<uint32_t>(handle->numFds),
                 static_cast<uint32_t>(handle->numInts));
        pthread_mutex_unlock(&g_mu);
        return Void();
    }

    Return<void> lock(void* buffer, uint64_t cpuUsage, const Rect& accessRegion,
                      const hidl_handle& acquireFence, lock_cb _hidl_cb) override {
        (void)accessRegion;
        const native_handle_t* fence = acquireFence.getNativeHandle();
        int fenceFd = (fence && fence->numFds >= 1) ? fence->data[0] : -1;
        pthread_mutex_lock(&g_mu);
        BufSlot* slot = find_buf(static_cast<native_handle_t*>(buffer));
        if (!slot) {
            pthread_mutex_unlock(&g_mu);
            _hidl_cb(Error::BAD_BUFFER, nullptr);
            return Void();
        }
        void* data = nullptr;
        Error err = lock_slot(slot, cpuUsage, fenceFd, &data);
        pthread_mutex_unlock(&g_mu);
        _hidl_cb(err, err == Error::NONE ? data : nullptr);
        return Void();
    }

    Return<void> unlock(void* buffer, unlock_cb _hidl_cb) override {
        pthread_mutex_lock(&g_mu);
        BufSlot* slot = find_buf(static_cast<native_handle_t*>(buffer));
        if (!slot) {
            pthread_mutex_unlock(&g_mu);
            _hidl_cb(Error::BAD_BUFFER, hidl_handle());
            return Void();
        }
        Error err = unlock_slot(slot);
        pthread_mutex_unlock(&g_mu);
        // No release fence: the buffer is ready for the GPU immediately.
        _hidl_cb(err, hidl_handle());
        return Void();
    }

    Return<void> flushLockedBuffer(void* buffer, flushLockedBuffer_cb _hidl_cb) override {
        pthread_mutex_lock(&g_mu);
        bool ok = find_buf(static_cast<native_handle_t*>(buffer)) != NULL;
        pthread_mutex_unlock(&g_mu);
        _hidl_cb(ok ? Error::NONE : Error::BAD_BUFFER, hidl_handle());
        return Void();
    }

    Return<Error> rereadLockedBuffer(void* buffer) override {
        pthread_mutex_lock(&g_mu);
        bool ok = find_buf(static_cast<native_handle_t*>(buffer)) != NULL;
        pthread_mutex_unlock(&g_mu);
        return ok ? Error::NONE : Error::BAD_BUFFER;
    }

    Return<void> isSupported(const BufferDescriptorInfo& description,
                             isSupported_cb _hidl_cb) override {
        int fmt = static_cast<int32_t>(description.format);
        bool fmtOk = bufferFormat(fmt).bytes != 0 || fmt == 34 /* IMPLEMENTATION_DEFINED */;
        // BLOB buffers are width bytes x 1 row (camera JPEG).
        bool blob = fmt == 0x21;
        uint32_t maxWidth = blob ? (64u << 20) : 16384u;
        bool supported = fmtOk && description.width > 0 && description.height > 0 &&
                         description.width <= maxWidth &&
                         description.height <= (blob ? 1u : 16384u) &&
                         description.layerCount == 1 && !(description.usage & (1ULL << 14));
        _hidl_cb(Error::NONE, supported);
        return Void();
    }

    Return<void> get(void* buffer, const MetadataType& metadataType, get_cb _hidl_cb) override {
        if (!metadataType.name.c_str() ||
            strcmp(metadataType.name.c_str(), kStdMetaName) != 0) {
            _hidl_cb(Error::UNSUPPORTED, hidl_vec<uint8_t>());
            return Void();
        }
        pthread_mutex_lock(&g_mu);
        BufSlot* slot = find_buf(static_cast<native_handle_t*>(buffer));
        BufSlot info;
        if (!slot) {
            pthread_mutex_unlock(&g_mu);
            _hidl_cb(Error::BAD_BUFFER, hidl_vec<uint8_t>());
            return Void();
        }
        info = *slot;
        pthread_mutex_unlock(&g_mu);

        int64_t wanted = getStandardMetadata(info, metadataType.value, nullptr, 0);
        Error err = metadataError(wanted);
        if (err != Error::NONE) {
            _hidl_cb(err, hidl_vec<uint8_t>());
            return Void();
        }
        hidl_vec<uint8_t> out(static_cast<size_t>(wanted));
        getStandardMetadata(info, metadataType.value, out.data(), out.size());
        _hidl_cb(Error::NONE, out);
        return Void();
    }

    Return<Error> set(void* buffer, const MetadataType& metadataType,
                      const hidl_vec<uint8_t>& metadata) override {
        (void)buffer;
        (void)metadataType;
        (void)metadata;
        return Error::UNSUPPORTED;
    }

    Return<void> getFromBufferDescriptorInfo(const BufferDescriptorInfo& description,
                                             const MetadataType& metadataType,
                                             getFromBufferDescriptorInfo_cb _hidl_cb) override {
        if (!metadataType.name.c_str() ||
            strcmp(metadataType.name.c_str(), kStdMetaName) != 0) {
            _hidl_cb(Error::UNSUPPORTED, hidl_vec<uint8_t>());
            return Void();
        }
        int64_t wanted = getDescriptorMetadata(description, metadataType.value, nullptr, 0);
        Error err = metadataError(wanted);
        if (err != Error::NONE) {
            _hidl_cb(err, hidl_vec<uint8_t>());
            return Void();
        }
        hidl_vec<uint8_t> out(static_cast<size_t>(wanted));
        getDescriptorMetadata(description, metadataType.value, out.data(), out.size());
        _hidl_cb(Error::NONE, out);
        return Void();
    }

    Return<void> listSupportedMetadataTypes(
            listSupportedMetadataTypes_cb _hidl_cb) override {
        static const int64_t kTypes[] = {
                META_BUFFER_ID,    META_NAME,
                META_WIDTH,        META_HEIGHT,
                META_LAYER_COUNT,  META_PIXEL_FORMAT_REQUESTED,
                META_PIXEL_FORMAT_FOURCC, META_PIXEL_FORMAT_MODIFIER,
                META_USAGE,        META_ALLOCATION_SIZE,
                META_PROTECTED_CONTENT, META_COMPRESSION,
                META_INTERLACED,   META_CHROMA_SITING,
                META_PLANE_LAYOUTS, META_CROP,
                META_DATASPACE,    META_BLEND_MODE,
        };
        hidl_vec<MetadataTypeDescription> out;
        out.resize(sizeof(kTypes) / sizeof(kTypes[0]));
        for (size_t i = 0; i < out.size(); i++) {
            out[i].metadataType.name = kStdMetaName;
            out[i].metadataType.value = kTypes[i];
            out[i].description = "";
            out[i].isGettable = true;
            out[i].isSettable = false;
        }
        _hidl_cb(Error::NONE, out);
        return Void();
    }

    Return<void> dumpBuffer(void* buffer, dumpBuffer_cb _hidl_cb) override {
        (void)buffer;
        _hidl_cb(Error::NONE, BufferDump{});
        return Void();
    }

    Return<void> dumpBuffers(dumpBuffers_cb _hidl_cb) override {
        _hidl_cb(Error::NONE, hidl_vec<BufferDump>());
        return Void();
    }

    Return<void> getReservedRegion(void* buffer, getReservedRegion_cb _hidl_cb) override {
        pthread_mutex_lock(&g_mu);
        bool ok = find_buf(static_cast<native_handle_t*>(buffer)) != NULL;
        pthread_mutex_unlock(&g_mu);
        if (!ok) {
            _hidl_cb(Error::BAD_BUFFER, nullptr, 0);
            return Void();
        }
        _hidl_cb(Error::NONE, nullptr, 0);
        return Void();
    }
};

}  // namespace

}  // namespace V4_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android

using android::hardware::graphics::mapper::V4_0::IMapper;

extern "C" IMapper* HIDL_FETCH_IMapper(const char* /*name*/) {
    return new android::hardware::graphics::mapper::V4_0::MapperImpl();
}
