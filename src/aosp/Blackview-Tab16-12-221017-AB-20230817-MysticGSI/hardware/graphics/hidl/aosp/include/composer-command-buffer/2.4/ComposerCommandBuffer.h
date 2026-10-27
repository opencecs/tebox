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
#warn "ComposerCommandBuffer.h included without LOG_TAG"
#endif

#undef LOG_NDEBUG
#define LOG_NDEBUG 0

#include <android/hardware/graphics/composer/2.4/IComposer.h>
#include <android/hardware/graphics/composer/2.4/IComposerClient.h>
#include <composer-command-buffer/2.3/ComposerCommandBuffer.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_4 {

using android::hardware::MessageQueue;
using android::hardware::graphics::composer::V2_4::Error;
using android::hardware::graphics::composer::V2_4::IComposerClient;

// This class helps build a command queue.  Note that all sizes/lengths are in
// units of uint32_t's.
class CommandWriterBase : public V2_3::CommandWriterBase {
  public:
    static constexpr uint16_t kSetClientTargetPropertyLength = 2;

    CommandWriterBase(uint32_t initialMaxSize) : V2_3::CommandWriterBase(initialMaxSize) {}

    void setClientTargetProperty(
            const IComposerClient::ClientTargetProperty& clientTargetProperty) {
        beginCommand(IComposerClient::Command::SET_CLIENT_TARGET_PROPERTY,
                     kSetClientTargetPropertyLength);
        writeSigned(static_cast<int32_t>(clientTargetProperty.pixelFormat));
        writeSigned(static_cast<int32_t>(clientTargetProperty.dataspace));
        endCommand();
    }

    void setLayerGenericMetadata(const hidl_string& key, const bool mandatory,
                                 const hidl_vec<uint8_t>& value) {
        const size_t commandSize = 3 + sizeToElements(key.size()) + sizeToElements(value.size());
        if (commandSize > std::numeric_limits<uint16_t>::max()) {
            LOG_FATAL("Too much generic metadata (%zu elements)", commandSize);
            return;
        }

        beginCommand(IComposerClient::Command::SET_LAYER_GENERIC_METADATA,
                     static_cast<uint16_t>(commandSize));
        write(key.size());
        writeBlob(key.size(), reinterpret_cast<const unsigned char*>(key.c_str()));
        write(mandatory);
        write(value.size());
        writeBlob(value.size(), value.data());
        endCommand();
    }

  protected:
    uint32_t sizeToElements(uint32_t size) { return (size + 3) / 4; }
};

// This class helps parse a command queue.  Note that all sizes/lengths are in
// units of uint32_t's.
class CommandReaderBase : public V2_3::CommandReaderBase {
  public:
    CommandReaderBase() : V2_3::CommandReaderBase() {}
};

}  // namespace V2_4
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
