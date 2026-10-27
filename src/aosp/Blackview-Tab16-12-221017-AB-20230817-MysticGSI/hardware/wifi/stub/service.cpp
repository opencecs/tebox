// Soft WiFi AIDL v2: simulated chip + STA (wlan0) + fixed-for-boot fake scan list.
#include "wifi_stubs.h"

#include <aidl/android/hardware/wifi/BnWifi.h>
#include <aidl/android/hardware/wifi/BnWifiChip.h>
#include <aidl/android/hardware/wifi/BnWifiStaIface.h>
#include <aidl/android/hardware/wifi/CachedScanData.h>
#include <aidl/android/hardware/wifi/CachedScanResult.h>
#include <aidl/android/hardware/wifi/IfaceConcurrencyType.h>
#include <aidl/android/hardware/wifi/WifiChannelWidthInMhz.h>
#include <aidl/android/hardware/wifi/WifiRatePreamble.h>
#include <aidl/android/hardware/wifi/WifiStatusCode.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/log.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

using namespace aidl::android::hardware::wifi;
using ndk::ScopedAStatus;

namespace {

constexpr const char* kTag = "qemu-wifi";

struct FakeAp {
    std::string ssid;
    std::array<uint8_t, 6> bssid{};
    int32_t freq_mhz = 2412;
    int32_t rssi_dbm = -55;
};

// Built once when the HAL process starts — stable for the whole boot.
std::vector<FakeAp> gFakeAps;

uint32_t mix32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

void initFakeApsOnce() {
    static std::once_flag once;
    std::call_once(once, [] {
        struct Template {
            const char* fmt;
            unsigned arg_mask;
            uint8_t oui[3];
            int freq;
        };
        const Template templates[] = {
                {"TP-LINK_%04X", 0xffffu, {0x50, 0xC7, 0xBF}, 2412},
                {"TP-LINK_5G_%04X", 0xffffu, {0x50, 0xC7, 0xBF}, 5180},
                {"Xiaomi-%04X", 0xffffu, {0x64, 0xCC, 0x2E}, 2437},
                {"Xiaomi_5G-%04X", 0xffffu, {0x64, 0xCC, 0x2E}, 5745},
                {"ChinaNet-%04X", 0xffffu, {0xD4, 0xEE, 0x07}, 2462},
                {"CMCC-%04X", 0xffffu, {0x00, 0x1E, 0x10}, 2417},
                {"MERCURY_%04X", 0xffffu, {0xD8, 0x15, 0x0D}, 2437},
                {"Tenda_%04X", 0xffffu, {0xC8, 0x3A, 0x35}, 2412},
                {"HUAWEI-%06X", 0xffffffu, {0x48, 0x46, 0xFB}, 5200},
                {"HONOR_5G-%04X", 0xffffu, {0x20, 0xAB, 0x48}, 5220},
                {"ASUS_%04X", 0xffffu, {0x04, 0xD4, 0xC4}, 5785},
                {"NETGEAR%02X", 0xffu, {0xA0, 0x04, 0x60}, 5765},
                {"ChinaUnicom-%04X", 0xffffu, {0xB0, 0x95, 0x8E}, 2462},
                {"Redmi-%04X", 0xffffu, {0x28, 0x6C, 0x07}, 2417},
                {"cu_%04X", 0xffffu, {0x00, 0x1A, 0x2B}, 2437},
                {"DIRECT-%02X-HP DeskJet", 0xffu, {0x98, 0xE7, 0xF4}, 2412},
        };
        auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        uint32_t seed = mix32(static_cast<uint32_t>(now) ^ 0x51a11u);
        gFakeAps.clear();
        const int n = static_cast<int>(sizeof(templates) / sizeof(templates[0]));
        gFakeAps.reserve(static_cast<size_t>(n));
        for (int i = 0; i < n; ++i) {
            seed = mix32(seed + static_cast<uint32_t>(i) * 0x9e3779b9u);
            const Template& t = templates[i];
            FakeAp ap;
            char ssid[64];
            std::snprintf(ssid, sizeof(ssid), t.fmt, seed & t.arg_mask);
            ap.ssid = ssid;
            ap.bssid = {t.oui[0], t.oui[1], t.oui[2],
                        static_cast<uint8_t>((seed >> 16) & 0xff),
                        static_cast<uint8_t>((seed >> 8) & 0xff),
                        static_cast<uint8_t>(seed & 0xff)};
            ap.freq_mhz = t.freq;
            ap.rssi_dbm = -38 - (static_cast<int>(seed) % 45);
            gFakeAps.push_back(ap);
        }
        __android_log_print(ANDROID_LOG_INFO, kTag, "fixed fake SSID catalog: %zu entries",
                            gFakeAps.size());
    });
}

CachedScanData makeCachedScanData() {
    initFakeApsOnce();
    CachedScanData data;
    data.scannedFrequenciesMhz = {2412, 2437, 2462, 5180, 5200, 5745};
    auto ts = std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::steady_clock::now().time_since_epoch())
                      .count();
    for (const auto& ap : gFakeAps) {
        CachedScanResult r;
        r.ssid.assign(ap.ssid.begin(), ap.ssid.end());
        r.bssid = ap.bssid;
        r.rssiDbm = ap.rssi_dbm;
        r.frequencyMhz = ap.freq_mhz;
        r.timeStampInUs = ts;
        r.channelWidthMhz = WifiChannelWidthInMhz::WIDTH_20;
        r.preambleType = WifiRatePreamble::OFDM;
        data.cachedScanResults.push_back(std::move(r));
    }
    return data;
}

}  // namespace

