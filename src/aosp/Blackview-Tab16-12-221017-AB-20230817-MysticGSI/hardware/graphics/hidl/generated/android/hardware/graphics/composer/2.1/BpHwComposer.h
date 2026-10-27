#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_BPHWCOMPOSER_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_BPHWCOMPOSER_H

#include <hidl/HidlTransportSupport.h>

#include <android/hardware/graphics/composer/2.1/IHwComposer.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {

struct BpHwComposer : public ::android::hardware::BpInterface<IComposer>, public ::android::hardware::details::HidlInstrumentor {
    explicit BpHwComposer(const ::android::sp<::android::hardware::IBinder> &_hidl_impl);

    /**
     * The pure class is what this class wraps.
     */
    typedef IComposer Pure;

    /**
     * Type tag for use in template logic that indicates this is a 'proxy' class.
     */
    typedef android::hardware::details::bphw_tag _hidl_tag;

    virtual bool isRemote() const override { return true; }

    // Methods from ::android::hardware::graphics::composer::V2_1::IComposer follow.
    static ::android::hardware::Return<void>  _hidl_getCapabilities(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, getCapabilities_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_dumpDebugInfo(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, dumpDebugInfo_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_createClient(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, createClient_cb _hidl_cb);

    // Methods from ::android::hardware::graphics::composer::V2_1::IComposer follow.
    ::android::hardware::Return<void> getCapabilities(getCapabilities_cb _hidl_cb) override;
    ::android::hardware::Return<void> dumpDebugInfo(dumpDebugInfo_cb _hidl_cb) override;
    ::android::hardware::Return<void> createClient(createClient_cb _hidl_cb) override;

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

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_BPHWCOMPOSER_H
