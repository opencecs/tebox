/*
 * Copyright 2016 The Android Open Source Project
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
#warning "Composer.h included without LOG_TAG"
#endif

#include <array>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

#include <android/hardware/graphics/composer/2.1/IComposer.h>
#include <android/hardware/graphics/composer/2.1/IComposerClient.h>
#include <composer-hal/2.1/ComposerClient.h>
#include <composer-hal/2.1/ComposerHal.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace hal {

namespace detail {

// ComposerImpl implements V2_*::IComposer on top of V2_*::ComposerHal
template <typename Interface, typename Hal>
class ComposerImpl : public Interface {
   public:
    static std::unique_ptr<ComposerImpl> create(std::unique_ptr<Hal> hal) {
        return std::make_unique<ComposerImpl>(std::move(hal));
    }

    ComposerImpl(std::unique_ptr<Hal> hal) : mHal(std::move(hal)) {}

    // IComposer 2.1 interface

    Return<void> getCapabilities(IComposer::getCapabilities_cb hidl_cb) override {
        const std::array<IComposer::Capability, 3> all_caps = {{
            IComposer::Capability::SIDEBAND_STREAM,
            IComposer::Capability::SKIP_CLIENT_COLOR_TRANSFORM,
            IComposer::Capability::PRESENT_FENCE_IS_NOT_RELIABLE,
        }};

        std::vector<IComposer::Capability> caps;
        for (auto cap : all_caps) {
            if (mHal->hasCapability(static_cast<hwc2_capability_t>(cap))) {
                caps.push_back(cap);
            }
        }

        // we do not have HWC2_CAPABILITY_SKIP_VALIDATE defined in
        // IComposer::Capability.  However, this is defined in hwcomposer2.h,
        // so if the device returns it, add it manually to be returned to the
        // client
        if (mHal->hasCapability(HWC2_CAPABILITY_SKIP_VALIDATE)) {
            caps.push_back(static_cast<IComposer::Capability>(HWC2_CAPABILITY_SKIP_VALIDATE));
        }

        hidl_vec<IComposer::Capability> caps_reply;
        caps_reply.setToExternal(caps.data(), caps.size());
        hidl_cb(caps_reply);
        return Void();
    }

    Return<void> dumpDebugInfo(IComposer::dumpDebugInfo_cb hidl_cb) override {
        hidl_cb(mHal->dumpDebugInfo());
        return Void();
    }

    Return<void> createClient(IComposer::createClient_cb hidl_cb) override {
        std::unique_lock<std::mutex> lock(mClientMutex);
        if (!waitForClientDestroyedLocked(lock)) {
            hidl_cb(Error::NO_RESOURCES, nullptr);
            return Void();
        }

        sp<IComposerClient> client = createClient();
        if (!client) {
            hidl_cb(Error::NO_RESOURCES, nullptr);
            return Void();
        }

        mClient = client;
        hidl_cb(Error::NONE, client);
        return Void();
    }

   protected:
    bool waitForClientDestroyedLocked(std::unique_lock<std::mutex>& lock) {
        if (mClient != nullptr) {
            using namespace std::chrono_literals;

            // In surface flinger we delete a composer client on one thread and
            // then create a new client on another thread. Although surface
            // flinger ensures the calls are made in that sequence (destroy and
            // then create), sometimes the calls land in the composer service
            // inverted (create and then destroy). Wait for a brief period to
            // see if the existing client is destroyed.
            ALOGD("waiting for previous client to be destroyed");
            mClientDestroyedCondition.wait_for(lock, 1s,
                                               [this]() -> bool { return mClient == nullptr; });
            if (mClient != nullptr) {
                ALOGD("previous client was not destroyed");
            }
        }

        return mClient == nullptr;
    }

    void onClientDestroyed() {
        std::lock_guard<std::mutex> lock(mClientMutex);
        mClient.clear();
        mClientDestroyedCondition.notify_all();
    }

    virtual IComposerClient* createClient() {
        auto client = ComposerClient::create(mHal.get());
        if (!client) {
            return nullptr;
        }

        auto clientDestroyed = [this]() { onClientDestroyed(); };
        client->setOnClientDestroyed(clientDestroyed);

        return client.release();
    }

    const std::unique_ptr<Hal> mHal;

    std::mutex mClientMutex;
    wp<IComposerClient> mClient;
    std::condition_variable mClientDestroyedCondition;
};

}  // namespace detail

using Composer = detail::ComposerImpl<IComposer, ComposerHal>;

}  // namespace hal
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