class QemuStaIface : public StubBnWifiStaIface {
  public:
    explicit QemuStaIface(std::string name) : name_(std::move(name)) { initFakeApsOnce(); }

    ScopedAStatus getName(std::string* out) override {
        *out = name_;
        return ScopedAStatus::ok();
    }
    ScopedAStatus getFeatureSet(int32_t* out) override {
        *out = static_cast<int32_t>(IWifiStaIface::FeatureSetMask::CACHED_SCAN_DATA) |
               static_cast<int32_t>(IWifiStaIface::FeatureSetMask::STA_5G) |
               static_cast<int32_t>(IWifiStaIface::FeatureSetMask::SCAN_RAND);
        return ScopedAStatus::ok();
    }
    ScopedAStatus getFactoryMacAddress(std::array<uint8_t, 6>* out) override {
        *out = mac_;
        return ScopedAStatus::ok();
    }
    ScopedAStatus setMacAddress(const std::array<uint8_t, 6>& mac) override {
        mac_ = mac;
        return ScopedAStatus::ok();
    }
    ScopedAStatus getCachedScanData(CachedScanData* out) override {
        *out = makeCachedScanData();
        return ScopedAStatus::ok();
    }
    ScopedAStatus registerEventCallback(
            const std::shared_ptr<IWifiStaIfaceEventCallback>& cb) override {
        callback_ = cb;
        return ScopedAStatus::ok();
    }

  private:
    std::string name_;
    std::array<uint8_t, 6> mac_{{0x02, 0x00, 0x00, 0x00, 0x00, 0x01}};
    std::shared_ptr<IWifiStaIfaceEventCallback> callback_;
};

