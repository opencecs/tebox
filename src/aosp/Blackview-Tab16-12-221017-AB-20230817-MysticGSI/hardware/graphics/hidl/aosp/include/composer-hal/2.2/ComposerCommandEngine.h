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
#warning "ComposerCommandEngine.h included without LOG_TAG"
#endif

#include <composer-command-buffer/2.2/ComposerCommandBuffer.h>
#include <composer-hal/2.1/ComposerCommandEngine.h>
#include <composer-hal/2.2/ComposerHal.h>
#include <composer-resources/2.2/ComposerResources.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_2 {
namespace hal {

class ComposerCommandEngine : public V2_1::hal::ComposerCommandEngine {
   public:
    ComposerCommandEngine(ComposerHal* hal, ComposerResources* resources)
        : BaseType2_1(hal, resources), mHal(hal) {}

   protected:
    bool executeCommand(V2_1::IComposerClient::Command command, uint16_t length) override {
        switch (static_cast<IComposerClient::Command>(command)) {
            case IComposerClient::Command::SET_LAYER_PER_FRAME_METADATA:
                return executeSetLayerPerFrameMetadata(length);
            case IComposerClient::Command::SET_LAYER_FLOAT_COLOR:
                return executeSetLayerFloatColor(length);
            default:
                return BaseType2_1::executeCommand(command, length);
        }
    }

    std::unique_ptr<V2_1::CommandWriterBase> createCommandWriter(
            size_t writerInitialSize) override {
        return std::make_unique<CommandWriterBase>(writerInitialSize);
    }

    bool executeSetLayerPerFrameMetadata(uint16_t length) {
        // (key, value) pairs
        if (length % 2 != 0) {
            return false;
        }

        std::vector<IComposerClient::PerFrameMetadata> metadata;
        metadata.reserve(length / 2);
        while (length > 0) {
            metadata.emplace_back(IComposerClient::PerFrameMetadata{
                static_cast<IComposerClient::PerFrameMetadataKey>(readSigned()), readFloat()});
            length -= 2;
        }

        auto err = mHal->setLayerPerFrameMetadata(mCurrentDisplay, mCurrentLayer, metadata);
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executeSetLayerFloatColor(uint16_t length) {
        if (length != CommandWriterBase::kSetLayerFloatColorLength) {
            return false;
        }

        auto err = mHal->setLayerFloatColor(mCurrentDisplay, mCurrentLayer, readFloatColor());
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    IComposerClient::FloatColor readFloatColor() {
        return IComposerClient::FloatColor{readFloat(), readFloat(), readFloat(), readFloat()};
    }

   private:
    using BaseType2_1 = V2_1::hal::ComposerCommandEngine;
    using BaseType2_1::mWriter;

    ComposerHal* mHal;
};

}  // namespace hal
}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
