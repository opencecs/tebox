#ifndef HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_0_BPHWHEALTH_H
#define HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_0_BPHWHEALTH_H

#include <hidl/HidlTransportSupport.h>

#include <android/hardware/health/2.0/IHwHealth.h>

namespace android {
namespace hardware {
namespace health {
namespace V2_0 {

struct BpHwHealth : public ::android::hardware::BpInterface<IHealth>, public ::android::hardware::details::HidlInstrumentor {
    explicit BpHwHealth(const ::android::sp<::android::hardware::IBinder> &_hidl_impl);

    /**
     * The pure class is what this class wraps.
     */
    typedef IHealth Pure;

    /**
     * Type tag for use in template logic that indicates this is a 'proxy' class.
     */
    typedef android::hardware::details::bphw_tag _hidl_tag;

    virtual bool isRemote() const override { return true; }

    // Methods from ::android::hardware::health::V2_0::IHealth follow.
    static ::android::hardware::Return<::android::hardware::health::V2_0::Result>  _hidl_registerCallback(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, const ::android::sp<::android::hardware::health::V2_0::IHealthInfoCallback>& callback);
    static ::android::hardware::Return<::android::hardware::health::V2_0::Result>  _hidl_unregisterCallback(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, const ::android::sp<::android::hardware::health::V2_0::IHealthInfoCallback>& callback);
    static ::android::hardware::Return<::android::hardware::health::V2_0::Result>  _hidl_update(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor);
    static ::android::hardware::Return<void>  _hidl_getChargeCounter(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, getChargeCounter_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_getCurrentNow(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, getCurrentNow_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_getCurrentAverage(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, getCurrentAverage_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_getCapacity(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, getCapacity_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_getEnergyCounter(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, getEnergyCounter_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_getChargeStatus(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, getChargeStatus_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_getStorageInfo(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, getStorageInfo_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_getDiskStats(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, getDiskStats_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_getHealthInfo(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, getHealthInfo_cb _hidl_cb);

    // Methods from ::android::hardware::health::V2_0::IHealth follow.
    ::android::hardware::Return<::android::hardware::health::V2_0::Result> registerCallback(const ::android::sp<::android::hardware::health::V2_0::IHealthInfoCallback>& callback) override;
    ::android::hardware::Return<::android::hardware::health::V2_0::Result> unregisterCallback(const ::android::sp<::android::hardware::health::V2_0::IHealthInfoCallback>& callback) override;
    ::android::hardware::Return<::android::hardware::health::V2_0::Result> update() override;
    ::android::hardware::Return<void> getChargeCounter(getChargeCounter_cb _hidl_cb) override;
    ::android::hardware::Return<void> getCurrentNow(getCurrentNow_cb _hidl_cb) override;
    ::android::hardware::Return<void> getCurrentAverage(getCurrentAverage_cb _hidl_cb) override;
    ::android::hardware::Return<void> getCapacity(getCapacity_cb _hidl_cb) override;
    ::android::hardware::Return<void> getEnergyCounter(getEnergyCounter_cb _hidl_cb) override;
    ::android::hardware::Return<void> getChargeStatus(getChargeStatus_cb _hidl_cb) override;
    ::android::hardware::Return<void> getStorageInfo(getStorageInfo_cb _hidl_cb) override;
    ::android::hardware::Return<void> getDiskStats(getDiskStats_cb _hidl_cb) override;
    ::android::hardware::Return<void> getHealthInfo(getHealthInfo_cb _hidl_cb) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.
    ::android::hardware::Return<void> interfaceChain(interfaceChain_cb _hidl_cb) override;
    ::android::hardware::Return<void> debug(const ::android::hardware::hidl_handle& fd, const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& options) override;
    ::android::hardware::Return<void> interfaceDescriptor(interfaceDescriptor_cb _hidl_cb) override;
    ::android::hardware::Return<void> getHashChain(getHashChain_cb _hidl_cb) override;
    ::android::hardware::Return<void> setHALInstrumentation() override;
    ::android::hardware::Return<bool> linkToDeath(const ::android::sp<::android::hardware::hidl_death_recipient>& recipient, uint64_t cookie) override;
    ::android::hardware::Return<void> ping() override;
    ::android::hardware::Return<void> getDebugInfo(getDebugInfo_cb _hidl_cb) override;
    ::android::hardware::Return<void> notifySyspropsChanged() override;
    ::android::hardware::Return<bool> unlinkToDeath(const ::android::sp<::android::hardware::hidl_death_recipient>& recipient) override;

private:
    std::mutex _hidl_mMutex;
    std::vector<::android::sp<::android::hardware::hidl_binder_death_recipient>> _hidl_mDeathRecipients;
};

}  // namespace V2_0
}  // namespace health
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_0_BPHWHEALTH_H
