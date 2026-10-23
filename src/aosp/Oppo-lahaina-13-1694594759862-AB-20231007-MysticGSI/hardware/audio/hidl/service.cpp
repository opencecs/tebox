/*
 * Minimal HIDL audio HAL for the Android 13 Mystic GSI bring-up.
 *
 * The GSI's audioserver (libaudiohal@7.1.so) only has the HIDL audio HAL
 * client: android.hardware.audio@7.1::IDevicesFactory plus
 * android.hardware.audio.effect@7.0::IEffectsFactory. It has no AIDL audio
 * path, so the AIDL stub used by the Android 16 variant is never seen.
 *
 * AudioFlinger's constructor dereferences the result of
 * DevicesFactoryHalInterface::create() without a null check, and
 * DevicesFactoryHalInterface::create() only returns non-null when both HIDL
 * factories are registered with hwservicemanager. Without this service
 * audioserver SIGSEGVs before it can publish media.audio_policy, which
 * system_server waits for during boot.
 *
 * There is no audio hardware in the QEMU bring-up, so every open/create call
 * reports an error and a null result. AudioPolicyManager handles that by
 * failing to open the primary module; audioserver still publishes its
 * services.
 */

#include <android/hardware/audio/7.1/IDevicesFactory.h>
#include <android/hardware/audio/effect/7.0/IEffectsFactory.h>
#include <android/hardware/audio/effect/7.0/types.h>

#include <hidl/HidlTransportSupport.h>
#include <log/log.h>

#include <unistd.h>

namespace {

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::sp;

namespace audio7_0 = ::android::hardware::audio::V7_0;
namespace audio7_1 = ::android::hardware::audio::V7_1;
namespace effect7_0 = ::android::hardware::audio::effect::V7_0;

class DevicesFactory : public audio7_1::IDevicesFactory {
  public:
    Return<void> openDevice(const hidl_string& device, openDevice_cb cb) override {
        ALOGW("openDevice(%s): no audio hardware in this bring-up", device.c_str());
        cb(audio7_0::Result::INVALID_ARGUMENTS, nullptr);
        return Void();
    }

    Return<void> openPrimaryDevice(openPrimaryDevice_cb cb) override {
        ALOGW("openPrimaryDevice: no audio hardware in this bring-up");
        cb(audio7_0::Result::INVALID_ARGUMENTS, nullptr);
        return Void();
    }

    Return<void> openDevice_7_1(const hidl_string& device, openDevice_7_1_cb cb) override {
        ALOGW("openDevice_7_1(%s): no audio hardware in this bring-up", device.c_str());
        cb(audio7_0::Result::INVALID_ARGUMENTS, nullptr);
        return Void();
    }

    Return<void> openPrimaryDevice_7_1(openPrimaryDevice_7_1_cb cb) override {
        ALOGW("openPrimaryDevice_7_1: no audio hardware in this bring-up");
        cb(audio7_0::Result::INVALID_ARGUMENTS, nullptr);
        return Void();
    }
};

class EffectsFactory : public effect7_0::IEffectsFactory {
  public:
    Return<void> getAllDescriptors(getAllDescriptors_cb cb) override {
        cb(effect7_0::Result::NOT_SUPPORTED, hidl_vec<effect7_0::EffectDescriptor>());
        return Void();
    }

    Return<void> getDescriptor(const ::android::hardware::audio::common::V7_0::Uuid& uid,
                               getDescriptor_cb cb) override {
        (void)uid;
        cb(effect7_0::Result::INVALID_ARGUMENTS, effect7_0::EffectDescriptor());
        return Void();
    }

    Return<void> createEffect(const ::android::hardware::audio::common::V7_0::Uuid& uid,
                              int32_t session, int32_t ioHandle, int32_t device,
                              createEffect_cb cb) override {
        (void)uid;
        (void)session;
        (void)ioHandle;
        (void)device;
        cb(effect7_0::Result::INVALID_ARGUMENTS, nullptr, 0);
        return Void();
    }
};

}  // namespace

// hwservicemanager may not be up yet when early_hal services start, and
// registerAsService() only succeeds once it is.
template <typename T>
static ::android::status_t registerWithRetry(const sp<T>& service, const char* name) {
    ::android::status_t status = ::android::INVALID_OPERATION;
    for (int i = 0; i < 100; ++i) {
        status = service->registerAsService("default");
        if (status == ::android::OK) return status;
        usleep(100 * 1000);
    }
    ALOGE("cannot register %s: %d", name, status);
    return status;
}

int main() {
    // Two threads: one serves the audio factory, one the effect factory.
    ::android::hardware::configureRpcThreadpool(2, true /* callerWillJoin */);

    sp<audio7_1::IDevicesFactory> devices = new DevicesFactory();
    if (registerWithRetry(devices, "android.hardware.audio@7.1::IDevicesFactory") !=
        ::android::OK) {
        return 1;
    }

    sp<effect7_0::IEffectsFactory> effects = new EffectsFactory();
    if (registerWithRetry(effects, "android.hardware.audio.effect@7.0::IEffectsFactory") !=
        ::android::OK) {
        return 1;
    }

    ALOGI("HIDL audio stubs registered");
    ::android::hardware::joinRpcThreadpool();
    return 0;
}
