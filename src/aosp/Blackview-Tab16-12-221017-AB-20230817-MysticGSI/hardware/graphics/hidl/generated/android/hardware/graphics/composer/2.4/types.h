#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_TYPES_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_TYPES_H

#include <android/hardware/graphics/composer/2.1/types.h>

#include <hidl/HidlSupport.h>
#include <hidl/MQDescriptor.h>
#include <utils/NativeHandle.h>
#include <utils/misc.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_4 {

// Forward declaration for forward reference support:
enum class Error : int32_t;
struct VsyncPeriodChangeTimeline;

enum class Error : int32_t {
    NONE = 0,
    BAD_CONFIG = 1,
    BAD_DISPLAY = 2,
    BAD_LAYER = 3,
    BAD_PARAMETER = 4,
    NO_RESOURCES = 6,
    NOT_VALIDATED = 7,
    UNSUPPORTED = 8,
    /**
     * Seamless cannot be required for configurations that don't share a config group
     */
    SEAMLESS_NOT_ALLOWED = 9,
    /**
     * Seamless requirements cannot be met
     */
    SEAMLESS_NOT_POSSIBLE = 10,
};

/**
 * Timing for a vsync period change.
 */
struct VsyncPeriodChangeTimeline final {
    int64_t newVsyncAppliedTimeNanos __attribute__ ((aligned(8)));
    bool refreshRequired __attribute__ ((aligned(1)));
    int64_t refreshTimeNanos __attribute__ ((aligned(8)));
};

static_assert(offsetof(::android::hardware::graphics::composer::V2_4::VsyncPeriodChangeTimeline, newVsyncAppliedTimeNanos) == 0, "wrong offset");
static_assert(offsetof(::android::hardware::graphics::composer::V2_4::VsyncPeriodChangeTimeline, refreshRequired) == 8, "wrong offset");
static_assert(offsetof(::android::hardware::graphics::composer::V2_4::VsyncPeriodChangeTimeline, refreshTimeNanos) == 16, "wrong offset");
static_assert(sizeof(::android::hardware::graphics::composer::V2_4::VsyncPeriodChangeTimeline) == 24, "wrong size");
static_assert(__alignof(::android::hardware::graphics::composer::V2_4::VsyncPeriodChangeTimeline) == 8, "wrong alignment");

typedef uint32_t VsyncPeriodNanos;

//
// type declarations for package
//

template<typename>
static inline std::string toString(int32_t o);
static inline std::string toString(::android::hardware::graphics::composer::V2_4::Error o);

constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_4::Error lhs, const ::android::hardware::graphics::composer::V2_4::Error rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const int32_t lhs, const ::android::hardware::graphics::composer::V2_4::Error rhs) {
    return static_cast<int32_t>(lhs | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_4::Error lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | rhs);
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_4::Error lhs, const ::android::hardware::graphics::composer::V2_4::Error rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const int32_t lhs, const ::android::hardware::graphics::composer::V2_4::Error rhs) {
    return static_cast<int32_t>(lhs & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_4::Error lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & rhs);
}
constexpr int32_t &operator|=(int32_t& v, const ::android::hardware::graphics::composer::V2_4::Error e) {
    v |= static_cast<int32_t>(e);
    return v;
}
constexpr int32_t &operator&=(int32_t& v, const ::android::hardware::graphics::composer::V2_4::Error e) {
    v &= static_cast<int32_t>(e);
    return v;
}

static inline std::string toString(const ::android::hardware::graphics::composer::V2_4::VsyncPeriodChangeTimeline& o);
static inline bool operator==(const ::android::hardware::graphics::composer::V2_4::VsyncPeriodChangeTimeline& lhs, const ::android::hardware::graphics::composer::V2_4::VsyncPeriodChangeTimeline& rhs);
static inline bool operator!=(const ::android::hardware::graphics::composer::V2_4::VsyncPeriodChangeTimeline& lhs, const ::android::hardware::graphics::composer::V2_4::VsyncPeriodChangeTimeline& rhs);

//
// type header definitions for package
//

