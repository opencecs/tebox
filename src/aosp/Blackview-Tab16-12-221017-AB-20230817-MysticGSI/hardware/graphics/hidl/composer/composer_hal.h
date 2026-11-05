#pragma once

// HIDL composer 2.4 HAL on top of the synchronous KMS scanout stub.

#include <android-base/unique_fd.h>
// ComposerHal 2.3 uses common@1.2 types that only the generated composer
// headers pull in; AOSP includes those via composer-hal/2.x/Composer.h.
#include <android/hardware/graphics/composer/2.4/IComposer.h>
#include <composer-hal/2.4/ComposerHal.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "kms_display.h"

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_4 {
namespace hal {

class TeBoxComposerHal : public ComposerHal {
  public:
    TeBoxComposerHal();
    ~TeBoxComposerHal() override;

    // 2.1
    bool hasCapability(hwc2_capability_t capability) override;
    std::string dumpDebugInfo() override;
    void registerEventCallback(EventCallback* callback) override;
    void unregisterEventCallback() override;
    uint32_t getMaxVirtualDisplayCount() override;
    V2_1::Error destroyVirtualDisplay(Display display) override;
    V2_1::Error createLayer(Display display, Layer* outLayer) override;
    V2_1::Error destroyLayer(Display display, Layer layer) override;
    V2_1::Error getActiveConfig(Display display, Config* outConfig) override;
    V2_1::Error getDisplayAttribute(Display display, Config config,
                                    V2_1::IComposerClient::Attribute attribute,
                                    int32_t* outValue) override;
    V2_1::Error getDisplayConfigs(Display display, hidl_vec<Config>* outConfigs) override;
    V2_1::Error getDisplayName(Display display, hidl_string* outName) override;
    V2_1::Error getDisplayType(Display display,
                               V2_1::IComposerClient::DisplayType* outType) override;
    V2_1::Error getDozeSupport(Display display, bool* outSupport) override;
    V2_1::Error setActiveConfig(Display display, Config config) override;
    V2_1::Error setVsyncEnabled(Display display, V2_1::IComposerClient::Vsync enabled) override;
    V2_1::Error setColorTransform(Display display, const float* matrix, int32_t hint) override;
    V2_1::Error setClientTarget(Display display, buffer_handle_t target, int32_t acquireFence,
                                int32_t dataspace, const std::vector<hwc_rect_t>& damage) override;
    V2_1::Error setOutputBuffer(Display display, buffer_handle_t buffer,
                                int32_t releaseFence) override;
    V2_1::Error validateDisplay(Display display, std::vector<Layer>* outChangedLayers,
                                std::vector<V2_1::IComposerClient::Composition>* outCompositionTypes,
                                uint32_t* outDisplayRequestMask,
                                std::vector<Layer>* outRequestedLayers,
                                std::vector<uint32_t>* outRequestMasks) override;
    V2_1::Error acceptDisplayChanges(Display display) override;
    V2_1::Error presentDisplay(Display display, int32_t* outPresentFence,
                               std::vector<Layer>* outLayers,
                               std::vector<int32_t>* outReleaseFences) override;

    // 2.1 layer setters
    V2_1::Error setLayerCursorPosition(Display display, Layer layer, int32_t x, int32_t y) override;
    V2_1::Error setLayerBuffer(Display display, Layer layer, buffer_handle_t buffer,
                               int32_t acquireFence) override;
    V2_1::Error setLayerSurfaceDamage(Display display, Layer layer,
                                      const std::vector<hwc_rect_t>& damage) override;
    V2_1::Error setLayerBlendMode(Display display, Layer layer, int32_t mode) override;
    V2_1::Error setLayerColor(Display display, Layer layer,
                              V2_1::IComposerClient::Color color) override;
    V2_1::Error setLayerCompositionType(Display display, Layer layer, int32_t type) override;
    V2_1::Error setLayerDataspace(Display display, Layer layer, int32_t dataspace) override;
    V2_1::Error setLayerDisplayFrame(Display display, Layer layer,
                                     const hwc_rect_t& frame) override;
    V2_1::Error setLayerPlaneAlpha(Display display, Layer layer, float alpha) override;
    V2_1::Error setLayerSidebandStream(Display display, Layer layer, buffer_handle_t stream) override;
    V2_1::Error setLayerSourceCrop(Display display, Layer layer, const hwc_frect_t& crop) override;
    V2_1::Error setLayerTransform(Display display, Layer layer, int32_t transform) override;
    V2_1::Error setLayerVisibleRegion(Display display, Layer layer,
                                      const std::vector<hwc_rect_t>& visible) override;
    V2_1::Error setLayerZOrder(Display display, Layer layer, uint32_t z) override;

    // 2.2
    V2_1::Error setReadbackBuffer(Display display, const native_handle_t* bufferHandle,
                                  base::unique_fd fenceFd) override;
    V2_1::Error getReadbackBufferFence(Display display, base::unique_fd* outFenceFd) override;
    V2_1::Error createVirtualDisplay_2_2(uint32_t width, uint32_t height,
                                         common::V1_1::PixelFormat* format,
                                         Display* outDisplay) override;
    V2_1::Error setPowerMode_2_2(Display display, IComposerClient::PowerMode mode) override;
    V2_1::Error setLayerFloatColor(Display display, Layer layer,
                                   V2_2::IComposerClient::FloatColor color) override;
    V2_1::Error getRenderIntents(Display display, common::V1_1::ColorMode mode,
                                 std::vector<RenderIntent>* outIntents) override;
    std::array<float, 16> getDataspaceSaturationMatrix(common::V1_1::Dataspace dataspace) override;

    // 2.3
    V2_1::Error getPerFrameMetadataKeys_2_3(
            Display display,
            std::vector<V2_3::IComposerClient::PerFrameMetadataKey>* outKeys) override;
    V2_1::Error setColorMode_2_3(Display display, ColorMode mode, RenderIntent intent) override;
    V2_1::Error getRenderIntents_2_3(Display display, ColorMode mode,
                                     std::vector<RenderIntent>* outIntents) override;
    V2_1::Error getColorModes_2_3(Display display, hidl_vec<ColorMode>* outModes) override;
    V2_1::Error getClientTargetSupport_2_3(Display display, uint32_t width, uint32_t height,
                                           PixelFormat format, Dataspace dataspace) override;
    V2_1::Error getReadbackBufferAttributes_2_3(Display display, PixelFormat* outFormat,
                                                Dataspace* outDataspace) override;
    V2_1::Error getHdrCapabilities_2_3(Display display, hidl_vec<Hdr>* outTypes,
                                       float* outMaxLuminance, float* outMaxAverageLuminance,
                                       float* outMinLuminance) override;
    V2_1::Error setLayerPerFrameMetadata_2_3(
            Display display, Layer layer,
            const std::vector<V2_3::IComposerClient::PerFrameMetadata>& metadata) override;
    V2_1::Error getDisplayIdentificationData(Display display, uint8_t* outPort,
                                             std::vector<uint8_t>* outData) override;
    V2_1::Error setLayerColorTransform(Display display, Layer layer, const float* matrix) override;
    V2_1::Error getDisplayedContentSamplingAttributes(
            uint64_t display, PixelFormat& format, Dataspace& dataspace,
            hidl_bitfield<V2_3::IComposerClient::FormatColorComponent>& componentMask) override;
    V2_1::Error setDisplayedContentSamplingEnabled(
            uint64_t display, V2_3::IComposerClient::DisplayedContentSampling enable,
            hidl_bitfield<V2_3::IComposerClient::FormatColorComponent> componentMask,
            uint64_t maxFrames) override;
    V2_1::Error getDisplayedContentSample(uint64_t display, uint64_t maxFrames, uint64_t timestamp,
                                          uint64_t& frameCount, hidl_vec<uint64_t>& sampleComponent0,
                                          hidl_vec<uint64_t>& sampleComponent1,
                                          hidl_vec<uint64_t>& sampleComponent2,
                                          hidl_vec<uint64_t>& sampleComponent3) override;
    V2_1::Error getDisplayCapabilities(
            Display display,
            std::vector<V2_3::IComposerClient::DisplayCapability>* outCapabilities) override;
    V2_1::Error setLayerPerFrameMetadataBlobs(
            Display display, Layer layer,
            std::vector<V2_3::IComposerClient::PerFrameMetadataBlob>& blobs) override;
    V2_1::Error getDisplayBrightnessSupport(Display display, bool* outSupport) override;
    V2_1::Error setDisplayBrightness(Display display, float brightness) override;

    // 2.4
    void registerEventCallback_2_4(EventCallback_2_4* callback) override;
    void unregisterEventCallback_2_4() override;
    Error getDisplayCapabilities_2_4(
            Display display, std::vector<IComposerClient::DisplayCapability>* outCapabilities) override;
    Error getDisplayConnectionType(Display display,
                                   IComposerClient::DisplayConnectionType* outType) override;
    Error getDisplayAttribute_2_4(Display display, Config config,
                                  IComposerClient::Attribute attribute, int32_t* outValue) override;
    Error getDisplayVsyncPeriod(Display display, VsyncPeriodNanos* outVsyncPeriod) override;
    Error setActiveConfigWithConstraints(
            Display display, Config config,
            const IComposerClient::VsyncPeriodChangeConstraints& vsyncPeriodChangeConstraints,
            VsyncPeriodChangeTimeline* timeline) override;
    Error setAutoLowLatencyMode(Display display, bool on) override;
    Error getSupportedContentTypes(
            Display display,
            std::vector<IComposerClient::ContentType>* outSupportedContentTypes) override;
    Error setContentType(Display display, IComposerClient::ContentType contentType) override;
    Error validateDisplay_2_4(Display display, std::vector<Layer>* outChangedLayers,
                              std::vector<IComposerClient::Composition>* outCompositionTypes,
                              uint32_t* outDisplayRequestMask,
                              std::vector<Layer>* outRequestedLayers,
                              std::vector<uint32_t>* outRequestMasks,
                              IComposerClient::ClientTargetProperty* outClientTargetProperty) override;
    Error setLayerGenericMetadata(Display display, Layer layer, const std::string& key,
                                  bool mandatory, const std::vector<uint8_t>& value) override;
    Error getLayerGenericMetadataKeys(
            std::vector<IComposerClient::LayerGenericMetadataKey>* outKeys) override;

  private:
    bool isPrimary(Display display) const { return display == tebox::kPrimaryDisplay; }
    void startVsync();
    void stopVsync();
    void sendHotplug(V2_1::IComposerCallback::Connection connection);

    std::mutex mu_;
    EventCallback* callback_ = nullptr;
    EventCallback_2_4* callback_2_4_ = nullptr;
    std::atomic<bool> vsync_running_{false};
    std::thread vsync_thread_;
    int32_t vsync_period_ns_ = tebox::vsyncPeriodNs();
    int32_t active_config_ = tebox::kConfigId;
    int64_t next_layer_ = 1;
    std::unordered_map<int64_t, V2_1::IComposerClient::Composition> layers_;
    tebox::KmsDisplay display_;
};

}  // namespace hal
}  // namespace V2_4
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
