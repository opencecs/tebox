#ifndef HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_0_BNHWHEALTHINFOCALLBACK_H
#define HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_0_BNHWHEALTHINFOCALLBACK_H

#include <android/hardware/health/2.0/IHwHealthInfoCallback.h>

namespace android {
namespace hardware {
namespace health {
namespace V2_0 {

struct BnHwHealthInfoCallback : public ::android::hidl::base::V1_0::BnHwBase {
    explicit BnHwHealthInfoCallback(const ::android::sp<IHealthInfoCallback> &_hidl_impl);
    explicit BnHwHealthInfoCallback(const ::android::sp<IHealthInfoCallback> &_hidl_impl, const std::string& HidlInstrumentor_package, const std::string& HidlInstrumentor_interface);

    virtual ~BnHwHealthInfoCallback();

    ::android::status_t onTransact(
            uint32_t _hidl_code,
            const ::android::hardware::Parcel &_hidl_data,
            ::android::hardware::Parcel *_hidl_reply,
            uint32_t _hidl_flags = 0,
            TransactCallback _hidl_cb = nullptr) override;


    /**
     * The pure class is what this class wraps.
     */
    typedef IHealthInfoCallback Pure;

    /**
     * Type tag for use in template logic that indicates this is a 'native' class.
     */
    typedef android::hardware::details::bnhw_tag _hidl_tag;

    ::android::sp<IHealthInfoCallback> getImpl() { return _hidl_mImpl; }
    // Methods from ::android::hardware::health::V2_0::IHealthInfoCallback follow.
    static ::android::status_t _hidl_healthInfoChanged(
            ::android::hidl::base::V1_0::BnHwBase* _hidl_this,
            const ::android::hardware::Parcel &_hidl_data,
            ::android::hardware::Parcel *_hidl_reply,
            TransactCallback _hidl_cb);



private:
    // Methods from ::android::hardware::health::V2_0::IHealthInfoCallback follow.

    // Methods from ::android::hidl::base::V1_0::IBase follow.
    ::android::hardware::Return<void> ping();
    using getDebugInfo_cb = ::android::hidl::base::V1_0::IBase::getDebugInfo_cb;
    ::android::hardware::Return<void> getDebugInfo(getDebugInfo_cb _hidl_cb);

    ::android::sp<IHealthInfoCallback> _hidl_mImpl;
};

}  // namespace V2_0
}  // namespace health
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_0_BNHWHEALTHINFOCALLBACK_H
