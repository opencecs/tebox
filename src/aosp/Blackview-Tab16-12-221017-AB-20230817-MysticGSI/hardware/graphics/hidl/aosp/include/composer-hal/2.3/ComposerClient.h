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

#ifndef LOG_TAG
#warning "ComposerClient.h included without LOG_TAG"
#endif

#include <android/hardware/graphics/composer/2.3/IComposerClient.h>
#include <composer-hal/2.3/ComposerCommandEngine.h>
#include <composer-hal/2.3/ComposerHal.h>
#include <composer-resources/2.2/ComposerResources.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_3 {
namespace hal {

namespace detail {

// ComposerClientImpl implements V2_*::IComposerClient on top of V2_*::ComposerHal
template <typename Interface, typename Hal>
class ComposerClientImpl : public V2_2::hal::detail::ComposerClientImpl<Interface, Hal> {
   public:
    Return<void> getPerFrameMetadataKeys_2_3(
        Display display, IComposerClient::getPerFrameMetadataKeys_2_3_cb hidl_cb) override {
        std::vector<IComposerClient::PerFrameMetadataKey> keys;
        Error error = mHal->getPerFrameMetadataKeys_2_3(display, &keys);
        hidl_cb(error, keys);
        return Void();
    }

    Return<Error> setColorMode_2_3(Display display, ColorMode mode, RenderIntent intent) override {
        return mHal->setColorMode_2_3(display, mode, intent);
    }

    Return<void> getRenderIntents_2_3(Display display, ColorMode mode,
                                      IComposerClient::getRenderIntents_2_3_cb hidl_cb) override {
        std::vector<RenderIntent> intents;
        Error err = mHal->getRenderIntents_2_3(display, mode, &intents);
        hidl_cb(err, intents);
        return Void();
    }

    Return<void> getColorModes_2_3(Display display,
                                   IComposerClient::getColorModes_2_3_cb hidl_cb) override {
        hidl_vec<ColorMode> modes;
        Error err = mHal->getColorModes_2_3(display, &modes);
        hidl_cb(err, modes);
        return Void();
    }

    Return<void> getReadbackBufferAttributes_2_3(
        Display display, IComposerClient::getReadbackBufferAttributes_2_3_cb hidl_cb) override {
        PixelFormat format = PixelFormat::RGB_888;
        Dataspace dataspace = Dataspace::UNKNOWN;
        Error error = mHal->getReadbackBufferAttributes_2_3(display, &format, &dataspace);
        hidl_cb(error, format, dataspace);
        return Void();
    }

    Return<void> getHdrCapabilities_2_3(
        Display display, IComposerClient::getHdrCapabilities_2_3_cb hidl_cb) override {
        hidl_vec<Hdr> types;
        float max_lumi = 0.0f;
        float max_avg_lumi = 0.0f;
        float min_lumi = 0.0f;
        Error err =
            mHal->getHdrCapabilities_2_3(display, &types, &max_lumi, &max_avg_lumi, &min_lumi);
        hidl_cb(err, types, max_lumi, max_avg_lumi, min_lumi);
        return Void();
    }

    Return<Error> getClientTargetSupport_2_3(Display display, uint32_t width, uint32_t height,
                                             PixelFormat format, Dataspace dataspace) override {
        Error err = mHal->getClientTargetSupport_2_3(display, width, height, format, dataspace);
        return err;
    }

    Return<void> getDisplayCapabilities(
        Display display, IComposerClient::getDisplayCapabilities_cb hidl_cb) override {
        std::vector<IComposerClient::DisplayCapability> capabilities;
        Error error = mHal->getDisplayCapabilities(display, &capabilities);
        hidl_cb(error, capabilities);
        return Void();
    }

    static std::unique_ptr<ComposerClientImpl> create(Hal* hal) {
        auto client = std::make_unique<ComposerClientImpl>(hal);
        return client->init() ? std::move(client) : nullptr;
    }

    ComposerClientImpl(Hal* hal) : BaseType2_2(hal) {}

