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

#include <composer-command-buffer/2.3/ComposerCommandBuffer.h>
#include <composer-hal/2.1/ComposerCommandEngine.h>
#include <composer-hal/2.2/ComposerCommandEngine.h>
#include <composer-hal/2.3/ComposerHal.h>
#include <composer-resources/2.2/ComposerResources.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_3 {
namespace hal {

class ComposerCommandEngine : public V2_2::hal::ComposerCommandEngine {
   public:
    ComposerCommandEngine(ComposerHal* hal, V2_2::hal::ComposerResources* resources)
        : BaseType2_2(hal, resources), mHal(hal) {}

   protected:
    bool executeCommand(V2_1::IComposerClient::Command command, uint16_t length) override {
        switch (static_cast<IComposerClient::Command>(command)) {
            case IComposerClient::Command::SET_LAYER_COLOR_TRANSFORM:
                return executeSetLayerColorTransform(length);
            case IComposerClient::Command::SET_LAYER_PER_FRAME_METADATA_BLOBS:
                return executeSetLayerPerFrameMetadataBlobs(length);
            default:
                return BaseType2_2::executeCommand(command, length);
        }
    }

    std::unique_ptr<V2_1::CommandWriterBase> createCommandWriter(
            size_t writerInitialSize) override {
        return std::make_unique<CommandWriterBase>(writerInitialSize);
    }

    bool executeSetLayerColorTransform(uint16_t length) {
        if (length != CommandWriterBase::kSetLayerColorTransformLength) {
            return false;
        }

        float matrix[16];
        for (int i = 0; i < 16; i++) {
            matrix[i] = readFloat();
        }
        auto err = mHal->setLayerColorTransform(mCurrentDisplay, mCurrentLayer, matrix);
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executeSetLayerPerFrameMetadataBlobs(uint16_t length) {
        // must have at least one metadata blob
        // of at least size 1 in queue (i.e {/*numBlobs=*/1, key, size, blob})
        if (length < 4) {
            return false;
        }

        uint32_t numBlobs = read();
        length--;

        std::vector<IComposerClient::PerFrameMetadataBlob> metadata;

        for (size_t i = 0; i < numBlobs; i++) {
            IComposerClient::PerFrameMetadataKey key =
                static_cast<IComposerClient::PerFrameMetadataKey>(readSigned());
            uint32_t blobSize = read();

            length -= 2;

            if (length * sizeof(uint32_t) < blobSize) {
                return false;
            }

            metadata.push_back({key, std::vector<uint8_t>()});
            IComposerClient::PerFrameMetadataBlob& metadataBlob = metadata.back();
            metadataBlob.blob.resize(blobSize);
            readBlob(blobSize, metadataBlob.blob.data());
        }
        auto err = mHal->setLayerPerFrameMetadataBlobs(mCurrentDisplay, mCurrentLayer, metadata);
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }
        return true;
    }

    void readBlob(uint32_t size, void* blob) {
        memcpy(blob, &mData[mDataRead], size);
        uint32_t numElements = size / sizeof(uint32_t);
        mDataRead += numElements;
        mDataRead += (size - numElements * sizeof(uint32_t) != 0) ? 1 : 0;
    }

   private:
    using BaseType2_1 = V2_1::hal::ComposerCommandEngine;
    using BaseType2_2 = V2_2::hal::ComposerCommandEngine;
    using BaseType2_1::mWriter;

    ComposerHal* mHal;
};

}  // namespace hal
}  // namespace V2_3
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
