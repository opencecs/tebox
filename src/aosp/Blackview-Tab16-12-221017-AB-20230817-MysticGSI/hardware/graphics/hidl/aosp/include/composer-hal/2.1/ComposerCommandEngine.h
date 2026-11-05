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

#include <vector>

#include <composer-command-buffer/2.1/ComposerCommandBuffer.h>
#include <composer-hal/2.1/ComposerHal.h>
#include <composer-resources/2.1/ComposerResources.h>
// TODO remove hwcomposer_defs.h dependency
#include <hardware/hwcomposer_defs.h>
#include <log/log.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace hal {

// TODO own a CommandReaderBase rather than subclassing
class ComposerCommandEngine : protected CommandReaderBase {
   public:
     ComposerCommandEngine(ComposerHal* hal, ComposerResources* resources)
         : mHal(hal), mResources(resources) {
         mWriter = createCommandWriter(kWriterInitialSize);
     }

    virtual ~ComposerCommandEngine() = default;

    bool setInputMQDescriptor(const MQDescriptorSync<uint32_t>& descriptor) {
        return setMQDescriptor(descriptor);
    }

    Error execute(uint32_t inLength, const hidl_vec<hidl_handle>& inHandles, bool* outQueueChanged,
                  uint32_t* outCommandLength, hidl_vec<hidl_handle>* outCommandHandles) {
        if (!readQueue(inLength, inHandles)) {
            return Error::BAD_PARAMETER;
        }

        IComposerClient::Command command;
        uint16_t length = 0;
        while (!isEmpty()) {
            if (!beginCommand(&command, &length)) {
                break;
            }

            bool parsed = executeCommand(command, length);
            endCommand();

            if (!parsed) {
                ALOGE("failed to parse command 0x%x, length %" PRIu16, command, length);
                break;
            }
        }

        if (!isEmpty()) {
            return Error::BAD_PARAMETER;
        }

        return mWriter->writeQueue(outQueueChanged, outCommandLength, outCommandHandles)
                       ? Error::NONE
                       : Error::NO_RESOURCES;
    }

    const MQDescriptorSync<uint32_t>* getOutputMQDescriptor() { return mWriter->getMQDescriptor(); }

    void reset() {
        CommandReaderBase::reset();
        mWriter->reset();
    }

   protected:
    virtual bool executeCommand(IComposerClient::Command command, uint16_t length) {
        switch (command) {
            case IComposerClient::Command::SELECT_DISPLAY:
                return executeSelectDisplay(length);
            case IComposerClient::Command::SELECT_LAYER:
                return executeSelectLayer(length);
            case IComposerClient::Command::SET_COLOR_TRANSFORM:
                return executeSetColorTransform(length);
            case IComposerClient::Command::SET_CLIENT_TARGET:
                return executeSetClientTarget(length);
            case IComposerClient::Command::SET_OUTPUT_BUFFER:
                return executeSetOutputBuffer(length);
            case IComposerClient::Command::VALIDATE_DISPLAY:
                return executeValidateDisplay(length);
            case IComposerClient::Command::PRESENT_OR_VALIDATE_DISPLAY:
                return executePresentOrValidateDisplay(length);
            case IComposerClient::Command::ACCEPT_DISPLAY_CHANGES:
                return executeAcceptDisplayChanges(length);
            case IComposerClient::Command::PRESENT_DISPLAY:
                return executePresentDisplay(length);
            case IComposerClient::Command::SET_LAYER_CURSOR_POSITION:
                return executeSetLayerCursorPosition(length);
            case IComposerClient::Command::SET_LAYER_BUFFER:
                return executeSetLayerBuffer(length);
            case IComposerClient::Command::SET_LAYER_SURFACE_DAMAGE:
                return executeSetLayerSurfaceDamage(length);
            case IComposerClient::Command::SET_LAYER_BLEND_MODE:
                return executeSetLayerBlendMode(length);
            case IComposerClient::Command::SET_LAYER_COLOR:
                return executeSetLayerColor(length);
            case IComposerClient::Command::SET_LAYER_COMPOSITION_TYPE:
                return executeSetLayerCompositionType(length);
            case IComposerClient::Command::SET_LAYER_DATASPACE:
                return executeSetLayerDataspace(length);
            case IComposerClient::Command::SET_LAYER_DISPLAY_FRAME:
                return executeSetLayerDisplayFrame(length);
            case IComposerClient::Command::SET_LAYER_PLANE_ALPHA:
                return executeSetLayerPlaneAlpha(length);
            case IComposerClient::Command::SET_LAYER_SIDEBAND_STREAM:
                return executeSetLayerSidebandStream(length);
            case IComposerClient::Command::SET_LAYER_SOURCE_CROP:
                return executeSetLayerSourceCrop(length);
            case IComposerClient::Command::SET_LAYER_TRANSFORM:
                return executeSetLayerTransform(length);
            case IComposerClient::Command::SET_LAYER_VISIBLE_REGION:
                return executeSetLayerVisibleRegion(length);
            case IComposerClient::Command::SET_LAYER_Z_ORDER:
                return executeSetLayerZOrder(length);
            default:
                return false;
        }
    }

