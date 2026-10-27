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
#warning "ComposerResources.h included without LOG_TAG"
#endif

#include <composer-resources/2.1/ComposerResources.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_2 {
namespace hal {

using V2_1::Display;
using V2_1::Error;
using V2_1::hal::ComposerHandleCache;
using V2_1::hal::ComposerHandleImporter;

class ComposerDisplayResource : public V2_1::hal::ComposerDisplayResource {
  public:
    ComposerDisplayResource(DisplayType type, ComposerHandleImporter& importer,
                            uint32_t outputBufferCacheSize)
        : V2_1::hal::ComposerDisplayResource(type, importer, outputBufferCacheSize),
          mReadbackBufferCache(importer, ComposerHandleCache::HandleType::BUFFER, 1) {}

    Error getReadbackBuffer(const native_handle_t* inHandle, const native_handle_t** outHandle,
                            const native_handle** outReplacedHandle) {
        const uint32_t slot = 0;
        const bool fromCache = false;
        return mReadbackBufferCache.getHandle(slot, fromCache, inHandle, outHandle,
                                              outReplacedHandle);
    }

  protected:
    ComposerHandleCache mReadbackBufferCache;
};

class ComposerResources : public V2_1::hal::ComposerResources {
  public:
    static std::unique_ptr<ComposerResources> create() {
        auto resources = std::make_unique<ComposerResources>();
        return resources->init() ? std::move(resources) : nullptr;
    }

    Error getDisplayReadbackBuffer(Display display, const native_handle_t* rawHandle,
                                   const native_handle_t** outHandle,
                                   ReplacedHandle* outReplacedHandle) {
        // import buffer
        const native_handle_t* importedHandle;
        Error error = mImporter.importBuffer(rawHandle, &importedHandle);
        if (error != Error::NONE) {
            return error;
        }

        std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);

        auto iter = mDisplayResources.find(display);
        if (iter == mDisplayResources.end()) {
            mImporter.freeBuffer(importedHandle);
            return Error::BAD_DISPLAY;
        }
        ComposerDisplayResource& displayResource =
                *static_cast<ComposerDisplayResource*>(iter->second.get());

        // update cache
        const native_handle_t* replacedHandle;
        error = displayResource.getReadbackBuffer(importedHandle, outHandle, &replacedHandle);
        if (error != Error::NONE) {
            mImporter.freeBuffer(importedHandle);
            return error;
        }

        outReplacedHandle->reset(&mImporter, replacedHandle);
        return Error::NONE;
    }

  protected:
    std::unique_ptr<V2_1::hal::ComposerDisplayResource> createDisplayResource(
            ComposerDisplayResource::DisplayType type, uint32_t outputBufferCacheSize) override {
        return std::make_unique<ComposerDisplayResource>(type, mImporter, outputBufferCacheSize);
    }
};

}  // namespace hal
}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
