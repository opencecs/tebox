#define LOG_TAG "tebox-composer"

#include "composer_hal.h"

#include <string.h>
#include <unistd.h>

#include <array>
#include <cinttypes>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_4 {
namespace hal {

namespace {

using V2_1::Config;
using V2_1::Display;
using V2_1::Layer;
using V2_1::hal::ComposerHal;

constexpr int32_t kClientTargetSlot = 0;

// The 2.1 Attribute enum has no CONFIG_GROUP; the 2.4 one adds it.
int32_t displayAttributeValue(V2_1::IComposerClient::Attribute attribute) {
    switch (attribute) {
        case V2_1::IComposerClient::Attribute::WIDTH:
            return tebox::kWidth;
        case V2_1::IComposerClient::Attribute::HEIGHT:
            return tebox::kHeight;
        case V2_1::IComposerClient::Attribute::VSYNC_PERIOD:
            return tebox::vsyncPeriodNs();
        case V2_1::IComposerClient::Attribute::DPI_X:
            return tebox::kDpiX;
        case V2_1::IComposerClient::Attribute::DPI_Y:
            return tebox::kDpiY;
        default:
            return -1;
    }
}

}  // namespace

TeBoxComposerHal::TeBoxComposerHal() { display_.initialize(); }

TeBoxComposerHal::~TeBoxComposerHal() { stopVsync(); }

bool TeBoxComposerHal::hasCapability(hwc2_capability_t) { return false; }

std::string TeBoxComposerHal::dumpDebugInfo() { return "tebox-hidl-composer"; }

void TeBoxComposerHal::sendHotplug(V2_1::IComposerCallback::Connection connection) {
    EventCallback* callback = nullptr;
    EventCallback_2_4* callback_2_4 = nullptr;
    {
        std::lock_guard<std::mutex> lock(mu_);
        callback = callback_;
        callback_2_4 = callback_2_4_;
    }
    if (callback_2_4) {
        callback_2_4->onHotplug(tebox::kPrimaryDisplay, connection);
    } else if (callback) {
        callback->onHotplug(tebox::kPrimaryDisplay, connection);
    }
}

void TeBoxComposerHal::registerEventCallback(EventCallback* callback) {
    ALOGI("registerEventCallback %p", callback);
    {
        std::lock_guard<std::mutex> lock(mu_);
        callback_ = callback;
    }
    if (callback) {
        // Critical: SF waits for hotplug before creating the primary display.
        sendHotplug(V2_1::IComposerCallback::Connection::CONNECTED);
    }
}

void TeBoxComposerHal::unregisterEventCallback() {
    bool hadCallback = false;
    {
        std::lock_guard<std::mutex> lock(mu_);
        hadCallback = callback_ != nullptr;
        callback_ = nullptr;
    }
    if (hadCallback) stopVsync();
}

void TeBoxComposerHal::registerEventCallback_2_4(EventCallback_2_4* callback) {
    ALOGI("registerEventCallback_2_4 %p", callback);
    {
        std::lock_guard<std::mutex> lock(mu_);
        callback_2_4_ = callback;
    }
    if (callback) {
        sendHotplug(V2_1::IComposerCallback::Connection::CONNECTED);
    }
}

void TeBoxComposerHal::unregisterEventCallback_2_4() {
    bool hadCallback = false;
    {
        std::lock_guard<std::mutex> lock(mu_);
        hadCallback = callback_2_4_ != nullptr;
        callback_2_4_ = nullptr;
    }
    if (hadCallback) stopVsync();
}

uint32_t TeBoxComposerHal::getMaxVirtualDisplayCount() { return 0; }

V2_1::Error TeBoxComposerHal::createVirtualDisplay_2_2(uint32_t, uint32_t,
                                                       common::V1_1::PixelFormat*, Display*) {
    return V2_1::Error::UNSUPPORTED;
}

V2_1::Error TeBoxComposerHal::destroyVirtualDisplay(Display) {
    return V2_1::Error::UNSUPPORTED;
}

V2_1::Error TeBoxComposerHal::createLayer(Display display, Layer* outLayer) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    *outLayer = next_layer_++;
    layers_[*outLayer] = V2_1::IComposerClient::Composition::DEVICE;
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::destroyLayer(Display display, Layer layer) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    layers_.erase(layer);
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::getActiveConfig(Display display, Config* outConfig) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    *outConfig = active_config_;
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::getDisplayAttribute(Display display, Config config,
                                                  V2_1::IComposerClient::Attribute attribute,
                                                  int32_t* outValue) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    if (config != active_config_) return V2_1::Error::BAD_CONFIG;
    const int32_t value = displayAttributeValue(attribute);
    if (value < 0) return V2_1::Error::UNSUPPORTED;
    *outValue = value;
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::getDisplayConfigs(Display display, hidl_vec<Config>* outConfigs) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    outConfigs->resize(1);
    (*outConfigs)[0] = tebox::kConfigId;
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::getDisplayName(Display display, hidl_string* outName) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    *outName = "qemu-virtio-stub";
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::getDisplayType(Display display,
                                             V2_1::IComposerClient::DisplayType* outType) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    *outType = V2_1::IComposerClient::DisplayType::PHYSICAL;
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::getDozeSupport(Display display, bool* outSupport) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    *outSupport = false;
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::setActiveConfig(Display display, Config config) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    if (config != tebox::kConfigId) return V2_1::Error::BAD_CONFIG;
    active_config_ = config;
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::setVsyncEnabled(Display display,
                                              V2_1::IComposerClient::Vsync enabled) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    ALOGI("setVsyncEnabled %d", static_cast<int>(enabled));
    switch (enabled) {
        case V2_1::IComposerClient::Vsync::ENABLE:
            startVsync();
            return V2_1::Error::NONE;
        case V2_1::IComposerClient::Vsync::DISABLE:
            stopVsync();
            return V2_1::Error::NONE;
        default:
            return V2_1::Error::BAD_PARAMETER;
    }
}

