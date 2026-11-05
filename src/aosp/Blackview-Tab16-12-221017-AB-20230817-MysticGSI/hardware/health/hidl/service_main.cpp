// Android 12 HIDL health HAL for the TCLUI GSI.
//
// Android 12's system_server BatteryService talks to
// android.hardware.health@2.1::IHealth/default over hwbinder (the AIDL health
// HAL only exists from Android 13). This service reports a static, healthy,
// fully charged battery so BatteryService can start; there is no real fuel
// gauge in the emulator.
#include <android-base/logging.h>
#include <android/hardware/health/2.0/IHealthInfoCallback.h>
#include <android/hardware/health/2.1/IHealth.h>
#include <android/hardware/health/2.1/IHealthInfoCallback.h>
#include <hidl/HidlTransportSupport.h>

#include <mutex>
#include <vector>

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::Return;
using android::hardware::hidl_vec;
using android::hardware::health::V1_0::BatteryHealth;
using android::hardware::health::V1_0::BatteryStatus;
using android::hardware::health::V2_0::HealthInfo;
using android::hardware::health::V2_0::IHealthInfoCallback;
using android::hardware::health::V2_0::Result;
using android::hardware::health::V2_1::BatteryCapacityLevel;
using android::hardware::health::V2_1::IHealth;

namespace {

constexpr int32_t kChargeCounterUah = 4000000;
constexpr int32_t kCapacityPercent = 100;
constexpr int32_t kBatteryVoltageMv = 4200;
constexpr int32_t kBatteryTemperatureTenths = 250;

HealthInfo healthInfo20() {
    HealthInfo info{};
    // V2.0::HealthInfo wraps the V1.0 struct in `legacy`.
    info.legacy.chargerAcOnline = true;
    info.legacy.chargerUsbOnline = false;
    info.legacy.chargerWirelessOnline = false;
    info.legacy.maxChargingCurrent = 3000000;
    info.legacy.maxChargingVoltage = 5000000;
    info.legacy.batteryStatus = BatteryStatus::FULL;
    info.legacy.batteryHealth = BatteryHealth::GOOD;
    info.legacy.batteryPresent = true;
    info.legacy.batteryLevel = kCapacityPercent;
    info.legacy.batteryVoltage = kBatteryVoltageMv;
    info.legacy.batteryTemperature = kBatteryTemperatureTenths;
    info.legacy.batteryCurrent = 0;
    info.legacy.batteryCycleCount = 1;
    info.legacy.batteryFullCharge = kChargeCounterUah;
    info.legacy.batteryChargeCounter = kChargeCounterUah;
    info.legacy.batteryTechnology = "Li-ion";
    return info;
}

android::hardware::health::V2_1::HealthInfo healthInfo21() {
    android::hardware::health::V2_1::HealthInfo info{};
    info.legacy = healthInfo20();
    info.batteryCapacityLevel = BatteryCapacityLevel::FULL;
    // 0 is required when the level is FULL/UNKNOWN.
    info.batteryChargeTimeToFullNowSeconds = 0;
    info.batteryFullChargeDesignCapacityUah = kChargeCounterUah;
    return info;
}

class HealthStub : public IHealth {
  public:
    Return<Result> registerCallback(const android::sp<IHealthInfoCallback>& callback) override {
        if (callback == nullptr) return Result::UNKNOWN;
        std::lock_guard<std::mutex> lock(mutex_);
        callbacks_.push_back(callback);
        return Result::SUCCESS;
    }

    Return<Result> unregisterCallback(const android::sp<IHealthInfoCallback>& callback) override {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = callbacks_.begin(); it != callbacks_.end(); ++it) {
            if (*it == callback) {
                callbacks_.erase(it);
                return Result::SUCCESS;
            }
        }
        return Result::NOT_FOUND;
    }

