#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <sys/system_properties.h>

namespace tebox {

constexpr int64_t kPrimaryDisplay = 0;
constexpr int32_t kConfigId = 0;
constexpr int32_t kWidth = 1080;
constexpr int32_t kHeight = 2400;
constexpr int32_t kDpiX = 420000;
constexpr int32_t kDpiY = 420000;

// Display refresh rate from androidboot.tebox.refresh_hz (default 60, 30..240).
// The vsync rate caps every app's frame rate, so boot-qemu.sh raises it where
// the host GPU has headroom.
inline int32_t vsyncPeriodNs() {
    char value[PROP_VALUE_MAX] = {};
    int hz = __system_property_get("ro.boot.tebox.refresh_hz", value) > 0 ? atoi(value) : 0;
    if (hz < 30 || hz > 240) hz = 60;
    return static_cast<int32_t>(1000000000LL / hz);
}

}  // namespace tebox
