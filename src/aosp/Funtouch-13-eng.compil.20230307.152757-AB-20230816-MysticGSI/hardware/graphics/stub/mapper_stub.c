/*
 * Soft AIMAPPER5 mapper (C) for GKI+QEMU bring-up.
 * Handle layout: fds[0]=ashmem, ints={w,h,stride,format,usage_lo,usage_hi}.
 */

#include <android/hardware/graphics/mapper/IMapper.h>
#include <android/log.h>
#include <android/rect.h>
#include <cutils/native_handle.h>

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <poll.h>
#include <unistd.h>

#define LOG_TAG "mapper-stub"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

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

typedef struct {
    const native_handle_t* handle;
    int32_t width, height, stride, format;
    uint64_t usage, id, size;
    void* mapped;
    int lock_count;
    int used;
    int mapped_anonymous;
    int cpu_mappable;
} BufSlot;

#define MAX_BUFS 256
static BufSlot g_bufs[MAX_BUFS];
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static uint64_t g_next_id = 1;

typedef struct {
    uint8_t* dest;
    size_t cap;
    int32_t wanted;
} MetaWriter;

static void mw_bytes(MetaWriter* w, const void* src, size_t n) {
    if (w->wanted < 0) return;
    if (__builtin_add_overflow(w->wanted, (int32_t)n, &w->wanted)) {
        w->wanted = -AIMAPPER_ERROR_BAD_VALUE;
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
    int64_t nlen = (int64_t)strlen(kStdMetaName);
    mw_i64(w, nlen);
    mw_bytes(w, kStdMetaName, (size_t)nlen);
    mw_i64(w, type);
}

static void mw_str(MetaWriter* w, const char* s) {
    int64_t n = (int64_t)strlen(s);
    mw_i64(w, n);
    mw_bytes(w, s, (size_t)n);
}

static int parse_handle(const native_handle_t* h, BufSlot* info) {
    if (!h || h->version != (int)sizeof(native_handle_t) || h->numFds < 1 || h->numInts < 6)
        return 0;
    const int* ints = &h->data[h->numFds];
    info->width = ints[0];
    info->height = ints[1];
    info->stride = ints[2];
    info->format = ints[3];
    info->usage = ((uint64_t)(uint32_t)ints[5] << 32) | (uint32_t)ints[4];
    if (info->width <= 0 || info->height <= 0 || info->stride < info->width) return 0;
    // YUV_420_888 is backed by a tightly packed Y + interleaved UV buffer.
    // Other formats use the packed RGBA-sized mapper storage used by this
    // bring-up implementation.
    info->size = (uint64_t)info->stride * (uint64_t)info->height;
    if (info->format == 35) info->size = info->size * 3 / 2;
    else if (info->format != 0x21) info->size *= 4;  // BLOB is one byte per "pixel"
    info->cpu_mappable = 0;
    struct stat st;
    if (fstat(h->data[0], &st) != 0) {
        ALOGW("import fstat failed w=%d h=%d stride=%d format=%d errno=%d", info->width,
              info->height, info->stride, info->format, errno);
        return 1;
    }
    if (st.st_size == 0 || (uint64_t)st.st_size >= info->size) info->cpu_mappable = 1;
    else
        ALOGW("import undersized dma-buf w=%d h=%d stride=%d format=%d logical=%llu st_size=%lld",
              info->width, info->height, info->stride, info->format,
              (unsigned long long)info->size, (long long)st.st_size);
    return 1;
}

static BufSlot* find_buf(const native_handle_t* h) {
    for (int i = 0; i < MAX_BUFS; i++)
        if (g_bufs[i].used && g_bufs[i].handle == h) return &g_bufs[i];
    return NULL;
}

static BufSlot* alloc_slot(void) {
    for (int i = 0; i < MAX_BUFS; i++)
        if (!g_bufs[i].used) {
            memset(&g_bufs[i], 0, sizeof(g_bufs[i]));
            g_bufs[i].used = 1;
            return &g_bufs[i];
        }
    return NULL;
}

static AIMapper_Error importBuffer(const native_handle_t* handle, buffer_handle_t* out) {
    BufSlot tmp;
    memset(&tmp, 0, sizeof(tmp));
    if (!parse_handle(handle, &tmp)) {
        ALOGE("importBuffer reject fds=%d ints=%d", handle ? handle->numFds : -1,
              handle ? handle->numInts : -1);
        return AIMAPPER_ERROR_BAD_BUFFER;
    }
    native_handle_t* clone = native_handle_clone(handle);
    if (!clone) return AIMAPPER_ERROR_NO_RESOURCES;
    pthread_mutex_lock(&g_mu);
    BufSlot* slot = alloc_slot();
    if (!slot) {
        pthread_mutex_unlock(&g_mu);
        native_handle_close(clone);
        native_handle_delete(clone);
        return AIMAPPER_ERROR_NO_RESOURCES;
    }
    *slot = tmp;
    slot->handle = clone;
    slot->id = g_next_id++;
    slot->used = 1;
    pthread_mutex_unlock(&g_mu);
    *out = clone;
    return AIMAPPER_ERROR_NONE;
}

static AIMapper_Error freeBuffer(buffer_handle_t buffer) {
    pthread_mutex_lock(&g_mu);
    BufSlot* slot = find_buf(buffer);
    if (!slot) {
        pthread_mutex_unlock(&g_mu);
        return AIMAPPER_ERROR_BAD_BUFFER;
    }
    if (slot->mapped) munmap(slot->mapped, (size_t)slot->size);
    slot->used = 0;
    pthread_mutex_unlock(&g_mu);
    native_handle_close((native_handle_t*)buffer);
    native_handle_delete((native_handle_t*)buffer);
    return AIMAPPER_ERROR_NONE;
}

static AIMapper_Error getTransportSize(buffer_handle_t buffer, uint32_t* outFds, uint32_t* outInts) {
    pthread_mutex_lock(&g_mu);
    BufSlot* slot = find_buf(buffer);
    if (!slot) {
        pthread_mutex_unlock(&g_mu);
        return AIMAPPER_ERROR_BAD_BUFFER;
    }
    *outFds = (uint32_t)buffer->numFds;
    *outInts = (uint32_t)buffer->numInts;
    pthread_mutex_unlock(&g_mu);
    return AIMAPPER_ERROR_NONE;
}

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
    close(acquireFence);
    if (ret > 0 && (revents & POLLIN) && !(revents & (POLLERR | POLLNVAL | POLLHUP))) return 1;
    ALOGW("acquire fence not signaled ret=%d revents=0x%x errno=%d", ret, revents,
          ret < 0 ? err : 0);
    return 0;
}

