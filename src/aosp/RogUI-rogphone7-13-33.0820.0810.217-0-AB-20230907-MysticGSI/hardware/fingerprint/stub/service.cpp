// Soft Fingerprint AIDL v4: advertise one rear STRONG sensor; no real enroll/auth.
#include <aidl/android/hardware/biometrics/common/BnCancellationSignal.h>
#include <aidl/android/hardware/biometrics/common/OperationContext.h>
#include <aidl/android/hardware/biometrics/common/SensorStrength.h>
#include <aidl/android/hardware/biometrics/fingerprint/BnFingerprint.h>
#include <aidl/android/hardware/biometrics/fingerprint/BnSession.h>
#include <aidl/android/hardware/biometrics/fingerprint/Error.h>
#include <aidl/android/hardware/biometrics/fingerprint/FingerprintSensorType.h>
#include <aidl/android/hardware/biometrics/fingerprint/ISessionCallback.h>
#include <aidl/android/hardware/biometrics/fingerprint/PointerContext.h>
#include <aidl/android/hardware/biometrics/fingerprint/SensorLocation.h>
#include <aidl/android/hardware/biometrics/fingerprint/SensorProps.h>
#include <aidl/android/hardware/keymaster/HardwareAuthToken.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/log.h>

#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

using namespace aidl::android::hardware::biometrics::common;
using namespace aidl::android::hardware::biometrics::fingerprint;
using aidl::android::hardware::keymaster::HardwareAuthToken;
using ndk::ScopedAStatus;

namespace {

constexpr const char* kTag = "qemu-fp";

class QemuCancellationSignal : public BnCancellationSignal {
  public:
    ScopedAStatus cancel() override { return ScopedAStatus::ok(); }
};

class QemuSession : public BnSession {
  public:
    explicit QemuSession(std::shared_ptr<ISessionCallback> cb) : cb_(std::move(cb)) {}

    ScopedAStatus generateChallenge() override {
        auto cb = cb_;
        if (cb) {
            std::thread([cb]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                cb->onChallengeGenerated(0x51454d55ULL);  // "QEMU"
            }).detach();
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus revokeChallenge(int64_t challenge) override {
        auto cb = cb_;
        if (cb) {
            std::thread([cb, challenge]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                cb->onChallengeRevoked(challenge);
            }).detach();
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus enroll(const HardwareAuthToken&,
                         std::shared_ptr<ICancellationSignal>* out) override {
        return startFailingOp(out, Error::HW_UNAVAILABLE);
    }

    ScopedAStatus authenticate(int64_t,
                               std::shared_ptr<ICancellationSignal>* out) override {
        return startAuthFail(out);
    }

    ScopedAStatus detectInteraction(std::shared_ptr<ICancellationSignal>* out) override {
        return startFailingOp(out, Error::UNABLE_TO_PROCESS);
    }

    ScopedAStatus enumerateEnrollments() override {
        auto cb = cb_;
        if (cb) {
            std::thread([cb]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                cb->onEnrollmentsEnumerated({});
            }).detach();
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus removeEnrollments(const std::vector<int32_t>& enrollmentIds) override {
        auto cb = cb_;
        if (cb) {
            std::thread([cb, enrollmentIds]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                cb->onEnrollmentsRemoved(enrollmentIds);
            }).detach();
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus getAuthenticatorId() override {
        auto cb = cb_;
        if (cb) {
            std::thread([cb]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                cb->onAuthenticatorIdRetrieved(0);
            }).detach();
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus invalidateAuthenticatorId() override {
        auto cb = cb_;
        if (cb) {
            std::thread([cb]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                cb->onAuthenticatorIdInvalidated(0);
            }).detach();
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus resetLockout(const HardwareAuthToken&) override {
        auto cb = cb_;
        if (cb) {
            std::thread([cb]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                cb->onLockoutCleared();
            }).detach();
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus close() override {
        auto cb = cb_;
        if (cb) {
            std::thread([cb]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                cb->onSessionClosed();
            }).detach();
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus onPointerDown(int32_t, int32_t, int32_t, float, float) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus onPointerUp(int32_t) override { return ScopedAStatus::ok(); }
    ScopedAStatus onUiReady() override { return ScopedAStatus::ok(); }

    ScopedAStatus authenticateWithContext(int64_t operationId, const OperationContext&,
                                          std::shared_ptr<ICancellationSignal>* out) override {
        return authenticate(operationId, out);
    }

    ScopedAStatus enrollWithContext(const HardwareAuthToken& hat, const OperationContext&,
                                    std::shared_ptr<ICancellationSignal>* out) override {
        return enroll(hat, out);
    }

    ScopedAStatus detectInteractionWithContext(const OperationContext&,
                                               std::shared_ptr<ICancellationSignal>* out) override {
        return detectInteraction(out);
    }

    ScopedAStatus onPointerDownWithContext(const PointerContext&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus onPointerUpWithContext(const PointerContext&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus onContextChanged(const OperationContext&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus onPointerCancelWithContext(const PointerContext&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus setIgnoreDisplayTouches(bool) override { return ScopedAStatus::ok(); }

  private:
    ScopedAStatus startFailingOp(std::shared_ptr<ICancellationSignal>* out, Error err) {
        *out = ndk::SharedRefBase::make<QemuCancellationSignal>();
        auto cb = cb_;
        if (cb) {
            std::thread([cb, err]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                cb->onError(err, 0);
            }).detach();
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus startAuthFail(std::shared_ptr<ICancellationSignal>* out) {
        *out = ndk::SharedRefBase::make<QemuCancellationSignal>();
        auto cb = cb_;
        if (cb) {
            std::thread([cb]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                cb->onAuthenticationFailed();
            }).detach();
        }
        return ScopedAStatus::ok();
    }

    std::shared_ptr<ISessionCallback> cb_;
};

class QemuFingerprint : public BnFingerprint {
  public:
    ScopedAStatus getSensorProps(std::vector<SensorProps>* _aidl_return) override {
        SensorProps props;
        props.commonProps.sensorId = 0;
        props.commonProps.sensorStrength = SensorStrength::STRONG;
        props.commonProps.maxEnrollmentsPerUser = 5;
        props.sensorType = FingerprintSensorType::REAR;
        props.supportsNavigationGestures = false;
        props.supportsDetectInteraction = false;
        props.halHandlesDisplayTouches = false;
        props.halControlsIllumination = false;

        SensorLocation loc;
        loc.sensorLocationX = 0;
        loc.sensorLocationY = 0;
        loc.sensorRadius = 0;
        loc.display = "";
        props.sensorLocations = {loc};

        *_aidl_return = {props};
        return ScopedAStatus::ok();
    }

    ScopedAStatus createSession(int32_t /*sensorId*/, int32_t /*userId*/,
                                const std::shared_ptr<ISessionCallback>& cb,
                                std::shared_ptr<ISession>* out) override {
        *out = ndk::SharedRefBase::make<QemuSession>(cb);
        __android_log_print(ANDROID_LOG_INFO, kTag, "createSession()");
        return ScopedAStatus::ok();
    }
};

}  // namespace

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(4);
    ABinderProcess_startThreadPool();
    auto service = ndk::SharedRefBase::make<QemuFingerprint>();
    constexpr auto instance = "android.hardware.biometrics.fingerprint.IFingerprint/default";
    auto status = AServiceManager_addService(service->asBinder().get(), instance);
    __android_log_print(status == STATUS_OK ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR, kTag,
                        "register %s: %d", instance, status);
    if (status != STATUS_OK) return 1;
    ABinderProcess_joinThreadPool();
    return 1;
}
