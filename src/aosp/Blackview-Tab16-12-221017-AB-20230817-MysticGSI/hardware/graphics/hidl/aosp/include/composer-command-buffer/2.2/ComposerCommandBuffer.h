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
#warn "ComposerCommandBuffer.h included without LOG_TAG"
#endif

#undef LOG_NDEBUG
#define LOG_NDEBUG 0

#include <algorithm>
#include <limits>
#include <memory>
#include <vector>

#include <inttypes.h>
#include <string.h>

#include <android/hardware/graphics/composer/2.2/IComposer.h>
#include <android/hardware/graphics/composer/2.2/IComposerClient.h>
#include <fmq/MessageQueue.h>
#include <log/log.h>
#include <sync/sync.h>

#include <composer-command-buffer/2.1/ComposerCommandBuffer.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_2 {

using android::hardware::MessageQueue;
using android::hardware::graphics::common::V1_0::ColorTransform;
using android::hardware::graphics::common::V1_0::Transform;
using android::hardware::graphics::common::V1_1::Dataspace;
using android::hardware::graphics::composer::V2_1::Config;
using android::hardware::graphics::composer::V2_1::Display;
using android::hardware::graphics::composer::V2_1::Error;
using android::hardware::graphics::composer::V2_1::IComposerCallback;
using android::hardware::graphics::composer::V2_1::Layer;
using android::hardware::graphics::composer::V2_2::IComposerClient;

using CommandQueueType = MessageQueue<uint32_t, kSynchronizedReadWrite>;

// This class helps build a command queue.  Note that all sizes/lengths are in
// units of uint32_t's.
class CommandWriterBase : public V2_1::CommandWriterBase {
   public:
    CommandWriterBase(uint32_t initialMaxSize) : V2_1::CommandWriterBase(initialMaxSize) {}

    void setClientTarget(uint32_t slot, const native_handle_t* target, int acquireFence,
                         Dataspace dataspace, const std::vector<IComposerClient::Rect>& damage) {
        setClientTargetInternal(slot, target, acquireFence, static_cast<int32_t>(dataspace),
                                damage);
    }

    void setLayerDataspace(Dataspace dataspace) {
        setLayerDataspaceInternal(static_cast<int32_t>(dataspace));
    }

    static constexpr uint16_t kSetLayerFloatColorLength = 4;
    void setLayerFloatColor(IComposerClient::FloatColor color) {
        beginCommand(IComposerClient::Command::SET_LAYER_FLOAT_COLOR, kSetLayerFloatColorLength);
        writeFloatColor(color);
        endCommand();
    }

    void setLayerPerFrameMetadata(const hidl_vec<IComposerClient::PerFrameMetadata>& metadataVec) {
        beginCommand(IComposerClient::Command::SET_LAYER_PER_FRAME_METADATA,
                     metadataVec.size() * 2);
        for (const auto& metadata : metadataVec) {
            writeSigned(static_cast<int32_t>(metadata.key));
            writeFloat(metadata.value);
        }
        endCommand();
    }

   protected:
    void writeFloatColor(const IComposerClient::FloatColor& color) {
        writeFloat(color.r);
        writeFloat(color.g);
        writeFloat(color.b);
        writeFloat(color.a);
    }
};

// This class helps parse a command queue.  Note that all sizes/lengths are in
// units of uint32_t's.
class CommandReaderBase : public V2_1::CommandReaderBase {
   public:
    CommandReaderBase() : V2_1::CommandReaderBase(){};

   protected:
    IComposerClient::FloatColor readFloatColor() {
        float r = readFloat();
        float g = readFloat();
        float b = readFloat();
        float a = readFloat();
        return IComposerClient::FloatColor{r, g, b, a};
    }
};

}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