V2_1::Error TeBoxComposerHal::setColorTransform(Display display, const float*, int32_t) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::setClientTarget(Display display, buffer_handle_t target,
                                              int32_t acquireFence, int32_t,
                                              const std::vector<hwc_rect_t>&) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    if (!display_.setTarget(kClientTargetSlot, target, acquireFence)) {
        ALOGE("setClientTarget rejected buffer");
        return V2_1::Error::BAD_PARAMETER;
    }
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::setOutputBuffer(Display display, buffer_handle_t,
                                              int32_t releaseFence) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    // Only virtual displays have output buffers; this HAL creates none.
    return V2_1::Error::UNSUPPORTED;
}

V2_1::Error TeBoxComposerHal::validateDisplay(
        Display display, std::vector<Layer>* outChangedLayers,
        std::vector<V2_1::IComposerClient::Composition>* outCompositionTypes,
        uint32_t* outDisplayRequestMask, std::vector<Layer>* outRequestedLayers,
        std::vector<uint32_t>* outRequestMasks) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    outChangedLayers->clear();
    outCompositionTypes->clear();
    {
        std::lock_guard<std::mutex> lock(mu_);
        for (auto& entry : layers_) {
            if (entry.second == V2_1::IComposerClient::Composition::CLIENT) continue;
            entry.second = V2_1::IComposerClient::Composition::CLIENT;
            outChangedLayers->push_back(entry.first);
            outCompositionTypes->push_back(V2_1::IComposerClient::Composition::CLIENT);
        }
    }
    *outDisplayRequestMask = 0;
    outRequestedLayers->clear();
    outRequestMasks->clear();
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::acceptDisplayChanges(Display display) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::presentDisplay(Display display, int32_t* outPresentFence,
                                             std::vector<Layer>* outLayers,
                                             std::vector<int32_t>* outReleaseFences) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    if (!display_.present()) return V2_1::Error::NO_RESOURCES;
    // The scanout copy is synchronous, so the present fence is already signaled.
    *outPresentFence = -1;
    outLayers->clear();
    outReleaseFences->clear();
    return V2_1::Error::NONE;
}

// ---- layer setters ----

V2_1::Error TeBoxComposerHal::setLayerCursorPosition(Display display, Layer layer, int32_t,
                                                     int32_t) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    return layers_.count(layer) ? V2_1::Error::NONE : V2_1::Error::BAD_LAYER;
}

