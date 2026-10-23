// Soft dual-SIM radio for QEMU:
//   slot1 = T-Mobile US 5G (NR), slot2 = Vodafone DE 4G (LTE).
#include "radio_stubs.h"

#include <aidl/android/hardware/radio/RadioAccessFamily.h>
#include <aidl/android/hardware/radio/RadioIndicationType.h>
#include <aidl/android/hardware/radio/RadioTechnology.h>
#include <aidl/android/hardware/radio/config/PhoneCapability.h>
#include <aidl/android/hardware/radio/config/SimPortInfo.h>
#include <aidl/android/hardware/radio/config/SimSlotStatus.h>
#include <aidl/android/hardware/radio/config/SlotPortMapping.h>
#include <aidl/android/hardware/radio/modem/ImeiInfo.h>
#include <aidl/android/hardware/radio/modem/RadioCapability.h>
#include <aidl/android/hardware/radio/modem/RadioState.h>
#include <aidl/android/hardware/radio/network/CellIdentity.h>
#include <aidl/android/hardware/radio/network/CellIdentityLte.h>
#include <aidl/android/hardware/radio/network/CellIdentityNr.h>
#include <aidl/android/hardware/radio/network/OperatorInfo.h>
#include <aidl/android/hardware/radio/network/RegState.h>
#include <aidl/android/hardware/radio/network/RegStateResult.h>
#include <aidl/android/hardware/radio/network/RegistrationFailCause.h>
#include <aidl/android/hardware/radio/network/SignalStrength.h>
#include <aidl/android/hardware/radio/sim/AppStatus.h>
#include <aidl/android/hardware/radio/sim/CardStatus.h>
#include <aidl/android/hardware/radio/sim/PersoSubstate.h>
#include <aidl/android/hardware/radio/sim/PinState.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/log.h>

#include <limits>
#include <string>

using ::ndk::ScopedAStatus;
namespace radio = ::aidl::android::hardware::radio;
namespace cfg = ::aidl::android::hardware::radio::config;
namespace modem = ::aidl::android::hardware::radio::modem;
namespace net = ::aidl::android::hardware::radio::network;
namespace sim = ::aidl::android::hardware::radio::sim;

static constexpr int kInvalid = std::numeric_limits<int32_t>::max();

static radio::RadioResponseInfo okInfo(int32_t serial) {
    return {radio::RadioResponseType::SOLICITED, serial, radio::RadioError::NONE};
}

enum class RatKind { Nr5g, Lte4g };

struct SlotProfile {
    int slot;
    RatKind rat;
    const char* alphaLong;
    const char* alphaShort;
    const char* mcc;
    const char* mnc;
    const char* plmn;
    const char* iccid;
    const char* imsi;
    const char* imei;
    const char* eid;
};

// Foreign carriers (non-CN): T-Mobile US 5G + Vodafone DE 4G.
static const SlotProfile kSlots[] = {
        {1, RatKind::Nr5g, "T-Mobile", "TMO", "310", "260", "310260",
         "89014103211118510720", "310260123456789", "359250060001001", "89049032000000000001"},
        {2, RatKind::Lte4g, "Vodafone DE", "Voda DE", "262", "02", "26202",
         "8949020000012345678", "262021234567890", "359250060001002", "89049032000000000002"},
};

static const SlotProfile& profileFor(int slot) {
    return slot <= 1 ? kSlots[0] : kSlots[1];
}

static void fillUnavailable(net::SignalStrength* ss) {
    ss->gsm = {kInvalid, kInvalid, kInvalid};
    ss->cdma = {kInvalid, kInvalid};
    ss->evdo = {kInvalid, kInvalid, kInvalid};
    ss->lte = {kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid};
    ss->tdscdma = {kInvalid, kInvalid, kInvalid};
    ss->wcdma = {kInvalid, kInvalid, kInvalid, kInvalid};
    ss->nr = {kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, {}, kInvalid};
}

static net::SignalStrength makeSignal(const SlotProfile& p) {
    net::SignalStrength ss;
    fillUnavailable(&ss);
    if (p.rat == RatKind::Nr5g) {
        ss.nr.ssRsrp = 85;
        ss.nr.ssRsrq = 11;
        ss.nr.ssSinr = 20;
        ss.nr.csiRsrp = 88;
        ss.nr.csiRsrq = 12;
        ss.nr.csiSinr = 18;
        ss.nr.csiCqiTableIndex = 1;
        ss.nr.timingAdvance = 0;
    } else {
        ss.lte.signalStrength = 18;
        ss.lte.rsrp = 90;
        ss.lte.rsrq = 12;
        ss.lte.rssnr = 18;
        ss.lte.cqi = 10;
        ss.lte.timingAdvance = 0;
        ss.lte.cqiTableIndex = 1;
    }
    return ss;
}