    virtual std::unique_ptr<CommandWriterBase> createCommandWriter(size_t writerInitialSize) {
        return std::make_unique<CommandWriterBase>(writerInitialSize);
    }

    virtual Error executeValidateDisplayInternal() {
        std::vector<Layer> changedLayers;
        std::vector<IComposerClient::Composition> compositionTypes;
        uint32_t displayRequestMask = 0x0;
        std::vector<Layer> requestedLayers;
        std::vector<uint32_t> requestMasks;

        auto err = mHal->validateDisplay(mCurrentDisplay, &changedLayers, &compositionTypes,
                                         &displayRequestMask, &requestedLayers, &requestMasks);
        mResources->setDisplayMustValidateState(mCurrentDisplay, false);
        if (err == Error::NONE) {
            mWriter->setChangedCompositionTypes(changedLayers, compositionTypes);
            mWriter->setDisplayRequests(displayRequestMask, requestedLayers, requestMasks);
        } else {
            mWriter->setError(getCommandLoc(), err);
        }
        return err;
    }

    bool executeSelectDisplay(uint16_t length) {
        if (length != CommandWriterBase::kSelectDisplayLength) {
            return false;
        }

        mCurrentDisplay = read64();
        mWriter->selectDisplay(mCurrentDisplay);

        return true;
    }

    bool executeSelectLayer(uint16_t length) {
        if (length != CommandWriterBase::kSelectLayerLength) {
            return false;
        }

        mCurrentLayer = read64();

        return true;
    }