V2_1::Error TeBoxComposerHal::setLayerBuffer(Display display, Layer layer, buffer_handle_t,
                                             int32_t acquireFence) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (!layers_.count(layer)) return V2_1::Error::BAD_LAYER;
    }
    // All layers are client-composed, so the layer buffer itself is unused.
    // On success this HAL owns the fence and must close it.
    if (acquireFence >= 0) close(acquireFence);
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::setLayerSurfaceDamage(Display display, Layer layer,
                                                    const std::vector<hwc_rect_t>&) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    return layers_.count(layer) ? V2_1::Error::NONE : V2_1::Error::BAD_LAYER;
}

V2_1::Error TeBoxComposerHal::setLayerBlendMode(Display display, Layer layer, int32_t) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    return layers_.count(layer) ? V2_1::Error::NONE : V2_1::Error::BAD_LAYER;
}

V2_1::Error TeBoxComposerHal::setLayerColor(Display display, Layer layer,
                                            V2_1::IComposerClient::Color) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    return layers_.count(layer) ? V2_1::Error::NONE : V2_1::Error::BAD_LAYER;
}

V2_1::Error TeBoxComposerHal::setLayerCompositionType(Display display, Layer layer, int32_t type) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    auto it = layers_.find(layer);
    if (it == layers_.end()) return V2_1::Error::BAD_LAYER;
    it->second = static_cast<V2_1::IComposerClient::Composition>(type);
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::setLayerDataspace(Display display, Layer layer, int32_t) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    return layers_.count(layer) ? V2_1::Error::NONE : V2_1::Error::BAD_LAYER;
}

V2_1::Error TeBoxComposerHal::setLayerDisplayFrame(Display display, Layer layer,
                                                   const hwc_rect_t&) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    return layers_.count(layer) ? V2_1::Error::NONE : V2_1::Error::BAD_LAYER;
}

V2_1::Error TeBoxComposerHal::setLayerPlaneAlpha(Display display, Layer layer, float) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    return layers_.count(layer) ? V2_1::Error::NONE : V2_1::Error::BAD_LAYER;
}

V2_1::Error TeBoxComposerHal::setLayerSidebandStream(Display display, Layer layer,
                                                     buffer_handle_t) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    return layers_.count(layer) ? V2_1::Error::NONE : V2_1::Error::BAD_LAYER;
}

V2_1::Error TeBoxComposerHal::setLayerSourceCrop(Display display, Layer layer,
                                                 const hwc_frect_t&) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    return layers_.count(layer) ? V2_1::Error::NONE : V2_1::Error::BAD_LAYER;
}

V2_1::Error TeBoxComposerHal::setLayerTransform(Display display, Layer layer, int32_t) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    return layers_.count(layer) ? V2_1::Error::NONE : V2_1::Error::BAD_LAYER;
}

V2_1::Error TeBoxComposerHal::setLayerVisibleRegion(Display display, Layer layer,
                                                    const std::vector<hwc_rect_t>&) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    return layers_.count(layer) ? V2_1::Error::NONE : V2_1::Error::BAD_LAYER;
}

V2_1::Error TeBoxComposerHal::setLayerZOrder(Display display, Layer layer, uint32_t) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    return layers_.count(layer) ? V2_1::Error::NONE : V2_1::Error::BAD_LAYER;
}

// ---- 2.2 ----

V2_1::Error TeBoxComposerHal::setReadbackBuffer(Display display, const native_handle_t*,
                                                base::unique_fd) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    return V2_1::Error::UNSUPPORTED;
}

V2_1::Error TeBoxComposerHal::getReadbackBufferFence(Display display, base::unique_fd* outFenceFd) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    outFenceFd->reset();
    return V2_1::Error::UNSUPPORTED;
}

V2_1::Error TeBoxComposerHal::setPowerMode_2_2(Display display,
                                               IComposerClient::PowerMode mode) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    ALOGI("setPowerMode %d", static_cast<int>(mode));
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::setLayerFloatColor(Display display, Layer layer,
                                                 V2_2::IComposerClient::FloatColor) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    return layers_.count(layer) ? V2_1::Error::NONE : V2_1::Error::BAD_LAYER;
}

V2_1::Error TeBoxComposerHal::getRenderIntents(Display display, common::V1_1::ColorMode,
                                               std::vector<RenderIntent>* outIntents) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    outIntents->clear();
    outIntents->push_back(RenderIntent::COLORIMETRIC);
    return V2_1::Error::NONE;
}