static net::OperatorInfo makeOperator(const SlotProfile& p) {
    net::OperatorInfo op;
    op.alphaLong = p.alphaLong;
    op.alphaShort = p.alphaShort;
    op.operatorNumeric = p.plmn;
    op.status = net::OperatorInfo::STATUS_CURRENT;
    return op;
}

static net::RegStateResult makeReg(const SlotProfile& p) {
    net::RegStateResult reg;
    reg.regState = net::RegState::REG_HOME;
    reg.rat = (p.rat == RatKind::Nr5g) ? radio::RadioTechnology::NR : radio::RadioTechnology::LTE;
    reg.reasonForDenial = net::RegistrationFailCause::NONE;
    reg.registeredPlmn = p.plmn;
    if (p.rat == RatKind::Nr5g) {
        net::CellIdentityNr nr;
        nr.mcc = p.mcc;
        nr.mnc = p.mnc;
        nr.nci = 0x0000001001ULL;
        nr.pci = 120;
        nr.tac = 0x1A2B;
        nr.nrarfcn = 636666;
        nr.operatorNames = makeOperator(p);
        reg.cellIdentity.set<net::CellIdentity::nr>(std::move(nr));
    } else {
        net::CellIdentityLte lte;
        lte.mcc = p.mcc;
        lte.mnc = p.mnc;
        lte.ci = 0x00ABCDEF;
        lte.pci = 88;
        lte.tac = 0x0C0D;
        lte.earfcn = 1850;
        lte.operatorNames = makeOperator(p);
        lte.bandwidth = 20000;
        reg.cellIdentity.set<net::CellIdentity::lte>(std::move(lte));
    }
    return reg;
}

static sim::CardStatus makeCard(const SlotProfile& p) {
    sim::AppStatus usim;
    usim.appType = sim::AppStatus::APP_TYPE_USIM;
    usim.appState = sim::AppStatus::APP_STATE_READY;
    usim.persoSubstate = sim::PersoSubstate::READY;
    usim.aidPtr = "A0000000871002";
    usim.appLabelPtr = "USIM";
    usim.pin1Replaced = false;
    usim.pin1 = sim::PinState::DISABLED;
    usim.pin2 = sim::PinState::DISABLED;

    sim::CardStatus card;
    card.cardState = sim::CardStatus::STATE_PRESENT;
    card.universalPinState = sim::PinState::DISABLED;
    card.gsmUmtsSubscriptionAppIndex = 0;
    card.cdmaSubscriptionAppIndex = -1;
    card.imsSubscriptionAppIndex = -1;
    card.applications = {usim};
    card.atr = "";
    card.iccid = p.iccid;
    card.eid = p.eid;
    card.slotMap.physicalSlotId = p.slot - 1;
    card.slotMap.portId = 0;
    return card;
}

static std::vector<cfg::SimSlotStatus> makeSlotStatuses() {
    std::vector<cfg::SimSlotStatus> slots;
    for (const auto& p : kSlots) {
        cfg::SimSlotStatus s;
        s.cardState = sim::CardStatus::STATE_PRESENT;
        s.atr = "";
        s.eid = p.eid;
        cfg::SimPortInfo port;
        port.iccId = p.iccid;
        port.logicalSlotId = p.slot - 1;
        port.portActive = true;
        s.portInfo = {port};
        slots.push_back(std::move(s));
    }
    return slots;
}

