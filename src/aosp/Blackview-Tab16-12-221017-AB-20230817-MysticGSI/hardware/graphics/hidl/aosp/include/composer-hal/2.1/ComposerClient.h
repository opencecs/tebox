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

#include <memory>
#include <mutex>
#include <vector>

#include <android/hardware/graphics/composer/2.1/IComposerClient.h>
#include <composer-hal/2.1/ComposerCommandEngine.h>
#include <composer-hal/2.1/ComposerHal.h>
#include <composer-resources/2.1/ComposerResources.h>
#include <log/log.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace hal {

namespace detail {

// ComposerClientImpl implements V2_*::IComposerClient on top of V2_*::ComposerHal
template <typename Interface, typename Hal>
class ComposerClientImpl : public Interface {
   public:
    static std::unique_ptr<ComposerClientImpl> create(Hal* hal) {
        auto client = std::make_unique<ComposerClientImpl>(hal);
        return client->init() ? std::move(client) : nullptr;
    }

    ComposerClientImpl(Hal* hal) : mHal(hal) {}

    virtual ~ComposerClientImpl() {
        // not initialized
        if (!mCommandEngine) {
            return;
        }

        ALOGD("destroying composer client");

        mHal->unregisterEventCallback();
        destroyResources();
        if (mOnClientDestroyed) {
            mOnClientDestroyed();
        }

        ALOGD("removed composer client");
    }

    bool init() {
        mResources = createResources();
        if (!mResources) {
            ALOGE("failed to create composer resources");
            return false;
        }

        mCommandEngine = createCommandEngine();

        return true;
    }

    void setOnClientDestroyed(std::function<void()> onClientDestroyed) {
        mOnClientDestroyed = onClientDestroyed;
    }

    // IComposerClient 2.1 interface

    class HalEventCallback : public Hal::EventCallback {
       public:
         HalEventCallback(Hal* hal, const sp<IComposerCallback> callback,
                          ComposerResources* resources)
             : mHal(hal), mCallback(callback), mResources(resources) {}

         void onHotplug(Display display, IComposerCallback::Connection connected) {
             if (connected == IComposerCallback::Connection::CONNECTED) {
                 if (mResources->hasDisplay(display)) {
                     // This is a subsequent hotplug "connected" for a display. This signals a
                     // display change and thus the framework may want to reallocate buffers. We
                     // need to free all cached handles, since they are holding a strong reference
                     // to the underlying buffers.
                     cleanDisplayResources(display, mResources, mHal);
                     mResources->removeDisplay(display);
                 }
                 mResources->addPhysicalDisplay(display);
             } else if (connected == IComposerCallback::Connection::DISCONNECTED) {
                 mResources->removeDisplay(display);
             }

             auto ret = mCallback->onHotplug(display, connected);
             ALOGE_IF(!ret.isOk(), "failed to send onHotplug: %s", ret.description().c_str());
         }

        void onRefresh(Display display) {
            mResources->setDisplayMustValidateState(display, true);
            auto ret = mCallback->onRefresh(display);
            ALOGE_IF(!ret.isOk(), "failed to send onRefresh: %s", ret.description().c_str());
        }

        void onVsync(Display display, int64_t timestamp) {
            auto ret = mCallback->onVsync(display, timestamp);
            ALOGE_IF(!ret.isOk(), "failed to send onVsync: %s", ret.description().c_str());
        }

       protected:
         Hal* const mHal;
         const sp<IComposerCallback> mCallback;
         ComposerResources* const mResources;
    };

    Return<void> registerCallback(const sp<IComposerCallback>& callback) override {
        // no locking as we require this function to be called only once
        mHalEventCallback = std::make_unique<HalEventCallback>(mHal, callback, mResources.get());
        mHal->registerEventCallback(mHalEventCallback.get());
        return Void();
    }

    Return<uint32_t> getMaxVirtualDisplayCount() override {
        return mHal->getMaxVirtualDisplayCount();
    }

    Return<void> createVirtualDisplay(uint32_t width, uint32_t height, PixelFormat formatHint,
                                      uint32_t outputBufferSlotCount,
                                      IComposerClient::createVirtualDisplay_cb hidl_cb) override {
        Display display = 0;
        Error err = mHal->createVirtualDisplay(width, height, &formatHint, &display);
        if (err == Error::NONE) {
            mResources->addVirtualDisplay(display, outputBufferSlotCount);
        }

        hidl_cb(err, display, formatHint);
        return Void();
    }

    Return<Error> destroyVirtualDisplay(Display display) override {
        Error err = mHal->destroyVirtualDisplay(display);
        if (err == Error::NONE) {
            mResources->removeDisplay(display);
        }

        return err;
    }

    Return<void> createLayer(Display display, uint32_t bufferSlotCount,
                             IComposerClient::createLayer_cb hidl_cb) override {
        Layer layer = 0;
        Error err = mHal->createLayer(display, &layer);
        if (err == Error::NONE) {
            err = mResources->addLayer(display, layer, bufferSlotCount);
            if (err != Error::NONE) {
                // The display entry may have already been removed by onHotplug.
                // Note: We do not destroy the layer on this error as the hotplug
                // disconnect invalidates the display id. The implementation should
                // ensure all layers for the display are destroyed.
                layer = 0;
            }
        }

        hidl_cb(err, layer);
        return Void();
    }

