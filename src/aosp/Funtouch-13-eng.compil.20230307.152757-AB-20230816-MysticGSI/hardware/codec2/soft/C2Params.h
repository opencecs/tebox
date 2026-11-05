#pragma once

// Minimal Codec2 binary param helpers (see Params.aidl / C2Param layout).
// C2Param is serialized in its native layout: [uint32 size][uint32 index]
// followed by the payload, with each object start 8-byte aligned.

#include <cstdint>
#include <cstring>
#include <vector>

namespace gki {
namespace c2param {

// From frameworks/av media/codec2 C2Config.h (Android 16 / BP4A).
// These are the platform C2 core indices from C2Config.h.  The high bits of
// the index carry the parameter kind and port/stream direction, so matching
// must mask only the stream id and keep the 16-bit core index.
constexpr uint32_t kParamIndexPictureSize = 0x1800;  // C2_PARAM_INDEX_PICTURE_PARAM_START
constexpr uint32_t kParamIndexKind = 0x804;
constexpr uint32_t kParamIndexDomain = 0x805;
constexpr uint32_t kParamIndexMediaType = 0x1080c;  // flexible string
constexpr uint32_t kParamIndexBufferType = 0x81b;
constexpr uint32_t kParamIndexMime = 0x800 + 5;     // approx; also match via payload scan

constexpr uint32_t kParamIndexPixelFormat = 0x1802;
constexpr uint32_t kParamIndexProfileLevel = 0x808;
constexpr uint32_t kParamIndexDelay = 0x80e;
// C2PortAllocatorsTuning::input (flexible array of allocator ids).
constexpr uint32_t kInputAllocatorsIndex = 0x40010819u;
constexpr uint32_t kAllocatorBlob = 19;  // C2PlatformAllocatorStore::BLOB

constexpr uint32_t kIndexMask = 0x0001ffffu;

inline uint32_t coreIndex(uint32_t index) { return index & kIndexMask; }

struct PictureSize {
    uint32_t width = 0;
    uint32_t height = 0;
};

inline bool parseParams(const std::vector<uint8_t>& blob, PictureSize* pic) {
    if (!pic) return false;
    size_t off = 0;
    while (off + 8 <= blob.size()) {
        uint32_t size = 0, index = 0;
        memcpy(&size, blob.data() + off, 4);
        memcpy(&index, blob.data() + off + 4, 4);
        if (size < 8 || off + size > blob.size()) break;
        if (coreIndex(index) == (kParamIndexPictureSize & kIndexMask) && size >= 16) {
            memcpy(&pic->width, blob.data() + off + 8, 4);
            memcpy(&pic->height, blob.data() + off + 12, 4);
        }
        size_t aligned = (size + 7u) & ~7u;
        off += aligned;
    }
    // Be tolerant of vendor/framework parameter blobs that contain an
    // unsupported object whose declared size cannot be walked by this small
    // helper.  Picture size is a fixed 16-byte object, so locate it on the
    // required 8-byte boundaries as a fallback.
    if (pic->width == 0 || pic->height == 0) {
        for (size_t at = 0; at + 16 <= blob.size(); at += 8) {
            uint32_t size = 0, index = 0;
            memcpy(&size, blob.data() + at, 4);
            memcpy(&index, blob.data() + at + 4, 4);
            if (size >= 16 && at + size <= blob.size() &&
                coreIndex(index) == (kParamIndexPictureSize & kIndexMask)) {
                memcpy(&pic->width, blob.data() + at + 8, 4);
                memcpy(&pic->height, blob.data() + at + 12, 4);
                break;
            }
        }
    }
    return pic->width > 0 && pic->height > 0;
}

inline std::vector<uint8_t> makePictureSize(uint32_t index, uint32_t w, uint32_t h) {
    // index + size(16) + width + height
    std::vector<uint8_t> out(16, 0);
    uint32_t size = 16;
    memcpy(out.data(), &size, 4);
    memcpy(out.data() + 4, &index, 4);
    memcpy(out.data() + 8, &w, 4);
    memcpy(out.data() + 12, &h, 4);
    return out;
}

inline std::vector<uint8_t> makeU32(uint32_t index, uint32_t value) {
    // C2Param header (8 bytes) followed by C2SimpleValueStruct<uint32_t>.
    std::vector<uint8_t> out(12, 0);
    uint32_t size = 12;
    memcpy(out.data(), &size, 4);
    memcpy(out.data() + 4, &index, 4);
    memcpy(out.data() + 8, &value, 4);
    return out;
}

inline std::vector<uint8_t> makeString(uint32_t index, const char* value) {
    const size_t length = value ? strlen(value) + 1 : 1;
    const size_t size = 8 + length;
    const size_t aligned = (size + 7u) & ~7u;
    std::vector<uint8_t> out(aligned, 0);
    const uint32_t size32 = static_cast<uint32_t>(size);
    memcpy(out.data(), &size32, 4);
    memcpy(out.data() + 4, &index, 4);
    if (value) memcpy(out.data() + 8, value, length);
    return out;
}

// C2StoreDmaBufUsageInfo: {u64 usage, u32 capacity, i32 allocFlags,
// char heapName[]}. Codec2 asks the preferred (vendor) store which DMA-BUF
// heap backs linear/graphic blocks; an unanswered query leaves the name empty
// and every software codec in the app then fails to allocate.
constexpr uint32_t kParamIndexStoreDmaBufUsage = 0x14019;  // flexible
inline std::vector<uint8_t> makeDmaBufUsage(uint32_t index, const char* heap) {
    const size_t length = strlen(heap) + 1;
    const size_t size = 8 + 16 + length;
    std::vector<uint8_t> out((size + 7u) & ~7u, 0);
    const uint32_t size32 = static_cast<uint32_t>(size);
    memcpy(out.data(), &size32, 4);
    memcpy(out.data() + 4, &index, 4);
    memcpy(out.data() + 24, heap, length);
    return out;
}

}  // namespace c2param
}  // namespace gki
