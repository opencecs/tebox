// Soft ISupplicant AIDL v3: STA iface + network stubs so ClientModeManager stays up.
#include "supplicant_stubs.h"

#include <aidl/android/hardware/wifi/supplicant/DebugLevel.h>
#include <aidl/android/hardware/wifi/supplicant/IfaceInfo.h>
#include <aidl/android/hardware/wifi/supplicant/IfaceType.h>
#include <aidl/android/hardware/wifi/supplicant/SupplicantStatusCode.h>
#include <aidl/android/hardware/wifi/supplicant/WpaDriverCapabilitiesMask.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/log.h>

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

using namespace aidl::android::hardware::wifi::supplicant;
using ndk::ScopedAStatus;

namespace {

constexpr const char* kTag = "qemu-supplicant";

ScopedAStatus unsupported() {
    return ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(SupplicantStatusCode::FAILURE_UNSUPPORTED));
}

ScopedAStatus invalidArgs() {
    return ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(SupplicantStatusCode::FAILURE_ARGS_INVALID));
}

ScopedAStatus unknownIface() {
    return ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(SupplicantStatusCode::FAILURE_IFACE_UNKNOWN));
}

}  // namespace

class QemuStaNetwork : public StubBnSupplicantStaNetwork {
  public:
    explicit QemuStaNetwork(int32_t id) : id_(id) {}

    ScopedAStatus getId(int32_t* out) override {
        *out = id_;
        return ScopedAStatus::ok();
    }

  private:
    int32_t id_;
};

class QemuStaIface : public StubBnSupplicantStaIface {
  public:
    explicit QemuStaIface(std::string name) : name_(std::move(name)) {}

    ScopedAStatus getName(std::string* out) override {
        *out = name_;
        return ScopedAStatus::ok();
    }
    ScopedAStatus getType(IfaceType* out) override {
        *out = IfaceType::STA;
        return ScopedAStatus::ok();
    }
    ScopedAStatus getMacAddress(std::vector<uint8_t>* out) override {
        *out = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
        return ScopedAStatus::ok();
    }
    ScopedAStatus registerCallback(
            const std::shared_ptr<ISupplicantStaIfaceCallback>& cb) override {
        callback_ = cb;
        return ScopedAStatus::ok();
    }
    ScopedAStatus getWpaDriverCapabilities(WpaDriverCapabilitiesMask* out) override {
        *out = static_cast<WpaDriverCapabilitiesMask>(0);
        return ScopedAStatus::ok();
    }
    ScopedAStatus addNetwork(std::shared_ptr<ISupplicantStaNetwork>* out) override {
        std::lock_guard lock(mu_);
        const int32_t id = next_network_id_++;
        auto net = ndk::SharedRefBase::make<QemuStaNetwork>(id);
        networks_[id] = net;
        *out = net;
        __android_log_print(ANDROID_LOG_INFO, kTag, "%s addNetwork -> id=%d", name_.c_str(),
                            id);
        return ScopedAStatus::ok();
    }
    ScopedAStatus getNetwork(int32_t id, std::shared_ptr<ISupplicantStaNetwork>* out) override {
        std::lock_guard lock(mu_);
        auto it = networks_.find(id);
        if (it == networks_.end()) {
            out->reset();
            return ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(SupplicantStatusCode::FAILURE_NETWORK_UNKNOWN));
        }
        *out = it->second;
        return ScopedAStatus::ok();
    }
    ScopedAStatus listNetworks(std::vector<int32_t>* out) override {
        std::lock_guard lock(mu_);
        out->clear();
        for (const auto& [id, _] : networks_) out->push_back(id);
        return ScopedAStatus::ok();
    }
    ScopedAStatus removeNetwork(int32_t id) override {
        std::lock_guard lock(mu_);
        networks_.erase(id);
        return ScopedAStatus::ok();
    }
    ScopedAStatus disconnect() override { return ScopedAStatus::ok(); }
    ScopedAStatus reconnect() override { return ScopedAStatus::ok(); }
    ScopedAStatus reassociate() override { return ScopedAStatus::ok(); }
    ScopedAStatus setPowerSave(bool /*enable*/) override { return ScopedAStatus::ok(); }
    ScopedAStatus setBtCoexistenceMode(BtCoexistenceMode /*mode*/) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus setBtCoexistenceScanModeEnabled(bool /*enable*/) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus setSuspendModeEnabled(bool /*enable*/) override { return ScopedAStatus::ok(); }
    ScopedAStatus setCountryCode(const std::vector<uint8_t>& /*code*/) override {
        return ScopedAStatus::ok();
    }

  private:
    std::mutex mu_;
    std::string name_;
    std::shared_ptr<ISupplicantStaIfaceCallback> callback_;
    std::map<int32_t, std::shared_ptr<QemuStaNetwork>> networks_;
    int32_t next_network_id_ = 0;
};

