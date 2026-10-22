// Use the software KeyMint implementation shipped with this GSI. The default
// instance emulates the TEE slot required by keystore2; no hardware security.
#include <AndroidKeyMintDevice.h>
#include <AndroidSecureClock.h>
#include <AndroidSharedSecret.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/log.h>
#include <keymaster/logger.h>
#include <cstdint>
#include <cstdlib>

// Exported by libkeymint_fake_latest; its full configuration header pulls in
// legacy hardware/keymaster2.h, which this NDK launcher does not otherwise need.
namespace keymaster {
uint32_t GetOsVersion();
uint32_t GetOsPatchlevel();
uint32_t GetVendorPatchlevel();
}

using namespace aidl::android::hardware::security;

// The GSI library leaves Keymaster's logger unset when embedded in a launcher.
// Preserve its diagnostic errors (especially key blob version checks) in logcat.
class KeymasterLogger final : public ::keymaster::Logger {
  public:
    KeymasterLogger() { set_instance(this); }
    int log_msg(LogLevel level, const char* fmt, va_list args) const override {
        const int priorities[] = {ANDROID_LOG_DEBUG, ANDROID_LOG_INFO, ANDROID_LOG_WARN,
                                  ANDROID_LOG_ERROR, ANDROID_LOG_ERROR};
        return __android_log_vprint(priorities[level], "qemu-keymint", fmt, args);
    }
};

class QemuKeyMintDevice final : public keymint::AndroidKeyMintDevice {
  public:
    QemuKeyMintDevice() : AndroidKeyMintDevice(keymint::SecurityLevel::TRUSTED_ENVIRONMENT) {}
    ndk::ScopedAStatus begin(keymint::KeyPurpose purpose, const std::vector<uint8_t>& blob,
                            const std::vector<keymint::KeyParameter>& params,
                            const std::optional<keymint::HardwareAuthToken>& auth,
                            keymint::BeginResult* result) override {
        auto status = AndroidKeyMintDevice::begin(purpose, blob, params, auth, result);
        if (!status.isOk()) {
            uint64_t fingerprint = 14695981039346656037ULL;
            for (uint8_t byte : blob) fingerprint = (fingerprint ^ byte) * 1099511628211ULL;
            std::vector<keymint::KeyCharacteristics> characteristics;
            auto parsed = AndroidKeyMintDevice::getKeyCharacteristics(blob, {}, {}, &characteristics);
            __android_log_print(ANDROID_LOG_ERROR, "qemu-keymint",
                                "begin purpose=%d blob-size=%zu format=%u error=%d id=%llx parse=%d",
                                static_cast<int>(purpose), blob.size(),
                                blob.empty() ? 0 : blob[0], status.getServiceSpecificError(),
                                static_cast<unsigned long long>(fingerprint),
                                parsed.getServiceSpecificError());
            for (const auto& param : params) {
                __android_log_print(ANDROID_LOG_ERROR, "qemu-keymint", "begin tag=0x%x",
                                    static_cast<unsigned>(param.tag));
            }
        }
        return status;
    }
};

template<class Interface>
void publish(const std::shared_ptr<Interface>& service) {
    const std::string name = std::string(Interface::descriptor) + "/default";
    auto status = AServiceManager_addService(service->asBinder().get(), name.c_str());
    __android_log_print(status == STATUS_OK ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR,
                        "qemu-keymint", "register %s: %d (software emulation)",
                        name.c_str(), status);
    if (status != STATUS_OK) std::abort();
}

int main() {
    KeymasterLogger logger;
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    __android_log_print(ANDROID_LOG_INFO, "qemu-keymint",
                        "OS version=%u OS patch=%u vendor patch=%u",
                        ::keymaster::GetOsVersion(), ::keymaster::GetOsPatchlevel(),
                        ::keymaster::GetVendorPatchlevel());
    std::shared_ptr<keymint::AndroidKeyMintDevice> device =
            ndk::SharedRefBase::make<QemuKeyMintDevice>();
    publish(device);
    publish(ndk::SharedRefBase::make<secureclock::AndroidSecureClock>(device));
    publish(ndk::SharedRefBase::make<sharedsecret::AndroidSharedSecret>(device));
    ABinderProcess_joinThreadPool();
    return 1;
}
