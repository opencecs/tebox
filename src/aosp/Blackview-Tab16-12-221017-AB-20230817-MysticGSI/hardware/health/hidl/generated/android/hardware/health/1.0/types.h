#ifndef HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V1_0_TYPES_H
#define HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V1_0_TYPES_H

#include <hidl/HidlSupport.h>
#include <hidl/MQDescriptor.h>
#include <utils/NativeHandle.h>
#include <utils/misc.h>

namespace android {
namespace hardware {
namespace health {
namespace V1_0 {

// Forward declaration for forward reference support:
enum class Result : int32_t;
enum class BatteryStatus : int32_t;
enum class BatteryHealth : int32_t;
struct HealthConfig;
struct HealthInfo;

/**
 * Possible return values for optional HAL method(s) like
 * IHealth::energyCounter()
 */
enum class Result : int32_t {
    SUCCESS = 0,
    NOT_SUPPORTED = 1 /* (::android::hardware::health::V1_0::Result.SUCCESS implicitly + 1) */,
    UNKNOWN = 2 /* (::android::hardware::health::V1_0::Result.NOT_SUPPORTED implicitly + 1) */,
};

/**
 * Possible values for Battery Status.
 * Note: These are currently in sync with BatteryManager and must not
 * be extended / altered.
 */
enum class BatteryStatus : int32_t {
    UNKNOWN = 1,
    CHARGING = 2,
    DISCHARGING = 3,
    /**
     * Battery is *not* charging - special case when charger is present
     * but battery isn't charging
     */
    NOT_CHARGING = 4,
    FULL = 5,
};

/**
 * Possible values for Battery Health.
 * Note: These are currently in sync with BatteryManager and must not
 * be extended / altered.
 */
enum class BatteryHealth : int32_t {
    UNKNOWN = 1,
    GOOD = 2,
    OVERHEAT = 3,
    DEAD = 4,
    OVER_VOLTAGE = 5,
    /**
     * Battery experienced an unknown/unspecifid failure.
     */
    UNSPECIFIED_FAILURE = 6,
    COLD = 7,
};

struct HealthConfig final {
    int32_t periodicChoresIntervalFast __attribute__ ((aligned(4)));
    int32_t periodicChoresIntervalSlow __attribute__ ((aligned(4)));
    ::android::hardware::hidl_string batteryStatusPath __attribute__ ((aligned(8)));
    ::android::hardware::hidl_string batteryHealthPath __attribute__ ((aligned(8)));
    ::android::hardware::hidl_string batteryPresentPath __attribute__ ((aligned(8)));
    ::android::hardware::hidl_string batteryCapacityPath __attribute__ ((aligned(8)));
    ::android::hardware::hidl_string batteryVoltagePath __attribute__ ((aligned(8)));
    ::android::hardware::hidl_string batteryTemperaturePath __attribute__ ((aligned(8)));
    ::android::hardware::hidl_string batteryTechnologyPath __attribute__ ((aligned(8)));
    ::android::hardware::hidl_string batteryCurrentNowPath __attribute__ ((aligned(8)));
    ::android::hardware::hidl_string batteryCurrentAvgPath __attribute__ ((aligned(8)));
    ::android::hardware::hidl_string batteryChargeCounterPath __attribute__ ((aligned(8)));
    ::android::hardware::hidl_string batteryFullChargePath __attribute__ ((aligned(8)));
    ::android::hardware::hidl_string batteryCycleCountPath __attribute__ ((aligned(8)));
};

static_assert(offsetof(::android::hardware::health::V1_0::HealthConfig, periodicChoresIntervalFast) == 0, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthConfig, periodicChoresIntervalSlow) == 4, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthConfig, batteryStatusPath) == 8, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthConfig, batteryHealthPath) == 24, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthConfig, batteryPresentPath) == 40, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthConfig, batteryCapacityPath) == 56, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthConfig, batteryVoltagePath) == 72, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthConfig, batteryTemperaturePath) == 88, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthConfig, batteryTechnologyPath) == 104, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthConfig, batteryCurrentNowPath) == 120, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthConfig, batteryCurrentAvgPath) == 136, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthConfig, batteryChargeCounterPath) == 152, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthConfig, batteryFullChargePath) == 168, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthConfig, batteryCycleCountPath) == 184, "wrong offset");
static_assert(sizeof(::android::hardware::health::V1_0::HealthConfig) == 200, "wrong size");
static_assert(__alignof(::android::hardware::health::V1_0::HealthConfig) == 8, "wrong alignment");

