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
#warning "Composer.h included without LOG_TAG"
#endif

#include <android/hardware/graphics/composer/2.3/IComposer.h>
#include <composer-hal/2.2/Composer.h>
#include <composer-hal/2.3/ComposerClient.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_3 {
namespace hal {

namespace detail {

// ComposerImpl implements V2_*::IComposer on top of V2_*::ComposerHal
template <typename Interface, typename Hal>
class ComposerImpl : public V2_2::hal::detail::ComposerImpl<Interface, Hal> {
   public:
    static std::unique_ptr<ComposerImpl> create(std::unique_ptr<Hal> hal) {
        return std::make_unique<ComposerImpl>(std::move(hal));
    }

    explicit ComposerImpl(std::unique_ptr<Hal> hal) : BaseType2_2(std::move(hal)) {}

    // IComposer 2.3 interface

    Return<void> createClient_2_3(IComposer::createClient_2_3_cb hidl_cb) override {
        std::unique_lock<std::mutex> lock(mClientMutex);
        if (!waitForClientDestroyedLocked(lock)) {
            hidl_cb(Error::NO_RESOURCES, nullptr);
            return Void();
        }

        sp<ComposerClient> client = ComposerClient::create(mHal.get()).release();
        if (!client) {
            hidl_cb(Error::NO_RESOURCES, nullptr);
            return Void();
        }

        auto clientDestroyed = [this]() { onClientDestroyed(); };
        client->setOnClientDestroyed(clientDestroyed);

        mClient = client;
        hidl_cb(Error::NONE, client);
        return Void();
    }

   private:
    using BaseType2_2 = V2_2::hal::detail::ComposerImpl<Interface, Hal>;
    using BaseType2_1 = V2_1::hal::detail::ComposerImpl<Interface, Hal>;

    using BaseType2_1::mClient;
    using BaseType2_1::mClientMutex;
    using BaseType2_1::mHal;
    using BaseType2_1::onClientDestroyed;
    using BaseType2_1::waitForClientDestroyedLocked;
};

}  // namespace detail

using Composer = detail::ComposerImpl<IComposer, ComposerHal>;

}  // namespace hal
}  // namespace V2_3
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
