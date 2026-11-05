#include <algorithm>
/*
 * Soft Health HAL stub for GKI+QEMU bring-up (NDK AIDL v3).
 * Always-present AC-powered "battery" so BatteryService / system_server can boot.
 */

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/log.h>

#include <aidl/android/hardware/health/BatteryCapacityLevel.h>
#include <aidl/android/hardware/health/BatteryChargingPolicy.h>
#include <aidl/android/hardware/health/BatteryChargingState.h>
#include <aidl/android/hardware/health/BatteryHealth.h>
#include <aidl/android/hardware/health/BatteryHealthData.h>
#include <aidl/android/hardware/health/BatteryStatus.h>
#include <aidl/android/hardware/health/BnHealth.h>
#include <aidl/android/hardware/health/DiskStats.h>
#include <aidl/android/hardware/health/HealthInfo.h>
#include <aidl/android/hardware/health/IHealthInfoCallback.h>
#include <aidl/android/hardware/health/StorageInfo.h>

#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#define LOG_TAG "health-stub"

#define ALOGI(...)                                                                 \
    do {                                                                           \
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__);               \
        fprintf(stderr, "health-stub I: ");                                        \
        fprintf(stderr, __VA_ARGS__);                                              \
        fprintf(stderr, "\n");                                                     \
    } while (0)
#define ALOGE(...)                                                                 \
    do {                                                                           \
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);              \
        fprintf(stderr, "health-stub E: ");                                        \
        fprintf(stderr, __VA_ARGS__);                                              \
        fprintf(stderr, "\n");                                                     \
    } while (0)

using namespace aidl::android::hardware::health;
using ndk::ScopedAStatus;
using ndk::SharedRefBase;

