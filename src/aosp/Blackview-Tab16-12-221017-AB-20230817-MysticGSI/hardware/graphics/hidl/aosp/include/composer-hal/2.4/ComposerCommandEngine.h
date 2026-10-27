/*
 * Copyright 2020 The Android Open Source Project
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
#warning "ComposerCommandEngine.h included without LOG_TAG"
#endif

#include <composer-command-buffer/2.4/ComposerCommandBuffer.h>
#include <composer-hal/2.1/ComposerCommandEngine.h>
#include <composer-hal/2.3/ComposerCommandEngine.h>
#include <composer-hal/2.4/ComposerHal.h>
#include <composer-resources/2.2/ComposerResources.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_4 {
namespace hal {

class ComposerCommandEngine : public V2_3::hal::ComposerCommandEngine {
  public:
    ComposerCommandEngine(ComposerHal* hal, V2_2::hal::ComposerResources* resources)
        : BaseType2_3(hal, resources), mHal(hal) {}

  protected:
    std::unique_ptr<V2_1::CommandWriterBase> createCommandWriter(
            size_t writerInitialSize) override {
        return std::make_unique<CommandWriterBase>(writerInitialSize);
    }

  private:
    using BaseType2_1 = V2_1::hal::ComposerCommandEngine;
    using BaseType2_3 = V2_3::hal::ComposerCommandEngine;
    using BaseType2_1::mWriter;

    V2_1::Error executeValidateDisplayInternal() override {
        std::vector<Layer> changedLayers;
        std::vector<IComposerClient::Composition> compositionTypes;
        uint32_t displayRequestMask = 0x0;
        std::vector<Layer> requestedLayers;
        std::vector<uint32_t> requestMasks;
        IComposerClient::ClientTargetProperty clientTargetProperty{PixelFormat::RGBA_8888,
                                                                   Dataspace::UNKNOWN};

        auto err = mHal->validateDisplay_2_4(mCurrentDisplay, &changedLayers, &compositionTypes,
                                             &displayRequestMask, &requestedLayers, &requestMasks,
                                             &clientTargetProperty);
        mResources->setDisplayMustValidateState(mCurrentDisplay, false);
        if (err == Error::NONE) {
            mWriter->setChangedCompositionTypes(changedLayers, compositionTypes);
            mWriter->setDisplayRequests(displayRequestMask, requestedLayers, requestMasks);
            getWriter()->setClientTargetProperty(clientTargetProperty);
        } else {
            mWriter->setError(getCommandLoc(), static_cast<V2_1::Error>(err));
        }
        return static_cast<V2_1::Error>(err);
    }

    CommandWriterBase* getWriter() { return static_cast<CommandWriterBase*>(mWriter.get()); }

    bool executeCommand(V2_1::IComposerClient::Command command, uint16_t length) override {
        switch (static_cast<IComposerClient::Command>(command)) {
            case IComposerClient::Command::SET_LAYER_GENERIC_METADATA:
                return executeSetLayerGenericMetadata(length);
            default:
                return BaseType2_3::executeCommand(command, length);
        }
    }

    bool executeSetLayerGenericMetadata(uint16_t length) {
        // We expect at least two buffer lengths and a mandatory flag
        if (length < 3) {
            return false;
        }

        const uint32_t keySize = read();
        std::string key;
        key.resize(keySize);
        readBlob(keySize, key.data());

        const bool mandatory = read();

        const uint32_t valueSize = read();
        std::vector<uint8_t> value(valueSize);
        readBlob(valueSize, value.data());

        auto error = mHal->setLayerGenericMetadata(mCurrentDisplay, mCurrentLayer, key, mandatory,
                                                   value);
        if (error != Error::NONE) {
            // The error cast is safe because setLayerGenericMetadata doesn't
            // return any of the new values added in V2_4::Error
            mWriter->setError(getCommandLoc(), static_cast<V2_1::Error>(error));
        }

        return true;
    }

    ComposerHal* mHal;
};

}  // namespace hal
}  // namespace V2_4
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
