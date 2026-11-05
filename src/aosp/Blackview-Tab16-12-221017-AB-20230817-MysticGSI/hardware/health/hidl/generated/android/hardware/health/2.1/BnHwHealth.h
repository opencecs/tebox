#ifndef HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_1_BNHWHEALTH_H
#define HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_1_BNHWHEALTH_H

#include <android/hardware/health/2.1/IHwHealth.h>

namespace android {
namespace hardware {
namespace health {
namespace V2_1 {

struct BnHwHealth : public ::android::hidl::base::V1_0::BnHwBase {
    explicit BnHwHealth(const ::android::sp<IHealth> &_hidl_impl);
    explicit BnHwHealth(const ::android::sp<IHealth> &_hidl_impl, const std::string& HidlInstrumentor_package, const std::string& HidlInstrumentor_interface);

    virtual ~BnHwHealth();

    ::android::status_t onTransact(
            uint32_t _hidl_code,
            const ::android::hardware::Parcel &_hidl_data,
            ::android::hardware::Parcel *_hidl_reply,
            uint32_t _hidl_flags = 0,
            TransactCallback _hidl_cb = nullptr) override;


    /**
     * The pure class is what this class wraps.
     */
    typedef IHealth Pure;

    /**
     * Type tag for use in template logic that indicates this is a 'native' class.
     */
    typedef android::hardware::details::bnhw_tag _hidl_tag;

    ::android::sp<IHealth> getImpl() { return _hidl_mImpl; }
    // Methods from ::android::hardware::health::V2_1::IHealth follow.
    static ::android::status_t _hidl_getHealthConfig(
            ::android::hidl::base::V1_0::BnHwBase* _hidl_this,
            const ::android::hardware::Parcel &_hidl_data,
            ::android::hardware::Parcel *_hidl_reply,
            TransactCallback _hidl_cb);


    static ::android::status_t _hidl_getHealthInfo_2_1(
            ::android::hidl::base::V1_0::BnHwBase* _hidl_this,
            const ::android::hardware::Parcel &_hidl_data,
            ::android::hardware::Parcel *_hidl_reply,
            TransactCallback _hidl_cb);


    static ::android::status_t _hidl_shouldKeepScreenOn(
            ::android::hidl::base::V1_0::BnHwBase* _hidl_this,
            const ::android::hardware::Parcel &_hidl_data,
            ::android::hardware::Parcel *_hidl_reply,
            TransactCallback _hidl_cb);



private:
    // Methods from ::android::hardware::health::V2_0::IHealth follow.

    // Methods from ::android::hardware::health::V2_1::IHealth follow.

    // Methods from ::android::hidl::base::V1_0::IBase follow.
    ::android::hardware::Return<void> ping();
    using getDebugInfo_cb = ::android::hidl::base::V1_0::IBase::getDebugInfo_cb;
    ::android::hardware::Return<void> getDebugInfo(getDebugInfo_cb _hidl_cb);

    ::android::sp<IHealth> _hidl_mImpl;
};

}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_1_BNHWHEALTH_H
