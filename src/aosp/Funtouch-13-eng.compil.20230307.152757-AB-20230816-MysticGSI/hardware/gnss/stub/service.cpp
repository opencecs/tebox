// Soft GNSS AIDL HAL (android.hardware.gnss V2) for QEMU:
// virtual GPS receiver reporting a fixed Beijing fix (39.9042N, 116.4074E)
// at 1 Hz plus fake SV status and NMEA sentences.
#include <aidl/android/hardware/gnss/BnAGnss.h>
#include <aidl/android/hardware/gnss/BnAGnssRil.h>
#include <aidl/android/hardware/gnss/BnGnss.h>
#include <aidl/android/hardware/gnss/BnGnssAntennaInfo.h>
#include <aidl/android/hardware/gnss/BnGnssBatching.h>
#include <aidl/android/hardware/gnss/BnGnssConfiguration.h>
#include <aidl/android/hardware/gnss/BnGnssDebug.h>
#include <aidl/android/hardware/gnss/BnGnssGeofence.h>
#include <aidl/android/hardware/gnss/BnGnssMeasurementInterface.h>
#include <aidl/android/hardware/gnss/BnGnssNavigationMessageInterface.h>
#include <aidl/android/hardware/gnss/BnGnssPowerIndication.h>
#include <aidl/android/hardware/gnss/BnGnssPsds.h>
#include <aidl/android/hardware/gnss/ElapsedRealtime.h>
#include <aidl/android/hardware/gnss/GnssConstellationType.h>
#include <aidl/android/hardware/gnss/GnssLocation.h>
#include <aidl/android/hardware/gnss/GnssSignalType.h>
#include <aidl/android/hardware/gnss/IAGnss.h>
#include <aidl/android/hardware/gnss/IAGnssRil.h>
#include <aidl/android/hardware/gnss/IGnss.h>
#include <aidl/android/hardware/gnss/IGnssCallback.h>
#include <aidl/android/hardware/gnss/IGnssDebug.h>
#include <aidl/android/hardware/gnss/IGnssMeasurementInterface.h>
#include <aidl/android/hardware/gnss/SatellitePvt.h>
#include <aidl/android/hardware/gnss/measurement_corrections/BnMeasurementCorrectionsInterface.h>
#include <aidl/android/hardware/gnss/measurement_corrections/IMeasurementCorrectionsCallback.h>
#include <aidl/android/hardware/gnss/measurement_corrections/MeasurementCorrections.h>
#include <aidl/android/hardware/gnss/visibility_control/BnGnssVisibilityControl.h>
#include <aidl/android/hardware/gnss/visibility_control/IGnssVisibilityControlCallback.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/log.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using ::ndk::ScopedAStatus;
namespace gnss = ::aidl::android::hardware::gnss;
namespace vis = ::aidl::android::hardware::gnss::visibility_control;
namespace corr = ::aidl::android::hardware::gnss::measurement_corrections;

static constexpr const char* kTag = "qemu-gnss";

// Fixed virtual fix: Tiananmen, Beijing.
static constexpr double kLat = 39.9042;
static constexpr double kLon = 116.4074;
static constexpr double kAlt = 43.5;      // meters above WGS84 ellipsoid
static constexpr double kHAcc = 5.0;      // meters
static constexpr double kVAcc = 10.0;
static constexpr double kSpeed = 0.0;     // stationary
static constexpr double kBearing = 0.0;
static constexpr int kFixIntervalMs = 1000;

static int64_t bootMillis() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

static int64_t bootNanos() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