class QemuSupplicant : public StubBnSupplicant {
  public:
    ScopedAStatus addStaInterface(const std::string& ifName,
                                  std::shared_ptr<ISupplicantStaIface>* out) override {
        if (ifName.empty()) {
            out->reset();
            return invalidArgs();
        }
        std::lock_guard lock(mu_);
        auto it = sta_.find(ifName);
        if (it == sta_.end()) {
            auto iface = ndk::SharedRefBase::make<QemuStaIface>(ifName);
            it = sta_.emplace(ifName, iface).first;
            __android_log_print(ANDROID_LOG_INFO, kTag, "addStaInterface %s", ifName.c_str());
        }
        *out = it->second;
        return ScopedAStatus::ok();
    }
    ScopedAStatus getStaInterface(const std::string& ifName,
                                  std::shared_ptr<ISupplicantStaIface>* out) override {
        std::lock_guard lock(mu_);
        auto it = sta_.find(ifName);
        if (it == sta_.end()) {
            out->reset();
            return unknownIface();
        }
        *out = it->second;
        return ScopedAStatus::ok();
    }
    ScopedAStatus listInterfaces(std::vector<IfaceInfo>* out) override {
        std::lock_guard lock(mu_);
        out->clear();
        for (const auto& [name, _] : sta_) {
            IfaceInfo info;
            info.name = name;
            info.type = IfaceType::STA;
            out->push_back(std::move(info));
        }
        return ScopedAStatus::ok();
    }
    ScopedAStatus removeInterface(const IfaceInfo& ifaceInfo) override {
        std::lock_guard lock(mu_);
        if (ifaceInfo.type == IfaceType::STA) {
            sta_.erase(ifaceInfo.name);
            return ScopedAStatus::ok();
        }
        return unsupported();
    }
    ScopedAStatus addP2pInterface(const std::string& /*ifName*/,
                                  std::shared_ptr<ISupplicantP2pIface>* out) override {
        out->reset();
        return unsupported();
    }
    ScopedAStatus getP2pInterface(const std::string& /*ifName*/,
                                  std::shared_ptr<ISupplicantP2pIface>* out) override {
        out->reset();
        return unsupported();
    }
    ScopedAStatus registerCallback(const std::shared_ptr<ISupplicantCallback>& cb) override {
        callback_ = cb;
        return ScopedAStatus::ok();
    }
    ScopedAStatus setDebugParams(DebugLevel level, bool showTimestamp, bool showKeys) override {
        debug_level_ = level;
        show_timestamp_ = showTimestamp;
        show_keys_ = showKeys;
        return ScopedAStatus::ok();
    }
    ScopedAStatus getDebugLevel(DebugLevel* out) override {
        *out = debug_level_;
        return ScopedAStatus::ok();
    }
    ScopedAStatus isDebugShowTimestampEnabled(bool* out) override {
        *out = show_timestamp_;
        return ScopedAStatus::ok();
    }
    ScopedAStatus isDebugShowKeysEnabled(bool* out) override {
        *out = show_keys_;
        return ScopedAStatus::ok();
    }
    ScopedAStatus setConcurrencyPriority(IfaceType /*type*/) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus terminate() override {
        __android_log_print(ANDROID_LOG_INFO, kTag, "terminate()");
        return ScopedAStatus::ok();
    }

  private:
    std::mutex mu_;
    std::map<std::string, std::shared_ptr<QemuStaIface>> sta_;
    std::shared_ptr<ISupplicantCallback> callback_;
    DebugLevel debug_level_ = DebugLevel::INFO;
    bool show_timestamp_ = false;
    bool show_keys_ = false;
};

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(4);
    ABinderProcess_startThreadPool();
    auto service = ndk::SharedRefBase::make<QemuSupplicant>();
    constexpr auto instance = "android.hardware.wifi.supplicant.ISupplicant/default";
    auto status = AServiceManager_addService(service->asBinder().get(), instance);
    __android_log_print(status == STATUS_OK ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR, kTag,
                        "register %s: %d", instance, status);
    if (status != STATUS_OK) return 1;
    ABinderProcess_joinThreadPool();
    return 1;
}
