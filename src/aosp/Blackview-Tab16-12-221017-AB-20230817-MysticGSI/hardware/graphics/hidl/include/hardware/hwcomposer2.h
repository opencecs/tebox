#pragma once

// Minimal stand-in for the platform <hardware/hwcomposer2.h>. The AOSP
// composer HAL utility headers only need the capability enum and the two
// rectangle types; the full HWC2 device API is not used by this HAL.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum hwc2_capability_t {
    HWC2_CAPABILITY_INVALID = 0,
    HWC2_CAPABILITY_SIDEBAND_STREAM = 1,
    HWC2_CAPABILITY_SKIP_CLIENT_COLOR_TRANSFORM = 2,
    HWC2_CAPABILITY_SKIP_VALIDATE = 3,
    HWC2_CAPABILITY_PRESENT_FENCE_IS_NOT_RELIABLE = 4,
} hwc2_capability_t;

typedef struct hwc_rect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} hwc_rect_t;

typedef struct hwc_frect {
    float left;
    float top;
    float right;
    float bottom;
} hwc_frect_t;

#ifdef __cplusplus
}
#endif