    bool executeSetColorTransform(uint16_t length) {
        if (length != CommandWriterBase::kSetColorTransformLength) {
            return false;
        }

        float matrix[16];
        for (int i = 0; i < 16; i++) {
            matrix[i] = readFloat();
        }
        auto transform = readSigned();

        auto err = mHal->setColorTransform(mCurrentDisplay, matrix, transform);
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executeSetClientTarget(uint16_t length) {
        // 4 parameters followed by N rectangles
        if ((length - 4) % 4 != 0) {
            return false;
        }

        bool useCache = false;
        auto slot = read();
        auto rawHandle = readHandle(&useCache);
        auto fence = readFence();
        auto dataspace = readSigned();
        auto damage = readRegion((length - 4) / 4);
        bool closeFence = true;

        const native_handle_t* clientTarget;
        ComposerResources::ReplacedHandle replacedClientTarget(true);
        auto err = mResources->getDisplayClientTarget(mCurrentDisplay, slot, useCache, rawHandle,
                                                      &clientTarget, &replacedClientTarget);
        if (err == Error::NONE) {
            err = mHal->setClientTarget(mCurrentDisplay, clientTarget, fence, dataspace, damage);
            if (err == Error::NONE) {
                closeFence = false;
            }
        }
        if (closeFence) {
            close(fence);
        }
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executeSetOutputBuffer(uint16_t length) {
        if (length != CommandWriterBase::kSetOutputBufferLength) {
            return false;
        }

        bool useCache = false;
        auto slot = read();
        auto rawhandle = readHandle(&useCache);
        auto fence = readFence();
        bool closeFence = true;

        const native_handle_t* outputBuffer;
        ComposerResources::ReplacedHandle replacedOutputBuffer(true);
        auto err = mResources->getDisplayOutputBuffer(mCurrentDisplay, slot, useCache, rawhandle,
                                                      &outputBuffer, &replacedOutputBuffer);
        if (err == Error::NONE) {
            err = mHal->setOutputBuffer(mCurrentDisplay, outputBuffer, fence);
            if (err == Error::NONE) {
                closeFence = false;
            }
        }
        if (closeFence) {
            close(fence);
        }
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executeValidateDisplay(uint16_t length) {
        if (length != CommandWriterBase::kValidateDisplayLength) {
            return false;
        }
        executeValidateDisplayInternal();
        return true;
    }

    bool executePresentOrValidateDisplay(uint16_t length) {
        if (length != CommandWriterBase::kPresentOrValidateDisplayLength) {
            return false;
        }

        // First try to Present as is.
        if (mHal->hasCapability(HWC2_CAPABILITY_SKIP_VALIDATE)) {
            int presentFence = -1;
            std::vector<Layer> layers;
            std::vector<int> fences;
            auto err = mResources->mustValidateDisplay(mCurrentDisplay)
                           ? Error::NOT_VALIDATED
                           : mHal->presentDisplay(mCurrentDisplay, &presentFence, &layers, &fences);
            if (err == Error::NONE) {
                mWriter->setPresentOrValidateResult(1);
                mWriter->setPresentFence(presentFence);
                mWriter->setReleaseFences(layers, fences);
                return true;
            }
        }

        // Present has failed. We need to fallback to validate
        auto err = executeValidateDisplayInternal();
        if (err == Error::NONE) {
            mWriter->setPresentOrValidateResult(0);
        }

        return true;
    }

    bool executeAcceptDisplayChanges(uint16_t length) {
        if (length != CommandWriterBase::kAcceptDisplayChangesLength) {
            return false;
        }

        auto err = mHal->acceptDisplayChanges(mCurrentDisplay);
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executePresentDisplay(uint16_t length) {
        if (length != CommandWriterBase::kPresentDisplayLength) {
            return false;
        }

        int presentFence = -1;
        std::vector<Layer> layers;
        std::vector<int> fences;
        auto err = mHal->presentDisplay(mCurrentDisplay, &presentFence, &layers, &fences);
        if (err == Error::NONE) {
            mWriter->setPresentFence(presentFence);
            mWriter->setReleaseFences(layers, fences);
        } else {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executeSetLayerCursorPosition(uint16_t length) {
        if (length != CommandWriterBase::kSetLayerCursorPositionLength) {
            return false;
        }

        auto err = mHal->setLayerCursorPosition(mCurrentDisplay, mCurrentLayer, readSigned(),
                                                readSigned());
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executeSetLayerBuffer(uint16_t length) {
        if (length != CommandWriterBase::kSetLayerBufferLength) {
            return false;
        }

        bool useCache = false;
        auto slot = read();
        auto rawHandle = readHandle(&useCache);
        auto fence = readFence();
        bool closeFence = true;

        const native_handle_t* buffer;
        ComposerResources::ReplacedHandle replacedBuffer(true);
        auto err = mResources->getLayerBuffer(mCurrentDisplay, mCurrentLayer, slot, useCache,
                                              rawHandle, &buffer, &replacedBuffer);
        if (err == Error::NONE) {
            err = mHal->setLayerBuffer(mCurrentDisplay, mCurrentLayer, buffer, fence);
            if (err == Error::NONE) {
                closeFence = false;
            }
        }
        if (closeFence) {
            close(fence);
        }
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executeSetLayerSurfaceDamage(uint16_t length) {
        // N rectangles
        if (length % 4 != 0) {
            return false;
        }

        auto damage = readRegion(length / 4);
        auto err = mHal->setLayerSurfaceDamage(mCurrentDisplay, mCurrentLayer, damage);
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executeSetLayerBlendMode(uint16_t length) {
        if (length != CommandWriterBase::kSetLayerBlendModeLength) {
            return false;
        }

        auto err = mHal->setLayerBlendMode(mCurrentDisplay, mCurrentLayer, readSigned());
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executeSetLayerColor(uint16_t length) {
        if (length != CommandWriterBase::kSetLayerColorLength) {
            return false;
        }

        auto err = mHal->setLayerColor(mCurrentDisplay, mCurrentLayer, readColor());
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executeSetLayerCompositionType(uint16_t length) {
        if (length != CommandWriterBase::kSetLayerCompositionTypeLength) {
            return false;
        }

        auto err = mHal->setLayerCompositionType(mCurrentDisplay, mCurrentLayer, readSigned());
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executeSetLayerDataspace(uint16_t length) {
        if (length != CommandWriterBase::kSetLayerDataspaceLength) {
            return false;
        }

        auto err = mHal->setLayerDataspace(mCurrentDisplay, mCurrentLayer, readSigned());
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executeSetLayerDisplayFrame(uint16_t length) {
        if (length != CommandWriterBase::kSetLayerDisplayFrameLength) {
            return false;
        }

        auto err = mHal->setLayerDisplayFrame(mCurrentDisplay, mCurrentLayer, readRect());
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executeSetLayerPlaneAlpha(uint16_t length) {
        if (length != CommandWriterBase::kSetLayerPlaneAlphaLength) {
            return false;
        }

        auto err = mHal->setLayerPlaneAlpha(mCurrentDisplay, mCurrentLayer, readFloat());
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executeSetLayerSidebandStream(uint16_t length) {
        if (length != CommandWriterBase::kSetLayerSidebandStreamLength) {
            return false;
        }

        auto rawHandle = readHandle();

        const native_handle_t* stream;
        ComposerResources::ReplacedHandle replacedStream(false);
        auto err = mResources->getLayerSidebandStream(mCurrentDisplay, mCurrentLayer, rawHandle,
                                                      &stream, &replacedStream);
        if (err == Error::NONE) {
            err = mHal->setLayerSidebandStream(mCurrentDisplay, mCurrentLayer, stream);
        }
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executeSetLayerSourceCrop(uint16_t length) {
        if (length != CommandWriterBase::kSetLayerSourceCropLength) {
            return false;
        }

        auto err = mHal->setLayerSourceCrop(mCurrentDisplay, mCurrentLayer, readFRect());
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executeSetLayerTransform(uint16_t length) {
        if (length != CommandWriterBase::kSetLayerTransformLength) {
            return false;
        }

        auto err = mHal->setLayerTransform(mCurrentDisplay, mCurrentLayer, readSigned());
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executeSetLayerVisibleRegion(uint16_t length) {
        // N rectangles
        if (length % 4 != 0) {
            return false;
        }

        auto region = readRegion(length / 4);
        auto err = mHal->setLayerVisibleRegion(mCurrentDisplay, mCurrentLayer, region);
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    bool executeSetLayerZOrder(uint16_t length) {
        if (length != CommandWriterBase::kSetLayerZOrderLength) {
            return false;
        }

        auto err = mHal->setLayerZOrder(mCurrentDisplay, mCurrentLayer, read());
        if (err != Error::NONE) {
            mWriter->setError(getCommandLoc(), err);
        }

        return true;
    }

    hwc_rect_t readRect() {
        return hwc_rect_t{
            readSigned(), readSigned(), readSigned(), readSigned(),
        };
    }

    std::vector<hwc_rect_t> readRegion(size_t count) {
        std::vector<hwc_rect_t> region;
        region.reserve(count);
        while (count > 0) {
            region.emplace_back(readRect());
            count--;
        }

        return region;
    }

    hwc_frect_t readFRect() {
        return hwc_frect_t{
            readFloat(), readFloat(), readFloat(), readFloat(),
        };
    }

    // 64KiB minus a small space for metadata such as read/write pointers
    static constexpr size_t kWriterInitialSize = 64 * 1024 / sizeof(uint32_t) - 16;

    ComposerHal* mHal;
    ComposerResources* mResources;
    std::unique_ptr<CommandWriterBase> mWriter;

    Display mCurrentDisplay = 0;
    Layer mCurrentLayer = 0;
};

}  // namespace hal
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