class QemuWifiChip : public StubBnWifiChip {
  public:
    ScopedAStatus configureChip(int32_t modeId) override {
        mode_ = modeId;
        if (callback_) callback_->onChipReconfigured(modeId);
        return ScopedAStatus::ok();
    }
    ScopedAStatus createStaIface(std::shared_ptr<IWifiStaIface>* out) override {
        std::lock_guard lock(mu_);
        if (!sta_) sta_ = ndk::SharedRefBase::make<QemuStaIface>("wlan0");
        *out = sta_;
        __android_log_print(ANDROID_LOG_INFO, kTag, "createStaIface -> wlan0");
        return ScopedAStatus::ok();
    }
    ScopedAStatus getStaIface(const std::string& ifname,
                              std::shared_ptr<IWifiStaIface>* out) override {
        std::lock_guard lock(mu_);
        if (sta_ && ifname == "wlan0") {
            *out = sta_;
            return ScopedAStatus::ok();
        }
        out->reset();
        return ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(WifiStatusCode::ERROR_INVALID_ARGS));
    }
    ScopedAStatus getStaIfaceNames(std::vector<std::string>* out) override {
        std::lock_guard lock(mu_);
        out->clear();
        if (sta_) out->push_back("wlan0");
        return ScopedAStatus::ok();
    }
    ScopedAStatus removeStaIface(const std::string& ifname) override {
        std::lock_guard lock(mu_);
        if (sta_ && ifname == "wlan0") sta_.reset();
        return ScopedAStatus::ok();
    }
    ScopedAStatus getAvailableModes(std::vector<IWifiChip::ChipMode>* out) override {
        IWifiChip::ChipConcurrencyCombinationLimit limit;
        limit.types = {IfaceConcurrencyType::STA};
        limit.maxIfaces = 1;
        IWifiChip::ChipConcurrencyCombination combo;
        combo.limits = {limit};
        IWifiChip::ChipMode mode;
        mode.id = 0;
        mode.availableCombinations = {combo};
        *out = {mode};
        return ScopedAStatus::ok();
    }
    ScopedAStatus getFeatureSet(int32_t* out) override {
        *out = 0;
        return ScopedAStatus::ok();
    }
    ScopedAStatus getId(int32_t* out) override {
        *out = 0;
        return ScopedAStatus::ok();
    }
    ScopedAStatus getMode(int32_t* out) override {
        *out = mode_;
        return ScopedAStatus::ok();
    }
    ScopedAStatus registerEventCallback(
            const std::shared_ptr<IWifiChipEventCallback>& cb) override {
        callback_ = cb;
        return ScopedAStatus::ok();
    }
    ScopedAStatus setCountryCode(const std::array<uint8_t, 2>& code) override {
        country_ = code;
        return ScopedAStatus::ok();
    }

  private:
    std::mutex mu_;
    int32_t mode_ = 0;
    std::array<uint8_t, 2> country_{{'U', 'S'}};
    std::shared_ptr<QemuStaIface> sta_;
    std::shared_ptr<IWifiChipEventCallback> callback_;
};

class QemuWifi : public StubBnWifi {
  public:
    QemuWifi() : chip_(ndk::SharedRefBase::make<QemuWifiChip>()) { initFakeApsOnce(); }

    ScopedAStatus getChip(int32_t chipId, std::shared_ptr<IWifiChip>* out) override {
        if (chipId != 0) {
            out->reset();
            return ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(WifiStatusCode::ERROR_INVALID_ARGS));
        }
        *out = chip_;
        return ScopedAStatus::ok();
    }
    ScopedAStatus getChipIds(std::vector<int32_t>* out) override {
        *out = {0};
        return ScopedAStatus::ok();
    }
    ScopedAStatus isStarted(bool* out) override {
        *out = started_;
        return ScopedAStatus::ok();
    }
    ScopedAStatus registerEventCallback(const std::shared_ptr<IWifiEventCallback>& cb) override {
        callback_ = cb;
        return ScopedAStatus::ok();
    }
    ScopedAStatus start() override {
        if (!started_) {
            started_ = true;
            if (callback_) callback_->onStart();
            __android_log_print(ANDROID_LOG_INFO, kTag, "IWifi.start()");
        }
        return ScopedAStatus::ok();
    }
    ScopedAStatus stop() override {
        if (started_) {
            started_ = false;
            if (callback_) callback_->onStop();
        }
        return ScopedAStatus::ok();
    }

  private:
    bool started_ = false;
    std::shared_ptr<QemuWifiChip> chip_;
    std::shared_ptr<IWifiEventCallback> callback_;
};

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(4);
    ABinderProcess_startThreadPool();
    auto service = ndk::SharedRefBase::make<QemuWifi>();
    constexpr auto instance = "android.hardware.wifi.IWifi/default";
    auto status = AServiceManager_addService(service->asBinder().get(), instance);
    __android_log_print(status == STATUS_OK ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR, kTag,
                        "register %s: %d (soft chip0/wlan0 + cached scan)", instance, status);
    if (status != STATUS_OK) return 1;
    ABinderProcess_joinThreadPool();
    return 1;
}