static AIMapper_Error lock(buffer_handle_t buffer, uint64_t cpuUsage, ARect accessRegion,
                           int acquireFence, void** outData) {
    (void)cpuUsage;
    (void)accessRegion;
    int signaled = fence_is_signaled(acquireFence);
    pthread_mutex_lock(&g_mu);
    BufSlot* slot = find_buf(buffer);
    if (!slot) {
        pthread_mutex_unlock(&g_mu);
        return AIMAPPER_ERROR_BAD_BUFFER;
    }
    if (!slot->mapped) {
        void* p = MAP_FAILED;
        int anonymous = 0;
        if (signaled && slot->cpu_mappable) {
            p = mmap(NULL, (size_t)slot->size, PROT_READ | PROT_WRITE, MAP_SHARED, buffer->data[0],
                     0);
        }
        if (p == MAP_FAILED) {
            p = mmap(NULL, (size_t)slot->size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
                     -1, 0);
            if (p == MAP_FAILED) {
                int err = errno;
                pthread_mutex_unlock(&g_mu);
                ALOGE("mmap failed size=%llu errno=%d", (unsigned long long)slot->size, err);
                return AIMAPPER_ERROR_NO_RESOURCES;
            }
            anonymous = 1;
            memset(p, 0, (size_t)slot->size);
            ALOGI("CPU-lock fallback to anonymous mapping size=%llu signaled=%d mappable=%d",
                  (unsigned long long)slot->size, signaled, slot->cpu_mappable);
        }
        slot->mapped = p;
        slot->mapped_anonymous = anonymous;
    }
    slot->lock_count++;
    *outData = slot->mapped;
    pthread_mutex_unlock(&g_mu);
    return AIMAPPER_ERROR_NONE;
}

