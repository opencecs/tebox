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

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <android/hardware/graphics/composer/2.1/types.h>

#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <android/hardware/graphics/mapper/3.0/IMapper.h>
#include <android/hardware/graphics/mapper/4.0/IMapper.h>
#include <log/log.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace hal {

// wrapper for IMapper to import buffers and sideband streams
class ComposerHandleImporter {
  public:
    bool init();

    Error importBuffer(const native_handle_t* rawHandle, const native_handle_t** outBufferHandle);
    void freeBuffer(const native_handle_t* bufferHandle);
    Error importStream(const native_handle_t* rawHandle, const native_handle_t** outStreamHandle);
    void freeStream(const native_handle_t* streamHandle);

  private:
    sp<mapper::V2_0::IMapper> mMapper2;
    sp<mapper::V3_0::IMapper> mMapper3;
    sp<mapper::V4_0::IMapper> mMapper4;
};

class ComposerHandleCache {
  public:
    enum class HandleType {
        INVALID,
        BUFFER,
        STREAM,
    };

    ComposerHandleCache(ComposerHandleImporter& importer, HandleType type, uint32_t cacheSize);

    // must be initialized later with initCache
    ComposerHandleCache(ComposerHandleImporter& importer);

    ~ComposerHandleCache();

    ComposerHandleCache(const ComposerHandleCache&) = delete;
    ComposerHandleCache& operator=(const ComposerHandleCache&) = delete;

    bool initCache(HandleType type, uint32_t cacheSize);
    size_t getCacheSize() const;
    Error lookupCache(uint32_t slot, const native_handle_t** outHandle);
    Error updateCache(uint32_t slot, const native_handle_t* handle,
                      const native_handle** outReplacedHandle);

    // when fromCache is true, look up in the cache; otherwise, update the cache
    Error getHandle(uint32_t slot, bool fromCache, const native_handle_t* inHandle,
                    const native_handle_t** outHandle, const native_handle** outReplacedHandle);

  private:
    ComposerHandleImporter& mImporter;
    HandleType mHandleType = HandleType::INVALID;
    std::vector<const native_handle_t*> mHandles;
};

// layer resource
class ComposerLayerResource {
  public:
    ComposerLayerResource(ComposerHandleImporter& importer, uint32_t bufferCacheSize);

    virtual ~ComposerLayerResource() = default;

    Error getBuffer(uint32_t slot, bool fromCache, const native_handle_t* inHandle,
                    const native_handle_t** outHandle, const native_handle** outReplacedHandle);
    Error getSidebandStream(uint32_t slot, bool fromCache, const native_handle_t* inHandle,
                            const native_handle_t** outHandle,
                            const native_handle** outReplacedHandle);

  protected:
    ComposerHandleCache mBufferCache;
    ComposerHandleCache mSidebandStreamCache;
};

// display resource
class ComposerDisplayResource {
  public:
    enum class DisplayType {
        PHYSICAL,
        VIRTUAL,
    };

    virtual ~ComposerDisplayResource() = default;

    ComposerDisplayResource(DisplayType type, ComposerHandleImporter& importer,
                            uint32_t outputBufferCacheSize);

    bool initClientTargetCache(uint32_t cacheSize);
    size_t getClientTargetCacheSize() const;
    size_t getOutputBufferCacheSize() const;
    bool isVirtual() const;

    Error getClientTarget(uint32_t slot, bool fromCache, const native_handle_t* inHandle,
                          const native_handle_t** outHandle,
                          const native_handle** outReplacedHandle);

    Error getOutputBuffer(uint32_t slot, bool fromCache, const native_handle_t* inHandle,
                          const native_handle_t** outHandle,
                          const native_handle** outReplacedHandle);

    bool addLayer(Layer layer, std::unique_ptr<ComposerLayerResource> layerResource);
    bool removeLayer(Layer layer);
    ComposerLayerResource* findLayerResource(Layer layer);
    std::vector<Layer> getLayers() const;

    void setMustValidateState(bool mustValidate);

    bool mustValidate() const;

  protected:
    const DisplayType mType;
    ComposerHandleCache mClientTargetCache;
    ComposerHandleCache mOutputBufferCache;
    bool mMustValidate;