/**
 * The parameter to healthd mainloop update calls
 */
struct HealthInfo final {
    bool chargerAcOnline __attribute__ ((aligned(1)));
    bool chargerUsbOnline __attribute__ ((aligned(1)));
    bool chargerWirelessOnline __attribute__ ((aligned(1)));
    int32_t maxChargingCurrent __attribute__ ((aligned(4)));
    int32_t maxChargingVoltage __attribute__ ((aligned(4)));
    ::android::hardware::health::V1_0::BatteryStatus batteryStatus __attribute__ ((aligned(4)));
    ::android::hardware::health::V1_0::BatteryHealth batteryHealth __attribute__ ((aligned(4)));
    bool batteryPresent __attribute__ ((aligned(1)));
    int32_t batteryLevel __attribute__ ((aligned(4)));
    int32_t batteryVoltage __attribute__ ((aligned(4)));
    int32_t batteryTemperature __attribute__ ((aligned(4)));
    int32_t batteryCurrent __attribute__ ((aligned(4)));
    int32_t batteryCycleCount __attribute__ ((aligned(4)));
    int32_t batteryFullCharge __attribute__ ((aligned(4)));
    int32_t batteryChargeCounter __attribute__ ((aligned(4)));
    ::android::hardware::hidl_string batteryTechnology __attribute__ ((aligned(8)));
};

static_assert(offsetof(::android::hardware::health::V1_0::HealthInfo, chargerAcOnline) == 0, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthInfo, chargerUsbOnline) == 1, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthInfo, chargerWirelessOnline) == 2, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthInfo, maxChargingCurrent) == 4, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthInfo, maxChargingVoltage) == 8, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthInfo, batteryStatus) == 12, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthInfo, batteryHealth) == 16, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthInfo, batteryPresent) == 20, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthInfo, batteryLevel) == 24, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthInfo, batteryVoltage) == 28, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthInfo, batteryTemperature) == 32, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthInfo, batteryCurrent) == 36, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthInfo, batteryCycleCount) == 40, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthInfo, batteryFullCharge) == 44, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthInfo, batteryChargeCounter) == 48, "wrong offset");
static_assert(offsetof(::android::hardware::health::V1_0::HealthInfo, batteryTechnology) == 56, "wrong offset");
static_assert(sizeof(::android::hardware::health::V1_0::HealthInfo) == 72, "wrong size");
static_assert(__alignof(::android::hardware::health::V1_0::HealthInfo) == 8, "wrong alignment");

//
// type declarations for package
//

template<typename>
static inline std::string toString(int32_t o);
static inline std::string toString(::android::hardware::health::V1_0::Result o);

constexpr int32_t operator|(const ::android::hardware::health::V1_0::Result lhs, const ::android::hardware::health::V1_0::Result rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const int32_t lhs, const ::android::hardware::health::V1_0::Result rhs) {
    return static_cast<int32_t>(lhs | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const ::android::hardware::health::V1_0::Result lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | rhs);
}
constexpr int32_t operator&(const ::android::hardware::health::V1_0::Result lhs, const ::android::hardware::health::V1_0::Result rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const int32_t lhs, const ::android::hardware::health::V1_0::Result rhs) {
    return static_cast<int32_t>(lhs & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const ::android::hardware::health::V1_0::Result lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & rhs);
}
constexpr int32_t &operator|=(int32_t& v, const ::android::hardware::health::V1_0::Result e) {
    v |= static_cast<int32_t>(e);
    return v;
}
constexpr int32_t &operator&=(int32_t& v, const ::android::hardware::health::V1_0::Result e) {
    v &= static_cast<int32_t>(e);
    return v;
}

template<typename>
static inline std::string toString(int32_t o);
static inline std::string toString(::android::hardware::health::V1_0::BatteryStatus o);

