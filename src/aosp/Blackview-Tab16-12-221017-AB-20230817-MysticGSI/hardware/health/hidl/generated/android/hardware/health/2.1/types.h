#ifndef HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_1_TYPES_H
#define HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_1_TYPES_H

#include <android/hardware/health/1.0/types.h>
#include <android/hardware/health/2.0/types.h>

#include <hidl/HidlSupport.h>
#include <hidl/MQDescriptor.h>
#include <utils/NativeHandle.h>
#include <utils/misc.h>

namespace android {
namespace hardware {
namespace health {
namespace V2_1 {

// Forward declaration for forward reference support:
enum class Constants : int64_t;
enum class BatteryCapacityLevel : int32_t;
struct HealthInfo;
struct HealthConfig;

enum class Constants : int64_t {
    BATTERY_CHARGE_TIME_TO_FULL_NOW_SECONDS_UNSUPPORTED = -1ll /* (-1) */,
};

/**
 * Battery capacity level. This enum provides additional information along side
 * with the battery capacity.
 * Clients of this HAL must use this value before inferring it from the
 * battery capacity.
 */
enum class BatteryCapacityLevel : int32_t {
    /**
     * Battery capacity level is unsupported.
     * Battery capacity level must be set to this value if and only if the
     * implementation is unsupported.
     */
    UNSUPPORTED = -1 /* (-1) */,
    /**
     * Battery capacity level is unknown.
     * Battery capacity level must be set to this value if and only if battery
     * is not present or the battery capacity level is unknown/uninitialized.
     */
    UNKNOWN = 0 /* (::android::hardware::health::V2_1::BatteryCapacityLevel.UNSUPPORTED implicitly + 1) */,
    /**
     * Battery is at critical level. The Android framework must schedule a
     * shutdown when it sees this value from the HAL.
     */
    CRITICAL = 1 /* (::android::hardware::health::V2_1::BatteryCapacityLevel.UNKNOWN implicitly + 1) */,
    /**
     * Battery is low. The Android framework may limit the performance of
     * the device when it sees this value from the HAL.
     */
    LOW = 2 /* (::android::hardware::health::V2_1::BatteryCapacityLevel.CRITICAL implicitly + 1) */,
    /**
     * Battery level is normal.
     */
    NORMAL = 3 /* (::android::hardware::health::V2_1::BatteryCapacityLevel.LOW implicitly + 1) */,
    /**
     * Battery level is high.
     */
    HIGH = 4 /* (::android::hardware::health::V2_1::BatteryCapacityLevel.NORMAL implicitly + 1) */,
    /**
     * Battery is full. It must be set to FULL if and only if battery level is
     * 100.
     */
    FULL = 5 /* (::android::hardware::health::V2_1::BatteryCapacityLevel.HIGH implicitly + 1) */,
};

/**
 * Combined Health Information.
 */
struct HealthInfo final {
    ::android::hardware::health::V2_0::HealthInfo legacy __attribute__ ((aligned(8)));
    ::android::hardware::health::V2_1::BatteryCapacityLevel batteryCapacityLevel __attribute__ ((aligned(4)));
    int64_t batteryChargeTimeToFullNowSeconds __attribute__ ((aligned(8)));
    int32_t batteryFullChargeDesignCapacityUah __attribute__ ((aligned(4)));
};

static_assert(offsetof(::android::hardware::health::V2_1::HealthInfo, legacy) == 0, "wrong offset");
static_assert(offsetof(::android::hardware::health::V2_1::HealthInfo, batteryCapacityLevel) == 112, "wrong offset");
static_assert(offsetof(::android::hardware::health::V2_1::HealthInfo, batteryChargeTimeToFullNowSeconds) == 120, "wrong offset");
static_assert(offsetof(::android::hardware::health::V2_1::HealthInfo, batteryFullChargeDesignCapacityUah) == 128, "wrong offset");
static_assert(sizeof(::android::hardware::health::V2_1::HealthInfo) == 136, "wrong size");
static_assert(__alignof(::android::hardware::health::V2_1::HealthInfo) == 8, "wrong alignment");

/**
 * Combined configuration of a health HAL implementation.
 */
struct HealthConfig final {
    ::android::hardware::health::V1_0::HealthConfig battery __attribute__ ((aligned(8)));
    int32_t bootMinCap __attribute__ ((aligned(4)));
};

static_assert(offsetof(::android::hardware::health::V2_1::HealthConfig, battery) == 0, "wrong offset");
static_assert(offsetof(::android::hardware::health::V2_1::HealthConfig, bootMinCap) == 200, "wrong offset");
static_assert(sizeof(::android::hardware::health::V2_1::HealthConfig) == 208, "wrong size");
static_assert(__alignof(::android::hardware::health::V2_1::HealthConfig) == 8, "wrong alignment");

//
// type declarations for package
//

template<typename>
static inline std::string toString(int64_t o);
static inline std::string toString(::android::hardware::health::V2_1::Constants o);

constexpr int64_t operator|(const ::android::hardware::health::V2_1::Constants lhs, const ::android::hardware::health::V2_1::Constants rhs) {
    return static_cast<int64_t>(static_cast<int64_t>(lhs) | static_cast<int64_t>(rhs));
}
constexpr int64_t operator|(const int64_t lhs, const ::android::hardware::health::V2_1::Constants rhs) {
    return static_cast<int64_t>(lhs | static_cast<int64_t>(rhs));
}
constexpr int64_t operator|(const ::android::hardware::health::V2_1::Constants lhs, const int64_t rhs) {
    return static_cast<int64_t>(static_cast<int64_t>(lhs) | rhs);
}
constexpr int64_t operator&(const ::android::hardware::health::V2_1::Constants lhs, const ::android::hardware::health::V2_1::Constants rhs) {
    return static_cast<int64_t>(static_cast<int64_t>(lhs) & static_cast<int64_t>(rhs));
}
constexpr int64_t operator&(const int64_t lhs, const ::android::hardware::health::V2_1::Constants rhs) {
    return static_cast<int64_t>(lhs & static_cast<int64_t>(rhs));
}
constexpr int64_t operator&(const ::android::hardware::health::V2_1::Constants lhs, const int64_t rhs) {
    return static_cast<int64_t>(static_cast<int64_t>(lhs) & rhs);
}
constexpr int64_t &operator|=(int64_t& v, const ::android::hardware::health::V2_1::Constants e) {
    v |= static_cast<int64_t>(e);
    return v;
}
constexpr int64_t &operator&=(int64_t& v, const ::android::hardware::health::V2_1::Constants e) {
    v &= static_cast<int64_t>(e);
    return v;
}

template<typename>
static inline std::string toString(int32_t o);
static inline std::string toString(::android::hardware::health::V2_1::BatteryCapacityLevel o);

constexpr int32_t operator|(const ::android::hardware::health::V2_1::BatteryCapacityLevel lhs, const ::android::hardware::health::V2_1::BatteryCapacityLevel rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const int32_t lhs, const ::android::hardware::health::V2_1::BatteryCapacityLevel rhs) {
    return static_cast<int32_t>(lhs | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const ::android::hardware::health::V2_1::BatteryCapacityLevel lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | rhs);
}
constexpr int32_t operator&(const ::android::hardware::health::V2_1::BatteryCapacityLevel lhs, const ::android::hardware::health::V2_1::BatteryCapacityLevel rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const int32_t lhs, const ::android::hardware::health::V2_1::BatteryCapacityLevel rhs) {
    return static_cast<int32_t>(lhs & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const ::android::hardware::health::V2_1::BatteryCapacityLevel lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & rhs);
}
constexpr int32_t &operator|=(int32_t& v, const ::android::hardware::health::V2_1::BatteryCapacityLevel e) {
    v |= static_cast<int32_t>(e);
    return v;
}
constexpr int32_t &operator&=(int32_t& v, const ::android::hardware::health::V2_1::BatteryCapacityLevel e) {
    v &= static_cast<int32_t>(e);
    return v;
}

static inline std::string toString(const ::android::hardware::health::V2_1::HealthInfo& o);
static inline bool operator==(const ::android::hardware::health::V2_1::HealthInfo& lhs, const ::android::hardware::health::V2_1::HealthInfo& rhs);
static inline bool operator!=(const ::android::hardware::health::V2_1::HealthInfo& lhs, const ::android::hardware::health::V2_1::HealthInfo& rhs);

static inline std::string toString(const ::android::hardware::health::V2_1::HealthConfig& o);
static inline bool operator==(const ::android::hardware::health::V2_1::HealthConfig& lhs, const ::android::hardware::health::V2_1::HealthConfig& rhs);
static inline bool operator!=(const ::android::hardware::health::V2_1::HealthConfig& lhs, const ::android::hardware::health::V2_1::HealthConfig& rhs);

//
// type header definitions for package
//

template<>
inline std::string toString<::android::hardware::health::V2_1::Constants>(int64_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::health::V2_1::Constants> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::health::V2_1::Constants::BATTERY_CHARGE_TIME_TO_FULL_NOW_SECONDS_UNSUPPORTED) == static_cast<int64_t>(::android::hardware::health::V2_1::Constants::BATTERY_CHARGE_TIME_TO_FULL_NOW_SECONDS_UNSUPPORTED)) {
        os += (first ? "" : " | ");
        os += "BATTERY_CHARGE_TIME_TO_FULL_NOW_SECONDS_UNSUPPORTED";
        first = false;
        flipped |= ::android::hardware::health::V2_1::Constants::BATTERY_CHARGE_TIME_TO_FULL_NOW_SECONDS_UNSUPPORTED;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::health::V2_1::Constants o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::health::V2_1::Constants::BATTERY_CHARGE_TIME_TO_FULL_NOW_SECONDS_UNSUPPORTED) {
        return "BATTERY_CHARGE_TIME_TO_FULL_NOW_SECONDS_UNSUPPORTED";
    }
    std::string os;
    os += toHexString(static_cast<int64_t>(o));
    return os;
}

template<>
inline std::string toString<::android::hardware::health::V2_1::BatteryCapacityLevel>(int32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::health::V2_1::BatteryCapacityLevel> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::health::V2_1::BatteryCapacityLevel::UNSUPPORTED) == static_cast<int32_t>(::android::hardware::health::V2_1::BatteryCapacityLevel::UNSUPPORTED)) {
        os += (first ? "" : " | ");
        os += "UNSUPPORTED";
        first = false;
        flipped |= ::android::hardware::health::V2_1::BatteryCapacityLevel::UNSUPPORTED;
    }
    if ((o & ::android::hardware::health::V2_1::BatteryCapacityLevel::UNKNOWN) == static_cast<int32_t>(::android::hardware::health::V2_1::BatteryCapacityLevel::UNKNOWN)) {
        os += (first ? "" : " | ");
        os += "UNKNOWN";
        first = false;
        flipped |= ::android::hardware::health::V2_1::BatteryCapacityLevel::UNKNOWN;
    }
    if ((o & ::android::hardware::health::V2_1::BatteryCapacityLevel::CRITICAL) == static_cast<int32_t>(::android::hardware::health::V2_1::BatteryCapacityLevel::CRITICAL)) {
        os += (first ? "" : " | ");
        os += "CRITICAL";
        first = false;
        flipped |= ::android::hardware::health::V2_1::BatteryCapacityLevel::CRITICAL;
    }
    if ((o & ::android::hardware::health::V2_1::BatteryCapacityLevel::LOW) == static_cast<int32_t>(::android::hardware::health::V2_1::BatteryCapacityLevel::LOW)) {
        os += (first ? "" : " | ");
        os += "LOW";
        first = false;
        flipped |= ::android::hardware::health::V2_1::BatteryCapacityLevel::LOW;
    }
    if ((o & ::android::hardware::health::V2_1::BatteryCapacityLevel::NORMAL) == static_cast<int32_t>(::android::hardware::health::V2_1::BatteryCapacityLevel::NORMAL)) {
        os += (first ? "" : " | ");
        os += "NORMAL";
        first = false;
        flipped |= ::android::hardware::health::V2_1::BatteryCapacityLevel::NORMAL;
    }
    if ((o & ::android::hardware::health::V2_1::BatteryCapacityLevel::HIGH) == static_cast<int32_t>(::android::hardware::health::V2_1::BatteryCapacityLevel::HIGH)) {
        os += (first ? "" : " | ");
        os += "HIGH";
        first = false;
        flipped |= ::android::hardware::health::V2_1::BatteryCapacityLevel::HIGH;
    }
    if ((o & ::android::hardware::health::V2_1::BatteryCapacityLevel::FULL) == static_cast<int32_t>(::android::hardware::health::V2_1::BatteryCapacityLevel::FULL)) {
        os += (first ? "" : " | ");
        os += "FULL";
        first = false;
        flipped |= ::android::hardware::health::V2_1::BatteryCapacityLevel::FULL;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::health::V2_1::BatteryCapacityLevel o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::health::V2_1::BatteryCapacityLevel::UNSUPPORTED) {
        return "UNSUPPORTED";
    }
    if (o == ::android::hardware::health::V2_1::BatteryCapacityLevel::UNKNOWN) {
        return "UNKNOWN";
    }
    if (o == ::android::hardware::health::V2_1::BatteryCapacityLevel::CRITICAL) {
        return "CRITICAL";
    }
    if (o == ::android::hardware::health::V2_1::BatteryCapacityLevel::LOW) {
        return "LOW";
    }
    if (o == ::android::hardware::health::V2_1::BatteryCapacityLevel::NORMAL) {
        return "NORMAL";
    }
    if (o == ::android::hardware::health::V2_1::BatteryCapacityLevel::HIGH) {
        return "HIGH";
    }
    if (o == ::android::hardware::health::V2_1::BatteryCapacityLevel::FULL) {
        return "FULL";
    }
    std::string os;
    os += toHexString(static_cast<int32_t>(o));
    return os;
}