std::array<float, 16> TeBoxComposerHal::getDataspaceSaturationMatrix(
        common::V1_1::Dataspace) {
    // Identity 4x4.
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
}

// ---- 2.3 ----

V2_1::Error TeBoxComposerHal::getPerFrameMetadataKeys_2_3(
        Display display, std::vector<V2_3::IComposerClient::PerFrameMetadataKey>* outKeys) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    outKeys->clear();
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::setColorMode_2_3(Display display, ColorMode, RenderIntent) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::getRenderIntents_2_3(Display display, ColorMode,
                                                   std::vector<RenderIntent>* outIntents) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    outIntents->clear();
    outIntents->push_back(RenderIntent::COLORIMETRIC);
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::getColorModes_2_3(Display display, hidl_vec<ColorMode>* outModes) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    outModes->resize(1);
    (*outModes)[0] = ColorMode::NATIVE;
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::getClientTargetSupport_2_3(Display display, uint32_t width,
                                                         uint32_t height, PixelFormat format,
                                                         Dataspace) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    if (width != static_cast<uint32_t>(tebox::kWidth) ||
        height != static_cast<uint32_t>(tebox::kHeight))
        return V2_1::Error::UNSUPPORTED;
    if (format != PixelFormat::RGBA_8888 && format != PixelFormat::RGBX_8888)
        return V2_1::Error::UNSUPPORTED;
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::getReadbackBufferAttributes_2_3(Display display, PixelFormat*,
                                                              Dataspace*) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    return V2_1::Error::UNSUPPORTED;
}

V2_1::Error TeBoxComposerHal::getHdrCapabilities_2_3(Display display, hidl_vec<Hdr>* outTypes,
                                                     float* outMaxLuminance,
                                                     float* outMaxAverageLuminance,
                                                     float* outMinLuminance) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    outTypes->resize(0);
    *outMaxLuminance = 0.f;
    *outMaxAverageLuminance = 0.f;
    *outMinLuminance = 0.f;
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::setLayerPerFrameMetadata_2_3(
        Display display, Layer layer,
        const std::vector<V2_3::IComposerClient::PerFrameMetadata>&) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    return layers_.count(layer) ? V2_1::Error::NONE : V2_1::Error::BAD_LAYER;
}

V2_1::Error TeBoxComposerHal::getDisplayIdentificationData(Display display, uint8_t* outPort,
                                                           std::vector<uint8_t>* outData) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    *outPort = 0;
    // Minimal EDID-ish blob (128 bytes of zeros is rejected; put a tiny marker).
    outData->assign(128, 0);
    (*outData)[0] = 0x00;
    (*outData)[1] = 0xff;
    (*outData)[2] = 0xff;
    (*outData)[3] = 0xff;
    (*outData)[4] = 0xff;
    (*outData)[5] = 0xff;
    (*outData)[6] = 0xff;
    (*outData)[7] = 0x00;
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::setLayerColorTransform(Display display, Layer layer, const float*) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    return layers_.count(layer) ? V2_1::Error::NONE : V2_1::Error::BAD_LAYER;
}

V2_1::Error TeBoxComposerHal::getDisplayedContentSamplingAttributes(
        uint64_t display, PixelFormat&, Dataspace&,
        hidl_bitfield<V2_3::IComposerClient::FormatColorComponent>&) {
    if (display != static_cast<uint64_t>(tebox::kPrimaryDisplay))
        return V2_1::Error::BAD_DISPLAY;
    return V2_1::Error::UNSUPPORTED;
}

V2_1::Error TeBoxComposerHal::setDisplayedContentSamplingEnabled(
        uint64_t display, V2_3::IComposerClient::DisplayedContentSampling,
        hidl_bitfield<V2_3::IComposerClient::FormatColorComponent>, uint64_t) {
    if (display != static_cast<uint64_t>(tebox::kPrimaryDisplay))
        return V2_1::Error::BAD_DISPLAY;
    return V2_1::Error::UNSUPPORTED;
}

V2_1::Error TeBoxComposerHal::getDisplayedContentSample(
        uint64_t display, uint64_t, uint64_t, uint64_t& frameCount,
        hidl_vec<uint64_t>& sampleComponent0, hidl_vec<uint64_t>& sampleComponent1,
        hidl_vec<uint64_t>& sampleComponent2, hidl_vec<uint64_t>& sampleComponent3) {
    if (display != static_cast<uint64_t>(tebox::kPrimaryDisplay))
        return V2_1::Error::BAD_DISPLAY;
    frameCount = 0;
    sampleComponent0.resize(0);
    sampleComponent1.resize(0);
    sampleComponent2.resize(0);
    sampleComponent3.resize(0);
    return V2_1::Error::UNSUPPORTED;
}

