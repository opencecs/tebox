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

#include <android/hardware/graphics/composer/2.2/IComposer.h>
#include <composer-hal/2.1/Composer.h>
#include <composer-hal/2.2/ComposerClient.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_2 {
namespace hal {

namespace detail {

// ComposerImpl implements V2_*::IComposer on top of V2_*::ComposerHal
template <typename Interface, typename Hal>
class ComposerImpl : public V2_1::hal::detail::ComposerImpl<Interface, Hal> {
   public:
    static std::unique_ptr<ComposerImpl> create(std::unique_ptr<Hal> hal) {
        return std::make_unique<ComposerImpl>(std::move(hal));
    }

    ComposerImpl(std::unique_ptr<Hal> hal) : BaseType2_1(std::move(hal)) {}

   protected:
    V2_1::IComposerClient* createClient() override {
        auto client = ComposerClient::create(mHal.get());
        if (!client) {
            return nullptr;
        }

        auto clientDestroyed = [this]() { onClientDestroyed(); };
        client->setOnClientDestroyed(clientDestroyed);

        return client.release();
    }

   private:
    using BaseType2_1 = V2_1::hal::detail::ComposerImpl<Interface, Hal>;
    using BaseType2_1::mHal;
    using BaseType2_1::onClientDestroyed;
};

}  // namespace detail

using Composer = detail::ComposerImpl<IComposer, ComposerHal>;

}  // namespace hal
}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