    // IComposerClient 2.3 interface

    Return<void> getDisplayIdentificationData(
        Display display, IComposerClient::getDisplayIdentificationData_cb hidl_cb) override {
        uint8_t port = 0;
        std::vector<uint8_t> data;
        Error error = mHal->getDisplayIdentificationData(display, &port, &data);
        hidl_cb(error, port, data);
        return Void();
    }

    Return<void> getDisplayedContentSamplingAttributes(
        uint64_t display,
        IComposerClient::getDisplayedContentSamplingAttributes_cb hidl_cb) override {
        PixelFormat format;
        common::V1_2::Dataspace dataspace;
        hidl_bitfield<IComposerClient::FormatColorComponent> componentMask;
        Error error =
            mHal->getDisplayedContentSamplingAttributes(display, format, dataspace, componentMask);
        hidl_cb(error, format, dataspace, componentMask);
        return Void();
    }

    Return<Error> setDisplayedContentSamplingEnabled(
        uint64_t display, IComposerClient::DisplayedContentSampling enable,
        hidl_bitfield<IComposerClient::FormatColorComponent> componentMask,
        uint64_t maxFrames) override {
        return mHal->setDisplayedContentSamplingEnabled(display, enable, componentMask, maxFrames);
    }

    Return<void> getDisplayedContentSample(
        uint64_t display, uint64_t maxFrames, uint64_t timestamp,
        IComposerClient::getDisplayedContentSample_cb hidl_cb) override {
        uint64_t frameCount;
        hidl_vec<uint64_t> sampleComponent0;
        hidl_vec<uint64_t> sampleComponent1;
        hidl_vec<uint64_t> sampleComponent2;
        hidl_vec<uint64_t> sampleComponent3;

        Error error = mHal->getDisplayedContentSample(display, maxFrames, timestamp, frameCount,
                                                      sampleComponent0, sampleComponent1,
                                                      sampleComponent2, sampleComponent3);
        hidl_cb(error, frameCount, sampleComponent0, sampleComponent1, sampleComponent2,
                sampleComponent3);
        return Void();
    }

    Return<void> executeCommands_2_3(uint32_t inLength, const hidl_vec<hidl_handle>& inHandles,
                                     IComposerClient::executeCommands_2_2_cb hidl_cb) override {
        std::lock_guard<std::mutex> lock(mCommandEngineMutex);
        bool outChanged = false;
        uint32_t outLength = 0;
        hidl_vec<hidl_handle> outHandles;
        Error error =
            mCommandEngine->execute(inLength, inHandles, &outChanged, &outLength, &outHandles);

        hidl_cb(error, outChanged, outLength, outHandles);

        mCommandEngine->reset();

        return Void();
    }

    Return<void> getDisplayBrightnessSupport(
            Display display, IComposerClient::getDisplayBrightnessSupport_cb hidl_cb) override {
        bool support = false;
        Error error = mHal->getDisplayBrightnessSupport(display, &support);
        hidl_cb(error, support);
        return Void();
    }

    Return<Error> setDisplayBrightness(Display display, float brightness) override {
        return mHal->setDisplayBrightness(display, brightness);
    }

  protected:
    using BaseType2_1 = V2_1::hal::detail::ComposerClientImpl<Interface, Hal>;
    using BaseType2_1::mHal;
    using BaseType2_1::mResources;
    std::unique_ptr<V2_1::hal::ComposerCommandEngine> createCommandEngine() override {
        return std::make_unique<ComposerCommandEngine>(
            mHal, static_cast<V2_2::hal::ComposerResources*>(mResources.get()));
    }

  private:
    using BaseType2_2 = V2_2::hal::detail::ComposerClientImpl<Interface, Hal>;
    using BaseType2_1::mCommandEngine;
    using BaseType2_1::mCommandEngineMutex;
};

}  // namespace detail

using ComposerClient = detail::ComposerClientImpl<IComposerClient, ComposerHal>;

}  // namespace hal
}  // namespace V2_3
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
