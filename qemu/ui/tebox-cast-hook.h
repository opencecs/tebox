/* Local tebox cast capture hook; not an upstream QEMU submission. */
#ifndef TEBOX_CAST_HOOK_H
#define TEBOX_CAST_HOOK_H

#include <stdbool.h>
#include <stdint.h>

/* Capture the guest scanout framebuffer (current GL context) for cast.
 * Shared by the SDL and egl-headless displays. */
void tebox_cast_capture(unsigned int framebuffer, int src_w, int src_h,
                        bool y0_top);
/*
 * CPU surfaces (2D scanouts, e.g. virtio-gpu-rutabaga/gfxstream): x8r8g8b8
 * pixels, top-down. Same rate limit, size cap and drop policy as above.
 */
void tebox_cast_capture_pixels(const void *pixels, int width, int height,
                               int stride);
void tebox_cast_shutdown(void);

/* True when TEBOX_CAST=1. */
bool tebox_cast_enabled(void);

/* Target GUI update interval in ms (e.g. 16 for ~60 FPS). 0 = leave default. */
uint64_t tebox_cast_refresh_interval_ms(void);

/* Guest display rotation in degrees (0/90/180/270), reported by the guest over
 * virtio-serial (cast.rotation next to cast.sock). Defined in egl-headless.c.
 * Never queries the guest over adb. */
int tebox_display_rotation(void);

#endif
