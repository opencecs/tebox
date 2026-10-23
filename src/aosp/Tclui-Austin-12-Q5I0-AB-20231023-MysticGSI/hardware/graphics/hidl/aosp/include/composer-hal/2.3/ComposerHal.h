/*
 * Copyright 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <composer-hal/2.2/ComposerHal.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_3 {
namespace hal {

using common::V1_1::RenderIntent;
using common::V1_2::ColorMode;
using common::V1_2::Dataspace;
using common::V1_2::Hdr;
using common::V1_2::PixelFormat;
using V2_1::Display;
using V2_1::Error;
using V2_1::Layer;

class ComposerHal : public V2_2::hal::ComposerHal {
   public:
    Error getPerFrameMetadataKeys(
        Display display, std::vector<V2_2::IComposerClient::PerFrameMetadataKey>* outKeys) {
        return getPerFrameMetadataKeys_2_3(
            display, reinterpret_cast<std::vector<IComposerClient::PerFrameMetadataKey>*>(outKeys));
    }

    Error setColorMode_2_2(Display display, common::V1_1::ColorMode mode,
                           RenderIntent intent) override {
        return setColorMode_2_3(display, static_cast<ColorMode>(mode), intent);
    }

    Error getColorModes_2_2(Display display, hidl_vec<common::V1_1::ColorMode>* outModes) override {
        return getColorModes_2_3(display, reinterpret_cast<hidl_vec<ColorMode>*>(outModes));
    }

    Error getClientTargetSupport_2_2(Display display, uint32_t width, uint32_t height,
                                     common::V1_1::PixelFormat format,
                                     common::V1_1::Dataspace dataspace) override {
        return getClientTargetSupport_2_3(display, width, height, static_cast<PixelFormat>(format),
                                          static_cast<Dataspace>(dataspace));
    }

    Error getReadbackBufferAttributes(Display display, common::V1_1::PixelFormat* outFormat,
                                      common::V1_1::Dataspace* outDataspace) override {
        return getReadbackBufferAttributes_2_3(display, reinterpret_cast<PixelFormat*>(outFormat),
                                               reinterpret_cast<Dataspace*>(outDataspace));
    }

    Error getHdrCapabilities(Display display, hidl_vec<common::V1_0::Hdr>* outTypes,
                             float* outMaxLuminance, float* outMaxAverageLuminance,
                             float* outMinLuminance) override {
        return getHdrCapabilities_2_3(display, reinterpret_cast<hidl_vec<Hdr>*>(outTypes),
                                      outMaxLuminance, outMaxAverageLuminance, outMinLuminance);
    }

    Error setLayerPerFrameMetadata(
        Display display, Layer layer,
        const std::vector<V2_2::IComposerClient::PerFrameMetadata>& metadata) override {
        return setLayerPerFrameMetadata_2_3(
            display, layer,
            reinterpret_cast<const std::vector<IComposerClient::PerFrameMetadata>&>(metadata));
    }

    virtual Error getPerFrameMetadataKeys_2_3(
        Display display, std::vector<IComposerClient::PerFrameMetadataKey>* outKeys) = 0;

    virtual Error setColorMode_2_3(Display display, ColorMode mode, RenderIntent intent) = 0;

    virtual Error getRenderIntents_2_3(Display display, ColorMode mode,
                                       std::vector<RenderIntent>* outIntents) = 0;

    virtual Error getColorModes_2_3(Display display, hidl_vec<ColorMode>* outModes) = 0;

    virtual Error getClientTargetSupport_2_3(Display display, uint32_t width, uint32_t height,
                                             PixelFormat format, Dataspace dataspace) = 0;
    virtual Error getReadbackBufferAttributes_2_3(Display display, PixelFormat* outFormat,
                                                  Dataspace* outDataspace) = 0;
    virtual Error getHdrCapabilities_2_3(Display display, hidl_vec<Hdr>* outTypes,
                                         float* outMaxLuminance, float* outMaxAverageLuminance,
                                         float* outMinLuminance) = 0;
    virtual Error setLayerPerFrameMetadata_2_3(
        Display display, Layer layer,
        const std::vector<IComposerClient::PerFrameMetadata>& metadata) = 0;
    virtual Error getDisplayIdentificationData(Display display, uint8_t* outPort,
                                               std::vector<uint8_t>* outData) = 0;
    virtual Error setLayerColorTransform(Display display, Layer layer, const float* matrix) = 0;
    virtual Error getDisplayedContentSamplingAttributes(
        uint64_t display, PixelFormat& format, Dataspace& dataspace,
        hidl_bitfield<IComposerClient::FormatColorComponent>& componentMask) = 0;
    virtual Error setDisplayedContentSamplingEnabled(
        uint64_t display, IComposerClient::DisplayedContentSampling enable,
        hidl_bitfield<IComposerClient::FormatColorComponent> componentMask, uint64_t maxFrames) = 0;
    virtual Error getDisplayedContentSample(uint64_t display, uint64_t maxFrames,
                                            uint64_t timestamp, uint64_t& frameCount,
                                            hidl_vec<uint64_t>& sampleComponent0,
                                            hidl_vec<uint64_t>& sampleComponent1,
                                            hidl_vec<uint64_t>& sampleComponent2,
                                            hidl_vec<uint64_t>& sampleComponent3) = 0;
    virtual Error getDisplayCapabilities(
            Display display, std::vector<IComposerClient::DisplayCapability>* outCapabilities) = 0;
    virtual Error setLayerPerFrameMetadataBlobs(
        Display display, Layer layer,
        std::vector<IComposerClient::PerFrameMetadataBlob>& blobs) = 0;
    virtual Error getDisplayBrightnessSupport(Display display, bool* outSupport) = 0;
    virtual Error setDisplayBrightness(Display display, float brightness) = 0;
};

}  // namespace hal
}  // namespace V2_3
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