template<>
inline std::string toString<::android::hardware::graphics::composer::V2_4::Error>(int32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::graphics::composer::V2_4::Error> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::graphics::composer::V2_4::Error::NONE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_4::Error::NONE)) {
        os += (first ? "" : " | ");
        os += "NONE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_4::Error::NONE;
    }
    if ((o & ::android::hardware::graphics::composer::V2_4::Error::BAD_CONFIG) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_4::Error::BAD_CONFIG)) {
        os += (first ? "" : " | ");
        os += "BAD_CONFIG";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_4::Error::BAD_CONFIG;
    }
    if ((o & ::android::hardware::graphics::composer::V2_4::Error::BAD_DISPLAY) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_4::Error::BAD_DISPLAY)) {
        os += (first ? "" : " | ");
        os += "BAD_DISPLAY";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_4::Error::BAD_DISPLAY;
    }
    if ((o & ::android::hardware::graphics::composer::V2_4::Error::BAD_LAYER) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_4::Error::BAD_LAYER)) {
        os += (first ? "" : " | ");
        os += "BAD_LAYER";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_4::Error::BAD_LAYER;
    }
    if ((o & ::android::hardware::graphics::composer::V2_4::Error::BAD_PARAMETER) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_4::Error::BAD_PARAMETER)) {
        os += (first ? "" : " | ");
        os += "BAD_PARAMETER";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_4::Error::BAD_PARAMETER;
    }
    if ((o & ::android::hardware::graphics::composer::V2_4::Error::NO_RESOURCES) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_4::Error::NO_RESOURCES)) {
        os += (first ? "" : " | ");
        os += "NO_RESOURCES";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_4::Error::NO_RESOURCES;
    }
    if ((o & ::android::hardware::graphics::composer::V2_4::Error::NOT_VALIDATED) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_4::Error::NOT_VALIDATED)) {
        os += (first ? "" : " | ");
        os += "NOT_VALIDATED";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_4::Error::NOT_VALIDATED;
    }
    if ((o & ::android::hardware::graphics::composer::V2_4::Error::UNSUPPORTED) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_4::Error::UNSUPPORTED)) {
        os += (first ? "" : " | ");
        os += "UNSUPPORTED";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_4::Error::UNSUPPORTED;
    }
    if ((o & ::android::hardware::graphics::composer::V2_4::Error::SEAMLESS_NOT_ALLOWED) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_4::Error::SEAMLESS_NOT_ALLOWED)) {
        os += (first ? "" : " | ");
        os += "SEAMLESS_NOT_ALLOWED";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_4::Error::SEAMLESS_NOT_ALLOWED;
    }
    if ((o & ::android::hardware::graphics::composer::V2_4::Error::SEAMLESS_NOT_POSSIBLE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_4::Error::SEAMLESS_NOT_POSSIBLE)) {
        os += (first ? "" : " | ");
        os += "SEAMLESS_NOT_POSSIBLE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_4::Error::SEAMLESS_NOT_POSSIBLE;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::graphics::composer::V2_4::Error o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::graphics::composer::V2_4::Error::NONE) {
        return "NONE";
    }
    if (o == ::android::hardware::graphics::composer::V2_4::Error::BAD_CONFIG) {
        return "BAD_CONFIG";
    }
    if (o == ::android::hardware::graphics::composer::V2_4::Error::BAD_DISPLAY) {
        return "BAD_DISPLAY";
    }
    if (o == ::android::hardware::graphics::composer::V2_4::Error::BAD_LAYER) {
        return "BAD_LAYER";
    }
    if (o == ::android::hardware::graphics::composer::V2_4::Error::BAD_PARAMETER) {
        return "BAD_PARAMETER";
    }
    if (o == ::android::hardware::graphics::composer::V2_4::Error::NO_RESOURCES) {
        return "NO_RESOURCES";
    }
    if (o == ::android::hardware::graphics::composer::V2_4::Error::NOT_VALIDATED) {
        return "NOT_VALIDATED";
    }
    if (o == ::android::hardware::graphics::composer::V2_4::Error::UNSUPPORTED) {
        return "UNSUPPORTED";
    }
    if (o == ::android::hardware::graphics::composer::V2_4::Error::SEAMLESS_NOT_ALLOWED) {
        return "SEAMLESS_NOT_ALLOWED";
    }
    if (o == ::android::hardware::graphics::composer::V2_4::Error::SEAMLESS_NOT_POSSIBLE) {
        return "SEAMLESS_NOT_POSSIBLE";
    }
    std::string os;
    os += toHexString(static_cast<int32_t>(o));
    return os;
}

static inline std::string toString(const ::android::hardware::graphics::composer::V2_4::VsyncPeriodChangeTimeline& o) {
    using ::android::hardware::toString;
    std::string os;
    os += "{";
    os += ".newVsyncAppliedTimeNanos = ";
    os += ::android::hardware::toString(o.newVsyncAppliedTimeNanos);
    os += ", .refreshRequired = ";
    os += ::android::hardware::toString(o.refreshRequired);
    os += ", .refreshTimeNanos = ";
    os += ::android::hardware::toString(o.refreshTimeNanos);
    os += "}"; return os;
}

static inline bool operator==(const ::android::hardware::graphics::composer::V2_4::VsyncPeriodChangeTimeline& lhs, const ::android::hardware::graphics::composer::V2_4::VsyncPeriodChangeTimeline& rhs) {
    if (lhs.newVsyncAppliedTimeNanos != rhs.newVsyncAppliedTimeNanos) {
        return false;
    }
    if (lhs.refreshRequired != rhs.refreshRequired) {
        return false;
    }
    if (lhs.refreshTimeNanos != rhs.refreshTimeNanos) {
        return false;
    }
    return true;
}

static inline bool operator!=(const ::android::hardware::graphics::composer::V2_4::VsyncPeriodChangeTimeline& lhs, const ::android::hardware::graphics::composer::V2_4::VsyncPeriodChangeTimeline& rhs){
    return !(lhs == rhs);
}


}  // namespace V2_4
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android

//
// global type declarations for package
//

namespace android {
namespace hardware {
namespace details {
template<> constexpr std::array<::android::hardware::graphics::composer::V2_4::Error, 10> hidl_enum_values<::android::hardware::graphics::composer::V2_4::Error> = {
    ::android::hardware::graphics::composer::V2_4::Error::NONE,
    ::android::hardware::graphics::composer::V2_4::Error::BAD_CONFIG,
    ::android::hardware::graphics::composer::V2_4::Error::BAD_DISPLAY,
    ::android::hardware::graphics::composer::V2_4::Error::BAD_LAYER,
    ::android::hardware::graphics::composer::V2_4::Error::BAD_PARAMETER,
    ::android::hardware::graphics::composer::V2_4::Error::NO_RESOURCES,
    ::android::hardware::graphics::composer::V2_4::Error::NOT_VALIDATED,
    ::android::hardware::graphics::composer::V2_4::Error::UNSUPPORTED,
    ::android::hardware::graphics::composer::V2_4::Error::SEAMLESS_NOT_ALLOWED,
    ::android::hardware::graphics::composer::V2_4::Error::SEAMLESS_NOT_POSSIBLE,
};
}  // namespace details
}  // namespace hardware
}  // namespace android


#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_TYPES_H