class SoftRadioConfig : public QemuRadioConfig {
  public:
    ScopedAStatus setResponseFunctions(
            const std::shared_ptr<cfg::IRadioConfigResponse>& response,
            const std::shared_ptr<cfg::IRadioConfigIndication>& indication) override {
        auto st = QemuRadioConfig::setResponseFunctions(response, indication);
        if (indication_) {
            indication_->simSlotsStatusChanged(radio::RadioIndicationType::UNSOLICITED,
                                               makeSlotStatuses());
        }
        return st;
    }
    ScopedAStatus getNumOfLiveModems(int32_t serial) override {
        if (response_) response_->getNumOfLiveModemsResponse(okInfo(serial), num_modems_);
        return ScopedAStatus::ok();
    }
    ScopedAStatus getPhoneCapability(int32_t serial) override {
        cfg::PhoneCapability cap{};
        cap.maxActiveData = 2;
        cap.maxActiveInternetData = 2;
        cap.isInternetLingeringSupported = false;
        cap.logicalModemIds = {0, 1};
        cap.maxActiveVoice = 2;
        if (response_) response_->getPhoneCapabilityResponse(okInfo(serial), cap);
        return ScopedAStatus::ok();
    }
    ScopedAStatus getSimSlotsStatus(int32_t serial) override {
        if (response_) {
            response_->getSimSlotsStatusResponse(okInfo(serial), makeSlotStatuses());
        }
        return ScopedAStatus::ok();
    }
    ScopedAStatus setSimSlotsMapping(int32_t serial,
                                     const std::vector<cfg::SlotPortMapping>& slotMap) override {
        // Accept any mapping that activates both physical slots; keep default 1:1.
        (void)slotMap;
        if (response_) response_->setSimSlotsMappingResponse(okInfo(serial));
        if (indication_) {
            indication_->simSlotsStatusChanged(radio::RadioIndicationType::UNSOLICITED,
                                               makeSlotStatuses());
        }
        return ScopedAStatus::ok();
    }
    ScopedAStatus getHalDeviceCapabilities(int32_t serial) override {
        if (response_) response_->getHalDeviceCapabilitiesResponse(okInfo(serial), false);
        return ScopedAStatus::ok();
    }
    ScopedAStatus setPreferredDataModem(int32_t serial, int8_t) override {
        if (response_) response_->setPreferredDataModemResponse(okInfo(serial));
        return ScopedAStatus::ok();
    }
    ScopedAStatus setNumOfLiveModems(int32_t serial, int8_t numModems) override {
        if (numModems >= 1 && numModems <= 2) num_modems_ = numModems;
        if (response_) response_->setNumOfLiveModemsResponse(okInfo(serial));
        return ScopedAStatus::ok();
    }

  private:
    int8_t num_modems_ = 2;
};

class SoftRadioModem : public QemuRadioModem {
  public:
    explicit SoftRadioModem(int slot) : slot_(slot), profile_(profileFor(slot)) {}

    ScopedAStatus setResponseFunctions(
            const std::shared_ptr<modem::IRadioModemResponse>& response,
            const std::shared_ptr<modem::IRadioModemIndication>& indication) override {
        auto st = QemuRadioModem::setResponseFunctions(response, indication);
        if (indication_) {
            indication_->rilConnected(radio::RadioIndicationType::UNSOLICITED);
            indication_->radioStateChanged(radio::RadioIndicationType::UNSOLICITED,
                                           powered_ ? modem::RadioState::ON
                                                    : modem::RadioState::OFF);
        }
        return st;
    }
    ScopedAStatus setRadioPower(int32_t serial, bool powerOn, bool, bool) override {
        powered_ = powerOn;
        if (response_) response_->setRadioPowerResponse(okInfo(serial));
        if (indication_) {
            indication_->radioStateChanged(
                    radio::RadioIndicationType::UNSOLICITED,
                    powered_ ? modem::RadioState::ON : modem::RadioState::OFF);
        }
        return ScopedAStatus::ok();
    }
    ScopedAStatus getModemStackStatus(int32_t serial) override {
        if (response_) response_->getModemStackStatusResponse(okInfo(serial), true);
        return ScopedAStatus::ok();
    }
    ScopedAStatus enableModem(int32_t serial, bool on) override {
        powered_ = on;
        if (response_) response_->enableModemResponse(okInfo(serial));
        return ScopedAStatus::ok();
    }
    ScopedAStatus getBasebandVersion(int32_t serial) override {
        if (response_) {
            response_->getBasebandVersionResponse(okInfo(serial), "qemu-soft-radio-1.0");
        }
        return ScopedAStatus::ok();
    }
    ScopedAStatus getImei(int32_t serial) override {
        modem::ImeiInfo info;
        info.type = (slot_ == 1) ? modem::ImeiInfo::ImeiType::PRIMARY
                                 : modem::ImeiInfo::ImeiType::SECONDARY;
        info.imei = profile_.imei;
        info.svn = "01";
        if (response_) response_->getImeiResponse(okInfo(serial), info);
        return ScopedAStatus::ok();
    }
    ScopedAStatus getDeviceIdentity(int32_t serial) override {
        if (response_) {
            response_->getDeviceIdentityResponse(okInfo(serial), profile_.imei, "01", "", "");
        }
        return ScopedAStatus::ok();
    }
    ScopedAStatus getRadioCapability(int32_t serial) override {
        modem::RadioCapability rc;
        rc.session = 0;
        rc.phase = modem::RadioCapability::PHASE_CONFIGURED;
        rc.status = modem::RadioCapability::STATUS_SUCCESS;
        rc.logicalModemUuid = "qemu-modem-" + std::to_string(slot_);
        int32_t raf = static_cast<int32_t>(radio::RadioAccessFamily::LTE) |
                      static_cast<int32_t>(radio::RadioAccessFamily::GSM) |
                      static_cast<int32_t>(radio::RadioAccessFamily::UMTS);
        if (profile_.rat == RatKind::Nr5g) {
            raf |= static_cast<int32_t>(radio::RadioAccessFamily::NR);
        }
        rc.raf = raf;
        if (response_) response_->getRadioCapabilityResponse(okInfo(serial), rc);
        return ScopedAStatus::ok();
    }
    ScopedAStatus sendDeviceState(int32_t serial, modem::DeviceStateType, bool) override {
        if (response_) response_->sendDeviceStateResponse(okInfo(serial));
        return ScopedAStatus::ok();
    }

