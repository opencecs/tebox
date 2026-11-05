#pragma once

// Buffer handle layouts shared by the allocator, the mapper and the composer.
//
// VirGL (default layout): one fd, then ints
//   width, height, stride (pixels), PixelFormat, usage_lo, usage_hi [, flag]
//
// gfxstream: minigbm's cros_gralloc_handle, so the gfxstream guest Vulkan
// driver (vulkan.ranchu with ro.hardware.gralloc=minigbm, used by ANGLE)
// reads fds[0] (a virtio-gpu dma-buf), format (DRM fourcc) and droid_format
// straight from the handle. Layout copied from minigbm cros_gralloc_handle.h.

#include <stddef.h>
#include <stdint.h>
#include <cutils/native_handle.h>

#define TEBOX_CROS_MAX_PLANES 4
#define TEBOX_CROS_MAX_FDS (TEBOX_CROS_MAX_PLANES + 1)
#define TEBOX_CROS_MAGIC 0xABCDDCBAu

struct tebox_cros_handle {
    native_handle_t base;
    int32_t fds[TEBOX_CROS_MAX_FDS];
    uint32_t strides[TEBOX_CROS_MAX_PLANES];
    uint32_t offsets[TEBOX_CROS_MAX_PLANES];
    uint32_t sizes[TEBOX_CROS_MAX_PLANES];
    uint32_t id;
    uint32_t width;
    uint32_t height;
    uint32_t format;  // DRM fourcc
    uint32_t tiling;
    uint64_t format_modifier;
    uint64_t use_flags;  // tebox: the full 64-bit Android usage
    uint32_t magic;
    uint32_t pixel_stride;
    int32_t droid_format;
    int32_t usage;  // Android usage, low 32 bits
    uint32_t num_planes;
    uint64_t reserved_region_size;
    uint64_t total_size;
    uint32_t name_offset;
} __attribute__((packed));

// Ints after the single real fd (fds[1..4] count as ints, as in minigbm).
#define TEBOX_CROS_NUM_INTS \
    ((int)((sizeof(struct tebox_cros_handle) - sizeof(native_handle_t)) / sizeof(int)) - 1)

struct TeboxBufferInfo {
    int width;
    int height;
    int stride;  // pixels
    int format;  // Android PixelFormat
    uint64_t usage;
    int gfxstream;  // minigbm layout, a virtio-gpu resource on the gfxstream backend
};

// |ints| are the handle ints after one fd (AIDL NativeHandle.ints, or
// native_handle_t data[1..]).
static inline int teboxBufferInfoFromInts(const int* ints, size_t count, struct TeboxBufferInfo* out) {
    if (count == (size_t)TEBOX_CROS_NUM_INTS) {
        // The ints start at fds[1]; fds[0] is the real fd, not needed here.
        struct tebox_cros_handle h;
        const size_t after_fd0 = offsetof(struct tebox_cros_handle, fds) + sizeof(int32_t);
        __builtin_memcpy((char*)&h + after_fd0, ints, sizeof(h) - after_fd0);
        if (h.magic != TEBOX_CROS_MAGIC) return 0;
        out->width = (int)h.width;
        out->height = (int)h.height;
        out->stride = (int)h.pixel_stride;
        out->format = h.droid_format;
        out->usage = h.use_flags;
        out->gfxstream = 1;
        return 1;
    }
    if (count < 6) return 0;
    out->width = ints[0];
    out->height = ints[1];
    out->stride = ints[2];
    out->format = ints[3];
    out->usage = ((uint64_t)(uint32_t)ints[5] << 32) | (uint32_t)ints[4];
    out->gfxstream = 0;
    return 1;
}

static inline int teboxBufferInfo(const native_handle_t* h, struct TeboxBufferInfo* out) {
    if (!h || h->version != (int)sizeof(native_handle_t) || h->numFds != 1) return 0;
    return teboxBufferInfoFromInts(&h->data[1], (size_t)h->numInts, out);
}