    Return<Error> destroyLayer(Display display, Layer layer) override {
        Error err = mHal->destroyLayer(display, layer);
        if (err == Error::NONE) {
            mResources->removeLayer(display, layer);
        }

        return err;
    }

    Return<void> getActiveConfig(Display display,
                                 IComposerClient::getActiveConfig_cb hidl_cb) override {
        Config config = 0;
        Error err = mHal->getActiveConfig(display, &config);
        hidl_cb(err, config);
        return Void();
    }

    Return<Error> getClientTargetSupport(Display display, uint32_t width, uint32_t height,
                                         PixelFormat format, Dataspace dataspace) override {
        Error err = mHal->getClientTargetSupport(display, width, height, format, dataspace);
        return err;
    }

    Return<void> getColorModes(Display display,
                               IComposerClient::getColorModes_cb hidl_cb) override {
        hidl_vec<ColorMode> modes;
        Error err = mHal->getColorModes(display, &modes);
        hidl_cb(err, modes);
        return Void();
    }

    Return<void> getDisplayAttribute(Display display, Config config,
                                     IComposerClient::Attribute attribute,
                                     IComposerClient::getDisplayAttribute_cb hidl_cb) override {
        int32_t value = 0;
        Error err = mHal->getDisplayAttribute(display, config, attribute, &value);
        hidl_cb(err, value);
        return Void();
    }

    Return<void> getDisplayConfigs(Display display,
                                   IComposerClient::getDisplayConfigs_cb hidl_cb) override {
        hidl_vec<Config> configs;
        Error err = mHal->getDisplayConfigs(display, &configs);
        hidl_cb(err, configs);
        return Void();
    }

    Return<void> getDisplayName(Display display,
                                IComposerClient::getDisplayName_cb hidl_cb) override {
        hidl_string name;
        Error err = mHal->getDisplayName(display, &name);
        hidl_cb(err, name);
        return Void();
    }

    Return<void> getDisplayType(Display display,
                                IComposerClient::getDisplayType_cb hidl_cb) override {
        IComposerClient::DisplayType type = IComposerClient::DisplayType::INVALID;
        Error err = mHal->getDisplayType(display, &type);
        hidl_cb(err, type);
        return Void();
    }

    Return<void> getDozeSupport(Display display,
                                IComposerClient::getDozeSupport_cb hidl_cb) override {
        bool support = false;
        Error err = mHal->getDozeSupport(display, &support);
        hidl_cb(err, support);
        return Void();
    }

    Return<void> getHdrCapabilities(Display display,
                                    IComposerClient::getHdrCapabilities_cb hidl_cb) override {
        hidl_vec<Hdr> types;
        float max_lumi = 0.0f;
        float max_avg_lumi = 0.0f;
        float min_lumi = 0.0f;
        Error err = mHal->getHdrCapabilities(display, &types, &max_lumi, &max_avg_lumi, &min_lumi);
        hidl_cb(err, types, max_lumi, max_avg_lumi, min_lumi);
        return Void();
    }

    Return<Error> setActiveConfig(Display display, Config config) override {
        Error err = mHal->setActiveConfig(display, config);
        return err;
    }

    Return<Error> setColorMode(Display display, ColorMode mode) override {
        Error err = mHal->setColorMode(display, mode);
        return err;
    }

    Return<Error> setPowerMode(Display display, IComposerClient::PowerMode mode) override {
        Error err = mHal->setPowerMode(display, mode);
        return err;
    }

    Return<Error> setVsyncEnabled(Display display, IComposerClient::Vsync enabled) override {
        Error err = mHal->setVsyncEnabled(display, enabled);
        return err;
    }

    Return<Error> setClientTargetSlotCount(Display display,
                                           uint32_t clientTargetSlotCount) override {
        return mResources->setDisplayClientTargetCacheSize(display, clientTargetSlotCount);
    }

    Return<Error> setInputCommandQueue(const MQDescriptorSync<uint32_t>& descriptor) override {
        std::lock_guard<std::mutex> lock(mCommandEngineMutex);
        return mCommandEngine->setInputMQDescriptor(descriptor) ? Error::NONE : Error::NO_RESOURCES;
    }

    Return<void> getOutputCommandQueue(IComposerClient::getOutputCommandQueue_cb hidl_cb) override {
        // no locking as we require this function to be called inside
        // executeCommands_cb
        auto outDescriptor = mCommandEngine->getOutputMQDescriptor();
        if (outDescriptor) {
            hidl_cb(Error::NONE, *outDescriptor);
        } else {
            hidl_cb(Error::NO_RESOURCES, CommandQueueType::Descriptor());
        }

        return Void();
    }