V2_1::Error TeBoxComposerHal::getDisplayCapabilities(
        Display display, std::vector<V2_3::IComposerClient::DisplayCapability>* outCapabilities) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    outCapabilities->clear();
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::setLayerPerFrameMetadataBlobs(
        Display display, Layer layer, std::vector<V2_3::IComposerClient::PerFrameMetadataBlob>&) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    return layers_.count(layer) ? V2_1::Error::NONE : V2_1::Error::BAD_LAYER;
}

V2_1::Error TeBoxComposerHal::getDisplayBrightnessSupport(Display display, bool* outSupport) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    *outSupport = false;
    return V2_1::Error::NONE;
}

V2_1::Error TeBoxComposerHal::setDisplayBrightness(Display display, float) {
    if (!isPrimary(display)) return V2_1::Error::BAD_DISPLAY;
    return V2_1::Error::UNSUPPORTED;
}

// ---- 2.4 ----

Error TeBoxComposerHal::getDisplayCapabilities_2_4(
        Display display, std::vector<IComposerClient::DisplayCapability>* outCapabilities) {
    if (!isPrimary(display)) return Error::BAD_DISPLAY;
    outCapabilities->clear();
    return Error::NONE;
}

Error TeBoxComposerHal::getDisplayConnectionType(
        Display display, IComposerClient::DisplayConnectionType* outType) {
    if (!isPrimary(display)) return Error::BAD_DISPLAY;
    *outType = IComposerClient::DisplayConnectionType::INTERNAL;
    return Error::NONE;
}

Error TeBoxComposerHal::getDisplayAttribute_2_4(Display display, Config config,
                                                IComposerClient::Attribute attribute,
                                                int32_t* outValue) {
    if (!isPrimary(display)) return Error::BAD_DISPLAY;
    if (config != active_config_) return Error::BAD_CONFIG;
    switch (attribute) {
        case IComposerClient::Attribute::WIDTH:
            *outValue = tebox::kWidth;
            break;
        case IComposerClient::Attribute::HEIGHT:
            *outValue = tebox::kHeight;
            break;
        case IComposerClient::Attribute::VSYNC_PERIOD:
            *outValue = tebox::vsyncPeriodNs();
            break;
        case IComposerClient::Attribute::DPI_X:
            *outValue = tebox::kDpiX;
            break;
        case IComposerClient::Attribute::DPI_Y:
            *outValue = tebox::kDpiY;
            break;
        case IComposerClient::Attribute::CONFIG_GROUP:
            *outValue = 0;
            break;
        default:
            return Error::UNSUPPORTED;
    }
    return Error::NONE;
}

Error TeBoxComposerHal::getDisplayVsyncPeriod(Display display, VsyncPeriodNanos* outVsyncPeriod) {
    if (!isPrimary(display)) return Error::BAD_DISPLAY;
    *outVsyncPeriod = static_cast<VsyncPeriodNanos>(tebox::vsyncPeriodNs());
    return Error::NONE;
}

Error TeBoxComposerHal::setActiveConfigWithConstraints(
        Display display, Config config,
        const IComposerClient::VsyncPeriodChangeConstraints&, VsyncPeriodChangeTimeline* timeline) {
    if (!isPrimary(display)) return Error::BAD_DISPLAY;
    if (config != tebox::kConfigId) return Error::BAD_CONFIG;
    active_config_ = config;
    timeline->newVsyncAppliedTimeNanos = 0;
    timeline->refreshRequired = false;
    timeline->refreshTimeNanos = 0;
    return Error::NONE;
}

Error TeBoxComposerHal::setAutoLowLatencyMode(Display display, bool) {
    if (!isPrimary(display)) return Error::BAD_DISPLAY;
    return Error::UNSUPPORTED;
}

Error TeBoxComposerHal::getSupportedContentTypes(
        Display display, std::vector<IComposerClient::ContentType>* outSupportedContentTypes) {
    if (!isPrimary(display)) return Error::BAD_DISPLAY;
    outSupportedContentTypes->clear();
    return Error::NONE;
}

