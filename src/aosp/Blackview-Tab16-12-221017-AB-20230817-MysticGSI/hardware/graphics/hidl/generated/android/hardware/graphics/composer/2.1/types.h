#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_TYPES_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_TYPES_H

#include <hidl/HidlSupport.h>
#include <hidl/MQDescriptor.h>
#include <utils/NativeHandle.h>
#include <utils/misc.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {

// Forward declaration for forward reference support:
enum class Error : int32_t;

/**
 * Return codes from all functions.
 */
enum class Error : int32_t {
    NONE = 0,
    BAD_CONFIG = 1,
    BAD_DISPLAY = 2,
    BAD_LAYER = 3,
    BAD_PARAMETER = 4,
    NO_RESOURCES = 6,
    NOT_VALIDATED = 7,
    UNSUPPORTED = 8,
};

typedef uint32_t Config;

typedef uint64_t Display;

typedef uint64_t Layer;

//
// type declarations for package
//

template<typename>
static inline std::string toString(int32_t o);
static inline std::string toString(::android::hardware::graphics::composer::V2_1::Error o);

constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_1::Error lhs, const ::android::hardware::graphics::composer::V2_1::Error rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const int32_t lhs, const ::android::hardware::graphics::composer::V2_1::Error rhs) {
    return static_cast<int32_t>(lhs | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_1::Error lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | rhs);
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_1::Error lhs, const ::android::hardware::graphics::composer::V2_1::Error rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const int32_t lhs, const ::android::hardware::graphics::composer::V2_1::Error rhs) {
    return static_cast<int32_t>(lhs & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_1::Error lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & rhs);
}
constexpr int32_t &operator|=(int32_t& v, const ::android::hardware::graphics::composer::V2_1::Error e) {
    v |= static_cast<int32_t>(e);
    return v;
}
constexpr int32_t &operator&=(int32_t& v, const ::android::hardware::graphics::composer::V2_1::Error e) {
    v &= static_cast<int32_t>(e);
    return v;
}

//
// type header definitions for package
//

template<>
inline std::string toString<::android::hardware::graphics::composer::V2_1::Error>(int32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::graphics::composer::V2_1::Error> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::graphics::composer::V2_1::Error::NONE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::Error::NONE)) {
        os += (first ? "" : " | ");
        os += "NONE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::Error::NONE;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::Error::BAD_CONFIG) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::Error::BAD_CONFIG)) {
        os += (first ? "" : " | ");
        os += "BAD_CONFIG";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::Error::BAD_CONFIG;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::Error::BAD_DISPLAY) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::Error::BAD_DISPLAY)) {
        os += (first ? "" : " | ");
        os += "BAD_DISPLAY";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::Error::BAD_DISPLAY;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::Error::BAD_LAYER) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::Error::BAD_LAYER)) {
        os += (first ? "" : " | ");
        os += "BAD_LAYER";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::Error::BAD_LAYER;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::Error::BAD_PARAMETER) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::Error::BAD_PARAMETER)) {
        os += (first ? "" : " | ");
        os += "BAD_PARAMETER";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::Error::BAD_PARAMETER;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::Error::NO_RESOURCES) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::Error::NO_RESOURCES)) {
        os += (first ? "" : " | ");
        os += "NO_RESOURCES";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::Error::NO_RESOURCES;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::Error::NOT_VALIDATED) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::Error::NOT_VALIDATED)) {
        os += (first ? "" : " | ");
        os += "NOT_VALIDATED";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::Error::NOT_VALIDATED;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::Error::UNSUPPORTED) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::Error::UNSUPPORTED)) {
        os += (first ? "" : " | ");
        os += "UNSUPPORTED";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::Error::UNSUPPORTED;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::graphics::composer::V2_1::Error o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::graphics::composer::V2_1::Error::NONE) {
        return "NONE";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::Error::BAD_CONFIG) {
        return "BAD_CONFIG";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::Error::BAD_DISPLAY) {
        return "BAD_DISPLAY";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::Error::BAD_LAYER) {
        return "BAD_LAYER";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::Error::BAD_PARAMETER) {
        return "BAD_PARAMETER";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::Error::NO_RESOURCES) {
        return "NO_RESOURCES";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::Error::NOT_VALIDATED) {
        return "NOT_VALIDATED";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::Error::UNSUPPORTED) {
        return "UNSUPPORTED";
    }
    std::string os;
    os += toHexString(static_cast<int32_t>(o));
    return os;
}


}  // namespace V2_1
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
template<> constexpr std::array<::android::hardware::graphics::composer::V2_1::Error, 8> hidl_enum_values<::android::hardware::graphics::composer::V2_1::Error> = {
    ::android::hardware::graphics::composer::V2_1::Error::NONE,
    ::android::hardware::graphics::composer::V2_1::Error::BAD_CONFIG,
    ::android::hardware::graphics::composer::V2_1::Error::BAD_DISPLAY,
    ::android::hardware::graphics::composer::V2_1::Error::BAD_LAYER,
    ::android::hardware::graphics::composer::V2_1::Error::BAD_PARAMETER,
    ::android::hardware::graphics::composer::V2_1::Error::NO_RESOURCES,
    ::android::hardware::graphics::composer::V2_1::Error::NOT_VALIDATED,
    ::android::hardware::graphics::composer::V2_1::Error::UNSUPPORTED,
};
}  // namespace details
}  // namespace hardware
}  // namespace android


#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_TYPES_H