    std::unordered_map<Layer, std::unique_ptr<ComposerLayerResource>> mLayerResources;
};

class ComposerResources {
  public:
    static std::unique_ptr<ComposerResources> create();

    ComposerResources() = default;
    virtual ~ComposerResources() = default;

    bool init();

    using RemoveDisplay =
            std::function<void(Display display, bool isVirtual, const std::vector<Layer>& layers)>;
    void clear(RemoveDisplay removeDisplay);

    bool hasDisplay(Display display);
    Error addPhysicalDisplay(Display display);
    Error addVirtualDisplay(Display display, uint32_t outputBufferCacheSize);

    Error removeDisplay(Display display);

    Error setDisplayClientTargetCacheSize(Display display, uint32_t clientTargetCacheSize);
    Error getDisplayClientTargetCacheSize(Display display, size_t* outCacheSize);
    Error getDisplayOutputBufferCacheSize(Display display, size_t* outCacheSize);

    Error addLayer(Display display, Layer layer, uint32_t bufferCacheSize);
    Error removeLayer(Display display, Layer layer);

    void setDisplayMustValidateState(Display display, bool mustValidate);

    bool mustValidateDisplay(Display display);

    // When a buffer in the cache is replaced by a new one, we must keep it
    // alive until it has been replaced in ComposerHal because it is still using
    // the old buffer.
    class ReplacedHandle {
      public:
        explicit ReplacedHandle(bool isBuffer) : mIsBuffer(isBuffer) {}
        ReplacedHandle(const ReplacedHandle&) = delete;
        ReplacedHandle& operator=(const ReplacedHandle&) = delete;

        ~ReplacedHandle() { reset(); }

        bool isBuffer() { return mIsBuffer; }

        void reset(ComposerHandleImporter* importer = nullptr,
                   const native_handle_t* handle = nullptr) {
            if (mHandle) {
                if (mIsBuffer) {
                    mImporter->freeBuffer(mHandle);
                } else {
                    mImporter->freeStream(mHandle);
                }
            }

            mImporter = importer;
            mHandle = handle;
        }

      private:
        bool mIsBuffer;
        ComposerHandleImporter* mImporter = nullptr;
        const native_handle_t* mHandle = nullptr;
    };

    Error getDisplayClientTarget(Display display, uint32_t slot, bool fromCache,
                                 const native_handle_t* rawHandle,
                                 const native_handle_t** outBufferHandle,
                                 ReplacedHandle* outReplacedBuffer);

    Error getDisplayOutputBuffer(Display display, uint32_t slot, bool fromCache,
                                 const native_handle_t* rawHandle,
                                 const native_handle_t** outBufferHandle,
                                 ReplacedHandle* outReplacedBuffer);

    Error getLayerBuffer(Display display, Layer layer, uint32_t slot, bool fromCache,
                         const native_handle_t* rawHandle, const native_handle_t** outBufferHandle,
                         ReplacedHandle* outReplacedBuffer);

    Error getLayerSidebandStream(Display display, Layer layer, const native_handle_t* rawHandle,
                                 const native_handle_t** outStreamHandle,
                                 ReplacedHandle* outReplacedStream);

  protected:
    virtual std::unique_ptr<ComposerDisplayResource> createDisplayResource(
            ComposerDisplayResource::DisplayType type, uint32_t outputBufferCacheSize);

    virtual std::unique_ptr<ComposerLayerResource> createLayerResource(uint32_t bufferCacheSize);

    ComposerDisplayResource* findDisplayResourceLocked(Display display);

    ComposerHandleImporter mImporter;

    std::mutex mDisplayResourcesMutex;
    std::unordered_map<Display, std::unique_ptr<ComposerDisplayResource>> mDisplayResources;

  private:
    enum class Cache {
        CLIENT_TARGET,
        OUTPUT_BUFFER,
        LAYER_BUFFER,
        LAYER_SIDEBAND_STREAM,
    };

    Error getHandle(Display display, Layer layer, uint32_t slot, Cache cache, bool fromCache,
                    const native_handle_t* rawHandle, const native_handle_t** outHandle,
                    ReplacedHandle* outReplacedHandle);
};

}  // namespace hal
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
