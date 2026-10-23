#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_MAPPER_V4_0_TYPES_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_MAPPER_V4_0_TYPES_H

#include <hidl/HidlSupport.h>
#include <hidl/MQDescriptor.h>
#include <utils/NativeHandle.h>
#include <utils/misc.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V4_0 {

// Forward declaration for forward reference support:
enum class Error : int32_t;

/**
 * Error values that may be returned by a method of IAllocator or IMapper.
 */
enum class Error : int32_t {
    /**
     * No error.
     */
    NONE = 0,
    /**
     * Invalid BufferDescriptor.
     */
    BAD_DESCRIPTOR = 1,
    /**
     * Invalid buffer handle.
     */
    BAD_BUFFER = 2,
    /**
     * Invalid HardwareBufferDescription.
     */
    BAD_VALUE = 3,
    /**
     * Resource unavailable.
     */
    NO_RESOURCES = 5,
    /**
     * Permanent failure.
     */
    UNSUPPORTED = 7,
};

/**
 * A buffer descriptor is an implementation-defined opaque data returned by
 * createDescriptor(). It describes the properties of a buffer and is consumed
 * by the allocator.
 */
typedef ::android::hardware::hidl_vec<uint8_t> BufferDescriptor;

//
// type declarations for package
//

template<typename>
static inline std::string toString(int32_t o);
static inline std::string toString(::android::hardware::graphics::mapper::V4_0::Error o);

constexpr int32_t operator|(const ::android::hardware::graphics::mapper::V4_0::Error lhs, const ::android::hardware::graphics::mapper::V4_0::Error rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const int32_t lhs, const ::android::hardware::graphics::mapper::V4_0::Error rhs) {
    return static_cast<int32_t>(lhs | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const ::android::hardware::graphics::mapper::V4_0::Error lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | rhs);
}
constexpr int32_t operator&(const ::android::hardware::graphics::mapper::V4_0::Error lhs, const ::android::hardware::graphics::mapper::V4_0::Error rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const int32_t lhs, const ::android::hardware::graphics::mapper::V4_0::Error rhs) {
    return static_cast<int32_t>(lhs & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const ::android::hardware::graphics::mapper::V4_0::Error lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & rhs);
}
constexpr int32_t &operator|=(int32_t& v, const ::android::hardware::graphics::mapper::V4_0::Error e) {
    v |= static_cast<int32_t>(e);
    return v;
}
constexpr int32_t &operator&=(int32_t& v, const ::android::hardware::graphics::mapper::V4_0::Error e) {
    v &= static_cast<int32_t>(e);
    return v;
}

//
// type header definitions for package
//

template<>
inline std::string toString<::android::hardware::graphics::mapper::V4_0::Error>(int32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::graphics::mapper::V4_0::Error> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::graphics::mapper::V4_0::Error::NONE) == static_cast<int32_t>(::android::hardware::graphics::mapper::V4_0::Error::NONE)) {
        os += (first ? "" : " | ");
        os += "NONE";
        first = false;
        flipped |= ::android::hardware::graphics::mapper::V4_0::Error::NONE;
    }
    if ((o & ::android::hardware::graphics::mapper::V4_0::Error::BAD_DESCRIPTOR) == static_cast<int32_t>(::android::hardware::graphics::mapper::V4_0::Error::BAD_DESCRIPTOR)) {
        os += (first ? "" : " | ");
        os += "BAD_DESCRIPTOR";
        first = false;
        flipped |= ::android::hardware::graphics::mapper::V4_0::Error::BAD_DESCRIPTOR;
    }
    if ((o & ::android::hardware::graphics::mapper::V4_0::Error::BAD_BUFFER) == static_cast<int32_t>(::android::hardware::graphics::mapper::V4_0::Error::BAD_BUFFER)) {
        os += (first ? "" : " | ");
        os += "BAD_BUFFER";
        first = false;
        flipped |= ::android::hardware::graphics::mapper::V4_0::Error::BAD_BUFFER;
    }
    if ((o & ::android::hardware::graphics::mapper::V4_0::Error::BAD_VALUE) == static_cast<int32_t>(::android::hardware::graphics::mapper::V4_0::Error::BAD_VALUE)) {
        os += (first ? "" : " | ");
        os += "BAD_VALUE";
        first = false;
        flipped |= ::android::hardware::graphics::mapper::V4_0::Error::BAD_VALUE;
    }
    if ((o & ::android::hardware::graphics::mapper::V4_0::Error::NO_RESOURCES) == static_cast<int32_t>(::android::hardware::graphics::mapper::V4_0::Error::NO_RESOURCES)) {
        os += (first ? "" : " | ");
        os += "NO_RESOURCES";
        first = false;
        flipped |= ::android::hardware::graphics::mapper::V4_0::Error::NO_RESOURCES;
    }
    if ((o & ::android::hardware::graphics::mapper::V4_0::Error::UNSUPPORTED) == static_cast<int32_t>(::android::hardware::graphics::mapper::V4_0::Error::UNSUPPORTED)) {
        os += (first ? "" : " | ");
        os += "UNSUPPORTED";
        first = false;
        flipped |= ::android::hardware::graphics::mapper::V4_0::Error::UNSUPPORTED;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::graphics::mapper::V4_0::Error o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::graphics::mapper::V4_0::Error::NONE) {
        return "NONE";
    }
    if (o == ::android::hardware::graphics::mapper::V4_0::Error::BAD_DESCRIPTOR) {
        return "BAD_DESCRIPTOR";
    }
    if (o == ::android::hardware::graphics::mapper::V4_0::Error::BAD_BUFFER) {
        return "BAD_BUFFER";
    }
    if (o == ::android::hardware::graphics::mapper::V4_0::Error::BAD_VALUE) {
        return "BAD_VALUE";
    }
    if (o == ::android::hardware::graphics::mapper::V4_0::Error::NO_RESOURCES) {
        return "NO_RESOURCES";
    }
    if (o == ::android::hardware::graphics::mapper::V4_0::Error::UNSUPPORTED) {
        return "UNSUPPORTED";
    }
    std::string os;
    os += toHexString(static_cast<int32_t>(o));
    return os;
}


}  // namespace V4_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android

//
// global type declarations for package
//

namespace android {
namespace hardware {
namespace details {
template<> constexpr std::array<::android::hardware::graphics::mapper::V4_0::Error, 6> hidl_enum_values<::android::hardware::graphics::mapper::V4_0::Error> = {
    ::android::hardware::graphics::mapper::V4_0::Error::NONE,
    ::android::hardware::graphics::mapper::V4_0::Error::BAD_DESCRIPTOR,
    ::android::hardware::graphics::mapper::V4_0::Error::BAD_BUFFER,
    ::android::hardware::graphics::mapper::V4_0::Error::BAD_VALUE,
    ::android::hardware::graphics::mapper::V4_0::Error::NO_RESOURCES,
    ::android::hardware::graphics::mapper::V4_0::Error::UNSUPPORTED,
};
}  // namespace details
}  // namespace hardware
}  // namespace android


#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_MAPPER_V4_0_TYPES_H