static AIMapper_Error unlock(buffer_handle_t buffer, int* releaseFence) {
    pthread_mutex_lock(&g_mu);
    BufSlot* slot = find_buf(buffer);
    if (!slot) {
        pthread_mutex_unlock(&g_mu);
        return AIMAPPER_ERROR_BAD_BUFFER;
    }
    if (slot->lock_count > 0) slot->lock_count--;
    if (slot->lock_count == 0 && slot->mapped_anonymous && slot->mapped) {
        munmap(slot->mapped, (size_t)slot->size);
        slot->mapped = NULL;
        slot->mapped_anonymous = 0;
    }
    pthread_mutex_unlock(&g_mu);
    *releaseFence = -1;
    return AIMAPPER_ERROR_NONE;
}

static AIMapper_Error flushLockedBuffer(buffer_handle_t buffer) {
    pthread_mutex_lock(&g_mu);
    int ok = find_buf(buffer) != NULL;
    pthread_mutex_unlock(&g_mu);
    return ok ? AIMAPPER_ERROR_NONE : AIMAPPER_ERROR_BAD_BUFFER;
}

static AIMapper_Error rereadLockedBuffer(buffer_handle_t buffer) {
    return flushLockedBuffer(buffer);
}

static int32_t encode_u64(int64_t type, uint64_t v, void* dest, size_t cap) {
    MetaWriter w = {(uint8_t*)dest, cap, 0};
    mw_header(&w, type);
    mw_u64(&w, v);
    return w.wanted;
}

static int32_t encode_i32(int64_t type, int32_t v, void* dest, size_t cap) {
    MetaWriter w = {(uint8_t*)dest, cap, 0};
    mw_header(&w, type);
    mw_i32(&w, v);
    return w.wanted;
}

static int32_t encode_str(int64_t type, const char* s, void* dest, size_t cap) {
    MetaWriter w = {(uint8_t*)dest, cap, 0};
    mw_header(&w, type);
    mw_str(&w, s);
    return w.wanted;
}

static int32_t encode_ext_none(int64_t type, void* dest, size_t cap) {
    MetaWriter w = {(uint8_t*)dest, cap, 0};
    mw_header(&w, type);
    mw_str(&w, "android.hardware.graphics.common.Compression");
    mw_i64(&w, 0);
    return w.wanted;
}

/* One PlaneLayout: components are {type, offsetInBits, sizeInBits}. */
static void mw_plane(MetaWriter* w, const int64_t (*comps)[3], int ncomps, int64_t offset,
                     int64_t increment, int64_t stride, int64_t width, int64_t height,
                     int64_t size, int64_t hsub, int64_t vsub) {
    mw_i64(w, ncomps);
    for (int i = 0; i < ncomps; i++) {
        mw_str(w, "android.hardware.graphics.common.PlaneLayoutComponentType");
        mw_i64(w, comps[i][0]);
        mw_i64(w, comps[i][1]);
        mw_i64(w, comps[i][2]);
    }
    mw_i64(w, offset);
    mw_i64(w, increment);
    mw_i64(w, stride);
    mw_i64(w, width);
    mw_i64(w, height);
    mw_i64(w, size);
    mw_i64(w, hsub);
    mw_i64(w, vsub);
}