  private:
    int slot_;
    const SlotProfile& profile_;
    bool powered_ = true;
};

class SoftRadioNetwork : public QemuRadioNetwork {
  public:
    explicit SoftRadioNetwork(int slot) : profile_(profileFor(slot)) {}

    void pushNetworkState() {
        if (!indication_) return;
        auto rat = (profile_.rat == RatKind::Nr5g) ? radio::RadioTechnology::NR
                                                   : radio::RadioTechnology::LTE;
        indication_->networkStateChanged(radio::RadioIndicationType::UNSOLICITED);
        indication_->voiceRadioTechChanged(radio::RadioIndicationType::UNSOLICITED, rat);
        indication_->currentSignalStrength(radio::RadioIndicationType::UNSOLICITED,
                                           makeSignal(profile_));
    }

    ScopedAStatus setResponseFunctions(
            const std::shared_ptr<net::IRadioNetworkResponse>& response,
            const std::shared_ptr<net::IRadioNetworkIndication>& indication) override {
        auto st = QemuRadioNetwork::setResponseFunctions(response, indication);
        pushNetworkState();
        return st;
    }
    ScopedAStatus getSignalStrength(int32_t serial) override {
        if (response_) {
            response_->getSignalStrengthResponse(okInfo(serial), makeSignal(profile_));
        }
        return ScopedAStatus::ok();
    }
    ScopedAStatus getOperator(int32_t serial) override {
        if (response_) {
            response_->getOperatorResponse(okInfo(serial), profile_.alphaLong, profile_.alphaShort,
                                           profile_.plmn);
        }
        return ScopedAStatus::ok();
    }
    ScopedAStatus getVoiceRegistrationState(int32_t serial) override {
        if (response_) {
            response_->getVoiceRegistrationStateResponse(okInfo(serial), makeReg(profile_));
        }
        return ScopedAStatus::ok();
    }
    ScopedAStatus getDataRegistrationState(int32_t serial) override {
        if (response_) {
            response_->getDataRegistrationStateResponse(okInfo(serial), makeReg(profile_));
        }
        return ScopedAStatus::ok();
    }
    ScopedAStatus getVoiceRadioTechnology(int32_t serial) override {
        auto rat = (profile_.rat == RatKind::Nr5g) ? radio::RadioTechnology::NR
                                                   : radio::RadioTechnology::LTE;
        if (response_) response_->getVoiceRadioTechnologyResponse(okInfo(serial), rat);
        return ScopedAStatus::ok();
    }
    ScopedAStatus getNetworkSelectionMode(int32_t serial) override {
        if (response_) response_->getNetworkSelectionModeResponse(okInfo(serial), false);
        return ScopedAStatus::ok();
    }
    ScopedAStatus setIndicationFilter(int32_t serial, int32_t) override {
        if (response_) response_->setIndicationFilterResponse(okInfo(serial));
        return ScopedAStatus::ok();
    }
    ScopedAStatus setCellInfoListRate(int32_t serial, int32_t) override {
        if (response_) response_->setCellInfoListRateResponse(okInfo(serial));
        return ScopedAStatus::ok();
    }
    ScopedAStatus setLocationUpdates(int32_t serial, bool) override {
        if (response_) response_->setLocationUpdatesResponse(okInfo(serial));
        return ScopedAStatus::ok();
    }
    ScopedAStatus setNetworkSelectionModeAutomatic(int32_t serial) override {
        if (response_) response_->setNetworkSelectionModeAutomaticResponse(okInfo(serial));
        return ScopedAStatus::ok();
    }