    Return<void> executeCommands(uint32_t inLength, const hidl_vec<hidl_handle>& inHandles,
                                 IComposerClient::executeCommands_cb hidl_cb) override {
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
    virtual std::unique_ptr<ComposerResources> createResources() {
        return ComposerResources::create();
    }

    virtual std::unique_ptr<ComposerCommandEngine> createCommandEngine() {
        return std::make_unique<ComposerCommandEngine>(mHal, mResources.get());
    }

    static void cleanDisplayResources(Display display, ComposerResources* const resources,
                                      Hal* const hal) {
        size_t cacheSize;
        Error err = resources->getDisplayClientTargetCacheSize(display, &cacheSize);
        if (err == Error::NONE) {
            for (int slot = 0; slot < cacheSize; slot++) {
                ComposerResources::ReplacedHandle replacedBuffer(/*isBuffer*/ true);
                // Replace the buffer slots with NULLs. Keep the old handle until it is
                // replaced in ComposerHal, otherwise we risk leaving a dangling pointer.
                const native_handle_t* clientTarget = nullptr;
                err = resources->getDisplayClientTarget(display, slot, /*useCache*/ true,
                                                        /*rawHandle*/ nullptr, &clientTarget,
                                                        &replacedBuffer);
                if (err != Error::NONE) {
                    continue;
                }
                const std::vector<hwc_rect_t> damage;
                err = hal->setClientTarget(display, clientTarget, /*fence*/ -1, 0, damage);
                ALOGE_IF(err != Error::NONE,
                         "Can't clean slot %d of the client target buffer"
                         "cache for display %" PRIu64,
                         slot, display);
            }
        } else {
            ALOGE("Can't clean client target cache for display %" PRIu64, display);
        }

        err = resources->getDisplayOutputBufferCacheSize(display, &cacheSize);
        if (err == Error::NONE) {
            for (int slot = 0; slot < cacheSize; slot++) {
                // Replace the buffer slots with NULLs. Keep the old handle until it is
                // replaced in ComposerHal, otherwise we risk leaving a dangling pointer.
                ComposerResources::ReplacedHandle replacedBuffer(/*isBuffer*/ true);
                const native_handle_t* outputBuffer = nullptr;
                err = resources->getDisplayOutputBuffer(display, slot, /*useCache*/ true,
                                                        /*rawHandle*/ nullptr, &outputBuffer,
                                                        &replacedBuffer);
                if (err != Error::NONE) {
                    continue;
                }
                err = hal->setOutputBuffer(display, outputBuffer, /*fence*/ -1);
                ALOGE_IF(err != Error::NONE,
                         "Can't clean slot %d of the output buffer cache"
                         "for display %" PRIu64,
                         slot, display);
            }
        } else {
            ALOGE("Can't clean output buffer cache for display %" PRIu64, display);
        }
    }

    void destroyResources() {
        // We want to call hwc2_close here (and move hwc2_open to the
        // constructor), with the assumption that hwc2_close would
        //
        //  - clean up all resources owned by the client
        //  - make sure all displays are blank (since there is no layer)
        //
        // But since SF used to crash at this point, different hwcomposer2
        // implementations behave differently on hwc2_close.  Our only portable
        // choice really is to abort().  But that is not an option anymore
        // because we might also have VTS or VR as clients that can come and go.
        //
        // Below we manually clean all resources (layers and virtual
        // displays), and perform a presentDisplay afterwards.
        mResources->clear([this](Display display, bool isVirtual, const std::vector<Layer> layers) {
            ALOGW("destroying client resources for display %" PRIu64, display);

            for (auto layer : layers) {
                mHal->destroyLayer(display, layer);
            }

            if (isVirtual) {
                mHal->destroyVirtualDisplay(display);
            } else {
                ALOGW("performing a final presentDisplay");

                std::vector<Layer> changedLayers;
                std::vector<IComposerClient::Composition> compositionTypes;
                uint32_t displayRequestMask = 0;
                std::vector<Layer> requestedLayers;
                std::vector<uint32_t> requestMasks;
                mHal->validateDisplay(display, &changedLayers, &compositionTypes,
                                      &displayRequestMask, &requestedLayers, &requestMasks);

                mHal->acceptDisplayChanges(display);

                int32_t presentFence = -1;
                std::vector<Layer> releasedLayers;
                std::vector<int32_t> releaseFences;
                mHal->presentDisplay(display, &presentFence, &releasedLayers, &releaseFences);
                if (presentFence >= 0) {
                    close(presentFence);
                }
                for (auto fence : releaseFences) {
                    if (fence >= 0) {
                        close(fence);
                    }
                }
            }
        });

        mResources.reset();
    }

    Hal* const mHal;

    std::unique_ptr<ComposerResources> mResources;

    std::mutex mCommandEngineMutex;
    std::unique_ptr<ComposerCommandEngine> mCommandEngine;

    std::function<void()> mOnClientDestroyed;
    std::unique_ptr<HalEventCallback> mHalEventCallback;
};

}  // namespace detail

using ComposerClient = detail::ComposerClientImpl<IComposerClient, ComposerHal>;

}  // namespace hal
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