static int32_t encode_planes(const BufSlot* info, void* dest, size_t cap) {
    MetaWriter w = {(uint8_t*)dest, cap, 0};
    mw_header(&w, META_PLANE_LAYOUTS);
    if (info->format == 35) {
        /* NV12: Y plane, then interleaved Cb/Cr at half resolution. */
        const int64_t y[1][3] = {{1 /* Y */, 0, 8}};
        const int64_t uv[2][3] = {{2 /* CB */, 0, 8}, {4 /* CR */, 8, 8}};
        const int64_t luma = (int64_t)info->stride * info->height;
        mw_i64(&w, 2);
        mw_plane(&w, y, 1, 0, 8, info->stride, info->width, info->height, luma, 1, 1);
        mw_plane(&w, uv, 2, luma, 16, info->stride, info->width / 2, info->height / 2, luma / 2,
                 2, 2);
        return w.wanted;
    }
    if (info->format == 0x21) {
        const int64_t raw[1][3] = {{1 << 20 /* RAW */, 0, 8}};
        mw_i64(&w, 1);
        mw_plane(&w, raw, 1, 0, 8, info->stride, info->width, info->height,
                 (int64_t)info->stride * info->height, 1, 1);
        return w.wanted;
    }
    mw_i64(&w, 1);
    mw_i64(&w, 4);
    const char* kComp = "android.hardware.graphics.common.PlaneLayoutComponentType";
    int64_t comps[4][3] = {{1, 0, 8}, {2, 8, 8}, {3, 16, 8}, {6, 24, 8}};
    for (int i = 0; i < 4; i++) {
        mw_str(&w, kComp);
        mw_i64(&w, comps[i][0]);
        mw_i64(&w, comps[i][1]);
        mw_i64(&w, comps[i][2]);
    }
    int64_t strideBytes = (int64_t)info->stride * 4;
    mw_i64(&w, 0);
    mw_i64(&w, 32);
    mw_i64(&w, strideBytes);
    mw_i64(&w, info->width);
    mw_i64(&w, info->height);
    mw_i64(&w, strideBytes * info->height);
    mw_i64(&w, 1);
    mw_i64(&w, 1);
    return w.wanted;
}

static int32_t encode_crop(const BufSlot* info, void* dest, size_t cap) {
    MetaWriter w = {(uint8_t*)dest, cap, 0};
    mw_header(&w, META_CROP);
    mw_i64(&w, 1);
    mw_i32(&w, 0);
    mw_i32(&w, 0);
    mw_i32(&w, info->width);
    mw_i32(&w, info->height);
    return w.wanted;
}

static int32_t getStandardMetadata(buffer_handle_t buffer, int64_t type, void* dest,
                                   size_t destSize) {
    pthread_mutex_lock(&g_mu);
    BufSlot* slot = find_buf(buffer);
    BufSlot info;
    if (!slot) {
        pthread_mutex_unlock(&g_mu);
        return -AIMAPPER_ERROR_BAD_BUFFER;
    }
    info = *slot;
    pthread_mutex_unlock(&g_mu);

    switch (type) {
        case META_BUFFER_ID:
            return encode_u64(type, info.id, dest, destSize);
        case META_NAME:
            return encode_str(type, "graphics-stub", dest, destSize);
        case META_WIDTH:
            return encode_u64(type, (uint64_t)info.width, dest, destSize);
        case META_HEIGHT:
            return encode_u64(type, (uint64_t)info.height, dest, destSize);
        case META_LAYER_COUNT:
            return encode_u64(type, 1, dest, destSize);
        case META_PIXEL_FORMAT_REQUESTED:
            return encode_i32(type, info.format, dest, destSize);
        case META_PIXEL_FORMAT_FOURCC: {
            MetaWriter w = {(uint8_t*)dest, destSize, 0};
            mw_header(&w, type);
            mw_u32(&w, (uint32_t)('A' | ('B' << 8) | ('2' << 16) | ('4' << 24)));
            return w.wanted;
        }
        case META_PIXEL_FORMAT_MODIFIER:
            return encode_u64(type, 0, dest, destSize);
        case META_USAGE:
            return encode_u64(type, info.usage, dest, destSize);
        case META_ALLOCATION_SIZE:
            return encode_u64(type, info.size, dest, destSize);
        case META_PROTECTED_CONTENT:
            return encode_u64(type, 0, dest, destSize);
        case META_COMPRESSION:
        case META_INTERLACED:
        case META_CHROMA_SITING:
            return encode_ext_none(type, dest, destSize);
        case META_PLANE_LAYOUTS:
            return encode_planes(&info, dest, destSize);
        case META_CROP:
            return encode_crop(&info, dest, destSize);
        case META_DATASPACE:
        case META_BLEND_MODE:
            return encode_i32(type, 0, dest, destSize);
        case META_STRIDE:
            return encode_u64(type, (uint64_t)info.stride, dest, destSize);
        default:
            return -AIMAPPER_ERROR_UNSUPPORTED;
    }
}

static int32_t getMetadata(buffer_handle_t buffer, AIMapper_MetadataType metadataType, void* dest,
                           size_t destSize) {
    if (metadataType.name && strcmp(metadataType.name, kStdMetaName) == 0)
        return getStandardMetadata(buffer, metadataType.value, dest, destSize);
    return -AIMAPPER_ERROR_UNSUPPORTED;
}

