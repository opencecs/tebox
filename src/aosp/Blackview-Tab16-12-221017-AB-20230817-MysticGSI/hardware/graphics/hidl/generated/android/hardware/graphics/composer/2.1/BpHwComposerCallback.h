#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_BPHWCOMPOSERCALLBACK_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_BPHWCOMPOSERCALLBACK_H

#include <hidl/HidlTransportSupport.h>

#include <android/hardware/graphics/composer/2.1/IHwComposerCallback.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {

struct BpHwComposerCallback : public ::android::hardware::BpInterface<IComposerCallback>, public ::android::hardware::details::HidlInstrumentor {
    explicit BpHwComposerCallback(const ::android::sp<::android::hardware::IBinder> &_hidl_impl);

    /**
     * The pure class is what this class wraps.
     */
    typedef IComposerCallback Pure;

    /**
     * Type tag for use in template logic that indicates this is a 'proxy' class.
     */
    typedef android::hardware::details::bphw_tag _hidl_tag;

    virtual bool isRemote() const override { return true; }

    // Methods from ::android::hardware::graphics::composer::V2_1::IComposerCallback follow.
    static ::android::hardware::Return<void>  _hidl_onHotplug(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, ::android::hardware::graphics::composer::V2_1::IComposerCallback::Connection connected);
    static ::android::hardware::Return<void>  _hidl_onRefresh(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display);
    static ::android::hardware::Return<void>  _hidl_onVsync(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, int64_t timestamp);

    // Methods from ::android::hardware::graphics::composer::V2_1::IComposerCallback follow.
    ::android::hardware::Return<void> onHotplug(uint64_t display, ::android::hardware::graphics::composer::V2_1::IComposerCallback::Connection connected) override;
    ::android::hardware::Return<void> onRefresh(uint64_t display) override;
    ::android::hardware::Return<void> onVsync(uint64_t display, int64_t timestamp) override;

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

}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_BPHWCOMPOSERCALLBACK_H
