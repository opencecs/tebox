#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_BNHWCOMPOSERCALLBACK_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_BNHWCOMPOSERCALLBACK_H

#include <android/hardware/graphics/composer/2.4/IHwComposerCallback.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_4 {

struct BnHwComposerCallback : public ::android::hidl::base::V1_0::BnHwBase {
    explicit BnHwComposerCallback(const ::android::sp<IComposerCallback> &_hidl_impl);
    explicit BnHwComposerCallback(const ::android::sp<IComposerCallback> &_hidl_impl, const std::string& HidlInstrumentor_package, const std::string& HidlInstrumentor_interface);

    virtual ~BnHwComposerCallback();

    ::android::status_t onTransact(
            uint32_t _hidl_code,
            const ::android::hardware::Parcel &_hidl_data,
            ::android::hardware::Parcel *_hidl_reply,
            uint32_t _hidl_flags = 0,
            TransactCallback _hidl_cb = nullptr) override;


    /**
     * The pure class is what this class wraps.
     */
    typedef IComposerCallback Pure;

    /**
     * Type tag for use in template logic that indicates this is a 'native' class.
     */
    typedef android::hardware::details::bnhw_tag _hidl_tag;

    ::android::sp<IComposerCallback> getImpl() { return _hidl_mImpl; }
    // Methods from ::android::hardware::graphics::composer::V2_4::IComposerCallback follow.
    static ::android::status_t _hidl_onVsync_2_4(
            ::android::hidl::base::V1_0::BnHwBase* _hidl_this,
            const ::android::hardware::Parcel &_hidl_data,
            ::android::hardware::Parcel *_hidl_reply,
            TransactCallback _hidl_cb);


    static ::android::status_t _hidl_onVsyncPeriodTimingChanged(
            ::android::hidl::base::V1_0::BnHwBase* _hidl_this,
            const ::android::hardware::Parcel &_hidl_data,
            ::android::hardware::Parcel *_hidl_reply,
            TransactCallback _hidl_cb);


    static ::android::status_t _hidl_onSeamlessPossible(
            ::android::hidl::base::V1_0::BnHwBase* _hidl_this,
            const ::android::hardware::Parcel &_hidl_data,
            ::android::hardware::Parcel *_hidl_reply,
            TransactCallback _hidl_cb);



private:
    // Methods from ::android::hardware::graphics::composer::V2_1::IComposerCallback follow.

    // Methods from ::android::hardware::graphics::composer::V2_4::IComposerCallback follow.

    // Methods from ::android::hidl::base::V1_0::IBase follow.
    ::android::hardware::Return<void> ping();
    using getDebugInfo_cb = ::android::hidl::base::V1_0::IBase::getDebugInfo_cb;
    ::android::hardware::Return<void> getDebugInfo(getDebugInfo_cb _hidl_cb);

    ::android::sp<IComposerCallback> _hidl_mImpl;
};

}  // namespace V2_4
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_BNHWCOMPOSERCALLBACK_H