static inline std::string toString(const ::android::hardware::health::V2_1::HealthInfo& o) {
    using ::android::hardware::toString;
    std::string os;
    os += "{";
    os += ".legacy = ";
    os += ::android::hardware::health::V2_0::toString(o.legacy);
    os += ", .batteryCapacityLevel = ";
    os += ::android::hardware::health::V2_1::toString(o.batteryCapacityLevel);
    os += ", .batteryChargeTimeToFullNowSeconds = ";
    os += ::android::hardware::toString(o.batteryChargeTimeToFullNowSeconds);
    os += ", .batteryFullChargeDesignCapacityUah = ";
    os += ::android::hardware::toString(o.batteryFullChargeDesignCapacityUah);
    os += "}"; return os;
}

static inline bool operator==(const ::android::hardware::health::V2_1::HealthInfo& lhs, const ::android::hardware::health::V2_1::HealthInfo& rhs) {
    if (lhs.legacy != rhs.legacy) {
        return false;
    }
    if (lhs.batteryCapacityLevel != rhs.batteryCapacityLevel) {
        return false;
    }
    if (lhs.batteryChargeTimeToFullNowSeconds != rhs.batteryChargeTimeToFullNowSeconds) {
        return false;
    }
    if (lhs.batteryFullChargeDesignCapacityUah != rhs.batteryFullChargeDesignCapacityUah) {
        return false;
    }
    return true;
}

