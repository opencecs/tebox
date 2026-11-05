// Soft NFC AIDL v2: open/close complete so Settings toggle works; no tags.
#include <aidl/android/hardware/nfc/BnNfc.h>
#include <aidl/android/hardware/nfc/INfcClientCallback.h>
#include <aidl/android/hardware/nfc/NfcCloseType.h>
#include <aidl/android/hardware/nfc/NfcConfig.h>
#include <aidl/android/hardware/nfc/NfcEvent.h>
#include <aidl/android/hardware/nfc/NfcStatus.h>
#include <aidl/android/hardware/nfc/PresenceCheckAlgorithm.h>
#include <aidl/android/hardware/nfc/ProtocolDiscoveryConfig.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/log.h>

#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

using namespace aidl::android::hardware::nfc;
using ndk::ScopedAStatus;

namespace {

constexpr const char* kTag = "qemu-nfc";

class QemuNfc : public BnNfc {
  public:
    ScopedAStatus open(const std::shared_ptr<INfcClientCallback>& clientCallback) override {
        {
            std::lock_guard lock(mu_);
            cb_ = clientCallback;
        }
        __android_log_print(ANDROID_LOG_INFO, kTag, "open()");
        auto cb = clientCallback;
        std::thread([cb]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (cb) cb->sendEvent(NfcEvent::OPEN_CPLT, NfcStatus::OK);
        }).detach();
        return ScopedAStatus::ok();
    }

    ScopedAStatus close(NfcCloseType /*type*/) override {
        std::shared_ptr<INfcClientCallback> cb;
        {
            std::lock_guard lock(mu_);
            cb = cb_;
            cb_.reset();
        }
        __android_log_print(ANDROID_LOG_INFO, kTag, "close()");
        if (cb) cb->sendEvent(NfcEvent::CLOSE_CPLT, NfcStatus::OK);
        return ScopedAStatus::ok();
    }

    ScopedAStatus coreInitialized() override {
        std::shared_ptr<INfcClientCallback> cb;
        {
            std::lock_guard lock(mu_);
            cb = cb_;
        }
        if (cb) {
            std::thread([cb]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                cb->sendEvent(NfcEvent::POST_INIT_CPLT, NfcStatus::OK);
            }).detach();
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus factoryReset() override { return ScopedAStatus::ok(); }

    ScopedAStatus getConfig(NfcConfig* _aidl_return) override {
        NfcConfig cfg{};
        cfg.nfaPollBailOutMode = false;
        cfg.presenceCheckAlgorithm = PresenceCheckAlgorithm::DEFAULT;
        cfg.nfaProprietaryCfg = ProtocolDiscoveryConfig{};
        cfg.defaultOffHostRoute = 0;
        cfg.defaultOffHostRouteFelica = 0;
        cfg.defaultSystemCodeRoute = 0;
        cfg.defaultSystemCodePowerState = 0;
        cfg.defaultRoute = 0;
        cfg.offHostESEPipeId = 0;
        cfg.offHostSIMPipeId = 0;
        cfg.maxIsoDepTransceiveLength = 0xFEFF;
        cfg.hostAllowlist = {};
        cfg.offHostRouteUicc = {};
        cfg.offHostRouteEse = {};
        cfg.defaultIsoDepRoute = 0;
        cfg.offHostSimPipeIds = {};
        cfg.t4tNfceeEnable = false;
        *_aidl_return = cfg;
        return ScopedAStatus::ok();
    }

    ScopedAStatus powerCycle() override { return ScopedAStatus::ok(); }

    ScopedAStatus preDiscover() override {
        std::shared_ptr<INfcClientCallback> cb;
        {
            std::lock_guard lock(mu_);
            cb = cb_;
        }
        if (cb) {
            std::thread([cb]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                cb->sendEvent(NfcEvent::PRE_DISCOVER_CPLT, NfcStatus::OK);
            }).detach();
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus write(const std::vector<uint8_t>& data, int32_t* _aidl_return) override {
        *_aidl_return = static_cast<int32_t>(data.size());
        return ScopedAStatus::ok();
    }

    ScopedAStatus setEnableVerboseLogging(bool enable) override {
        verbose_ = enable;
        return ScopedAStatus::ok();
    }

    ScopedAStatus isVerboseLoggingEnabled(bool* _aidl_return) override {
        *_aidl_return = verbose_;
        return ScopedAStatus::ok();
    }

    ScopedAStatus controlGranted(NfcStatus* _aidl_return) override {
        *_aidl_return = NfcStatus::OK;
        return ScopedAStatus::ok();
    }

  private:
    std::mutex mu_;
    std::shared_ptr<INfcClientCallback> cb_;
    bool verbose_ = false;
};

}  // namespace

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(4);
    ABinderProcess_startThreadPool();
    auto service = ndk::SharedRefBase::make<QemuNfc>();
    constexpr auto instance = "android.hardware.nfc.INfc/default";
    auto status = AServiceManager_addService(service->asBinder().get(), instance);
    __android_log_print(status == STATUS_OK ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR, kTag,
                        "register %s: %d", instance, status);
    if (status != STATUS_OK) return 1;
    ABinderProcess_joinThreadPool();
    return 1;
}