constexpr int32_t operator|(const ::android::hardware::health::V1_0::BatteryStatus lhs, const ::android::hardware::health::V1_0::BatteryStatus rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const int32_t lhs, const ::android::hardware::health::V1_0::BatteryStatus rhs) {
    return static_cast<int32_t>(lhs | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const ::android::hardware::health::V1_0::BatteryStatus lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | rhs);
}
constexpr int32_t operator&(const ::android::hardware::health::V1_0::BatteryStatus lhs, const ::android::hardware::health::V1_0::BatteryStatus rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const int32_t lhs, const ::android::hardware::health::V1_0::BatteryStatus rhs) {
    return static_cast<int32_t>(lhs & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const ::android::hardware::health::V1_0::BatteryStatus lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & rhs);
}
constexpr int32_t &operator|=(int32_t& v, const ::android::hardware::health::V1_0::BatteryStatus e) {
    v |= static_cast<int32_t>(e);
    return v;
}
constexpr int32_t &operator&=(int32_t& v, const ::android::hardware::health::V1_0::BatteryStatus e) {
    v &= static_cast<int32_t>(e);
    return v;
}

template<typename>
static inline std::string toString(int32_t o);
static inline std::string toString(::android::hardware::health::V1_0::BatteryHealth o);

constexpr int32_t operator|(const ::android::hardware::health::V1_0::BatteryHealth lhs, const ::android::hardware::health::V1_0::BatteryHealth rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const int32_t lhs, const ::android::hardware::health::V1_0::BatteryHealth rhs) {
    return static_cast<int32_t>(lhs | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const ::android::hardware::health::V1_0::BatteryHealth lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | rhs);
}
constexpr int32_t operator&(const ::android::hardware::health::V1_0::BatteryHealth lhs, const ::android::hardware::health::V1_0::BatteryHealth rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const int32_t lhs, const ::android::hardware::health::V1_0::BatteryHealth rhs) {
    return static_cast<int32_t>(lhs & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const ::android::hardware::health::V1_0::BatteryHealth lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & rhs);
}
constexpr int32_t &operator|=(int32_t& v, const ::android::hardware::health::V1_0::BatteryHealth e) {
    v |= static_cast<int32_t>(e);
    return v;
}
constexpr int32_t &operator&=(int32_t& v, const ::android::hardware::health::V1_0::BatteryHealth e) {
    v &= static_cast<int32_t>(e);
    return v;
}

static inline std::string toString(const ::android::hardware::health::V1_0::HealthConfig& o);
static inline bool operator==(const ::android::hardware::health::V1_0::HealthConfig& lhs, const ::android::hardware::health::V1_0::HealthConfig& rhs);
static inline bool operator!=(const ::android::hardware::health::V1_0::HealthConfig& lhs, const ::android::hardware::health::V1_0::HealthConfig& rhs);

static inline std::string toString(const ::android::hardware::health::V1_0::HealthInfo& o);
static inline bool operator==(const ::android::hardware::health::V1_0::HealthInfo& lhs, const ::android::hardware::health::V1_0::HealthInfo& rhs);
static inline bool operator!=(const ::android::hardware::health::V1_0::HealthInfo& lhs, const ::android::hardware::health::V1_0::HealthInfo& rhs);

//
// type header definitions for package
//

template<>
inline std::string toString<::android::hardware::health::V1_0::Result>(int32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::health::V1_0::Result> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::health::V1_0::Result::SUCCESS) == static_cast<int32_t>(::android::hardware::health::V1_0::Result::SUCCESS)) {
        os += (first ? "" : " | ");
        os += "SUCCESS";
        first = false;
        flipped |= ::android::hardware::health::V1_0::Result::SUCCESS;
    }
    if ((o & ::android::hardware::health::V1_0::Result::NOT_SUPPORTED) == static_cast<int32_t>(::android::hardware::health::V1_0::Result::NOT_SUPPORTED)) {
        os += (first ? "" : " | ");
        os += "NOT_SUPPORTED";
        first = false;
        flipped |= ::android::hardware::health::V1_0::Result::NOT_SUPPORTED;
    }
    if ((o & ::android::hardware::health::V1_0::Result::UNKNOWN) == static_cast<int32_t>(::android::hardware::health::V1_0::Result::UNKNOWN)) {
        os += (first ? "" : " | ");
        os += "UNKNOWN";
        first = false;
        flipped |= ::android::hardware::health::V1_0::Result::UNKNOWN;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::health::V1_0::Result o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::health::V1_0::Result::SUCCESS) {
        return "SUCCESS";
    }
    if (o == ::android::hardware::health::V1_0::Result::NOT_SUPPORTED) {
        return "NOT_SUPPORTED";
    }
    if (o == ::android::hardware::health::V1_0::Result::UNKNOWN) {
        return "UNKNOWN";
    }
    std::string os;
    os += toHexString(static_cast<int32_t>(o));
    return os;
}