static inline bool operator!=(const ::android::hardware::health::V2_1::HealthInfo& lhs, const ::android::hardware::health::V2_1::HealthInfo& rhs){
    return !(lhs == rhs);
}

static inline std::string toString(const ::android::hardware::health::V2_1::HealthConfig& o) {
    using ::android::hardware::toString;
    std::string os;
    os += "{";
    os += ".battery = ";
    os += ::android::hardware::health::V1_0::toString(o.battery);
    os += ", .bootMinCap = ";
    os += ::android::hardware::toString(o.bootMinCap);
    os += "}"; return os;
}

static inline bool operator==(const ::android::hardware::health::V2_1::HealthConfig& lhs, const ::android::hardware::health::V2_1::HealthConfig& rhs) {
    if (lhs.battery != rhs.battery) {
        return false;
    }
    if (lhs.bootMinCap != rhs.bootMinCap) {
        return false;
    }
    return true;
}

static inline bool operator!=(const ::android::hardware::health::V2_1::HealthConfig& lhs, const ::android::hardware::health::V2_1::HealthConfig& rhs){
    return !(lhs == rhs);
}


}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android

//
// global type declarations for package
//

namespace android {
namespace hardware {
namespace details {
template<> constexpr std::array<::android::hardware::health::V2_1::Constants, 1> hidl_enum_values<::android::hardware::health::V2_1::Constants> = {
    ::android::hardware::health::V2_1::Constants::BATTERY_CHARGE_TIME_TO_FULL_NOW_SECONDS_UNSUPPORTED,
};
}  // namespace details
}  // namespace hardware
}  // namespace android

namespace android {
namespace hardware {
namespace details {
template<> constexpr std::array<::android::hardware::health::V2_1::BatteryCapacityLevel, 7> hidl_enum_values<::android::hardware::health::V2_1::BatteryCapacityLevel> = {
    ::android::hardware::health::V2_1::BatteryCapacityLevel::UNSUPPORTED,
    ::android::hardware::health::V2_1::BatteryCapacityLevel::UNKNOWN,
    ::android::hardware::health::V2_1::BatteryCapacityLevel::CRITICAL,
    ::android::hardware::health::V2_1::BatteryCapacityLevel::LOW,
    ::android::hardware::health::V2_1::BatteryCapacityLevel::NORMAL,
    ::android::hardware::health::V2_1::BatteryCapacityLevel::HIGH,
    ::android::hardware::health::V2_1::BatteryCapacityLevel::FULL,
};
}  // namespace details
}  // namespace hardware
}  // namespace android


#endif  // HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_1_TYPES_H