    Return<Result> update() override {
        std::vector<android::sp<IHealthInfoCallback>> callbacks;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            callbacks = callbacks_;
        }
        const auto info20 = healthInfo20();
        const auto info21 = healthInfo21();
        std::vector<android::sp<IHealthInfoCallback>> dead;
        for (const auto& callback : callbacks) {
            // A 2.1 callback must only ever see healthInfoChanged_2_1().
            auto callback21 =
                android::hardware::health::V2_1::IHealthInfoCallback::castFrom(callback)
                    .withDefault(nullptr);
            Return<void> ret = callback21 != nullptr
                                   ? callback21->healthInfoChanged_2_1(info21)
                                   : callback->healthInfoChanged(info20);
            // Ignoring this status aborts the process ("Failed HIDL return
            // status not checked"): the callback lives in system_server, and a
            // framework restart makes the call fail with DEAD_OBJECT.
            if (!ret.isOk()) dead.push_back(callback);
        }
        if (!dead.empty()) {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& gone : dead) {
                for (auto it = callbacks_.begin(); it != callbacks_.end(); ++it) {
                    if (*it == gone) {
                        callbacks_.erase(it);
                        break;
                    }
                }
            }
        }
        return Result::SUCCESS;
    }

    Return<void> getChargeCounter(getChargeCounter_cb cb) override {
        cb(Result::SUCCESS, kChargeCounterUah);
        return {};
    }

    Return<void> getCurrentNow(getCurrentNow_cb cb) override {
        cb(Result::SUCCESS, 0);
        return {};
    }

    Return<void> getCurrentAverage(getCurrentAverage_cb cb) override {
        cb(Result::SUCCESS, 0);
        return {};
    }

    Return<void> getCapacity(getCapacity_cb cb) override {
        cb(Result::SUCCESS, kCapacityPercent);
        return {};
    }

    Return<void> getEnergyCounter(getEnergyCounter_cb cb) override {
        // 4 Ah * 4.2 V ~= 16.8 Wh, in nWh.
        cb(Result::SUCCESS, static_cast<int64_t>(kChargeCounterUah) * kBatteryVoltageMv);
        return {};
    }

    Return<void> getChargeStatus(getChargeStatus_cb cb) override {
        cb(Result::SUCCESS, BatteryStatus::FULL);
        return {};
    }

    Return<void> getStorageInfo(getStorageInfo_cb cb) override {
        cb(Result::NOT_SUPPORTED, hidl_vec<android::hardware::health::V2_0::StorageInfo>{});
        return {};
    }

    Return<void> getDiskStats(getDiskStats_cb cb) override {
        cb(Result::NOT_SUPPORTED, hidl_vec<android::hardware::health::V2_0::DiskStats>{});
        return {};
    }

    Return<void> getHealthInfo(getHealthInfo_cb cb) override {
        cb(Result::SUCCESS, healthInfo20());
        return {};
    }

    Return<void> getHealthConfig(getHealthConfig_cb cb) override {
        android::hardware::health::V2_1::HealthConfig config{};
        config.battery.periodicChoresIntervalFast = 60;
        config.battery.periodicChoresIntervalSlow = 600;
        config.bootMinCap = 0;
        cb(Result::SUCCESS, config);
        return {};
    }

    Return<void> getHealthInfo_2_1(getHealthInfo_2_1_cb cb) override {
        cb(Result::SUCCESS, healthInfo21());
        return {};
    }

    Return<void> shouldKeepScreenOn(shouldKeepScreenOn_cb cb) override {
        cb(Result::SUCCESS, false);
        return {};
    }

  private:
    std::mutex mutex_;
    std::vector<android::sp<IHealthInfoCallback>> callbacks_;
};

}  // namespace

int main() {
    android::base::SetLogger(android::base::StderrLogger);
    std::fprintf(stderr, "tebox-health: start\n");
    configureRpcThreadpool(4, true /* callerWillJoin */);

    android::sp<IHealth> health = new HealthStub();
    android::status_t status = health->registerAsService("default");
    if (status != android::OK) {
        std::fprintf(stderr, "tebox-health: register failed: %d\n", status);
        return 1;
    }
    std::fprintf(stderr, "tebox-health: android.hardware.health@2.1 registered\n");
    joinRpcThreadpool();
    return 0;
}