Error TeBoxComposerHal::setContentType(Display display, IComposerClient::ContentType) {
    if (!isPrimary(display)) return Error::BAD_DISPLAY;
    return Error::NONE;
}

Error TeBoxComposerHal::validateDisplay_2_4(
        Display display, std::vector<Layer>* outChangedLayers,
        std::vector<IComposerClient::Composition>* outCompositionTypes,
        uint32_t* outDisplayRequestMask, std::vector<Layer>* outRequestedLayers,
        std::vector<uint32_t>* outRequestMasks,
        IComposerClient::ClientTargetProperty* outClientTargetProperty) {
    if (!isPrimary(display)) return Error::BAD_DISPLAY;
    outChangedLayers->clear();
    outCompositionTypes->clear();
    {
        std::lock_guard<std::mutex> lock(mu_);
        for (auto& entry : layers_) {
            if (entry.second == V2_1::IComposerClient::Composition::CLIENT) continue;
            entry.second = V2_1::IComposerClient::Composition::CLIENT;
            outChangedLayers->push_back(entry.first);
            outCompositionTypes->push_back(IComposerClient::Composition::CLIENT);
        }
    }
    *outDisplayRequestMask = 0;
    outRequestedLayers->clear();
    outRequestMasks->clear();
    outClientTargetProperty->pixelFormat = PixelFormat::RGBA_8888;
    outClientTargetProperty->dataspace = Dataspace::UNKNOWN;
    return Error::NONE;
}

Error TeBoxComposerHal::setLayerGenericMetadata(Display display, Layer layer,
                                                const std::string&, bool,
                                                const std::vector<uint8_t>&) {
    if (!isPrimary(display)) return Error::BAD_DISPLAY;
    std::lock_guard<std::mutex> lock(mu_);
    return layers_.count(layer) ? Error::NONE : Error::BAD_LAYER;
}

Error TeBoxComposerHal::getLayerGenericMetadataKeys(
        std::vector<IComposerClient::LayerGenericMetadataKey>* outKeys) {
    outKeys->clear();
    return Error::NONE;
}

// ---- vsync ----

void TeBoxComposerHal::startVsync() {
    bool expected = false;
    if (!vsync_running_.compare_exchange_strong(expected, true)) return;
    vsync_thread_ = std::thread([this] {
        // A fixed CLOCK_MONOTONIC timeline: each vsync fires at an absolute
        // deadline and reports that ideal time. Relative sleeps drifted
        // (every period ran long by the wakeup latency) and leaked that
        // jitter into the timestamps, so SurfaceFlinger's vsync predictor
        // mis-scheduled frames ("Frame time is in the future").
        const int32_t period = vsync_period_ns_;
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        int64_t next = int64_t(now.tv_sec) * 1000000000LL + now.tv_nsec + period;
        while (vsync_running_.load()) {
            struct timespec deadline = {static_cast<time_t>(next / 1000000000LL),
                                        static_cast<long>(next % 1000000000LL)};
            while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, nullptr) == EINTR) {
            }
            const int64_t ts = next;
            next += period;
            clock_gettime(CLOCK_MONOTONIC, &now);
            const int64_t nowNs = int64_t(now.tv_sec) * 1000000000LL + now.tv_nsec;
            if (nowNs > next) {  // stalled (suspend, heavy load): skip missed vsyncs
                next += ((nowNs - next) / period + 1) * period;
            }
            EventCallback* callback = nullptr;
            EventCallback_2_4* callback_2_4 = nullptr;
            {
                std::lock_guard<std::mutex> lock(mu_);
                callback = callback_;
                callback_2_4 = callback_2_4_;
            }
            if (callback_2_4) {
                // SurfaceFlinger ignores the 2.1 callback when composer 2.4 is
                // in use and waits for onVsync_2_4.
                callback_2_4->onVsync_2_4(tebox::kPrimaryDisplay, ts, period);
            } else if (callback) {
                callback->onVsync(tebox::kPrimaryDisplay, ts);
            }
        }
    });
}

void TeBoxComposerHal::stopVsync() {
    vsync_running_.store(false);
    if (vsync_thread_.joinable()) vsync_thread_.join();
}

}  // namespace hal
}  // namespace V2_4
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
