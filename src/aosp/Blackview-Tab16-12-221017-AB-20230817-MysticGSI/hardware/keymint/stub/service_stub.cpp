/*
 * Minimal KeyMint HAL stub for boot bring-up (NDK).
 * Registers IKeyMintDevice / ISecureClock / ISharedSecret.
 * Full soft crypto lives in the Soong tree (hal/keymint/src).
 */

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/log.h>

#include <cstdio>

#define ALOGI(...)                                                                 \
    do {                                                                           \
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__);             \
        fprintf(stderr, "keymint-service I: ");                                    \
        fprintf(stderr, __VA_ARGS__);                                              \
        fprintf(stderr, "\n");                                                     \
    } while (0)
#define ALOGE(...)                                                                 \
    do {                                                                           \
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);            \
        fprintf(stderr, "keymint-service E: ");                                    \
        fprintf(stderr, __VA_ARGS__);                                              \
        fprintf(stderr, "\n");                                                     \
    } while (0)

#include <aidl/android/hardware/security/keymint/BnKeyMintDevice.h>
#include <aidl/android/hardware/security/keymint/ErrorCode.h>
#include <aidl/android/hardware/security/keymint/SecurityLevel.h>
#include <aidl/android/hardware/security/secureclock/BnSecureClock.h>
#include <aidl/android/hardware/security/sharedsecret/BnSharedSecret.h>

#include <array>
#include <chrono>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace aidl::android::hardware::security::keymint;
using namespace aidl::android::hardware::security::secureclock;
using namespace aidl::android::hardware::security::sharedsecret;
using ndk::ScopedAStatus;
using ndk::SharedRefBase;

namespace {

ScopedAStatus kmError(ErrorCode code) {
    return ScopedAStatus::fromServiceSpecificError(static_cast<int32_t>(code));
}

class StubKeyMintDevice : public BnKeyMintDevice {
  public:
    ScopedAStatus getHardwareInfo(KeyMintHardwareInfo* info) override {
        ALOGI("getHardwareInfo");
        info->versionNumber = 4;
        info->securityLevel = SecurityLevel::TRUSTED_ENVIRONMENT;
        info->keyMintName = "TrustyKeyMintDevice";
        info->keyMintAuthorName = "Google";
        info->timestampTokenRequired = false;
        return ScopedAStatus::ok();
    }

    ScopedAStatus addRngEntropy(const std::vector<uint8_t>&) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus generateKey(const std::vector<KeyParameter>&,
                              const std::optional<AttestationKey>&,
                              KeyCreationResult*) override {
        return kmError(ErrorCode::UNIMPLEMENTED);
    }

    ScopedAStatus importKey(const std::vector<KeyParameter>&, KeyFormat,
                            const std::vector<uint8_t>&,
                            const std::optional<AttestationKey>&,
                            KeyCreationResult*) override {
        return kmError(ErrorCode::UNIMPLEMENTED);
    }

    ScopedAStatus importWrappedKey(const std::vector<uint8_t>&, const std::vector<uint8_t>&,
                                   const std::vector<uint8_t>&, const std::vector<KeyParameter>&,
                                   int64_t, int64_t, KeyCreationResult*) override {
        return kmError(ErrorCode::UNIMPLEMENTED);
    }

    ScopedAStatus upgradeKey(const std::vector<uint8_t>& keyBlob,
                             const std::vector<KeyParameter>&,
                             std::vector<uint8_t>* out) override {
        *out = keyBlob;
        return ScopedAStatus::ok();
    }

    ScopedAStatus deleteKey(const std::vector<uint8_t>&) override { return ScopedAStatus::ok(); }
    ScopedAStatus deleteAllKeys() override { return ScopedAStatus::ok(); }
    ScopedAStatus destroyAttestationIds() override { return ScopedAStatus::ok(); }

    ScopedAStatus begin(KeyPurpose, const std::vector<uint8_t>&, const std::vector<KeyParameter>&,
                        const std::optional<HardwareAuthToken>&, BeginResult*) override {
        return kmError(ErrorCode::UNIMPLEMENTED);
    }

    ScopedAStatus deviceLocked(bool, const std::optional<TimeStampToken>&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus earlyBootEnded() override { return ScopedAStatus::ok(); }

    ScopedAStatus convertStorageKeyToEphemeral(const std::vector<uint8_t>&,
                                               std::vector<uint8_t>*) override {
        return kmError(ErrorCode::UNIMPLEMENTED);
    }

    ScopedAStatus getKeyCharacteristics(const std::vector<uint8_t>&, const std::vector<uint8_t>&,
                                        const std::vector<uint8_t>&,
                                        std::vector<KeyCharacteristics>* out) override {
        out->clear();
        return ScopedAStatus::ok();
    }

    ScopedAStatus getRootOfTrustChallenge(std::array<uint8_t, 16>* out) override {
        out->fill(0);
        return ScopedAStatus::ok();
    }

    ScopedAStatus getRootOfTrust(const std::array<uint8_t, 16>&,
                                 std::vector<uint8_t>* out) override {
        out->assign(32, 0);
        return ScopedAStatus::ok();
    }

    ScopedAStatus sendRootOfTrust(const std::vector<uint8_t>&) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus setAdditionalAttestationInfo(const std::vector<KeyParameter>&) override {
        return ScopedAStatus::ok();
    }
};

class StubSecureClock : public BnSecureClock {
  public:
    ScopedAStatus generateTimeStamp(int64_t challenge, TimeStampToken* token) override {
        token->challenge = challenge;
        token->timestamp.milliSeconds =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now().time_since_epoch())
                        .count();
        token->mac.assign(32, 0);
        return ScopedAStatus::ok();
    }
};

class StubSharedSecret : public BnSharedSecret {
  public:
    ScopedAStatus getSharedSecretParameters(SharedSecretParameters* params) override {
        params->seed.assign(32, 0x42);
        params->nonce.assign(32, 0x24);
        return ScopedAStatus::ok();
    }

    ScopedAStatus computeSharedSecret(const std::vector<SharedSecretParameters>&,
                                      std::vector<uint8_t>* out) override {
        out->assign(32, 0);
        return ScopedAStatus::ok();
    }
};

template <typename T>
void addService(const std::shared_ptr<T>& ser) {
    const auto name = std::string(T::descriptor) + "/default";
    const binder_status_t st =
            AServiceManager_addService(ser->asBinder().get(), name.c_str());
    if (st != STATUS_OK) {
        ALOGE("addService %s failed: %d", name.c_str(), st);
        abort();
    }
    ALOGI("registered %s", name.c_str());
}

}  // namespace

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);

    addService(SharedRefBase::make<StubKeyMintDevice>());
    addService(SharedRefBase::make<StubSecureClock>());
    addService(SharedRefBase::make<StubSharedSecret>());

    ABinderProcess_joinThreadPool();
    return 1;
}