namespace {

HealthInfo makeInfo() {
    HealthInfo info;
    info.chargerAcOnline = true;
    info.chargerUsbOnline = false;
    info.chargerWirelessOnline = false;
    info.chargerDockOnline = false;
    info.maxChargingCurrentMicroamps = 500000;
    info.maxChargingVoltageMicrovolts = 5000000;
    info.batteryStatus = BatteryStatus::CHARGING;
    info.batteryHealth = BatteryHealth::GOOD;
    info.batteryPresent = true;
    info.batteryLevel = 100;
    info.batteryVoltageMillivolts = 4200;
    info.batteryTemperatureTenthsCelsius = 250;
    info.batteryCurrentMicroamps = 0;
    info.batteryCycleCount = 0;
    info.batteryFullChargeUah = 4000000;
    info.batteryChargeCounterUah = 4000000;
    info.batteryTechnology = "Li-ion";
    info.batteryCurrentAverageMicroamps = 0;
    info.batteryCapacityLevel = BatteryCapacityLevel::FULL;
    info.batteryChargeTimeToFullNowSeconds =
            HealthInfo::BATTERY_CHARGE_TIME_TO_FULL_NOW_SECONDS_UNSUPPORTED;
    info.batteryFullChargeDesignCapacityUah = 4000000;
    info.chargingState = BatteryChargingState::NORMAL;
    info.chargingPolicy = BatteryChargingPolicy::DEFAULT;
    return info;
}

class StubHealth : public BnHealth {
  public:
    ScopedAStatus registerCallback(const std::shared_ptr<IHealthInfoCallback>& cb) override {
        if (!cb) {
            return ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
        }
        ALOGI("registerCallback");
        {
            std::lock_guard<std::mutex> lock(mu_);
            callbacks_.push_back(cb);
        }
        // Push an immediate update so BatteryService does not wait.
        auto st = cb->healthInfoChanged(makeInfo());
        if (!st.isOk()) {
            ALOGE("initial healthInfoChanged failed: %s", st.getDescription().c_str());
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus unregisterCallback(const std::shared_ptr<IHealthInfoCallback>& cb) override {
        if (!cb) {
            return ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
        }
        std::lock_guard<std::mutex> lock(mu_);
        auto it = callbacks_.begin();
        bool found = false;
        while (it != callbacks_.end()) {
            if (*it == cb) {
                it = callbacks_.erase(it);
                found = true;
            } else {
                ++it;
            }
        }
        if (!found) {
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
        ALOGI("unregisterCallback");
        return ScopedAStatus::ok();
    }

    ScopedAStatus update() override {
        ALOGI("update");
        HealthInfo info = makeInfo();
        std::vector<std::shared_ptr<IHealthInfoCallback>> copy;
        {
            std::lock_guard<std::mutex> lock(mu_);
            copy = callbacks_;
        }
        // Like the reference HAL: drop callbacks that fail, but the update
        // itself succeeds (frameworks such as ColorOS do not catch an error
        // from update() during BatteryService start).
        for (auto& cb : copy) {
            auto st = cb->healthInfoChanged(info);
            if (st.isOk()) continue;
            ALOGE("healthInfoChanged failed, dropping callback: %s", st.getDescription().c_str());
            std::lock_guard<std::mutex> lock(mu_);
            callbacks_.erase(std::remove(callbacks_.begin(), callbacks_.end(), cb), callbacks_.end());
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus getChargeCounterUah(int32_t* out) override {
        *out = 4000000;
        return ScopedAStatus::ok();
    }

    ScopedAStatus getCurrentNowMicroamps(int32_t* out) override {
        *out = 0;
        return ScopedAStatus::ok();
    }

    ScopedAStatus getCurrentAverageMicroamps(int32_t* out) override {
        *out = 0;
        return ScopedAStatus::ok();
    }

    ScopedAStatus getCapacity(int32_t* out) override {
        *out = 100;
        return ScopedAStatus::ok();
    }

    ScopedAStatus getEnergyCounterNwh(int64_t* out) override {
        *out = 15000000;  // nanowatt-hours, roughly consistent with µAh*mV
        return ScopedAStatus::ok();
    }

    ScopedAStatus getChargeStatus(BatteryStatus* out) override {
        *out = BatteryStatus::CHARGING;
        return ScopedAStatus::ok();
    }

    ScopedAStatus getStorageInfo(std::vector<StorageInfo>* out) override {
        out->clear();
        return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    ScopedAStatus getDiskStats(std::vector<DiskStats>* out) override {
        out->clear();
        return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    ScopedAStatus getHealthInfo(HealthInfo* out) override {
        *out = makeInfo();
        return ScopedAStatus::ok();
    }

    ScopedAStatus setChargingPolicy(BatteryChargingPolicy value) override {
        ALOGI("setChargingPolicy %d", static_cast<int>(value));
        policy_ = value;
        return ScopedAStatus::ok();
    }

    ScopedAStatus getChargingPolicy(BatteryChargingPolicy* out) override {
        *out = policy_;
        return ScopedAStatus::ok();
    }

    ScopedAStatus getBatteryHealthData(BatteryHealthData* out) override {
        out->batteryManufacturingDateSeconds = 0;
        out->batteryFirstUsageSeconds = 0;
        out->batteryStateOfHealth = 100;
        out->batterySerialNumber = std::nullopt;
        return ScopedAStatus::ok();
    }

  private:
    std::mutex mu_;
    std::vector<std::shared_ptr<IHealthInfoCallback>> callbacks_;
    BatteryChargingPolicy policy_ = BatteryChargingPolicy::DEFAULT;
};

}  // namespace

int main() {
    ALOGI("starting soft Health HAL (IHealth v3)");
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();

    auto svc = SharedRefBase::make<StubHealth>();
    const char* instance = "android.hardware.health.IHealth/default";
    binder_status_t st = AServiceManager_addService(svc->asBinder().get(), instance);
    if (st != STATUS_OK) {
        ALOGE("addService %s failed: %d", instance, st);
        return 1;
    }
    ALOGI("registered %s", instance);
    ABinderProcess_joinThreadPool();
    return 1;
}