template<>
inline std::string toString<::android::hardware::health::V1_0::BatteryStatus>(int32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::health::V1_0::BatteryStatus> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::health::V1_0::BatteryStatus::UNKNOWN) == static_cast<int32_t>(::android::hardware::health::V1_0::BatteryStatus::UNKNOWN)) {
        os += (first ? "" : " | ");
        os += "UNKNOWN";
        first = false;
        flipped |= ::android::hardware::health::V1_0::BatteryStatus::UNKNOWN;
    }
    if ((o & ::android::hardware::health::V1_0::BatteryStatus::CHARGING) == static_cast<int32_t>(::android::hardware::health::V1_0::BatteryStatus::CHARGING)) {
        os += (first ? "" : " | ");
        os += "CHARGING";
        first = false;
        flipped |= ::android::hardware::health::V1_0::BatteryStatus::CHARGING;
    }
    if ((o & ::android::hardware::health::V1_0::BatteryStatus::DISCHARGING) == static_cast<int32_t>(::android::hardware::health::V1_0::BatteryStatus::DISCHARGING)) {
        os += (first ? "" : " | ");
        os += "DISCHARGING";
        first = false;
        flipped |= ::android::hardware::health::V1_0::BatteryStatus::DISCHARGING;
    }
    if ((o & ::android::hardware::health::V1_0::BatteryStatus::NOT_CHARGING) == static_cast<int32_t>(::android::hardware::health::V1_0::BatteryStatus::NOT_CHARGING)) {
        os += (first ? "" : " | ");
        os += "NOT_CHARGING";
        first = false;
        flipped |= ::android::hardware::health::V1_0::BatteryStatus::NOT_CHARGING;
    }
    if ((o & ::android::hardware::health::V1_0::BatteryStatus::FULL) == static_cast<int32_t>(::android::hardware::health::V1_0::BatteryStatus::FULL)) {
        os += (first ? "" : " | ");
        os += "FULL";
        first = false;
        flipped |= ::android::hardware::health::V1_0::BatteryStatus::FULL;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::health::V1_0::BatteryStatus o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::health::V1_0::BatteryStatus::UNKNOWN) {
        return "UNKNOWN";
    }
    if (o == ::android::hardware::health::V1_0::BatteryStatus::CHARGING) {
        return "CHARGING";
    }
    if (o == ::android::hardware::health::V1_0::BatteryStatus::DISCHARGING) {
        return "DISCHARGING";
    }
    if (o == ::android::hardware::health::V1_0::BatteryStatus::NOT_CHARGING) {
        return "NOT_CHARGING";
    }
    if (o == ::android::hardware::health::V1_0::BatteryStatus::FULL) {
        return "FULL";
    }
    std::string os;
    os += toHexString(static_cast<int32_t>(o));
    return os;
}