static gnss::GnssLocation makeLocation() {
    gnss::GnssLocation loc;
    loc.gnssLocationFlags = gnss::GnssLocation::HAS_LAT_LONG |
                            gnss::GnssLocation::HAS_ALTITUDE |
                            gnss::GnssLocation::HAS_SPEED |
                            gnss::GnssLocation::HAS_BEARING |
                            gnss::GnssLocation::HAS_HORIZONTAL_ACCURACY |
                            gnss::GnssLocation::HAS_VERTICAL_ACCURACY |
                            gnss::GnssLocation::HAS_SPEED_ACCURACY |
                            gnss::GnssLocation::HAS_BEARING_ACCURACY;
    loc.latitudeDegrees = kLat;
    loc.longitudeDegrees = kLon;
    loc.altitudeMeters = kAlt;
    loc.speedMetersPerSec = kSpeed;
    loc.bearingDegrees = kBearing;
    loc.horizontalAccuracyMeters = kHAcc;
    loc.verticalAccuracyMeters = kVAcc;
    loc.speedAccuracyMetersPerSecond = 0.1;
    loc.bearingAccuracyDegrees = 1.0;
    loc.timestampMillis = bootMillis();
    loc.elapsedRealtime.flags = gnss::ElapsedRealtime::HAS_TIMESTAMP_NS |
                                gnss::ElapsedRealtime::HAS_TIME_UNCERTAINTY_NS;
    loc.elapsedRealtime.timestampNs = bootNanos();
    loc.elapsedRealtime.timeUncertaintyNs = 1.0;
    return loc;
}

static std::vector<gnss::IGnssCallback::GnssSvInfo> makeSvInfos() {
    using C = gnss::GnssConstellationType;
    using F = gnss::IGnssCallback::GnssSvFlags;
    std::vector<gnss::IGnssCallback::GnssSvInfo> svs;
    struct SvDef {
        int svid;
        C constellation;
        float elev;
        float azim;
    };
    const SvDef defs[] = {
            {3, C::GPS, 55.0f, 120.0f},   {7, C::GPS, 35.0f, 210.0f},
            {16, C::GPS, 70.0f, 45.0f},   {23, C::GPS, 20.0f, 300.0f},
            {9, C::GLONASS, 48.0f, 90.0f}, {14, C::GLONASS, 30.0f, 250.0f},
            {6, C::BEIDOU, 60.0f, 160.0f}, {21, C::BEIDOU, 25.0f, 330.0f},
    };
    for (const auto& d : defs) {
        gnss::IGnssCallback::GnssSvInfo sv;
        sv.svid = d.svid;
        sv.constellation = d.constellation;
        sv.cN0Dbhz = 30.0f + (d.elev / 70.0f) * 20.0f;   // 30-50 dB-Hz
        sv.basebandCN0DbHz = sv.cN0Dbhz - 2.0f;
        sv.elevationDegrees = d.elev;
        sv.azimuthDegrees = d.azim;
        sv.carrierFrequencyHz = (d.constellation == C::GLONASS) ? 1602000000LL
                                : (d.constellation == C::BEIDOU) ? 1561098000LL
                                                                 : 1575450000LL;
        sv.svFlag = static_cast<int>(F::HAS_EPHEMERIS_DATA) |
                    static_cast<int>(F::USED_IN_FIX) |
                    static_cast<int>(F::HAS_CARRIER_FREQUENCY);
        svs.push_back(sv);
    }
    return svs;
}

static std::string nmeaChecksum(const char* sentence) {
    int sum = 0;
    for (const char* p = sentence + 1; *p && *p != '*'; ++p) {
        sum ^= static_cast<unsigned char>(*p);
    }
    char out[8];
    std::snprintf(out, sizeof(out), "*%02X\r\n", sum & 0xFF);
    return out;
}

