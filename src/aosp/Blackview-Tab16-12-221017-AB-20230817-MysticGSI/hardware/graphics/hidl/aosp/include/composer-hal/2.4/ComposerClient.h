/*
 * Copyright 2019 The Android Open Source Project
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

#include <android/hardware/graphics/composer/2.4/IComposerCallback.h>
#include <android/hardware/graphics/composer/2.4/IComposerClient.h>
#include <composer-hal/2.4/ComposerCommandEngine.h>
#include <composer-hal/2.4/ComposerHal.h>
#include <composer-resources/2.1/ComposerResources.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_4 {
namespace hal {

namespace detail {

// ComposerClientImpl implements V2_*::IComposerClient on top of V2_*::ComposerHal
template <typename Interface, typename Hal>
class ComposerClientImpl : public V2_3::hal::detail::ComposerClientImpl<Interface, Hal> {
  public:
    ComposerClientImpl(Hal* hal) : BaseType2_3(hal) {}

    ~ComposerClientImpl() override { mHal->unregisterEventCallback_2_4(); }

    class HalEventCallback : public Hal::EventCallback_2_4 {
      public:
        HalEventCallback(Hal* hal, const sp<IComposerCallback> callback,
                         V2_1::hal::ComposerResources* resources)
            : mHal(hal), mCallback(callback), mResources(resources) {}

        void onHotplug(Display display, IComposerCallback::Connection connected) override {
            if (connected == IComposerCallback::Connection::CONNECTED) {
                if (mResources->hasDisplay(display)) {
                    // This is a subsequent hotplug "connected" for a display. This signals a
                    // display change and thus the framework may want to reallocate buffers. We
                    // need to free all cached handles, since they are holding a strong reference
                    // to the underlying buffers.
                    V2_1::hal::detail::ComposerClientImpl<Interface, Hal>::cleanDisplayResources(
                            display, mResources, mHal);
                    mResources->removeDisplay(display);
                }
                mResources->addPhysicalDisplay(display);
            } else if (connected == IComposerCallback::Connection::DISCONNECTED) {
                mResources->removeDisplay(display);
            }

            auto ret = mCallback->onHotplug(display, connected);
            ALOGE_IF(!ret.isOk(), "failed to send onHotplug: %s", ret.description().c_str());
        }

        void onRefresh(Display display) override {
            mResources->setDisplayMustValidateState(display, true);
            auto ret = mCallback->onRefresh(display);
            ALOGE_IF(!ret.isOk(), "failed to send onRefresh: %s", ret.description().c_str());
        }

        void onVsync(Display display, int64_t timestamp) override {
            auto ret = mCallback->onVsync(display, timestamp);
            ALOGE_IF(!ret.isOk(), "failed to send onVsync: %s", ret.description().c_str());
        }

        void onVsync_2_4(Display display, int64_t timestamp,
                         VsyncPeriodNanos vsyncPeriodNanos) override {
            auto ret = mCallback->onVsync_2_4(display, timestamp, vsyncPeriodNanos);
            ALOGE_IF(!ret.isOk(), "failed to send onVsync_2_4: %s", ret.description().c_str());
        }

        void onVsyncPeriodTimingChanged(Display display,
                                        const VsyncPeriodChangeTimeline& updatedTimeline) override {
            auto ret = mCallback->onVsyncPeriodTimingChanged(display, updatedTimeline);
            ALOGE_IF(!ret.isOk(), "failed to send onVsyncPeriodTimingChanged: %s",
                     ret.description().c_str());
        }

        void onSeamlessPossible(Display display) override {
            auto ret = mCallback->onSeamlessPossible(display);
            ALOGE_IF(!ret.isOk(), "failed to send onSealmessPossible: %s",
                     ret.description().c_str());
        }

      protected:
        Hal* const mHal;
        const sp<IComposerCallback> mCallback;
        V2_1::hal::ComposerResources* const mResources;
    };

    Return<void> registerCallback_2_4(const sp<IComposerCallback>& callback) override {
        // no locking as we require this function to be called only once
        mHalEventCallback_2_4 =
                std::make_unique<HalEventCallback>(mHal, callback, mResources.get());
        mHal->registerEventCallback_2_4(mHalEventCallback_2_4.get());
        return Void();
    }

    Return<void> getDisplayCapabilities_2_4(
            Display display, IComposerClient::getDisplayCapabilities_2_4_cb hidl_cb) override {
        std::vector<IComposerClient::DisplayCapability> capabilities;
        Error error = mHal->getDisplayCapabilities_2_4(display, &capabilities);
        hidl_cb(error, capabilities);
        return Void();
    }

    Return<void> getDisplayConnectionType(
            Display display, IComposerClient::getDisplayConnectionType_cb hidl_cb) override {
        IComposerClient::DisplayConnectionType type;
        Error error = mHal->getDisplayConnectionType(display, &type);
        hidl_cb(error, type);
        return Void();
    }

    Return<void> getDisplayAttribute_2_4(
            Display display, Config config, IComposerClient::Attribute attribute,
            IComposerClient::getDisplayAttribute_2_4_cb hidl_cb) override {
        int32_t value = 0;
        Error error = mHal->getDisplayAttribute_2_4(display, config, attribute, &value);
        hidl_cb(error, value);
        return Void();
    }

    Return<void> getDisplayVsyncPeriod(Display display,
                                       IComposerClient::getDisplayVsyncPeriod_cb hidl_cb) override {
        VsyncPeriodNanos vsyncPeriods;
        Error error = mHal->getDisplayVsyncPeriod(display, &vsyncPeriods);
        hidl_cb(error, vsyncPeriods);
        return Void();
    }

    Return<void> setActiveConfigWithConstraints(
            Display display, Config config,
            const IComposerClient::VsyncPeriodChangeConstraints& vsyncPeriodChangeConstraints,
            IComposerClient::setActiveConfigWithConstraints_cb hidl_cb) override {
        VsyncPeriodChangeTimeline timeline = {};
        Error error = mHal->setActiveConfigWithConstraints(display, config,
                                                           vsyncPeriodChangeConstraints, &timeline);
        hidl_cb(error, timeline);
        return Void();
    }

    Return<Error> setAutoLowLatencyMode(Display display, bool on) override {
        return mHal->setAutoLowLatencyMode(display, on);
    }

    Return<void> getSupportedContentTypes(
            Display display, IComposerClient::getSupportedContentTypes_cb hidl_cb) override {
        std::vector<IComposerClient::ContentType> supportedContentTypes;
        Error error = mHal->getSupportedContentTypes(display, &supportedContentTypes);

        hidl_cb(error, supportedContentTypes);
        return Void();
    }

    Return<Error> setContentType(Display display,
                                 IComposerClient::ContentType contentType) override {
        return mHal->setContentType(display, contentType);
    }

    Return<void> getLayerGenericMetadataKeys(
            IComposerClient::getLayerGenericMetadataKeys_cb hidl_cb) override {
        std::vector<IComposerClient::LayerGenericMetadataKey> keys;
        Error error = mHal->getLayerGenericMetadataKeys(&keys);
        hidl_cb(error, keys);
        return Void();
    }

    static std::unique_ptr<ComposerClientImpl> create(Hal* hal) {
        auto client = std::make_unique<ComposerClientImpl>(hal);
        return client->init() ? std::move(client) : nullptr;
    }

  protected:
    std::unique_ptr<V2_1::hal::ComposerCommandEngine> createCommandEngine() override {
        return std::make_unique<ComposerCommandEngine>(
                mHal, static_cast<V2_2::hal::ComposerResources*>(mResources.get()));
    }

  private:
    using BaseType2_3 = V2_3::hal::detail::ComposerClientImpl<Interface, Hal>;
    using BaseType2_1 = V2_1::hal::detail::ComposerClientImpl<Interface, Hal>;
    using BaseType2_1::mHal;
    using BaseType2_1::mResources;
    std::unique_ptr<HalEventCallback> mHalEventCallback_2_4;
};

}  // namespace detail

using ComposerClient = detail::ComposerClientImpl<IComposerClient, ComposerHal>;

}  // namespace hal
}  // namespace V2_4
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