template<>
inline std::string toString<::android::hardware::health::V1_0::BatteryHealth>(int32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::health::V1_0::BatteryHealth> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::health::V1_0::BatteryHealth::UNKNOWN) == static_cast<int32_t>(::android::hardware::health::V1_0::BatteryHealth::UNKNOWN)) {
        os += (first ? "" : " | ");
        os += "UNKNOWN";
        first = false;
        flipped |= ::android::hardware::health::V1_0::BatteryHealth::UNKNOWN;
    }
    if ((o & ::android::hardware::health::V1_0::BatteryHealth::GOOD) == static_cast<int32_t>(::android::hardware::health::V1_0::BatteryHealth::GOOD)) {
        os += (first ? "" : " | ");
        os += "GOOD";
        first = false;
        flipped |= ::android::hardware::health::V1_0::BatteryHealth::GOOD;
    }
    if ((o & ::android::hardware::health::V1_0::BatteryHealth::OVERHEAT) == static_cast<int32_t>(::android::hardware::health::V1_0::BatteryHealth::OVERHEAT)) {
        os += (first ? "" : " | ");
        os += "OVERHEAT";
        first = false;
        flipped |= ::android::hardware::health::V1_0::BatteryHealth::OVERHEAT;
    }
    if ((o & ::android::hardware::health::V1_0::BatteryHealth::DEAD) == static_cast<int32_t>(::android::hardware::health::V1_0::BatteryHealth::DEAD)) {
        os += (first ? "" : " | ");
        os += "DEAD";
        first = false;
        flipped |= ::android::hardware::health::V1_0::BatteryHealth::DEAD;
    }
    if ((o & ::android::hardware::health::V1_0::BatteryHealth::OVER_VOLTAGE) == static_cast<int32_t>(::android::hardware::health::V1_0::BatteryHealth::OVER_VOLTAGE)) {
        os += (first ? "" : " | ");
        os += "OVER_VOLTAGE";
        first = false;
        flipped |= ::android::hardware::health::V1_0::BatteryHealth::OVER_VOLTAGE;
    }
    if ((o & ::android::hardware::health::V1_0::BatteryHealth::UNSPECIFIED_FAILURE) == static_cast<int32_t>(::android::hardware::health::V1_0::BatteryHealth::UNSPECIFIED_FAILURE)) {
        os += (first ? "" : " | ");
        os += "UNSPECIFIED_FAILURE";
        first = false;
        flipped |= ::android::hardware::health::V1_0::BatteryHealth::UNSPECIFIED_FAILURE;
    }
    if ((o & ::android::hardware::health::V1_0::BatteryHealth::COLD) == static_cast<int32_t>(::android::hardware::health::V1_0::BatteryHealth::COLD)) {
        os += (first ? "" : " | ");
        os += "COLD";
        first = false;
        flipped |= ::android::hardware::health::V1_0::BatteryHealth::COLD;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::health::V1_0::BatteryHealth o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::health::V1_0::BatteryHealth::UNKNOWN) {
        return "UNKNOWN";
    }
    if (o == ::android::hardware::health::V1_0::BatteryHealth::GOOD) {
        return "GOOD";
    }
    if (o == ::android::hardware::health::V1_0::BatteryHealth::OVERHEAT) {
        return "OVERHEAT";
    }
    if (o == ::android::hardware::health::V1_0::BatteryHealth::DEAD) {
        return "DEAD";
    }
    if (o == ::android::hardware::health::V1_0::BatteryHealth::OVER_VOLTAGE) {
        return "OVER_VOLTAGE";
    }
    if (o == ::android::hardware::health::V1_0::BatteryHealth::UNSPECIFIED_FAILURE) {
        return "UNSPECIFIED_FAILURE";
    }
    if (o == ::android::hardware::health::V1_0::BatteryHealth::COLD) {
        return "COLD";
    }
    std::string os;
    os += toHexString(static_cast<int32_t>(o));
    return os;
}

static inline std::string toString(const ::android::hardware::health::V1_0::HealthConfig& o) {
    using ::android::hardware::toString;
    std::string os;
    os += "{";
    os += ".periodicChoresIntervalFast = ";
    os += ::android::hardware::toString(o.periodicChoresIntervalFast);
    os += ", .periodicChoresIntervalSlow = ";
    os += ::android::hardware::toString(o.periodicChoresIntervalSlow);
    os += ", .batteryStatusPath = ";
    os += ::android::hardware::toString(o.batteryStatusPath);
    os += ", .batteryHealthPath = ";
    os += ::android::hardware::toString(o.batteryHealthPath);
    os += ", .batteryPresentPath = ";
    os += ::android::hardware::toString(o.batteryPresentPath);
    os += ", .batteryCapacityPath = ";
    os += ::android::hardware::toString(o.batteryCapacityPath);
    os += ", .batteryVoltagePath = ";
    os += ::android::hardware::toString(o.batteryVoltagePath);
    os += ", .batteryTemperaturePath = ";
    os += ::android::hardware::toString(o.batteryTemperaturePath);
    os += ", .batteryTechnologyPath = ";
    os += ::android::hardware::toString(o.batteryTechnologyPath);
    os += ", .batteryCurrentNowPath = ";
    os += ::android::hardware::toString(o.batteryCurrentNowPath);
    os += ", .batteryCurrentAvgPath = ";
    os += ::android::hardware::toString(o.batteryCurrentAvgPath);
    os += ", .batteryChargeCounterPath = ";
    os += ::android::hardware::toString(o.batteryChargeCounterPath);
    os += ", .batteryFullChargePath = ";
    os += ::android::hardware::toString(o.batteryFullChargePath);
    os += ", .batteryCycleCountPath = ";
    os += ::android::hardware::toString(o.batteryCycleCountPath);
    os += "}"; return os;
}

