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

#define LOG_TAG "ComposerResources"

#include "composer-resources/2.1/ComposerResources.h"

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace hal {

bool ComposerHandleImporter::init() {
    mMapper4 = mapper::V4_0::IMapper::getService();
    if (mMapper4) {
        return true;
    }
    ALOGI_IF(!mMapper4, "failed to get mapper 4.0 service, falling back to mapper 3.0");

    mMapper3 = mapper::V3_0::IMapper::getService();
    if (mMapper3) {
        return true;
    }
    ALOGI_IF(!mMapper3, "failed to get mapper 3.0 service, falling back to mapper 2.0");

    mMapper2 = mapper::V2_0::IMapper::getService();
    ALOGE_IF(!mMapper2, "failed to get mapper 2.0 service");

    return mMapper2 != nullptr;
}

Error ComposerHandleImporter::importBuffer(const native_handle_t* rawHandle,
                                           const native_handle_t** outBufferHandle) {
    if (!rawHandle || (!rawHandle->numFds && !rawHandle->numInts)) {
        *outBufferHandle = nullptr;
        return Error::NONE;
    }

    const native_handle_t* bufferHandle;
    if (mMapper2) {
        mapper::V2_0::Error error;
        mMapper2->importBuffer(rawHandle, [&](const auto& tmpError, const auto& tmpBufferHandle) {
            error = tmpError;
            bufferHandle = static_cast<const native_handle_t*>(tmpBufferHandle);
        });
        if (error != mapper::V2_0::Error::NONE) {
            return Error::NO_RESOURCES;
        }
    }
    if (mMapper3) {
        mapper::V3_0::Error error;
        mMapper3->importBuffer(rawHandle, [&](const auto& tmpError, const auto& tmpBufferHandle) {
            error = tmpError;
            bufferHandle = static_cast<const native_handle_t*>(tmpBufferHandle);
        });
        if (error != mapper::V3_0::Error::NONE) {
            return Error::NO_RESOURCES;
        }
    }
    if (mMapper4) {
        mapper::V4_0::Error error;
        mMapper4->importBuffer(rawHandle, [&](const auto& tmpError, const auto& tmpBufferHandle) {
            error = tmpError;
            bufferHandle = static_cast<const native_handle_t*>(tmpBufferHandle);
        });
        if (error != mapper::V4_0::Error::NONE) {
            return Error::NO_RESOURCES;
        }
    }

    *outBufferHandle = bufferHandle;
    return Error::NONE;
}

void ComposerHandleImporter::freeBuffer(const native_handle_t* bufferHandle) {
    if (bufferHandle) {
        if (mMapper2) {
            mMapper2->freeBuffer(static_cast<void*>(const_cast<native_handle_t*>(bufferHandle)));
        } else if (mMapper3) {
            mMapper3->freeBuffer(static_cast<void*>(const_cast<native_handle_t*>(bufferHandle)));
        } else if (mMapper4) {
            mMapper4->freeBuffer(static_cast<void*>(const_cast<native_handle_t*>(bufferHandle)));
        }
    }
}

Error ComposerHandleImporter::importStream(const native_handle_t* rawHandle,
                                           const native_handle_t** outStreamHandle) {
    const native_handle_t* streamHandle = nullptr;
    if (rawHandle) {
        streamHandle = native_handle_clone(rawHandle);
        if (!streamHandle) {
            return Error::NO_RESOURCES;
        }
    }

    *outStreamHandle = streamHandle;
    return Error::NONE;
}

void ComposerHandleImporter::freeStream(const native_handle_t* streamHandle) {
    if (streamHandle) {
        native_handle_close(streamHandle);
        native_handle_delete(const_cast<native_handle_t*>(streamHandle));
    }
}

ComposerHandleCache::ComposerHandleCache(ComposerHandleImporter& importer, HandleType type,
                                         uint32_t cacheSize)
    : mImporter(importer), mHandleType(type), mHandles(cacheSize, nullptr) {}

// must be initialized later with initCache
ComposerHandleCache::ComposerHandleCache(ComposerHandleImporter& importer) : mImporter(importer) {}

ComposerHandleCache::~ComposerHandleCache() {
    switch (mHandleType) {
        case HandleType::BUFFER:
            for (auto handle : mHandles) {
                mImporter.freeBuffer(handle);
            }
            break;
        case HandleType::STREAM:
            for (auto handle : mHandles) {
                mImporter.freeStream(handle);
            }
            break;
        default:
            break;
    }
}

size_t ComposerHandleCache::getCacheSize() const {
    return mHandles.size();
}

bool ComposerHandleCache::initCache(HandleType type, uint32_t cacheSize) {
    // already initialized
    if (mHandleType != HandleType::INVALID) {
        return false;
    }

    mHandleType = type;
    mHandles.resize(cacheSize, nullptr);

    return true;
}

Error ComposerHandleCache::lookupCache(uint32_t slot, const native_handle_t** outHandle) {
    if (slot >= 0 && slot < mHandles.size()) {
        *outHandle = mHandles[slot];
        return Error::NONE;
    } else {
        return Error::BAD_PARAMETER;
    }
}

Error ComposerHandleCache::updateCache(uint32_t slot, const native_handle_t* handle,
                                       const native_handle** outReplacedHandle) {
    if (slot >= 0 && slot < mHandles.size()) {
        auto& cachedHandle = mHandles[slot];
        *outReplacedHandle = cachedHandle;
        cachedHandle = handle;
        return Error::NONE;
    } else {
        return Error::BAD_PARAMETER;
    }
}

// when fromCache is true, look up in the cache; otherwise, update the cache
Error ComposerHandleCache::getHandle(uint32_t slot, bool fromCache, const native_handle_t* inHandle,
                                     const native_handle_t** outHandle,
                                     const native_handle** outReplacedHandle) {
    if (fromCache) {
        *outReplacedHandle = nullptr;
        return lookupCache(slot, outHandle);
    } else {
        *outHandle = inHandle;
        return updateCache(slot, inHandle, outReplacedHandle);
    }
}

ComposerLayerResource::ComposerLayerResource(ComposerHandleImporter& importer,
                                             uint32_t bufferCacheSize)
    : mBufferCache(importer, ComposerHandleCache::HandleType::BUFFER, bufferCacheSize),
      mSidebandStreamCache(importer, ComposerHandleCache::HandleType::STREAM, 1) {}

Error ComposerLayerResource::getBuffer(uint32_t slot, bool fromCache,
                                       const native_handle_t* inHandle,
                                       const native_handle_t** outHandle,
                                       const native_handle** outReplacedHandle) {
    return mBufferCache.getHandle(slot, fromCache, inHandle, outHandle, outReplacedHandle);
}

Error ComposerLayerResource::getSidebandStream(uint32_t slot, bool fromCache,
                                               const native_handle_t* inHandle,
                                               const native_handle_t** outHandle,
                                               const native_handle** outReplacedHandle) {
    return mSidebandStreamCache.getHandle(slot, fromCache, inHandle, outHandle, outReplacedHandle);
}

ComposerDisplayResource::ComposerDisplayResource(DisplayType type, ComposerHandleImporter& importer,
                                                 uint32_t outputBufferCacheSize)
    : mType(type),
      mClientTargetCache(importer),
      mOutputBufferCache(importer, ComposerHandleCache::HandleType::BUFFER, outputBufferCacheSize),
      mMustValidate(true) {}

bool ComposerDisplayResource::initClientTargetCache(uint32_t cacheSize) {
    return mClientTargetCache.initCache(ComposerHandleCache::HandleType::BUFFER, cacheSize);
}

size_t ComposerDisplayResource::getClientTargetCacheSize() const {
    return mClientTargetCache.getCacheSize();
}

size_t ComposerDisplayResource::getOutputBufferCacheSize() const {
    return mOutputBufferCache.getCacheSize();
}

bool ComposerDisplayResource::isVirtual() const {
    return mType == DisplayType::VIRTUAL;
}

Error ComposerDisplayResource::getClientTarget(uint32_t slot, bool fromCache,
                                               const native_handle_t* inHandle,
                                               const native_handle_t** outHandle,
                                               const native_handle** outReplacedHandle) {
    return mClientTargetCache.getHandle(slot, fromCache, inHandle, outHandle, outReplacedHandle);
}

Error ComposerDisplayResource::getOutputBuffer(uint32_t slot, bool fromCache,
                                               const native_handle_t* inHandle,
                                               const native_handle_t** outHandle,
                                               const native_handle** outReplacedHandle) {
    return mOutputBufferCache.getHandle(slot, fromCache, inHandle, outHandle, outReplacedHandle);
}

bool ComposerDisplayResource::addLayer(Layer layer,
                                       std::unique_ptr<ComposerLayerResource> layerResource) {
    auto result = mLayerResources.emplace(layer, std::move(layerResource));
    return result.second;
}

bool ComposerDisplayResource::removeLayer(Layer layer) {
    return mLayerResources.erase(layer) > 0;
}

ComposerLayerResource* ComposerDisplayResource::findLayerResource(Layer layer) {
    auto layerIter = mLayerResources.find(layer);
    if (layerIter == mLayerResources.end()) {
        return nullptr;
    }

    return layerIter->second.get();
}

std::vector<Layer> ComposerDisplayResource::getLayers() const {
    std::vector<Layer> layers;
    layers.reserve(mLayerResources.size());
    for (const auto& layerKey : mLayerResources) {
        layers.push_back(layerKey.first);
    }
    return layers;
}

void ComposerDisplayResource::setMustValidateState(bool mustValidate) {
    mMustValidate = mustValidate;
}

bool ComposerDisplayResource::mustValidate() const {
    return mMustValidate;
}

std::unique_ptr<ComposerResources> ComposerResources::create() {
    auto resources = std::make_unique<ComposerResources>();
    return resources->init() ? std::move(resources) : nullptr;
}

bool ComposerResources::init() {
    return mImporter.init();
}

void ComposerResources::clear(RemoveDisplay removeDisplay) {
    std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
    for (const auto& displayKey : mDisplayResources) {
        Display display = displayKey.first;
        const ComposerDisplayResource& displayResource = *displayKey.second;
        removeDisplay(display, displayResource.isVirtual(), displayResource.getLayers());
    }
    mDisplayResources.clear();
}

bool ComposerResources::hasDisplay(Display display) {
    return mDisplayResources.count(display) > 0;
}

Error ComposerResources::addPhysicalDisplay(Display display) {
    auto displayResource = createDisplayResource(ComposerDisplayResource::DisplayType::PHYSICAL, 0);

    std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
    auto result = mDisplayResources.emplace(display, std::move(displayResource));
    return result.second ? Error::NONE : Error::BAD_DISPLAY;
}

Error ComposerResources::addVirtualDisplay(Display display, uint32_t outputBufferCacheSize) {
    auto displayResource = createDisplayResource(ComposerDisplayResource::DisplayType::VIRTUAL,
                                                 outputBufferCacheSize);

    std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
    auto result = mDisplayResources.emplace(display, std::move(displayResource));
    return result.second ? Error::NONE : Error::BAD_DISPLAY;
}

Error ComposerResources::removeDisplay(Display display) {
    std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
    return mDisplayResources.erase(display) > 0 ? Error::NONE : Error::BAD_DISPLAY;
}

Error ComposerResources::setDisplayClientTargetCacheSize(Display display,
                                                         uint32_t clientTargetCacheSize) {
    std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
    ComposerDisplayResource* displayResource = findDisplayResourceLocked(display);
    if (!displayResource) {
        return Error::BAD_DISPLAY;
    }

    return displayResource->initClientTargetCache(clientTargetCacheSize) ? Error::NONE
                                                                         : Error::BAD_PARAMETER;
}

Error ComposerResources::getDisplayClientTargetCacheSize(Display display, size_t* outCacheSize) {
    std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
    ComposerDisplayResource* displayResource = findDisplayResourceLocked(display);
    if (!displayResource) {
        return Error::BAD_DISPLAY;
    }
    *outCacheSize = displayResource->getClientTargetCacheSize();
    return Error::NONE;
}

Error ComposerResources::getDisplayOutputBufferCacheSize(Display display, size_t* outCacheSize) {
    std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
    ComposerDisplayResource* displayResource = findDisplayResourceLocked(display);
    if (!displayResource) {
        return Error::BAD_DISPLAY;
    }
    *outCacheSize = displayResource->getOutputBufferCacheSize();
    return Error::NONE;
}

Error ComposerResources::addLayer(Display display, Layer layer, uint32_t bufferCacheSize) {
    auto layerResource = createLayerResource(bufferCacheSize);

    std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
    ComposerDisplayResource* displayResource = findDisplayResourceLocked(display);
    if (!displayResource) {
        return Error::BAD_DISPLAY;
    }

    return displayResource->addLayer(layer, std::move(layerResource)) ? Error::NONE
                                                                      : Error::BAD_LAYER;
}

Error ComposerResources::removeLayer(Display display, Layer layer) {
    std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
    ComposerDisplayResource* displayResource = findDisplayResourceLocked(display);
    if (!displayResource) {
        return Error::BAD_DISPLAY;
    }

    return displayResource->removeLayer(layer) ? Error::NONE : Error::BAD_LAYER;
}

Error ComposerResources::getDisplayClientTarget(Display display, uint32_t slot, bool fromCache,
                                                const native_handle_t* rawHandle,
                                                const native_handle_t** outBufferHandle,
                                                ReplacedHandle* outReplacedBuffer) {
    return getHandle(display, 0, slot, Cache::CLIENT_TARGET, fromCache, rawHandle, outBufferHandle,
                     outReplacedBuffer);
}

Error ComposerResources::getDisplayOutputBuffer(Display display, uint32_t slot, bool fromCache,
                                                const native_handle_t* rawHandle,
                                                const native_handle_t** outBufferHandle,
                                                ReplacedHandle* outReplacedBuffer) {
    return getHandle(display, 0, slot, Cache::OUTPUT_BUFFER, fromCache, rawHandle, outBufferHandle,
                     outReplacedBuffer);
}

Error ComposerResources::getLayerBuffer(Display display, Layer layer, uint32_t slot, bool fromCache,
                                        const native_handle_t* rawHandle,
                                        const native_handle_t** outBufferHandle,
                                        ReplacedHandle* outReplacedBuffer) {
    return getHandle(display, layer, slot, Cache::LAYER_BUFFER, fromCache, rawHandle,
                     outBufferHandle, outReplacedBuffer);
}

Error ComposerResources::getLayerSidebandStream(Display display, Layer layer,
                                                const native_handle_t* rawHandle,
                                                const native_handle_t** outStreamHandle,
                                                ReplacedHandle* outReplacedStream) {
    return getHandle(display, layer, 0, Cache::LAYER_SIDEBAND_STREAM, false, rawHandle,
                     outStreamHandle, outReplacedStream);
}

void ComposerResources::setDisplayMustValidateState(Display display, bool mustValidate) {
    std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
    auto* displayResource = findDisplayResourceLocked(display);
    if (displayResource) {
        displayResource->setMustValidateState(mustValidate);
    }
}

bool ComposerResources::mustValidateDisplay(Display display) {
    std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
    auto* displayResource = findDisplayResourceLocked(display);
    if (displayResource) {
        return displayResource->mustValidate();
    }
    return false;
}

std::unique_ptr<ComposerDisplayResource> ComposerResources::createDisplayResource(
        ComposerDisplayResource::DisplayType type, uint32_t outputBufferCacheSize) {
    return std::make_unique<ComposerDisplayResource>(type, mImporter, outputBufferCacheSize);
}

std::unique_ptr<ComposerLayerResource> ComposerResources::createLayerResource(
        uint32_t bufferCacheSize) {
    return std::make_unique<ComposerLayerResource>(mImporter, bufferCacheSize);
}

ComposerDisplayResource* ComposerResources::findDisplayResourceLocked(Display display) {
    auto iter = mDisplayResources.find(display);
    if (iter == mDisplayResources.end()) {
        return nullptr;
    }
    return iter->second.get();
}

Error ComposerResources::getHandle(Display display, Layer layer, uint32_t slot, Cache cache,
                                   bool fromCache, const native_handle_t* rawHandle,
                                   const native_handle_t** outHandle,
                                   ReplacedHandle* outReplacedHandle) {
    Error error;

    // import the raw handle (or ignore raw handle when fromCache is true)
    const native_handle_t* importedHandle = nullptr;
    if (!fromCache) {
        error = (outReplacedHandle->isBuffer())
                        ? mImporter.importBuffer(rawHandle, &importedHandle)
                        : mImporter.importStream(rawHandle, &importedHandle);
        if (error != Error::NONE) {
            return error;
        }
    }

    std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);

    // find display/layer resource
    const bool needLayerResource = (cache == ComposerResources::Cache::LAYER_BUFFER ||
                                    cache == ComposerResources::Cache::LAYER_SIDEBAND_STREAM);
    ComposerDisplayResource* displayResource = findDisplayResourceLocked(display);
    ComposerLayerResource* layerResource = (displayResource && needLayerResource)
                                                   ? displayResource->findLayerResource(layer)
                                                   : nullptr;

    // lookup or update cache
    const native_handle_t* replacedHandle = nullptr;
    if (displayResource && (!needLayerResource || layerResource)) {
        switch (cache) {
            case ComposerResources::Cache::CLIENT_TARGET:
                error = displayResource->getClientTarget(slot, fromCache, importedHandle, outHandle,
                                                         &replacedHandle);
                break;
            case ComposerResources::Cache::OUTPUT_BUFFER:
                error = displayResource->getOutputBuffer(slot, fromCache, importedHandle, outHandle,
                                                         &replacedHandle);
                break;
            case ComposerResources::Cache::LAYER_BUFFER:
                error = layerResource->getBuffer(slot, fromCache, importedHandle, outHandle,
                                                 &replacedHandle);
                break;
            case ComposerResources::Cache::LAYER_SIDEBAND_STREAM:
                error = layerResource->getSidebandStream(slot, fromCache, importedHandle, outHandle,
                                                         &replacedHandle);
                break;
            default:
                error = Error::BAD_PARAMETER;
                break;
        }

        if (error != Error::NONE) {
            ALOGW("invalid cache %d slot %d", int(cache), int(slot));
        }
    } else if (!displayResource) {
        error = Error::BAD_DISPLAY;
    } else {
        error = Error::BAD_LAYER;
    }

    // clean up on errors
    if (error != Error::NONE) {
        if (!fromCache) {
            if (outReplacedHandle->isBuffer()) {
                mImporter.freeBuffer(importedHandle);
            } else {
                mImporter.freeStream(importedHandle);
            }
        }
        return error;
    }

    outReplacedHandle->reset(&mImporter, replacedHandle);

    return Error::NONE;
}

}  // namespace hal
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
