#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_BPHWCOMPOSERCLIENT_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_BPHWCOMPOSERCLIENT_H

#include <hidl/HidlTransportSupport.h>

#include <android/hardware/graphics/composer/2.4/IHwComposerClient.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_4 {

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

    // Methods from ::android::hardware::graphics::composer::V2_4::IComposerClient follow.
    static ::android::hardware::Return<void>  _hidl_registerCallback_2_4(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, const ::android::sp<::android::hardware::graphics::composer::V2_4::IComposerCallback>& callback);
    static ::android::hardware::Return<void>  _hidl_getDisplayCapabilities_2_4(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, getDisplayCapabilities_2_4_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_getDisplayConnectionType(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, getDisplayConnectionType_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_getDisplayAttribute_2_4(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, uint32_t config, ::android::hardware::graphics::composer::V2_4::IComposerClient::Attribute attribute, getDisplayAttribute_2_4_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_getDisplayVsyncPeriod(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, getDisplayVsyncPeriod_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_setActiveConfigWithConstraints(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, uint32_t config, const ::android::hardware::graphics::composer::V2_4::IComposerClient::VsyncPeriodChangeConstraints& vsyncPeriodChangeConstraints, setActiveConfigWithConstraints_cb _hidl_cb);
    static ::android::hardware::Return<::android::hardware::graphics::composer::V2_4::Error>  _hidl_setAutoLowLatencyMode(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, bool on);
    static ::android::hardware::Return<void>  _hidl_getSupportedContentTypes(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, getSupportedContentTypes_cb _hidl_cb);
    static ::android::hardware::Return<::android::hardware::graphics::composer::V2_4::Error>  _hidl_setContentType(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, uint64_t display, ::android::hardware::graphics::composer::V2_4::IComposerClient::ContentType type);
    static ::android::hardware::Return<void>  _hidl_getLayerGenericMetadataKeys(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, getLayerGenericMetadataKeys_cb _hidl_cb);

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

    // Methods from ::android::hardware::graphics::composer::V2_2::IComposerClient follow.
    ::android::hardware::Return<void> getPerFrameMetadataKeys(uint64_t display, getPerFrameMetadataKeys_cb _hidl_cb) override;
    ::android::hardware::Return<void> getReadbackBufferAttributes(uint64_t display, getReadbackBufferAttributes_cb _hidl_cb) override;
    ::android::hardware::Return<void> getReadbackBufferFence(uint64_t display, getReadbackBufferFence_cb _hidl_cb) override;
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setReadbackBuffer(uint64_t display, const ::android::hardware::hidl_handle& buffer, const ::android::hardware::hidl_handle& releaseFence) override;
    ::android::hardware::Return<void> createVirtualDisplay_2_2(uint32_t width, uint32_t height, ::android::hardware::graphics::common::V1_1::PixelFormat formatHint, uint32_t outputBufferSlotCount, createVirtualDisplay_2_2_cb _hidl_cb) override;
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> getClientTargetSupport_2_2(uint64_t display, uint32_t width, uint32_t height, ::android::hardware::graphics::common::V1_1::PixelFormat format, ::android::hardware::graphics::common::V1_1::Dataspace dataspace) override;
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setPowerMode_2_2(uint64_t display, ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode mode) override;
    ::android::hardware::Return<void> getColorModes_2_2(uint64_t display, getColorModes_2_2_cb _hidl_cb) override;
    ::android::hardware::Return<void> getRenderIntents(uint64_t display, ::android::hardware::graphics::common::V1_1::ColorMode mode, getRenderIntents_cb _hidl_cb) override;
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setColorMode_2_2(uint64_t display, ::android::hardware::graphics::common::V1_1::ColorMode mode, ::android::hardware::graphics::common::V1_1::RenderIntent intent) override;
    ::android::hardware::Return<void> getDataspaceSaturationMatrix(::android::hardware::graphics::common::V1_1::Dataspace dataspace, getDataspaceSaturationMatrix_cb _hidl_cb) override;
    ::android::hardware::Return<void> executeCommands_2_2(uint32_t inLength, const ::android::hardware::hidl_vec<::android::hardware::hidl_handle>& inHandles, executeCommands_2_2_cb _hidl_cb) override;

    // Methods from ::android::hardware::graphics::composer::V2_3::IComposerClient follow.
    ::android::hardware::Return<void> getDisplayIdentificationData(uint64_t display, getDisplayIdentificationData_cb _hidl_cb) override;
    ::android::hardware::Return<void> getReadbackBufferAttributes_2_3(uint64_t display, getReadbackBufferAttributes_2_3_cb _hidl_cb) override;
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> getClientTargetSupport_2_3(uint64_t display, uint32_t width, uint32_t height, ::android::hardware::graphics::common::V1_2::PixelFormat format, ::android::hardware::graphics::common::V1_2::Dataspace dataspace) override;
    ::android::hardware::Return<void> getDisplayedContentSamplingAttributes(uint64_t display, getDisplayedContentSamplingAttributes_cb _hidl_cb) override;
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setDisplayedContentSamplingEnabled(uint64_t display, ::android::hardware::graphics::composer::V2_3::IComposerClient::DisplayedContentSampling enable, ::android::hardware::hidl_bitfield<::android::hardware::graphics::composer::V2_3::IComposerClient::FormatColorComponent> componentMask, uint64_t maxFrames) override;
    ::android::hardware::Return<void> getDisplayedContentSample(uint64_t display, uint64_t maxFrames, uint64_t timestamp, getDisplayedContentSample_cb _hidl_cb) override;
    ::android::hardware::Return<void> executeCommands_2_3(uint32_t inLength, const ::android::hardware::hidl_vec<::android::hardware::hidl_handle>& inHandles, executeCommands_2_3_cb _hidl_cb) override;
    ::android::hardware::Return<void> getRenderIntents_2_3(uint64_t display, ::android::hardware::graphics::common::V1_2::ColorMode mode, getRenderIntents_2_3_cb _hidl_cb) override;
    ::android::hardware::Return<void> getColorModes_2_3(uint64_t display, getColorModes_2_3_cb _hidl_cb) override;
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setColorMode_2_3(uint64_t display, ::android::hardware::graphics::common::V1_2::ColorMode mode, ::android::hardware::graphics::common::V1_1::RenderIntent intent) override;
    ::android::hardware::Return<void> getDisplayCapabilities(uint64_t display, getDisplayCapabilities_cb _hidl_cb) override;
    ::android::hardware::Return<void> getPerFrameMetadataKeys_2_3(uint64_t display, getPerFrameMetadataKeys_2_3_cb _hidl_cb) override;
    ::android::hardware::Return<void> getHdrCapabilities_2_3(uint64_t display, getHdrCapabilities_2_3_cb _hidl_cb) override;
    ::android::hardware::Return<void> getDisplayBrightnessSupport(uint64_t display, getDisplayBrightnessSupport_cb _hidl_cb) override;
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setDisplayBrightness(uint64_t display, float brightness) override;

    // Methods from ::android::hardware::graphics::composer::V2_4::IComposerClient follow.
    ::android::hardware::Return<void> registerCallback_2_4(const ::android::sp<::android::hardware::graphics::composer::V2_4::IComposerCallback>& callback) override;
    ::android::hardware::Return<void> getDisplayCapabilities_2_4(uint64_t display, getDisplayCapabilities_2_4_cb _hidl_cb) override;
    ::android::hardware::Return<void> getDisplayConnectionType(uint64_t display, getDisplayConnectionType_cb _hidl_cb) override;
    ::android::hardware::Return<void> getDisplayAttribute_2_4(uint64_t display, uint32_t config, ::android::hardware::graphics::composer::V2_4::IComposerClient::Attribute attribute, getDisplayAttribute_2_4_cb _hidl_cb) override;
    ::android::hardware::Return<void> getDisplayVsyncPeriod(uint64_t display, getDisplayVsyncPeriod_cb _hidl_cb) override;
    ::android::hardware::Return<void> setActiveConfigWithConstraints(uint64_t display, uint32_t config, const ::android::hardware::graphics::composer::V2_4::IComposerClient::VsyncPeriodChangeConstraints& vsyncPeriodChangeConstraints, setActiveConfigWithConstraints_cb _hidl_cb) override;
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_4::Error> setAutoLowLatencyMode(uint64_t display, bool on) override;
    ::android::hardware::Return<void> getSupportedContentTypes(uint64_t display, getSupportedContentTypes_cb _hidl_cb) override;
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_4::Error> setContentType(uint64_t display, ::android::hardware::graphics::composer::V2_4::IComposerClient::ContentType type) override;
    ::android::hardware::Return<void> getLayerGenericMetadataKeys(getLayerGenericMetadataKeys_cb _hidl_cb) override;

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

}  // namespace V2_4
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_BPHWCOMPOSERCLIENT_H
