#ifndef HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_0_BPHWHEALTHINFOCALLBACK_H
#define HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_0_BPHWHEALTHINFOCALLBACK_H

#include <hidl/HidlTransportSupport.h>

#include <android/hardware/health/2.0/IHwHealthInfoCallback.h>

namespace android {
namespace hardware {
namespace health {
namespace V2_0 {

struct BpHwHealthInfoCallback : public ::android::hardware::BpInterface<IHealthInfoCallback>, public ::android::hardware::details::HidlInstrumentor {
    explicit BpHwHealthInfoCallback(const ::android::sp<::android::hardware::IBinder> &_hidl_impl);

    /**
     * The pure class is what this class wraps.
     */
    typedef IHealthInfoCallback Pure;

    /**
     * Type tag for use in template logic that indicates this is a 'proxy' class.
     */
    typedef android::hardware::details::bphw_tag _hidl_tag;

    virtual bool isRemote() const override { return true; }

    // Methods from ::android::hardware::health::V2_0::IHealthInfoCallback follow.
    static ::android::hardware::Return<void>  _hidl_healthInfoChanged(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, const ::android::hardware::health::V2_0::HealthInfo& info);

    // Methods from ::android::hardware::health::V2_0::IHealthInfoCallback follow.
    ::android::hardware::Return<void> healthInfoChanged(const ::android::hardware::health::V2_0::HealthInfo& info) override;

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

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_0_BPHWHEALTHINFOCALLBACK_H
