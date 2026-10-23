// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "buffer_format.h"
#include <sys/system_properties.h>
#include <string.h>

inline int teboxRgbRowAlignment() {
    char value[PROP_VALUE_MAX] = {};
    __system_property_get("vendor.tebox.rgb_row_alignment", value);
    return strcmp(value, "64") == 0 ? 64 : 1;
}

// Marks a GBM RGB resource whose texture width is the recorded pixel stride.
// Handle width remains the logical image extent. Do not infer this contract
// for arbitrary imported buffers just because their rows contain padding.
constexpr int kTeBoxPaddedRgb = 0x54425231;
inline int teboxRgbStorageWidth(int format, int width, int rowAlignment) {
    const int bytes = bufferFormat(format).bytes;
    if (!bytes || format == 0x21 || width <= 0 || width > 16384)
        return 0;
    // Host-specific alignment is opt-in; ordinary GBM determines its own pitch.
    const uint64_t alignment = rowAlignment == 64 ? (bytes == 3 ? 192 : 64) : bytes;
    const uint64_t row = uint64_t(width) * bytes;
    const uint64_t pixels = ((row + alignment - 1) / alignment) * alignment / bytes;
    return pixels <= 16384 ? int(pixels) : 0;
}
inline bool teboxPaddedRgbValid(int format, int width, int stride) {
    return bufferFormat(format).bytes > 0 && width > 0 &&
           stride > width && stride <= 16384;
}