  private:
    const SlotProfile& profile_;
};

class SoftRadioSim : public QemuRadioSim {
  public:
    explicit SoftRadioSim(int slot) : profile_(profileFor(slot)) {}

    ScopedAStatus setResponseFunctions(
            const std::shared_ptr<sim::IRadioSimResponse>& response,
            const std::shared_ptr<sim::IRadioSimIndication>& indication) override {
        auto st = QemuRadioSim::setResponseFunctions(response, indication);
        if (indication_) {
            indication_->simStatusChanged(radio::RadioIndicationType::UNSOLICITED);
            indication_->subscriptionStatusChanged(radio::RadioIndicationType::UNSOLICITED, true);
            indication_->uiccApplicationsEnablementChanged(radio::RadioIndicationType::UNSOLICITED,
                                                           true);
        }
        return st;
    }
    ScopedAStatus getIccCardStatus(int32_t serial) override {
        if (response_) response_->getIccCardStatusResponse(okInfo(serial), makeCard(profile_));
        return ScopedAStatus::ok();
    }
    ScopedAStatus getImsiForApp(int32_t serial, const std::string&) override {
        if (response_) response_->getImsiForAppResponse(okInfo(serial), profile_.imsi);
        return ScopedAStatus::ok();
    }
    ScopedAStatus setSimCardPower(int32_t serial, sim::CardPowerState) override {
        if (response_) response_->setSimCardPowerResponse(okInfo(serial));
        return ScopedAStatus::ok();
    }
    ScopedAStatus areUiccApplicationsEnabled(int32_t serial) override {
        if (response_) response_->areUiccApplicationsEnabledResponse(okInfo(serial), true);
        return ScopedAStatus::ok();
    }
    ScopedAStatus enableUiccApplications(int32_t serial, bool) override {
        if (response_) response_->enableUiccApplicationsResponse(okInfo(serial));
        return ScopedAStatus::ok();
    }

  private:
    const SlotProfile& profile_;
};

static void addService(const char* instance, const std::shared_ptr<ndk::ICInterface>& svc) {
    auto status = AServiceManager_addService(svc->asBinder().get(), instance);
    __android_log_print(status == STATUS_OK ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR, "qemu-radio",
                        "register %s: %d", instance, status);
    if (status != STATUS_OK) abort();
}

static void publishSlot(int slot) {
    const std::string tag = "slot" + std::to_string(slot);
    const std::string modem = "android.hardware.radio.modem.IRadioModem/" + tag;
    const std::string network = "android.hardware.radio.network.IRadioNetwork/" + tag;
    const std::string simHal = "android.hardware.radio.sim.IRadioSim/" + tag;
    const std::string data = "android.hardware.radio.data.IRadioData/" + tag;
    const std::string messaging = "android.hardware.radio.messaging.IRadioMessaging/" + tag;
    const std::string voice = "android.hardware.radio.voice.IRadioVoice/" + tag;
    addService(modem.c_str(), ndk::SharedRefBase::make<SoftRadioModem>(slot));
    addService(network.c_str(), ndk::SharedRefBase::make<SoftRadioNetwork>(slot));
    addService(simHal.c_str(), ndk::SharedRefBase::make<SoftRadioSim>(slot));
    addService(data.c_str(), ndk::SharedRefBase::make<QemuRadioData>());
    addService(messaging.c_str(), ndk::SharedRefBase::make<QemuRadioMessaging>());
    addService(voice.c_str(), ndk::SharedRefBase::make<QemuRadioVoice>());
}

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(8);
    ABinderProcess_startThreadPool();

    addService("android.hardware.radio.config.IRadioConfig/default",
               ndk::SharedRefBase::make<SoftRadioConfig>());
    publishSlot(1);
    publishSlot(2);

    __android_log_print(ANDROID_LOG_INFO, "qemu-radio",
                        "dual-SIM soft radio ready: slot1=T-Mobile/NR slot2=Vodafone-DE/LTE");
    ABinderProcess_joinThreadPool();
    return 1;
}
