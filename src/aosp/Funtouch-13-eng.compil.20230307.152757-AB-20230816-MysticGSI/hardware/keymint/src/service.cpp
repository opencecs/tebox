/*
 * Soft KeyMint HAL service.
 * Attestation material: /vendor/etc/keymint/keybox.xml (AndroidAttestation).
 */

#define LOG_TAG "android.hardware.security.keymint-service"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <AndroidKeyMintDevice.h>
#include <AndroidRemotelyProvisionedComponentDevice.h>
#include <AndroidSecureClock.h>
#include <AndroidSharedSecret.h>
#include <keymaster/soft_keymaster_logger.h>

#include "KeyboxStore.h"

using aidl::android::hardware::security::keymint::AndroidKeyMintDevice;
using aidl::android::hardware::security::keymint::AndroidRemotelyProvisionedComponentDevice;
using aidl::android::hardware::security::keymint::SecurityLevel;
using aidl::android::hardware::security::secureclock::AndroidSecureClock;
using aidl::android::hardware::security::sharedsecret::AndroidSharedSecret;

namespace {

constexpr const char* kKeyboxPath = "/vendor/etc/keymint/keybox.xml";

template <typename T, class... Args>
std::shared_ptr<T> addService(Args&&... args) {
    std::shared_ptr<T> ser = ndk::SharedRefBase::make<T>(std::forward<Args>(args)...);
    auto instanceName = std::string(T::descriptor) + "/default";
    LOG(INFO) << "adding keymint service instance: " << instanceName;
    binder_status_t status =
            AServiceManager_addService(ser->asBinder().get(), instanceName.c_str());
    CHECK_EQ(status, STATUS_OK);
    return ser;
}

}  // namespace

int main() {
    keymaster::SoftKeymasterLogger km_logger;

    if (!keymint_vendor::KeyboxStore::get().load(kKeyboxPath)) {
        LOG(WARNING) << "keybox load failed; using built-in software attestation keys";
    }

    ABinderProcess_setThreadPoolMaxThreadCount(0);

    // Match Trusty HAL surface: TEE security level + Trusty naming in getHardwareInfo.
    std::shared_ptr<AndroidKeyMintDevice> keyMint =
            addService<AndroidKeyMintDevice>(SecurityLevel::TRUSTED_ENVIRONMENT);
    addService<AndroidSecureClock>(keyMint);
    addService<AndroidSharedSecret>(keyMint);
    addService<AndroidRemotelyProvisionedComponentDevice>(keyMint);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;
}