class SoftPsds : public gnss::BnGnssPsds {
  public:
    ScopedAStatus setCallback(const std::shared_ptr<gnss::IGnssPsdsCallback>&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus injectPsdsData(gnss::PsdsType, const std::vector<uint8_t>&) override {
        return ScopedAStatus::ok();
    }
};

class SoftConfig : public gnss::BnGnssConfiguration {
  public:
    ScopedAStatus setSuplVersion(int32_t) override { return ScopedAStatus::ok(); }
    ScopedAStatus setSuplMode(int32_t) override { return ScopedAStatus::ok(); }
    ScopedAStatus setLppProfile(int32_t) override { return ScopedAStatus::ok(); }
    ScopedAStatus setGlonassPositioningProtocol(int32_t) override { return ScopedAStatus::ok(); }
    ScopedAStatus setEmergencySuplPdn(bool) override { return ScopedAStatus::ok(); }
    ScopedAStatus setEsExtensionSec(int32_t) override { return ScopedAStatus::ok(); }
    ScopedAStatus setBlocklist(const std::vector<gnss::BlocklistedSource>&) override {
        return ScopedAStatus::ok();
    }
};

class SoftMeasurement : public gnss::BnGnssMeasurementInterface {
  public:
    ScopedAStatus setCallback(const std::shared_ptr<gnss::IGnssMeasurementCallback>&, bool,
                              bool) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus setCallbackWithOptions(const std::shared_ptr<gnss::IGnssMeasurementCallback>&,
                                         const IGnssMeasurementInterface::Options&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus close() override { return ScopedAStatus::ok(); }
};

class SoftPowerIndication : public gnss::BnGnssPowerIndication {
  public:
    ScopedAStatus setCallback(const std::shared_ptr<gnss::IGnssPowerIndicationCallback>&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus requestGnssPowerStats() override { return ScopedAStatus::ok(); }
};

class SoftBatching : public gnss::BnGnssBatching {
  public:
    ScopedAStatus init(const std::shared_ptr<gnss::IGnssBatchingCallback>&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus getBatchSize(int32_t* out) override {
        *out = 0;
        return ScopedAStatus::ok();
    }
    ScopedAStatus start(const IGnssBatching::Options&) override { return ScopedAStatus::ok(); }
    ScopedAStatus flush() override { return ScopedAStatus::ok(); }
    ScopedAStatus stop() override { return ScopedAStatus::ok(); }
    ScopedAStatus cleanup() override { return ScopedAStatus::ok(); }
};

class SoftGeofence : public gnss::BnGnssGeofence {
  public:
    ScopedAStatus setCallback(const std::shared_ptr<gnss::IGnssGeofenceCallback>&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus addGeofence(int32_t, double, double, double, int32_t, int32_t, int32_t,
                              int32_t) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus pauseGeofence(int32_t) override { return ScopedAStatus::ok(); }
    ScopedAStatus resumeGeofence(int32_t, int32_t) override { return ScopedAStatus::ok(); }
    ScopedAStatus removeGeofence(int32_t) override { return ScopedAStatus::ok(); }
};

class SoftNavigationMessage : public gnss::BnGnssNavigationMessageInterface {
  public:
    ScopedAStatus setCallback(const std::shared_ptr<gnss::IGnssNavigationMessageCallback>&)
            override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus close() override { return ScopedAStatus::ok(); }
};

class SoftAGnss : public gnss::BnAGnss {
  public:
    ScopedAStatus setCallback(const std::shared_ptr<gnss::IAGnssCallback>&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus dataConnClosed() override { return ScopedAStatus::ok(); }
    ScopedAStatus dataConnFailed() override { return ScopedAStatus::ok(); }
    ScopedAStatus setServer(gnss::IAGnssCallback::AGnssType, const std::string&, int32_t) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus dataConnOpen(int64_t, const std::string&, gnss::IAGnss::ApnIpType) override {
        return ScopedAStatus::ok();
    }
};

class SoftAGnssRil : public gnss::BnAGnssRil {
  public:
    ScopedAStatus setCallback(const std::shared_ptr<gnss::IAGnssRilCallback>&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus setRefLocation(const gnss::IAGnssRil::AGnssRefLocation&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus setSetId(gnss::IAGnssRil::SetIdType, const std::string&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus updateNetworkState(const gnss::IAGnssRil::NetworkAttributes&) override {
        return ScopedAStatus::ok();
    }
};

class SoftDebug : public gnss::BnGnssDebug {
  public:
    ScopedAStatus getDebugData(gnss::IGnssDebug::DebugData* out) override {
        gnss::IGnssDebug::DebugData data;
        data.position.valid = true;
        data.position.latitudeDegrees = kLat;
        data.position.longitudeDegrees = kLon;
        data.position.altitudeMeters = static_cast<float>(kAlt);
        data.position.speedMetersPerSec = static_cast<float>(kSpeed);
        data.position.bearingDegrees = static_cast<float>(kBearing);
        data.position.horizontalAccuracyMeters = kHAcc;
        data.position.verticalAccuracyMeters = kVAcc;
        data.position.speedAccuracyMetersPerSecond = 0.1;
        data.position.bearingAccuracyDegrees = 1.0;
        data.position.ageSeconds = 0.0f;
        data.time.timeEstimateMs = bootMillis();
        data.time.timeUncertaintyNs = 100.0f;
        data.time.frequencyUncertaintyNsPerSec = 0.0f;
        for (const auto& sv : makeSvInfos()) {
            gnss::IGnssDebug::SatelliteData sd;
            sd.svid = sv.svid;
            sd.constellation = sv.constellation;
            sd.ephemerisType = gnss::IGnssDebug::SatelliteEphemerisType::EPHEMERIS;
            sd.ephemerisSource = gnss::SatellitePvt::SatelliteEphemerisSource::DEMODULATED;
            sd.ephemerisHealth = gnss::IGnssDebug::SatelliteEphemerisHealth::GOOD;
            sd.ephemerisAgeSeconds = 10.0f;
            sd.serverPredictionIsAvailable = false;
            sd.serverPredictionAgeSeconds = 0.0f;
            data.satelliteDataArray.push_back(sd);
        }
        *out = std::move(data);
        return ScopedAStatus::ok();
    }
};

class SoftVisibilityControl : public vis::BnGnssVisibilityControl {
  public:
    ScopedAStatus enableNfwLocationAccess(const std::vector<std::string>&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus setCallback(
            const std::shared_ptr<vis::IGnssVisibilityControlCallback>&) override {
        return ScopedAStatus::ok();
    }
};

class SoftAntennaInfo : public gnss::BnGnssAntennaInfo {
  public:
    ScopedAStatus setCallback(const std::shared_ptr<gnss::IGnssAntennaInfoCallback>&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus close() override { return ScopedAStatus::ok(); }
};

class SoftMeasurementCorrections : public corr::BnMeasurementCorrectionsInterface {
  public:
    ScopedAStatus setCorrections(const corr::MeasurementCorrections&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus setCallback(
            const std::shared_ptr<corr::IMeasurementCorrectionsCallback>&) override {
        return ScopedAStatus::ok();
    }
};


class SoftGnss : public gnss::BnGnss {
  public:
    SoftGnss() {
        svInfos_ = makeSvInfos();
        signalTypes_ = {
                gnss::GnssSignalType{gnss::GnssConstellationType::GPS, 1575.45e6, "L1"},
                gnss::GnssSignalType{gnss::GnssConstellationType::GLONASS, 1602.0e6, "G1"},
                gnss::GnssSignalType{gnss::GnssConstellationType::BEIDOU, 1561.098e6, "B1"},
        };
    }

    ScopedAStatus setCallback(const std::shared_ptr<gnss::IGnssCallback>& cb) override {
        callback_ = cb;
        if (!callback_) return ScopedAStatus::ok();
        callback_->gnssSetCapabilitiesCb(
                gnss::IGnssCallback::CAPABILITY_SCHEDULING |
                gnss::IGnssCallback::CAPABILITY_MSB |
                gnss::IGnssCallback::CAPABILITY_SINGLE_SHOT |
                gnss::IGnssCallback::CAPABILITY_ON_DEMAND_TIME |
                gnss::IGnssCallback::CAPABILITY_LOW_POWER_MODE);
        callback_->gnssSetSystemInfoCb(gnss::IGnssCallback::GnssSystemInfo{
                2024, "QEMU Virtual GNSS Receiver v1.0"});
        callback_->gnssAcquireWakelockCb();
        if (running_.load()) {
            callback_->gnssStatusCb(gnss::IGnssCallback::GnssStatusValue::SESSION_BEGIN);
        }
        __android_log_print(ANDROID_LOG_INFO, kTag, "framework callback registered");
        return ScopedAStatus::ok();
    }

    ScopedAStatus close() override {
        if (callback_) callback_->gnssReleaseWakelockCb();
        callback_ = nullptr;
        return ScopedAStatus::ok();
    }

    ScopedAStatus start() override {
        bool wasRunning = running_.exchange(true);
        if (!wasRunning && callback_) {
            callback_->gnssStatusCb(gnss::IGnssCallback::GnssStatusValue::SESSION_BEGIN);
        }
        if (!worker_.joinable()) {
            worker_ = std::thread([this] { locationLoop(); });
        }
        __android_log_print(ANDROID_LOG_INFO, kTag, "gnss start (1Hz fix @ %.4f,%.4f)", kLat,
                            kLon);
        return ScopedAStatus::ok();
    }

    ScopedAStatus stop() override {
        running_.store(false);
        if (callback_) {
            callback_->gnssStatusCb(gnss::IGnssCallback::GnssStatusValue::SESSION_END);
        }
        __android_log_print(ANDROID_LOG_INFO, kTag, "gnss stop");
        return ScopedAStatus::ok();
    }

    ScopedAStatus injectTime(int64_t, int64_t, int32_t) override { return ScopedAStatus::ok(); }
    ScopedAStatus injectLocation(const gnss::GnssLocation&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus injectBestLocation(const gnss::GnssLocation&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus deleteAidingData(IGnss::GnssAidingData) override { return ScopedAStatus::ok(); }

    ScopedAStatus setPositionMode(const IGnss::PositionModeOptions& options) override {
        __android_log_print(ANDROID_LOG_INFO, kTag, "positionMode mode=%d interval=%dms",
                            static_cast<int>(options.mode), options.minIntervalMs);
        return ScopedAStatus::ok();
    }

    ScopedAStatus startSvStatus() override { return ScopedAStatus::ok(); }
    ScopedAStatus stopSvStatus() override { return ScopedAStatus::ok(); }

    ScopedAStatus startNmea() override {
        nmea_ = true;
        return ScopedAStatus::ok();
    }
    ScopedAStatus stopNmea() override {
        nmea_ = false;
        return ScopedAStatus::ok();
    }

    ScopedAStatus getExtensionPsds(std::shared_ptr<gnss::IGnssPsds>* out) override {
        *out = ndk::SharedRefBase::make<SoftPsds>();
        return ScopedAStatus::ok();
    }
    ScopedAStatus getExtensionGnssConfiguration(
            std::shared_ptr<gnss::IGnssConfiguration>* out) override {
        *out = ndk::SharedRefBase::make<SoftConfig>();
        return ScopedAStatus::ok();
    }
    ScopedAStatus getExtensionGnssMeasurement(
            std::shared_ptr<gnss::IGnssMeasurementInterface>* out) override {
        *out = ndk::SharedRefBase::make<SoftMeasurement>();
        return ScopedAStatus::ok();
    }
    ScopedAStatus getExtensionGnssPowerIndication(
            std::shared_ptr<gnss::IGnssPowerIndication>* out) override {
        *out = ndk::SharedRefBase::make<SoftPowerIndication>();
        return ScopedAStatus::ok();
    }
    ScopedAStatus getExtensionGnssBatching(std::shared_ptr<gnss::IGnssBatching>* out) override {
        *out = ndk::SharedRefBase::make<SoftBatching>();
        return ScopedAStatus::ok();
    }
    ScopedAStatus getExtensionGnssGeofence(std::shared_ptr<gnss::IGnssGeofence>* out) override {
        *out = ndk::SharedRefBase::make<SoftGeofence>();
        return ScopedAStatus::ok();
    }
    ScopedAStatus getExtensionGnssNavigationMessage(
            std::shared_ptr<gnss::IGnssNavigationMessageInterface>* out) override {
        *out = ndk::SharedRefBase::make<SoftNavigationMessage>();
        return ScopedAStatus::ok();
    }
    ScopedAStatus getExtensionAGnss(std::shared_ptr<gnss::IAGnss>* out) override {
        *out = ndk::SharedRefBase::make<SoftAGnss>();
        return ScopedAStatus::ok();
    }
    ScopedAStatus getExtensionAGnssRil(std::shared_ptr<gnss::IAGnssRil>* out) override {
        *out = ndk::SharedRefBase::make<SoftAGnssRil>();
        return ScopedAStatus::ok();
    }
    ScopedAStatus getExtensionGnssDebug(std::shared_ptr<gnss::IGnssDebug>* out) override {
        *out = ndk::SharedRefBase::make<SoftDebug>();
        return ScopedAStatus::ok();
    }
    ScopedAStatus getExtensionGnssVisibilityControl(
            std::shared_ptr<vis::IGnssVisibilityControl>* out) override {
        *out = ndk::SharedRefBase::make<SoftVisibilityControl>();
        return ScopedAStatus::ok();
    }
    ScopedAStatus getExtensionGnssAntennaInfo(
            std::shared_ptr<gnss::IGnssAntennaInfo>* out) override {
        *out = ndk::SharedRefBase::make<SoftAntennaInfo>();
        return ScopedAStatus::ok();
    }
    ScopedAStatus getExtensionMeasurementCorrections(
            std::shared_ptr<corr::IMeasurementCorrectionsInterface>* out) override {
        *out = ndk::SharedRefBase::make<SoftMeasurementCorrections>();
        return ScopedAStatus::ok();
    }

  private:
    void locationLoop() {
        int tick = 0;
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(kFixIntervalMs));
            if (!running_.load()) continue;
            auto cb = callback_;
            if (!cb) continue;
            cb->gnssLocationCb(makeLocation());
            if (++tick % 5 == 0) {
                cb->gnssSvStatusCb(svInfos_);
            }
            if (nmea_.load()) {
                // GGA with Beijing fix; lat = ddmm.mmmm, lon = dddmm.mmmm
                const char* gga =
                        "$GPGGA,121000.00,3904.2520,N,11624.4440,E,1,08,0.9,43.5,M,0.0,M,,";
                cb->gnssNmeaCb(bootMillis(), std::string(gga) + nmeaChecksum(gga));
            }
        }
    }

    std::shared_ptr<gnss::IGnssCallback> callback_;
    std::atomic<bool> running_{false};
    std::atomic<bool> nmea_{false};
    std::thread worker_;
    std::vector<gnss::IGnssCallback::GnssSvInfo> svInfos_;
    std::vector<gnss::GnssSignalType> signalTypes_;
};

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(2);
    ABinderProcess_startThreadPool();

    auto service = ndk::SharedRefBase::make<SoftGnss>();
    auto status = AServiceManager_addService(service->asBinder().get(),
                                             "android.hardware.gnss.IGnss/default");
    __android_log_print(status == STATUS_OK ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR, kTag,
                        "register android.hardware.gnss.IGnss/default: %d", status);
    if (status != STATUS_OK) abort();

    __android_log_print(ANDROID_LOG_INFO, kTag,
                        "virtual GNSS ready: fixed Beijing fix 39.9042N 116.4074E @1Hz");
    ABinderProcess_joinThreadPool();
    return 1;
}
