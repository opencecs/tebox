// Minimal Power AIDL v1 backend for QEMU. The host schedules the virtual CPUs;
// no device boost, performance session or headroom capability is advertised.
#include <aidl/android/hardware/power/BnPower.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/log.h>

using namespace aidl::android::hardware::power;
using ndk::ScopedAStatus;

class QemuPower : public BnPower {
  public:
    ScopedAStatus setMode(Mode, bool) override { return ScopedAStatus::ok(); }
    ScopedAStatus isModeSupported(Mode, bool* supported) override {
        *supported = false;
        return ScopedAStatus::ok();
    }
    ScopedAStatus setBoost(Boost, int32_t) override { return ScopedAStatus::ok(); }
    ScopedAStatus isBoostSupported(Boost, bool* supported) override {
        *supported = false;
        return ScopedAStatus::ok();
    }
};

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(2);
    auto service = ndk::SharedRefBase::make<QemuPower>();
    constexpr auto instance = "android.hardware.power.IPower/default";
    auto status = AServiceManager_addService(service->asBinder().get(), instance);
    __android_log_print(status == STATUS_OK ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR,
                        "qemu-power", "register %s: %d", instance, status);
    if (status != STATUS_OK) return 1;
    ABinderProcess_joinThreadPool();
    return 1;
}
