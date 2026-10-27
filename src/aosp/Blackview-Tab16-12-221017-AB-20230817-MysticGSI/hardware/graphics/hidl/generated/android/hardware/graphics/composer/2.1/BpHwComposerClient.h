#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_BPHWCOMPOSERCLIENT_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_BPHWCOMPOSERCLIENT_H

#include <hidl/HidlTransportSupport.h>

#include <android/hardware/graphics/composer/2.1/IHwComposerClient.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {

struct BpHwComposerClient : public ::android::hardware::BpInterface<IComposerClient>, public ::android::hardware::details::HidlInstrumentor {
    explicit BpHwComposerClient(const ::android::sp<::android::hardware::IBinder> &_hidl_impl);

    /**
     * The pure class is what this class wraps.
     */
    typedef IComposerClient Pure;

    /**
     * Type tag for use in template logic that indicates this is a 'proxy' class.
     */
    typedef android::hardware::details::bphw_tag _hidl_tag;

    virtual bool isRemote() const override { return true; }

    // Methods from ::android::hardware::graphics::composer::V2_1::IComposerClient follow.
    static ::android::hardware::Return<void>  _hidl_registerCallback(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, const ::android::sp<::android::hardware::graphics::composer::V2_1::IComposerCallback>& callback);
    static ::android::hardware::Return<uint32_t>  _hidl_getMaxVirtualDisplayCount(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor);
    static ::android::hardware::Return<void>  _hidl_createVirtualDisplay(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint32_t width, uint32_t height, ::android::hardware::graphics::common::V1_0::PixelFormat formatHint, uint32_t outputBufferSlotCount, createVirtualDisplay_cb _hidl_cb);
    static ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error>  _hidl_destroyVirtualDisplay(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display);
    static ::android::hardware::Return<void>  _hidl_createLayer(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, uint32_t bufferSlotCount, createLayer_cb _hidl_cb);
    static ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error>  _hidl_destroyLayer(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, uint64_t layer);
    static ::android::hardware::Return<void>  _hidl_getActiveConfig(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, getActiveConfig_cb _hidl_cb);
    static ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error>  _hidl_getClientTargetSupport(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, uint32_t width, uint32_t height, ::android::hardware::graphics::common::V1_0::PixelFormat format, ::android::hardware::graphics::common::V1_0::Dataspace dataspace);
    static ::android::hardware::Return<void>  _hidl_getColorModes(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, getColorModes_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_getDisplayAttribute(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, uint32_t config, ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute attribute, getDisplayAttribute_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_getDisplayConfigs(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, getDisplayConfigs_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_getDisplayName(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, getDisplayName_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_getDisplayType(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, getDisplayType_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_getDozeSupport(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, getDozeSupport_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_getHdrCapabilities(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, getHdrCapabilities_cb _hidl_cb);
    static ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error>  _hidl_setClientTargetSlotCount(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, uint32_t clientTargetSlotCount);
    static ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error>  _hidl_setActiveConfig(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, uint32_t config);
    static ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error>  _hidl_setColorMode(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, ::android::hardware::graphics::common::V1_0::ColorMode mode);
    static ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error>  _hidl_setPowerMode(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode mode);
    static ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error>  _hidl_setVsyncEnabled(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync enabled);
    static ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error>  _hidl_setInputCommandQueue(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, const ::android::hardware::MQDescriptorSync<uint32_t>& descriptor);
    static ::android::hardware::Return<void>  _hidl_getOutputCommandQueue(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, getOutputCommandQueue_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_executeCommands(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint32_t inLength, const ::android::hardware::hidl_vec<::android::hardware::hidl_handle>& inHandles, executeCommands_cb _hidl_cb);

    // Methods from ::android::hardware::graphics::composer::V2_1::IComposerClient follow.
    ::android::hardware::Return<void> registerCallback(const ::android::sp<::android::hardware::graphics::composer::V2_1::IComposerCallback>& callback) override;
    ::android::hardware::Return<uint32_t> getMaxVirtualDisplayCount() override;
    ::android::hardware::Return<void> createVirtualDisplay(uint32_t width, uint32_t height, ::android::hardware::graphics::common::V1_0::PixelFormat formatHint, uint32_t outputBufferSlotCount, createVirtualDisplay_cb _hidl_cb) override;
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> destroyVirtualDisplay(uint64_t display) override;
    ::android::hardware::Return<void> createLayer(uint64_t display, uint32_t bufferSlotCount, createLayer_cb _hidl_cb) override;
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> destroyLayer(uint64_t display, uint64_t layer) override;
    ::android::hardware::Return<void> getActiveConfig(uint64_t display, getActiveConfig_cb _hidl_cb) override;
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> getClientTargetSupport(uint64_t display, uint32_t width, uint32_t height, ::android::hardware::graphics::common::V1_0::PixelFormat format, ::android::hardware::graphics::common::V1_0::Dataspace dataspace) override;
    ::android::hardware::Return<void> getColorModes(uint64_t display, getColorModes_cb _hidl_cb) override;
    ::android::hardware::Return<void> getDisplayAttribute(uint64_t display, uint32_t config, ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute attribute, getDisplayAttribute_cb _hidl_cb) override;
    ::android::hardware::Return<void> getDisplayConfigs(uint64_t display, getDisplayConfigs_cb _hidl_cb) override;
    ::android::hardware::Return<void> getDisplayName(uint64_t display, getDisplayName_cb _hidl_cb) override;
    ::android::hardware::Return<void> getDisplayType(uint64_t display, getDisplayType_cb _hidl_cb) override;
    ::android::hardware::Return<void> getDozeSupport(uint64_t display, getDozeSupport_cb _hidl_cb) override;
    ::android::hardware::Return<void> getHdrCapabilities(uint64_t display, getHdrCapabilities_cb _hidl_cb) override;
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setClientTargetSlotCount(uint64_t display, uint32_t clientTargetSlotCount) override;
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setActiveConfig(uint64_t display, uint32_t config) override;
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setColorMode(uint64_t display, ::android::hardware::graphics::common::V1_0::ColorMode mode) override;
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setPowerMode(uint64_t display, ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode mode) override;
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setVsyncEnabled(uint64_t display, ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync enabled) override;
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setInputCommandQueue(const ::android::hardware::MQDescriptorSync<uint32_t>& descriptor) override;
    ::android::hardware::Return<void> getOutputCommandQueue(getOutputCommandQueue_cb _hidl_cb) override;
    ::android::hardware::Return<void> executeCommands(uint32_t inLength, const ::android::hardware::hidl_vec<::android::hardware::hidl_handle>& inHandles, executeCommands_cb _hidl_cb) override;

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

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_BPHWCOMPOSERCLIENT_H