static inline bool operator==(const ::android::hardware::health::V1_0::HealthConfig& lhs, const ::android::hardware::health::V1_0::HealthConfig& rhs) {
    if (lhs.periodicChoresIntervalFast != rhs.periodicChoresIntervalFast) {
        return false;
    }
    if (lhs.periodicChoresIntervalSlow != rhs.periodicChoresIntervalSlow) {
        return false;
    }
    if (lhs.batteryStatusPath != rhs.batteryStatusPath) {
        return false;
    }
    if (lhs.batteryHealthPath != rhs.batteryHealthPath) {
        return false;
    }
    if (lhs.batteryPresentPath != rhs.batteryPresentPath) {
        return false;
    }
    if (lhs.batteryCapacityPath != rhs.batteryCapacityPath) {
        return false;
    }
    if (lhs.batteryVoltagePath != rhs.batteryVoltagePath) {
        return false;
    }
    if (lhs.batteryTemperaturePath != rhs.batteryTemperaturePath) {
        return false;
    }
    if (lhs.batteryTechnologyPath != rhs.batteryTechnologyPath) {
        return false;
    }
    if (lhs.batteryCurrentNowPath != rhs.batteryCurrentNowPath) {
        return false;
    }
    if (lhs.batteryCurrentAvgPath != rhs.batteryCurrentAvgPath) {
        return false;
    }
    if (lhs.batteryChargeCounterPath != rhs.batteryChargeCounterPath) {
        return false;
    }
    if (lhs.batteryFullChargePath != rhs.batteryFullChargePath) {
        return false;
    }
    if (lhs.batteryCycleCountPath != rhs.batteryCycleCountPath) {
        return false;
    }
    return true;
}

static inline bool operator!=(const ::android::hardware::health::V1_0::HealthConfig& lhs, const ::android::hardware::health::V1_0::HealthConfig& rhs){
    return !(lhs == rhs);
}

static inline std::string toString(const ::android::hardware::health::V1_0::HealthInfo& o) {
    using ::android::hardware::toString;
    std::string os;
    os += "{";
    os += ".chargerAcOnline = ";
    os += ::android::hardware::toString(o.chargerAcOnline);
    os += ", .chargerUsbOnline = ";
    os += ::android::hardware::toString(o.chargerUsbOnline);
    os += ", .chargerWirelessOnline = ";
    os += ::android::hardware::toString(o.chargerWirelessOnline);
    os += ", .maxChargingCurrent = ";
    os += ::android::hardware::toString(o.maxChargingCurrent);
    os += ", .maxChargingVoltage = ";
    os += ::android::hardware::toString(o.maxChargingVoltage);
    os += ", .batteryStatus = ";
    os += ::android::hardware::health::V1_0::toString(o.batteryStatus);
    os += ", .batteryHealth = ";
    os += ::android::hardware::health::V1_0::toString(o.batteryHealth);
    os += ", .batteryPresent = ";
    os += ::android::hardware::toString(o.batteryPresent);
    os += ", .batteryLevel = ";
    os += ::android::hardware::toString(o.batteryLevel);
    os += ", .batteryVoltage = ";
    os += ::android::hardware::toString(o.batteryVoltage);
    os += ", .batteryTemperature = ";
    os += ::android::hardware::toString(o.batteryTemperature);
    os += ", .batteryCurrent = ";
    os += ::android::hardware::toString(o.batteryCurrent);
    os += ", .batteryCycleCount = ";
    os += ::android::hardware::toString(o.batteryCycleCount);
    os += ", .batteryFullCharge = ";
    os += ::android::hardware::toString(o.batteryFullCharge);
    os += ", .batteryChargeCounter = ";
    os += ::android::hardware::toString(o.batteryChargeCounter);
    os += ", .batteryTechnology = ";
    os += ::android::hardware::toString(o.batteryTechnology);
    os += "}"; return os;
}

