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

#include <android-base/unique_fd.h>
#include <android/hardware/graphics/composer/2.2/IComposer.h>
#include <android/hardware/graphics/composer/2.2/IComposerClient.h>
#include <composer-hal/2.1/ComposerHal.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_2 {
namespace hal {

using common::V1_1::ColorMode;
using common::V1_1::Dataspace;
using common::V1_1::PixelFormat;
using common::V1_1::RenderIntent;
using V2_1::Display;
using V2_1::Error;
using V2_1::Layer;

class ComposerHal : public V2_1::hal::ComposerHal {
   public:
    Error createVirtualDisplay(uint32_t width, uint32_t height, common::V1_0::PixelFormat* format,
                               Display* outDisplay) override {
        return createVirtualDisplay_2_2(width, height, reinterpret_cast<PixelFormat*>(format),
                                        outDisplay);
    }
    Error getClientTargetSupport(Display display, uint32_t width, uint32_t height,
                                 common::V1_0::PixelFormat format,
                                 common::V1_0::Dataspace dataspace) override {
        return getClientTargetSupport_2_2(display, width, height, static_cast<PixelFormat>(format),
                                          static_cast<Dataspace>(dataspace));
    }
    // superceded by setPowerMode_2_2
    Error setPowerMode(Display display, V2_1::IComposerClient::PowerMode mode) override {
        return setPowerMode_2_2(display, static_cast<IComposerClient::PowerMode>(mode));
    }

    // superceded by getColorModes_2_2
    Error getColorModes(Display display, hidl_vec<common::V1_0::ColorMode>* outModes) override {
        return getColorModes_2_2(display, reinterpret_cast<hidl_vec<ColorMode>*>(outModes));
    }

    // superceded by setColorMode_2_2
    Error setColorMode(Display display, common::V1_0::ColorMode mode) override {
        return setColorMode_2_2(display, static_cast<ColorMode>(mode), RenderIntent::COLORIMETRIC);
    }

    virtual Error getPerFrameMetadataKeys(
        Display display, std::vector<IComposerClient::PerFrameMetadataKey>* outKeys) = 0;
    virtual Error setLayerPerFrameMetadata(
        Display display, Layer layer,
        const std::vector<IComposerClient::PerFrameMetadata>& metadata) = 0;

    virtual Error getReadbackBufferAttributes(Display display, PixelFormat* outFormat,
                                              Dataspace* outDataspace) = 0;
    virtual Error setReadbackBuffer(Display display, const native_handle_t* bufferHandle,
                                    base::unique_fd fenceFd) = 0;
    virtual Error getReadbackBufferFence(Display display, base::unique_fd* outFenceFd) = 0;
    virtual Error createVirtualDisplay_2_2(uint32_t width, uint32_t height, PixelFormat* format,
                                           Display* outDisplay) = 0;
    virtual Error getClientTargetSupport_2_2(Display display, uint32_t width, uint32_t height,
                                             PixelFormat format, Dataspace dataspace) = 0;
    virtual Error setPowerMode_2_2(Display display, IComposerClient::PowerMode mode) = 0;

    virtual Error setLayerFloatColor(Display display, Layer layer,
                                     IComposerClient::FloatColor color) = 0;

    virtual Error getColorModes_2_2(Display display, hidl_vec<ColorMode>* outModes) = 0;
    virtual Error getRenderIntents(Display display, ColorMode mode,
                                   std::vector<RenderIntent>* outIntents) = 0;
    virtual Error setColorMode_2_2(Display display, ColorMode mode, RenderIntent intent) = 0;

    virtual std::array<float, 16> getDataspaceSaturationMatrix(Dataspace dataspace) = 0;
};

}  // namespace hal
}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
