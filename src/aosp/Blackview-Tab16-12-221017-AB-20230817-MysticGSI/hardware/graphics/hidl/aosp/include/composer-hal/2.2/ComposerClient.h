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

#include <android/hardware/graphics/composer/2.2/IComposerClient.h>
#include <composer-hal/2.1/ComposerClient.h>
#include <composer-hal/2.2/ComposerCommandEngine.h>
#include <composer-hal/2.2/ComposerHal.h>
#include <composer-resources/2.2/ComposerResources.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_2 {
namespace hal {

namespace detail {

// ComposerClientImpl implements V2_*::IComposerClient on top of V2_*::ComposerHal
template <typename Interface, typename Hal>
class ComposerClientImpl : public V2_1::hal::detail::ComposerClientImpl<Interface, Hal> {
   public:
    static std::unique_ptr<ComposerClientImpl> create(Hal* hal) {
        auto client = std::make_unique<ComposerClientImpl>(hal);
        return client->init() ? std::move(client) : nullptr;
    }

    ComposerClientImpl(Hal* hal) : BaseType2_1(hal) {}

    // IComposerClient 2.2 interface

    Return<void> getPerFrameMetadataKeys(
        Display display, IComposerClient::getPerFrameMetadataKeys_cb hidl_cb) override {
        std::vector<IComposerClient::PerFrameMetadataKey> keys;
        Error error = mHal->getPerFrameMetadataKeys(display, &keys);
        hidl_cb(error, keys);
        return Void();
    }

    Return<void> getReadbackBufferAttributes(
        Display display, IComposerClient::getReadbackBufferAttributes_cb hidl_cb) override {
        PixelFormat format = static_cast<PixelFormat>(0);
        Dataspace dataspace = Dataspace::UNKNOWN;
        Error error = mHal->getReadbackBufferAttributes(display, &format, &dataspace);
        hidl_cb(error, format, dataspace);
        return Void();
    }

    Return<void> getReadbackBufferFence(
        Display display, IComposerClient::getReadbackBufferFence_cb hidl_cb) override {
        base::unique_fd fenceFd;
        Error error = mHal->getReadbackBufferFence(display, &fenceFd);
        if (error != Error::NONE) {
            hidl_cb(error, nullptr);
            return Void();
        }

        NATIVE_HANDLE_DECLARE_STORAGE(fenceStorage, 1, 0);
        hidl_cb(error, getFenceHandle(fenceFd, fenceStorage));
        return Void();
    }

    Return<Error> setReadbackBuffer(Display display, const hidl_handle& buffer,
                                    const hidl_handle& releaseFence) override {
        base::unique_fd fenceFd;
        Error error = getFenceFd(releaseFence, &fenceFd);
        if (error != Error::NONE) {
            return error;
        }

        auto resources = static_cast<ComposerResources*>(mResources.get());
        const native_handle_t* readbackBuffer;
        ComposerResources::ReplacedHandle replacedReadbackBuffer(true);
        error = resources->getDisplayReadbackBuffer(display, buffer.getNativeHandle(),
                                                    &readbackBuffer, &replacedReadbackBuffer);
        if (error != Error::NONE) {
            return error;
        }

        return mHal->setReadbackBuffer(display, readbackBuffer, std::move(fenceFd));
    }

    Return<void> createVirtualDisplay_2_2(
        uint32_t width, uint32_t height, PixelFormat formatHint, uint32_t outputBufferSlotCount,
        IComposerClient::createVirtualDisplay_2_2_cb hidl_cb) override {
        Display display = 0;
        Error err = mHal->createVirtualDisplay_2_2(width, height, &formatHint, &display);
        if (err == Error::NONE) {
            mResources->addVirtualDisplay(display, outputBufferSlotCount);
        }

        hidl_cb(err, display, formatHint);
        return Void();
    }

    Return<Error> getClientTargetSupport_2_2(Display display, uint32_t width, uint32_t height,
                                             PixelFormat format, Dataspace dataspace) override {
        Error err = mHal->getClientTargetSupport_2_2(display, width, height, format, dataspace);
        return err;
    }

    Return<Error> setPowerMode_2_2(Display display, IComposerClient::PowerMode mode) override {
        return mHal->setPowerMode_2_2(display, mode);
    }

    Return<void> getColorModes_2_2(Display display,
                                   IComposerClient::getColorModes_2_2_cb hidl_cb) override {
        hidl_vec<ColorMode> modes;
        Error err = mHal->getColorModes_2_2(display, &modes);
        hidl_cb(err, modes);
        return Void();
    }

    Return<void> getRenderIntents(Display display, ColorMode mode,
                                  IComposerClient::getRenderIntents_cb hidl_cb) override {
        std::vector<RenderIntent> intents;
        Error err = mHal->getRenderIntents(display, mode, &intents);
        hidl_cb(err, intents);
        return Void();
    }

    Return<Error> setColorMode_2_2(Display display, ColorMode mode, RenderIntent intent) override {
        return mHal->setColorMode_2_2(display, mode, intent);
    }

    Return<void> getDataspaceSaturationMatrix(
        Dataspace dataspace, IComposerClient::getDataspaceSaturationMatrix_cb hidl_cb) override {
        if (dataspace != Dataspace::SRGB_LINEAR) {
            hidl_cb(Error::BAD_PARAMETER, std::array<float, 16>{0.0f}.data());
            return Void();
        }

        hidl_cb(Error::NONE, mHal->getDataspaceSaturationMatrix(dataspace).data());
        return Void();
    }

    Return<void> executeCommands_2_2(uint32_t inLength, const hidl_vec<hidl_handle>& inHandles,
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

   protected:
    std::unique_ptr<V2_1::hal::ComposerResources> createResources() override {
        return ComposerResources::create();
    }

    std::unique_ptr<V2_1::hal::ComposerCommandEngine> createCommandEngine() override {
        return std::make_unique<ComposerCommandEngine>(
            mHal, static_cast<ComposerResources*>(mResources.get()));
    }

    // convert fenceFd to or from hidl_handle
    static Error getFenceFd(const hidl_handle& fenceHandle, base::unique_fd* outFenceFd) {
        auto handle = fenceHandle.getNativeHandle();
        if (handle && handle->numFds > 1) {
            ALOGE("invalid fence handle with %d fds", handle->numFds);
            return Error::BAD_PARAMETER;
        }

        int fenceFd = (handle && handle->numFds == 1) ? handle->data[0] : -1;
        if (fenceFd >= 0) {
            fenceFd = dup(fenceFd);
            if (fenceFd < 0) {
                return Error::NO_RESOURCES;
            }
        }

        outFenceFd->reset(fenceFd);

        return Error::NONE;
    }

    static hidl_handle getFenceHandle(const base::unique_fd& fenceFd, char* handleStorage) {
        native_handle_t* handle = nullptr;
        if (fenceFd >= 0) {
            handle = native_handle_init(handleStorage, 1, 0);
            handle->data[0] = fenceFd;
        }

        return hidl_handle(handle);
    }

   private:
    using BaseType2_1 = V2_1::hal::detail::ComposerClientImpl<Interface, Hal>;
    using BaseType2_1::mCommandEngine;
    using BaseType2_1::mCommandEngineMutex;
    using BaseType2_1::mHal;
    using BaseType2_1::mResources;
};

}  // namespace detail

using ComposerClient = detail::ComposerClientImpl<IComposerClient, ComposerHal>;

}  // namespace hal
}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