static AIMapper_Error setMetadata(buffer_handle_t b, AIMapper_MetadataType t, const void* m,
                                  size_t s) {
    (void)b;
    (void)t;
    (void)m;
    (void)s;
    return AIMAPPER_ERROR_UNSUPPORTED;
}

static AIMapper_Error setStandardMetadata(buffer_handle_t b, int64_t t, const void* m, size_t s) {
    (void)b;
    (void)t;
    (void)m;
    (void)s;
    return AIMAPPER_ERROR_UNSUPPORTED;
}

static AIMapper_Error listSupportedMetadataTypes(const AIMapper_MetadataTypeDescription** outList,
                                                 size_t* outCount) {
    static const AIMapper_MetadataTypeDescription kDesc[] = {
            {{kStdMetaName, META_WIDTH}, NULL, true, false, {0}},
            {{kStdMetaName, META_HEIGHT}, NULL, true, false, {0}},
            {{kStdMetaName, META_STRIDE}, NULL, true, false, {0}},
            {{kStdMetaName, META_USAGE}, NULL, true, false, {0}},
            {{kStdMetaName, META_BUFFER_ID}, NULL, true, false, {0}},
            {{kStdMetaName, META_PIXEL_FORMAT_REQUESTED}, NULL, true, false, {0}},
            {{kStdMetaName, META_LAYER_COUNT}, NULL, true, false, {0}},
            {{kStdMetaName, META_PLANE_LAYOUTS}, NULL, true, false, {0}},
            {{kStdMetaName, META_ALLOCATION_SIZE}, NULL, true, false, {0}},
    };
    *outList = kDesc;
    *outCount = sizeof(kDesc) / sizeof(kDesc[0]);
    return AIMAPPER_ERROR_NONE;
}

static AIMapper_Error dumpBuffer(buffer_handle_t b, AIMapper_DumpBufferCallback cb, void* ctx) {
    (void)b;
    (void)cb;
    (void)ctx;
    return AIMAPPER_ERROR_NONE;
}

static AIMapper_Error dumpAllBuffers(AIMapper_BeginDumpBufferCallback b,
                                     AIMapper_DumpBufferCallback cb, void* ctx) {
    (void)b;
    (void)cb;
    (void)ctx;
    return AIMAPPER_ERROR_NONE;
}

static AIMapper_Error getReservedRegion(buffer_handle_t buffer, void** outRegion,
                                        uint64_t* outSize) {
    pthread_mutex_lock(&g_mu);
    int ok = find_buf(buffer) != NULL;
    pthread_mutex_unlock(&g_mu);
    if (!ok) return AIMAPPER_ERROR_BAD_BUFFER;
    *outRegion = NULL;
    *outSize = 0;
    return AIMAPPER_ERROR_NONE;
}

static AIMapper gMapper = {
        .version = AIMAPPER_VERSION_5,
        .v5 =
                {
                        .importBuffer = importBuffer,
                        .freeBuffer = freeBuffer,
                        .getTransportSize = getTransportSize,
                        .lock = lock,
                        .unlock = unlock,
                        .flushLockedBuffer = flushLockedBuffer,
                        .rereadLockedBuffer = rereadLockedBuffer,
                        .getMetadata = getMetadata,
                        .getStandardMetadata = getStandardMetadata,
                        .setMetadata = setMetadata,
                        .setStandardMetadata = setStandardMetadata,
                        .listSupportedMetadataTypes = listSupportedMetadataTypes,
                        .dumpBuffer = dumpBuffer,
                        .dumpAllBuffers = dumpAllBuffers,
                        .getReservedRegion = getReservedRegion,
                },
};

extern "C" {
uint32_t ANDROID_HAL_STABLEC_VERSION = AIMAPPER_VERSION_5;
int32_t ANDROID_HAL_MAPPER_VERSION = AIMAPPER_VERSION_5;

AIMapper_Error AIMapper_loadIMapper(AIMapper** outImplementation) {
    ALOGI("AIMapper_loadIMapper v5 stub");
    fprintf(stderr, "mapper-stub I: AIMapper_loadIMapper\n");
    *outImplementation = &gMapper;
    return AIMAPPER_ERROR_NONE;
}
}  // extern "C"