static inline bool operator==(const ::android::hardware::health::V1_0::HealthInfo& lhs, const ::android::hardware::health::V1_0::HealthInfo& rhs) {
    if (lhs.chargerAcOnline != rhs.chargerAcOnline) {
        return false;
    }
    if (lhs.chargerUsbOnline != rhs.chargerUsbOnline) {
        return false;
    }
    if (lhs.chargerWirelessOnline != rhs.chargerWirelessOnline) {
        return false;
    }
    if (lhs.maxChargingCurrent != rhs.maxChargingCurrent) {
        return false;
    }
    if (lhs.maxChargingVoltage != rhs.maxChargingVoltage) {
        return false;
    }
    if (lhs.batteryStatus != rhs.batteryStatus) {
        return false;
    }
    if (lhs.batteryHealth != rhs.batteryHealth) {
        return false;
    }
    if (lhs.batteryPresent != rhs.batteryPresent) {
        return false;
    }
    if (lhs.batteryLevel != rhs.batteryLevel) {
        return false;
    }
    if (lhs.batteryVoltage != rhs.batteryVoltage) {
        return false;
    }
    if (lhs.batteryTemperature != rhs.batteryTemperature) {
        return false;
    }
    if (lhs.batteryCurrent != rhs.batteryCurrent) {
        return false;
    }
    if (lhs.batteryCycleCount != rhs.batteryCycleCount) {
        return false;
    }
    if (lhs.batteryFullCharge != rhs.batteryFullCharge) {
        return false;
    }
    if (lhs.batteryChargeCounter != rhs.batteryChargeCounter) {
        return false;
    }
    if (lhs.batteryTechnology != rhs.batteryTechnology) {
        return false;
    }
    return true;
}

static inline bool operator!=(const ::android::hardware::health::V1_0::HealthInfo& lhs, const ::android::hardware::health::V1_0::HealthInfo& rhs){
    return !(lhs == rhs);
}


}  // namespace V1_0
}  // namespace health
}  // namespace hardware
}  // namespace android

//
// global type declarations for package
//

namespace android {
namespace hardware {
namespace details {
template<> constexpr std::array<::android::hardware::health::V1_0::Result, 3> hidl_enum_values<::android::hardware::health::V1_0::Result> = {
    ::android::hardware::health::V1_0::Result::SUCCESS,
    ::android::hardware::health::V1_0::Result::NOT_SUPPORTED,
    ::android::hardware::health::V1_0::Result::UNKNOWN,
};
}  // namespace details
}  // namespace hardware
}  // namespace android

namespace android {
namespace hardware {
namespace details {
template<> constexpr std::array<::android::hardware::health::V1_0::BatteryStatus, 5> hidl_enum_values<::android::hardware::health::V1_0::BatteryStatus> = {
    ::android::hardware::health::V1_0::BatteryStatus::UNKNOWN,
    ::android::hardware::health::V1_0::BatteryStatus::CHARGING,
    ::android::hardware::health::V1_0::BatteryStatus::DISCHARGING,
    ::android::hardware::health::V1_0::BatteryStatus::NOT_CHARGING,
    ::android::hardware::health::V1_0::BatteryStatus::FULL,
};
}  // namespace details
}  // namespace hardware
}  // namespace android

namespace android {
namespace hardware {
namespace details {
template<> constexpr std::array<::android::hardware::health::V1_0::BatteryHealth, 7> hidl_enum_values<::android::hardware::health::V1_0::BatteryHealth> = {
    ::android::hardware::health::V1_0::BatteryHealth::UNKNOWN,
    ::android::hardware::health::V1_0::BatteryHealth::GOOD,
    ::android::hardware::health::V1_0::BatteryHealth::OVERHEAT,
    ::android::hardware::health::V1_0::BatteryHealth::DEAD,
    ::android::hardware::health::V1_0::BatteryHealth::OVER_VOLTAGE,
    ::android::hardware::health::V1_0::BatteryHealth::UNSPECIFIED_FAILURE,
    ::android::hardware::health::V1_0::BatteryHealth::COLD,
};
}  // namespace details
}  // namespace hardware
}  // namespace android


#endif  // HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V1_0_TYPES_H
