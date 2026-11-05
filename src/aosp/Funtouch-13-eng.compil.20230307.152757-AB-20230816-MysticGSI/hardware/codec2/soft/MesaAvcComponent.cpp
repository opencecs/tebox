#include "../../graphics/stub/buffer_handle.h"
#include "MesaAvcComponent.h"

#include "C2Params.h"
#include "ComponentInterface.h"
#include "Configurable.h"

#include <aidl/android/hardware/media/c2/FrameData.h>
#include <aidl/android/hardware/media/c2/BnInputSink.h>
#include <aidl/android/hardware/media/c2/BnInputSurfaceConnection.h>
#include <aidl/android/hardware/media/c2/IComponentListener.h>
#include <aidl/android/hardware/media/c2/Status.h>
#include <aidl/android/hardware/media/c2/Work.h>
#include <aidl/android/hardware/media/c2/WorkBundle.h>
#include <aidl/android/hardware/media/c2/WorkOrdinal.h>
#include <android/hardware_buffer_aidl.h>

#include <android/log.h>
#include <cutils/ashmem.h>
#include <android/hardware/graphics/mapper/IMapper.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <zlib.h>

#include "dma-heap.h"

#include <array>
#include <utility>
#include <algorithm>
#include <limits>
#if defined(__aarch64__)
#include <arm_neon.h>
#endif

#define LOG_TAG "mesa-c2"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using aidl::android::hardware::media::c2::BaseBlock;
using aidl::android::hardware::media::c2::Buffer;
using aidl::android::hardware::media::c2::FrameData;
using aidl::android::hardware::media::c2::IComponent;
using aidl::android::hardware::media::c2::IComponentInterface;
using aidl::android::hardware::media::c2::IComponentListener;
using aidl::android::hardware::media::c2::IInputSink;
using aidl::android::hardware::media::c2::IInputSurface;
using aidl::android::hardware::media::c2::IInputSurfaceConnection;
using aidl::android::hardware::media::c2::Status;
using aidl::android::hardware::media::c2::Work;
using aidl::android::hardware::media::c2::WorkBundle;
using aidl::android::hardware::common::NativeHandle;
using aidl::android::hardware::HardwareBuffer;
using ndk::ScopedAStatus;

namespace gki {

namespace {

class MesaInputSurfaceConnection final
        : public aidl::android::hardware::media::c2::BnInputSurfaceConnection {
public:
    ScopedAStatus disconnect() override { return ScopedAStatus::ok(); }
    ScopedAStatus signalEndOfStream() override { return ScopedAStatus::ok(); }
};

ScopedAStatus notSupported() {
    return ScopedAStatus::fromServiceSpecificError(Status::OMITTED);
}
ScopedAStatus badState() {
    return ScopedAStatus::fromServiceSpecificError(Status::BAD_STATE);
}

int64_t framePtsUs(const FrameData& frame) {
    if (frame.ordinal.timestampUs != 0) return frame.ordinal.timestampUs;
    return static_cast<int64_t>(frame.ordinal.frameIndex) * 33333;
}

std::vector<uint8_t> readFd(int fd, size_t maxBytes) {
    std::vector<uint8_t> out;
    if (fd < 0) return out;
    off_t len = lseek(fd, 0, SEEK_END);
    if (len <= 0) return out;
    if (static_cast<size_t>(len) > maxBytes) len = static_cast<off_t>(maxBytes);
    out.resize(static_cast<size_t>(len));
    lseek(fd, 0, SEEK_SET);
    size_t got = 0;
    while (got < out.size()) {
        ssize_t n = pread(fd, out.data() + got, out.size() - got, static_cast<off_t>(got));
        if (n <= 0) break;
        got += static_cast<size_t>(n);
    }
    out.resize(got);
    return out;
}

uint32_t readBe32(const uint8_t* p) {
    return (static_cast<uint32_t>(p[0]) << 24) |
           (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}

uint8_t paeth(uint8_t a, uint8_t b, uint8_t c) {
    const int p = static_cast<int>(a) + static_cast<int>(b) - static_cast<int>(c);
    const int pa = std::abs(p - static_cast<int>(a));
    const int pb = std::abs(p - static_cast<int>(b));
    const int pc = std::abs(p - static_cast<int>(c));
    return pa <= pb && pa <= pc ? a : (pb <= pc ? b : c);
}

std::vector<uint8_t> captureScreenPng() {
    int pipefd[2] = {-1, -1};
    if (pipe2(pipefd, O_CLOEXEC) != 0) {
        ALOGE("screencap pipe failed errno=%d", errno);
        return {};
    }
    const pid_t child = fork();
    if (child < 0) {
        ALOGE("screencap fork failed errno=%d", errno);
        close(pipefd[0]);
        close(pipefd[1]);
        return {};
    }
    if (child == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) < 0) _exit(127);
        close(pipefd[1]);
        execl("/system/bin/screencap", "screencap", "-p", static_cast<char*>(nullptr));
        _exit(127);
    }
    close(pipefd[1]);
    std::vector<uint8_t> png;
    std::array<uint8_t, 64 * 1024> buf{};
    for (;;) {
        const ssize_t n = read(pipefd[0], buf.data(), buf.size());
        if (n <= 0) break;
        if (png.size() > 16 * 1024 * 1024 - static_cast<size_t>(n)) {
            png.clear();
            break;
        }
        png.insert(png.end(), buf.data(), buf.data() + n);
    }
    close(pipefd[0]);
    int status = 0;
    if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        ALOGE("screencap failed status=0x%x bytes=%zu errno=%d", status, png.size(), errno);
        return {};
    }
    ALOGI("screencap returned %zu bytes", png.size());
    return png;
}

std::vector<uint8_t> pngToNv12(const std::vector<uint8_t>& png, uint32_t wantWidth,
                               uint32_t wantHeight) {
    static constexpr uint8_t kPngSignature[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    if (png.size() < sizeof(kPngSignature) ||
        memcmp(png.data(), kPngSignature, sizeof(kPngSignature)) != 0) return {};
    uint32_t width = 0, height = 0;
    uint8_t bitDepth = 0, colorType = 0, interlace = 0;
    std::vector<uint8_t> compressed;
    size_t pos = sizeof(kPngSignature);
    while (pos + 12 <= png.size()) {
        const uint32_t length = readBe32(png.data() + pos);
        pos += 4;
        if (length > png.size() - pos - 8) return {};
        const uint8_t* type = png.data() + pos;
        const uint8_t* data = type + 4;
        pos += 8;
        if (memcmp(type, "IHDR", 4) == 0 && length >= 13) {
            width = readBe32(data);
            height = readBe32(data + 4);
            bitDepth = data[8];
            colorType = data[9];
            interlace = data[12];
        } else if (memcmp(type, "IDAT", 4) == 0) {
            compressed.insert(compressed.end(), data, data + length);
        } else if (memcmp(type, "IEND", 4) == 0) {
            break;
        }
        pos += length + 4;  // chunk CRC
    }
    const unsigned channels = colorType == 6 ? 4 : (colorType == 2 ? 3 : 0);
    if (!width || !height || width != wantWidth || height != wantHeight || bitDepth != 8 ||
        !channels || interlace != 0 || compressed.empty()) return {};
    const size_t rowBytes = static_cast<size_t>(width) * channels;
    if (rowBytes > std::numeric_limits<size_t>::max() / height - height) return {};
    std::vector<uint8_t> filtered((rowBytes + 1) * height);
    uLongf filteredSize = filtered.size();
    if (uncompress(filtered.data(), &filteredSize, compressed.data(), compressed.size()) != Z_OK ||
        filteredSize != filtered.size()) return {};
    std::vector<uint8_t> pixels(rowBytes * height);
    for (uint32_t y = 0; y < height; ++y) {
        const uint8_t filter = filtered[static_cast<size_t>(y) * (rowBytes + 1)];
        const uint8_t* src = filtered.data() + static_cast<size_t>(y) * (rowBytes + 1) + 1;
        uint8_t* dst = pixels.data() + static_cast<size_t>(y) * rowBytes;
        const uint8_t* above = y ? dst - rowBytes : nullptr;
        for (size_t x = 0; x < rowBytes; ++x) {
            const uint8_t a = x >= channels ? dst[x - channels] : 0;
            const uint8_t b = above ? above[x] : 0;
            const uint8_t c = above && x >= channels ? above[x - channels] : 0;
            switch (filter) {
                case 0: dst[x] = src[x]; break;
                case 1: dst[x] = static_cast<uint8_t>(src[x] + a); break;
                case 2: dst[x] = static_cast<uint8_t>(src[x] + b); break;
                case 3: dst[x] = static_cast<uint8_t>(src[x] + ((static_cast<unsigned>(a) + b) / 2)); break;
                case 4: dst[x] = static_cast<uint8_t>(src[x] + paeth(a, b, c)); break;
                default: return {};
            }
        }
    }
    const size_t ySize = static_cast<size_t>(width) * height;
    std::vector<uint8_t> nv12(ySize + ySize / 2, 128);
    auto clampByte = [](int value) -> uint8_t {
        return static_cast<uint8_t>(std::max(0, std::min(255, value)));
    };
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            const uint8_t* p = pixels.data() + static_cast<size_t>(y) * rowBytes + x * channels;
            const int r = p[0], g = p[1], b = p[2];
            nv12[static_cast<size_t>(y) * width + x] =
                    clampByte(((66 * r + 129 * g + 25 * b + 128) >> 8) + 16);
        }
    }
    uint8_t* uv = nv12.data() + ySize;
    for (uint32_t y = 0; y < height; y += 2) {
        for (uint32_t x = 0; x < width; x += 2) {
            int u = 0, v = 0, count = 0;
            for (uint32_t dy = 0; dy < 2 && y + dy < height; ++dy) {
                for (uint32_t dx = 0; dx < 2 && x + dx < width; ++dx) {
                    const uint8_t* p = pixels.data() + static_cast<size_t>(y + dy) * rowBytes +
                                       (x + dx) * channels;
                    const int r = p[0], g = p[1], b = p[2];
                    u += ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
                    v += ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
                    ++count;
                }
            }
            const size_t uvOffset = static_cast<size_t>(y / 2) * width + x;
            uv[uvOffset] = clampByte(u / count);
            if (x + 1 < width) uv[uvOffset + 1] = clampByte(v / count);
        }
    }
    return nv12;
}

std::vector<uint8_t> captureScreenNv12(uint32_t width, uint32_t height) {
    return pngToNv12(captureScreenPng(), width, height);
}

NativeHandle duplicateHandle(const NativeHandle& src) {
    NativeHandle out;
    out.ints = src.ints;
    for (const auto& fd : src.fds) {
        out.fds.emplace_back(fd.get() >= 0 ? dup(fd.get()) : -1);
    }
    return out;
}

BaseBlock duplicateBaseBlock(const BaseBlock& src) {
    switch (src.getTag()) {
        case BaseBlock::nativeBlock:
            return BaseBlock::make<BaseBlock::nativeBlock>(
                    duplicateHandle(src.get<BaseBlock::nativeBlock>()));
        case BaseBlock::hwbBlock:
            {
                const HardwareBuffer& source = src.get<BaseBlock::hwbBlock>();
                HardwareBuffer copy;
                if (source) {
                    AHardwareBuffer_acquire(source.get());
                    copy.reset(source.get());
                }
                return BaseBlock::make<BaseBlock::hwbBlock>(std::move(copy));
            }
        case BaseBlock::pooledBlock:
            // Buffer-pool references require an IClientManager implementation.
            // Surface input uses hwbBlock on this guest.
            return BaseBlock();
    }
    return BaseBlock();
}

std::vector<uint8_t> rgbToNv12(const uint8_t* pixels, uint32_t width, uint32_t height,
                               size_t rowBytes, unsigned channels) {
    if (!pixels || !width || !height || channels < 3 || rowBytes < static_cast<size_t>(width) * channels)
        return {};
    const size_t ySize = static_cast<size_t>(width) * height;
    std::vector<uint8_t> nv12(ySize + ySize / 2, 128);
    auto clampByte = [](int value) -> uint8_t {
        return static_cast<uint8_t>(std::max(0, std::min(255, value)));
    };
    for (uint32_t y = 0; y < height; ++y) {
        const uint8_t* row = pixels + static_cast<size_t>(y) * rowBytes;
        uint8_t* yout = nv12.data() + static_cast<size_t>(y) * width;
        uint32_t x = 0;
#if defined(__aarch64__)
        if (channels == 4) {
            for (; x + 8 <= width; x += 8) {
                uint8x8x4_t rgba = vld4_u8(row + static_cast<size_t>(x) * 4);
                uint16x8_t yv = vmull_u8(rgba.val[0], vdup_n_u8(66));
                yv = vmlal_u8(yv, rgba.val[1], vdup_n_u8(129));
                yv = vmlal_u8(yv, rgba.val[2], vdup_n_u8(25));
                yv = vaddq_u16(vshrq_n_u16(vaddq_u16(yv, vdupq_n_u16(128)), 8),
                               vdupq_n_u16(16));
                vst1_u8(yout + x, vmovn_u16(yv));
            }
        }
#endif
        for (; x < width; ++x) {
            const uint8_t* p = row + static_cast<size_t>(x) * channels;
            const int r = p[0], g = p[1], b = p[2];
            yout[x] = clampByte(((66 * r + 129 * g + 25 * b + 128) >> 8) + 16);
        }
    }
    uint8_t* uv = nv12.data() + ySize;
    for (uint32_t y = 0; y < height; y += 2) {
        for (uint32_t x = 0; x < width; x += 2) {
            int u = 0, v = 0, count = 0;
            for (uint32_t dy = 0; dy < 2 && y + dy < height; ++dy) {
                for (uint32_t dx = 0; dx < 2 && x + dx < width; ++dx) {
                    const uint8_t* p = pixels + static_cast<size_t>(y + dy) * rowBytes +
                                       (x + dx) * channels;
                    const int r = p[0], g = p[1], b = p[2];
                    u += ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
                    v += ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
                    ++count;
                }
            }
            const size_t uvOffset = static_cast<size_t>(y / 2) * width + x;
            uv[uvOffset] = clampByte(u / count);
            if (x + 1 < width) uv[uvOffset + 1] = clampByte(v / count);
        }
    }
    return nv12;
}

std::vector<uint8_t> readHardwareBuffer(const HardwareBuffer& hw) {
    AHardwareBuffer* buffer = hw.get();
    if (!buffer) return {};
    AHardwareBuffer_Desc desc = {};
    AHardwareBuffer_describe(buffer, &desc);
    if (!desc.width || !desc.height || desc.layers != 1) return {};
    if (desc.format == AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM ||
        desc.format == AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM ||
        desc.format == AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM) {
        void* data = nullptr;
        if (AHardwareBuffer_lock(buffer, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, nullptr, &data) != 0 ||
            !data) return {};
        const unsigned channels = desc.format == AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM ? 3 : 4;
        const size_t rowBytes = static_cast<size_t>(desc.stride) * channels;
        std::vector<uint8_t> out = rgbToNv12(static_cast<const uint8_t*>(data), desc.width,
                                             desc.height, rowBytes, channels);
        int fence = -1;
        AHardwareBuffer_unlock(buffer, &fence);
        if (fence >= 0) close(fence);
        return out;
    }
    return {};
}

std::vector<uint8_t> readNativeGraphic(const NativeHandle& handle) {
    // C2AllocatorGralloc wraps the allocator's native handle by appending
    // eleven metadata ints (width, height, format, usage, stride, ... magic).
    // The AIDL NativeHandle delivered by the HAL contains that complete
    // wrapped handle, so strip the metadata before handing it to mapper.
    constexpr size_t kC2ExtraInts = 11;
    // C2's multicharacter literal '\\xc2gr\\0' is laid out as 0xc2677200
    // by the Android clang toolchain (the value used by C2HandleGralloc).
    constexpr int32_t kC2Magic = static_cast<int32_t>(0xc2677200u);
    if (handle.fds.empty() || handle.ints.size() < 6) return {};

    size_t baseInts = handle.ints.size();
    const int32_t* metadata = nullptr;
    if (handle.ints.size() >= kC2ExtraInts) {
        const size_t extra = handle.ints.size() - kC2ExtraInts;
        if (handle.ints[extra + 10] == kC2Magic) {
            baseInts = extra;
            metadata = handle.ints.data() + extra;
        }
    }

    static std::atomic<int> graphicDiagnostics{0};
    if (graphicDiagnostics.fetch_add(1) < 8) {
        ALOGI("graphic handle fds=%zu ints=%zu base=%zu wrapped=%d first=%d,%d,%d,%d,%d,%d last=%d,%d,%d,%d,%d",
              handle.fds.size(), handle.ints.size(), baseInts, metadata ? 1 : 0,
              handle.ints[0], handle.ints[1], handle.ints[2], handle.ints[3], handle.ints[4],
              handle.ints[5], handle.ints[handle.ints.size() - 5], handle.ints[handle.ints.size() - 4],
              handle.ints[handle.ints.size() - 3], handle.ints[handle.ints.size() - 2],
              handle.ints[handle.ints.size() - 1]);
    }

    if (baseInts < 6) return {};
    const int width = metadata ? metadata[0] : handle.ints[0];
    const int height = metadata ? metadata[1] : handle.ints[1];
    const int format = metadata ? metadata[2] : handle.ints[3];
    const int stride = metadata ? metadata[5] : handle.ints[2];
    if (width <= 0 || height <= 0 || stride < width) return {};
    const unsigned channels = format == AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM ? 3 : 4;
    if (format != AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM &&
        format != AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM &&
        format != AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM) return {};

    native_handle_t* raw = native_handle_create(static_cast<int>(handle.fds.size()),
                                                 static_cast<int>(baseInts));
    if (!raw) return {};
    for (size_t i = 0; i < handle.fds.size(); ++i) raw->data[i] = dup(handle.fds[i].get());
    for (size_t i = 0; i < baseInts; ++i)
        raw->data[raw->numFds + i] = handle.ints[i];

    void* mapperLib = dlopen("/vendor/lib64/hw/mapper.stub.so", RTLD_NOW | RTLD_LOCAL);
    using LoadMapper = AIMapper_Error (*)(AIMapper**);
    auto loadMapper = mapperLib ? reinterpret_cast<LoadMapper>(dlsym(mapperLib, "AIMapper_loadIMapper"))
                                : nullptr;
    AIMapper* mapper = nullptr;
    std::vector<uint8_t> out;
    if (loadMapper && loadMapper(&mapper) == AIMAPPER_ERROR_NONE && mapper &&
        mapper->version == AIMAPPER_VERSION_5) {
        buffer_handle_t imported = nullptr;
        const AIMapper_Error importStatus = mapper->v5.importBuffer(raw, &imported);
            if (graphicDiagnostics.load() <= 8) {
                ALOGI("graphic mapper import status=%d imported=%p size=%d/%d/%d", importStatus,
                      imported, width, height, stride);
            }
        if (importStatus == AIMAPPER_ERROR_NONE && imported) {
            void* data = nullptr;
            ARect region{0, 0, width, height};
            const AIMapper_Error lockStatus = mapper->v5.lock(
                    imported, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, region, -1, &data);
            if (graphicDiagnostics.load() <= 8) {
                ALOGI("graphic mapper lock status=%d data=%p", lockStatus, data);
            }
            if (lockStatus == AIMAPPER_ERROR_NONE && data) {
                const uint8_t* sample = static_cast<const uint8_t*>(data);
                if (graphicDiagnostics.load() <= 8) {
                    ALOGI("graphic pixels first=%02x%02x%02x%02x center=%02x%02x%02x%02x",
                          sample[0], sample[1], sample[2], sample[3],
                          sample[(static_cast<size_t>(height / 2) * stride + width / 2) * channels],
                          sample[(static_cast<size_t>(height / 2) * stride + width / 2) * channels + 1],
                          sample[(static_cast<size_t>(height / 2) * stride + width / 2) * channels + 2],
                          sample[(static_cast<size_t>(height / 2) * stride + width / 2) * channels + 3]);
                }
                out = rgbToNv12(static_cast<const uint8_t*>(data), width, height,
                                static_cast<size_t>(stride) * channels, channels);
                int fence = -1;
                mapper->v5.unlock(imported, &fence);
                if (fence >= 0) close(fence);
            }
            mapper->v5.freeBuffer(imported);
        }
    }
    native_handle_close(raw);
    native_handle_delete(raw);
    if (mapperLib) dlclose(mapperLib);
    return out;
}

struct NativeGraphicInfo {
    int fd = -1;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t stride = 0;
    uint32_t format = 0;
};

bool getNativeGraphicInfo(const NativeHandle& handle, NativeGraphicInfo* out) {
    constexpr size_t kC2ExtraInts = 11;
    constexpr int32_t kC2Magic = static_cast<int32_t>(0xc2677200u);
    if (!out || handle.fds.empty() || handle.ints.size() < 6) return false;
    size_t baseInts = handle.ints.size();
    const int32_t* metadata = nullptr;
    if (handle.ints.size() >= kC2ExtraInts) {
        const size_t extra = handle.ints.size() - kC2ExtraInts;
        if (handle.ints[extra + 10] == kC2Magic) {
            baseInts = extra;
            metadata = handle.ints.data() + extra;
        }
    }
    if (baseInts < 6) return false;
    int width = metadata ? metadata[0] : handle.ints[0];
    int height = metadata ? metadata[1] : handle.ints[1];
    int format = metadata ? metadata[2] : handle.ints[3];
    int stride = metadata ? metadata[5] : handle.ints[2];
    // gfxstream buffers carry the minigbm layout (graphics/stub/buffer_handle.h).
    TeboxBufferInfo tb;
    if (!metadata && teboxBufferInfoFromInts(handle.ints.data(), baseInts, &tb) && tb.gfxstream) {
        width = tb.width;
        height = tb.height;
        format = tb.format;
        stride = tb.stride;
    }
    if (width <= 0 || height <= 0 || stride < width ||
        format != AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM)
        return false;
    out->fd = handle.fds[0].get();
    out->width = static_cast<uint32_t>(width);
    out->height = static_cast<uint32_t>(height);
    out->stride = static_cast<uint32_t>(stride);
    out->format = static_cast<uint32_t>(format);
    return out->fd >= 0;
}

Buffer duplicateBuffer(const Buffer& src) {
    Buffer out;
    out.info = src.info;
    out.blocks.reserve(src.blocks.size());
    for (const auto& block : src.blocks) {
        aidl::android::hardware::media::c2::Block copy;
        copy.index = block.index;
        copy.meta = block.meta;
        copy.fence = duplicateHandle(block.fence);
        out.blocks.push_back(std::move(copy));
    }
    return out;
}

FrameData duplicateFrameData(const FrameData& src) {
    FrameData out;
    out.flags = src.flags;
    out.ordinal = src.ordinal;
    out.configUpdate = src.configUpdate;
    out.buffers.reserve(src.buffers.size());
    for (const auto& buffer : src.buffers) out.buffers.push_back(duplicateBuffer(buffer));
    out.infoBuffers.reserve(src.infoBuffers.size());
    for (const auto& info : src.infoBuffers) {
        aidl::android::hardware::media::c2::InfoBuffer copy;
        copy.index = info.index;
        copy.buffer = duplicateBuffer(info.buffer);
        out.infoBuffers.push_back(std::move(copy));
    }
    return out;
}

Work duplicateWork(const Work& src) {
    Work out;
    out.chainInfo = src.chainInfo;
    out.input = duplicateFrameData(src.input);
    out.worklets.reserve(src.worklets.size());
    for (const auto& worklet : src.worklets) {
        aidl::android::hardware::media::c2::Worklet copy;
        copy.componentId = worklet.componentId;
        copy.tunings = worklet.tunings;
        copy.failures = worklet.failures;
        copy.output = duplicateFrameData(worklet.output);
        out.worklets.push_back(std::move(copy));
    }
    out.workletsProcessed = src.workletsProcessed;
    out.result = src.result;
    return out;
}

bool makeOutputBlock(const std::vector<uint8_t>& data, std::vector<BaseBlock>* baseBlocks,
                    FrameData* output) {
    if (data.empty() || !baseBlocks || !output) return false;
    int heap = open("/dev/dma_heap/system", O_RDONLY | O_CLOEXEC);
    int fd = -1;
    if (heap >= 0) {
        dma_heap_allocation_data alloc = {};
        alloc.len = data.size();
        alloc.fd_flags = O_RDWR | O_CLOEXEC;
        if (ioctl(heap, DMA_HEAP_IOCTL_ALLOC, &alloc) == 0) fd = static_cast<int>(alloc.fd);
        close(heap);
    }
    if (fd < 0) {
        // Keep a functional fallback for guests without DMA-BUF heaps.
        fd = ashmem_create_region("mesa-c2-output", data.size());
        if (fd >= 0) ashmem_set_prot_region(fd, PROT_READ | PROT_WRITE);
    }
    if (fd < 0) {
        ALOGE("output allocation failed size=%zu errno=%d", data.size(), errno);
        return false;
    }
    void* mapped = mmap(nullptr, data.size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        ALOGE("ashmem mmap failed size=%zu errno=%d", data.size(), errno);
        close(fd);
        return false;
    }
    memcpy(mapped, data.data(), data.size());
    munmap(mapped, data.size());
    lseek(fd, 0, SEEK_SET);
    NativeHandle handle;
    handle.fds.emplace_back(fd);
    // Codec2's nativeBlock is a serialized C2Handle.  A plain memfd is
    // rejected by the client-side C2BlockFactory during binder fd translation;
    // use the dmabuf/Ion handle layout (one fd, size lo/hi, \xc2io magic) so
    // the framework can import the returned linear block.
    constexpr int32_t kC2IoMagic = '\xc2io\x00';
    handle.ints = {static_cast<int32_t>(data.size()), 0, kC2IoMagic};
    BaseBlock block = BaseBlock::make<BaseBlock::nativeBlock>(std::move(handle));
    const int32_t index = static_cast<int32_t>(baseBlocks->size());
    baseBlocks->push_back(std::move(block));
    Buffer buffer;
    aidl::android::hardware::media::c2::Block ref;
    ref.index = index;
    // The AIDL Codec2 bridge requires C2Hal_RangeInfo metadata on every
    // linear block so it can turn the native handle into a C2LinearBlock.
    // C2Hal_RangeInfo is a global C2Info with core index 0 and two u32s.
    ref.meta.params.resize(16, 0);
    const uint32_t rangeSize = 16;
    const uint32_t rangeIndex = 0xE0000000u;
    const uint32_t zeroOffset = 0;
    const uint32_t rangeLength = static_cast<uint32_t>(data.size());
    memcpy(ref.meta.params.data(), &rangeSize, sizeof(rangeSize));
    memcpy(ref.meta.params.data() + 4, &rangeIndex, sizeof(rangeIndex));
    memcpy(ref.meta.params.data() + 8, &zeroOffset, sizeof(zeroOffset));
    memcpy(ref.meta.params.data() + 12, &rangeLength, sizeof(rangeLength));
    buffer.blocks.push_back(std::move(ref));
    output->buffers.push_back(std::move(buffer));
    return true;
}

bool avcHasIdr(const std::vector<uint8_t>& annexB) {
    for (size_t i = 0; i + 3 < annexB.size(); ++i) {
        if (annexB[i] == 0 && annexB[i + 1] == 0 && annexB[i + 2] == 1) {
            if ((annexB[i + 3] & 0x1f) == 5) return true;
            i += 2;
        }
    }
    return false;
}

std::vector<uint8_t> extractAvcCsd(const std::vector<uint8_t>& annexB) {
    std::vector<size_t> starts;
    for (size_t i = 0; i + 4 <= annexB.size();) {
        size_t n = 0;
        if (i + 4 <= annexB.size() && annexB[i] == 0 && annexB[i + 1] == 0 &&
            annexB[i + 2] == 0 && annexB[i + 3] == 1) {
            n = 4;
        } else if (i + 3 <= annexB.size() && annexB[i] == 0 && annexB[i + 1] == 0 &&
                   annexB[i + 2] == 1) {
            n = 3;
        }
        if (n) {
            starts.push_back(i);
            i += n;
        } else {
            ++i;
        }
    }
    if (starts.size() < 2) return {};
    size_t sps = starts[0];
    size_t ppsEnd = starts.size() > 2 ? starts[2] : annexB.size();
    if (sps >= ppsEnd || ppsEnd > annexB.size()) return {};
    const uint8_t* p = annexB.data() + sps;
    size_t pfx = (p + 4 <= annexB.data() + annexB.size() && p[2] == 0 && p[3] == 1) ? 4 : 3;
    if (sps + pfx >= annexB.size()) return {};
    uint8_t firstType = annexB[sps + pfx] & 0x1f;
    uint8_t secondType = annexB[starts[1] + ((starts[1] + 4 <= annexB.size() &&
                          annexB[starts[1] + 2] == 0 && annexB[starts[1] + 3] == 1) ? 4 : 3)] & 0x1f;
    if (firstType != 7 || secondType != 8) return {};
    return std::vector<uint8_t>(annexB.begin() + sps, annexB.begin() + ppsEnd);
}

}  // namespace

class MesaInputSink final : public aidl::android::hardware::media::c2::BnInputSink {
public:
    explicit MesaInputSink(MesaAvcComponent* component) : mComponent(component) {}
    ScopedAStatus queue(const WorkBundle& bundle) override {
        return mComponent ? mComponent->queue(bundle) : badState();
    }

private:
    MesaAvcComponent* const mComponent;
};

MesaAvcComponent::MesaAvcComponent(
        MesaCodecKind kind, std::string name, std::shared_ptr<ComponentInterface> intf,
        std::shared_ptr<Configurable> cfg, std::shared_ptr<IComponentListener> listener)
    : mKind(kind),
      mName(std::move(name)),
      mIntf(std::move(intf)),
      mCfg(std::move(cfg)),
      mListener(std::move(listener)) {}

MesaAvcComponent::~MesaAvcComponent() {
    // A client that dies without stop()/release() leaves both threads
    // running; a joinable std::thread in a destructor terminates the HAL.
    {
        std::lock_guard<std::mutex> lock(mLock);
        mReleased = true;
        mRunning = false;
    }
    mCv.notify_all();
    mInFlightCv.notify_all();
    if (mWorker.joinable()) mWorker.join();
    if (mCollector.joinable()) mCollector.join();
}

std::vector<uint8_t> MesaAvcComponent::extractLinearData(
        const Buffer& buf, const std::vector<BaseBlock>& baseBlocks) {
    static std::atomic<int> extractDiagnostics{0};
    if (extractDiagnostics.fetch_add(1) < 12) {
        ALOGI("extract buffer blocks=%zu base=%zu", buf.blocks.size(), baseBlocks.size());
        for (const auto& block : buf.blocks) {
            ALOGI("extract block index=%d", block.index);
        }
    }
    for (const auto& block : buf.blocks) {
        if (block.index < 0 || static_cast<size_t>(block.index) >= baseBlocks.size()) {
            continue;
        }
        const BaseBlock& bb = baseBlocks[static_cast<size_t>(block.index)];
        if (bb.getTag() == BaseBlock::nativeBlock) {
            const NativeHandle& nh = bb.get<BaseBlock::nativeBlock>();
            static std::atomic<int> nativeDiagnostics{0};
            if (nativeDiagnostics.fetch_add(1) < 8) {
                ALOGI("extract native fds=%zu ints=%zu", nh.fds.size(), nh.ints.size());
            }
            if (!nh.fds.empty()) {
                if (nh.ints.size() >= 6) {
                    std::vector<uint8_t> graphic = readNativeGraphic(nh);
                    if (!graphic.empty()) return graphic;
                }
                return readFd(nh.fds[0].get(), 16 * 1024 * 1024);
            }
        } else if (bb.getTag() == BaseBlock::hwbBlock) {
            return readHardwareBuffer(bb.get<BaseBlock::hwbBlock>());
        }
    }
    return {};
}

void MesaAvcComponent::processWork(PendingWork* pending) {
    Work* work = &pending->work;
    work->workletsProcessed = 0;
    work->result.status = Status::OK;
    if (work->worklets.empty()) {
        work->worklets.emplace_back();
    }
    auto& wl = work->worklets[0];
    wl.output = FrameData{};
    wl.output.ordinal = work->input.ordinal;

    const bool eos = (work->input.flags & FrameData::END_OF_STREAM) != 0;
    (void)((work->input.flags & FrameData::CODEC_CONFIG) != 0);

    if (mKind == MesaCodecKind::kDecoder) {
        std::vector<uint8_t> bs;
        for (const Buffer& b : work->input.buffers) {
            auto chunk = extractLinearData(b, pending->baseBlocks);
            bs.insert(bs.end(), chunk.begin(), chunk.end());
        }
        std::vector<uint8_t> nv12;
        int64_t pts = 0;
        bool outEos = false;
        int rc = mSession.decode(bs.data(), bs.size(), framePtsUs(work->input), &nv12, &pts,
                                 &outEos);
        if (rc == -ENOSYS) {
            work->result.status = Status::CANNOT_DO;
            return;
        }
        if (rc < 0 && !eos) {
            work->result.status = Status::CORRUPTED;
            return;
        }
        if (rc > 0) makeOutputBlock(nv12, &pending->baseBlocks, &wl.output);
        wl.output.flags = eos ? FrameData::END_OF_STREAM : 0;
        wl.output.ordinal.timestampUs = pts;
        work->workletsProcessed = 1;
        return;
    }

    // Encoders are pipelined: see submitEncode()/finishEncode().
    work->result.status = Status::CANNOT_DO;
}

bool MesaAvcComponent::submitEncode(PendingWork* pending, mesa_video::EncodeTicket* ticket) {
    Work* work = &pending->work;
    work->workletsProcessed = 0;
    work->result.status = Status::OK;
    if (work->worklets.empty()) work->worklets.emplace_back();
    auto& wl = work->worklets[0];
    wl.output = FrameData{};
    wl.output.ordinal = work->input.ordinal;
    // Hardware mode is GPU-only: import the surface dmabuf into VirGL video
    // and encode on the host. Never fall back to mapper CPU readback.
    int rc = -ENOSYS;
    bool submitted = false;
    // The producer (SurfaceFlinger composing a virtual display, a camera...)
    // may still be drawing into the buffer: wait for its acquire fence before
    // the host encoder reads the dma-buf, or it encodes stale content.
    for (const auto& buffer : work->input.buffers) {
        for (const auto& block : buffer.blocks) {
            for (const auto& fd : block.fence.fds) {
                if (fd.get() < 0) continue;
                struct pollfd p = {fd.get(), POLLIN, 0};
                int ret;
                do { ret = poll(&p, 1, 1000); } while (ret < 0 && errno == EINTR);
                if (ret <= 0) ALOGW("encode: input acquire fence did not signal (%d)", ret);
            }
        }
    }
    for (const BaseBlock& bb : pending->baseBlocks) {
        if (bb.getTag() != BaseBlock::nativeBlock) continue;
        NativeGraphicInfo info;
        if (!getNativeGraphicInfo(bb.get<BaseBlock::nativeBlock>(), &info)) continue;
        const bool keyframe = mSession.submittedCount() == 0;
        rc = mSession.submitGraphic(info.fd, info.width, info.height, info.stride, info.format,
                                    framePtsUs(work->input), keyframe, ticket);
        submitted = true;
        static std::atomic<int> encodeDiagnostics{0};
        if (encodeDiagnostics.fetch_add(1) < 8) {
            ALOGI("gpu encode submit fd=%d %ux%u stride=%u fmt=%u key=%d rc=%d", info.fd,
                  info.width, info.height, info.stride, info.format, keyframe ? 1 : 0, rc);
        }
        break;
    }
    if (!submitted) {
        static std::atomic<int> missingDiagnostics{0};
        if (missingDiagnostics.fetch_add(1) < 4) {
            ALOGW("%s: no graphic dmabuf in work (buffers=%zu base=%zu)", mName.c_str(),
                  work->input.buffers.size(), pending->baseBlocks.size());
        }
        work->result.status = Status::CORRUPTED;
        return false;
    }
    if (rc < 0) {
        finishEncode(pending, rc, {}, 0, 0);
        return false;
    }
    return true;
}

void MesaAvcComponent::finishEncode(PendingWork* pending, int rc,
                                    const std::vector<uint8_t>& bs, int64_t pts,
                                    uint64_t index) {
    Work* work = &pending->work;
    auto& wl = work->worklets[0];
    const bool eos = (work->input.flags & FrameData::END_OF_STREAM) != 0;
    if (rc == -ENOSYS) {
        work->result.status = Status::CANNOT_DO;
        return;
    }
    if (rc < 0 && !eos) {
        work->result.status = Status::CORRUPTED;
        return;
    }
    if (rc > 0 && !makeOutputBlock(bs, &pending->baseBlocks, &wl.output)) {
        ALOGE("%s: makeOutputBlock failed for %zu bytes", mName.c_str(), bs.size());
        work->result.status = Status::CORRUPTED;
    } else if (rc > 0 && !wl.output.buffers.empty()) {
        // C2StreamPictureTypeInfo::output: MediaCodec derives
        // BUFFER_FLAG_SYNCFRAME from it; MPEG4Writer drops samples until one.
        constexpr uint32_t kPictureTypeIndex = 0xD2001810u;
        constexpr uint32_t kSyncFrame = 1u << 0, kIFrame = 1u << 1, kPFrame = 1u << 2;
        const uint32_t type = avcHasIdr(bs) ? (kSyncFrame | kIFrame) : kPFrame;
        const uint32_t size = 12;
        auto& info = wl.output.buffers.back().info.params;
        info.resize(16, 0);
        memcpy(info.data(), &size, sizeof(size));
        memcpy(info.data() + 4, &kPictureTypeIndex, sizeof(kPictureTypeIndex));
        memcpy(info.data() + 8, &type, sizeof(type));
    }
    wl.output.flags = eos ? FrameData::END_OF_STREAM : 0;
    // First IDR carries SPS/PPS. Publish coded.init-data and mark CODEC_CONFIG
    // so scrcpy / MediaCodec treat the first packet as config, not a slice.
    if (rc > 0 && index == 0) {
        std::vector<uint8_t> csd = extractAvcCsd(bs);
        if (!csd.empty()) {
            constexpr uint32_t kInitDataIndex = 0xD2010809u;
            const uint32_t paramSize = static_cast<uint32_t>(8 + csd.size());
            aidl::android::hardware::media::c2::Params init;
            init.params.resize((paramSize + 7u) & ~7u, 0);
            memcpy(init.params.data(), &paramSize, sizeof(paramSize));
            memcpy(init.params.data() + 4, &kInitDataIndex, sizeof(kInitDataIndex));
            memcpy(init.params.data() + 8, csd.data(), csd.size());
            wl.output.configUpdate = std::move(init);
        } else {
            ALOGW("%s: first frame missing SPS/PPS (%zu bytes)", mName.c_str(), bs.size());
        }
    }
    wl.output.ordinal.timestampUs = pts;
    work->workletsProcessed = 1;
}

void MesaAvcComponent::markDropped(PendingWork* stale) {
    if (stale->work.worklets.empty()) stale->work.worklets.emplace_back();
    stale->work.workletsProcessed = 1;
    stale->work.result.status = Status::OK;
    auto& output = stale->work.worklets[0].output;
    output = FrameData{};
    output.ordinal = stale->work.input.ordinal;
    output.flags = FrameData::DROP_FRAME;
}

void MesaAvcComponent::emitWork(PendingWork* pending) {
    if (!mListener) return;
    WorkBundle bundle;
    bundle.works.push_back(std::move(pending->work));
    bundle.baseBlocks = std::move(pending->baseBlocks);
    mListener->onWorkDone(bundle);
}

void MesaAvcComponent::workerLoop() {
    const bool encoder = mKind == MesaCodecKind::kEncoder;
    while (true) {
        PendingWork pending;
        std::vector<PendingWork> dropped;
        bool havePending = false;
        {
            std::unique_lock<std::mutex> lock(mLock);
            mCv.wait(lock, [&] {
                return mReleased.load() || !mPending.empty() || !mDropped.empty() ||
                       (!mRunning.load() && mPending.empty() && mDropped.empty());
            });
            if (mReleased || (!mRunning && mPending.empty() && mDropped.empty())) break;
            dropped.swap(mDropped);
            if (!mPending.empty()) {
                pending = std::move(mPending.front());
                mPending.erase(mPending.begin());
                havePending = true;
            }
            if (encoder) {
                // Superseded frames complete through the collector so every
                // work finishes in input order.
                for (PendingWork& stale : dropped) {
                    InFlight entry;
                    entry.kind = InFlight::Kind::kDropped;
                    entry.pending = std::move(stale);
                    mInFlight.push_back(std::move(entry));
                }
                if (!dropped.empty()) mInFlightCv.notify_all();
                dropped.clear();
            }
        }
        for (PendingWork& stale : dropped) {
            markDropped(&stale);
            emitWork(&stale);
        }
        if (!havePending) continue;
        if (!encoder) {
            processWork(&pending);
            emitWork(&pending);
            continue;
        }
        {
            // Bound the pipeline: at most kMaxInFlight frames on the host.
            std::unique_lock<std::mutex> lock(mLock);
            mInFlightCv.wait(lock, [&] {
                return mReleased.load() || !mRunning.load() ||
                       mEncodedInFlight < mesa_video::Session::kMaxInFlight;
            });
            if (mReleased || !mRunning) break;
        }
        InFlight entry;
        const bool ok = submitEncode(&pending, &entry.ticket);
        entry.kind = ok ? InFlight::Kind::kEncoded : InFlight::Kind::kDone;
        entry.pending = std::move(pending);
        {
            std::lock_guard<std::mutex> lock(mLock);
            if (ok) ++mEncodedInFlight;
            mInFlight.push_back(std::move(entry));
        }
        mInFlightCv.notify_all();
    }
    {
        std::lock_guard<std::mutex> lock(mLock);
        mWorkerDone = true;
    }
    mInFlightCv.notify_all();
}

void MesaAvcComponent::collectorLoop() {
    while (true) {
        InFlight entry;
        {
            std::unique_lock<std::mutex> lock(mLock);
            mInFlightCv.wait(lock, [&] { return !mInFlight.empty() || mWorkerDone; });
            if (mInFlight.empty()) return;  // worker finished and queue drained
            entry = std::move(mInFlight.front());
            mInFlight.pop_front();
        }
        if (entry.kind == InFlight::Kind::kEncoded) {
            std::vector<uint8_t> bs;
            const int rc = mSession.collect(&entry.ticket, &bs);
            finishEncode(&entry.pending, rc, bs, entry.ticket.ptsUs, entry.ticket.index);
            {
                std::lock_guard<std::mutex> lock(mLock);
                --mEncodedInFlight;
            }
            mInFlightCv.notify_all();
        } else if (entry.kind == InFlight::Kind::kDropped) {
            markDropped(&entry.pending);
        }
        // After stop()/release() the framework no longer expects these.
        if (mRunning && !mReleased) emitWork(&entry.pending);
    }
}

ScopedAStatus MesaAvcComponent::getInterface(std::shared_ptr<IComponentInterface>* out) {
    *out = mIntf;
    return ScopedAStatus::ok();
}

ScopedAStatus MesaAvcComponent::start() {
    if (mReleased) return badState();
    if (mStarted) return ScopedAStatus::ok();
    mesa_video::Entrypoint ep = mKind == MesaCodecKind::kDecoder
                                        ? mesa_video::Entrypoint::kDecode
                                        : mesa_video::Entrypoint::kEncode;
    if (!mSession.open(ep, mesa_video::Profile::kAvcMain, mCfg->width(), mCfg->height())) {
        return ScopedAStatus::fromServiceSpecificError(Status::CANNOT_DO);
    }
    mRunning = true;
    mStarted = true;
    {
        std::lock_guard<std::mutex> lock(mLock);
        mWorkerDone = false;
    }
    if (!mWorker.joinable()) {
        mWorker = std::thread([this] { workerLoop(); });
    }
    if (mKind == MesaCodecKind::kEncoder && !mCollector.joinable()) {
        mCollector = std::thread([this] { collectorLoop(); });
    }
    ALOGI("%s started %ux%u", mName.c_str(), mCfg->width(), mCfg->height());
    return ScopedAStatus::ok();
}

ScopedAStatus MesaAvcComponent::stop() {
    {
        std::lock_guard<std::mutex> lock(mLock);
        mRunning = false;
        mPending.clear();
        mDropped.clear();
    }
    mCv.notify_all();
    mInFlightCv.notify_all();
    if (mWorker.joinable()) mWorker.join();
    if (mCollector.joinable()) mCollector.join();  // drains in-flight encodes
    mSession.close();
    mStarted = false;
    return ScopedAStatus::ok();
}

ScopedAStatus MesaAvcComponent::reset() {
    stop();
    mSession.flush();
    return ScopedAStatus::ok();
}

ScopedAStatus MesaAvcComponent::release() {
    {
        std::lock_guard<std::mutex> lock(mLock);
        mReleased = true;
        mRunning = false;
        mPending.clear();
        mDropped.clear();
    }
    mCv.notify_all();
    mInFlightCv.notify_all();
    if (mWorker.joinable()) mWorker.join();
    if (mCollector.joinable()) mCollector.join();
    mSession.close();
    return ScopedAStatus::ok();
}

ScopedAStatus MesaAvcComponent::queue(const WorkBundle& workBundle) {
    if (!mStarted) return badState();
    static std::atomic<int> queueDiagnostics{0};
    if (queueDiagnostics.fetch_add(1) < 8) {
        int native = 0, hwb = 0, pooled = 0;
        for (const auto& block : workBundle.baseBlocks) {
            if (block.getTag() == BaseBlock::nativeBlock) ++native;
            else if (block.getTag() == BaseBlock::hwbBlock) ++hwb;
            else if (block.getTag() == BaseBlock::pooledBlock) ++pooled;
        }
        ALOGI("queue works=%zu base=%zu native=%d hwb=%d pooled=%d", workBundle.works.size(),
              workBundle.baseBlocks.size(), native, hwb, pooled);
        for (const auto& block : workBundle.baseBlocks) {
            if (block.getTag() == BaseBlock::nativeBlock) {
                const NativeHandle& nh = block.get<BaseBlock::nativeBlock>();
                ALOGI("native handle fds=%zu ints=%zu [%d,%d,%d,%d,%d,%d]", nh.fds.size(),
                      nh.ints.size(), nh.ints.size() > 0 ? nh.ints[0] : 0,
                      nh.ints.size() > 1 ? nh.ints[1] : 0, nh.ints.size() > 2 ? nh.ints[2] : 0,
                      nh.ints.size() > 3 ? nh.ints[3] : 0, nh.ints.size() > 4 ? nh.ints[4] : 0,
                      nh.ints.size() > 5 ? nh.ints[5] : 0);
                break;
            }
        }
    }
    // Surface input is real-time: once encoding falls behind, processing every
    // queued frame only turns the encoder into a latency buffer. Keep one
    // waiting frame at most and complete superseded frames as DROP_FRAME so
    // Codec2 can release their buffers immediately.
    std::vector<PendingWork> dropped;
    std::unique_lock<std::mutex> lock(mLock);
    // AIDL owns the incoming file descriptors. Duplicate them before the
    // asynchronous worker outlives this binder call.
    for (const Work& src : workBundle.works) {
        const bool control = (src.input.flags &
                              (FrameData::END_OF_STREAM | FrameData::CODEC_CONFIG)) != 0;
        if (mKind == MesaCodecKind::kEncoder && !control) {
            while (!mPending.empty()) {
                dropped.push_back(std::move(mPending.front()));
                mPending.erase(mPending.begin());
            }
        }
        PendingWork pending;
        pending.work = duplicateWork(src);
        if (pending.work.worklets.empty()) pending.work.worklets.resize(1);
        pending.work.workletsProcessed = 0;
        pending.work.result.status = Status::OK;
        pending.baseBlocks.reserve(workBundle.baseBlocks.size());
        for (const auto& block : workBundle.baseBlocks) {
            pending.baseBlocks.push_back(duplicateBaseBlock(block));
        }
        mPending.push_back(std::move(pending));
    }
    for (PendingWork& pending : dropped) mDropped.push_back(std::move(pending));
    lock.unlock();
    mCv.notify_one();
    return ScopedAStatus::ok();
}

ScopedAStatus MesaAvcComponent::flush(WorkBundle* out) {
    std::unique_lock<std::mutex> lock(mLock);
    // Let submitted encodes complete through onWorkDone before returning.
    mInFlightCv.wait(lock, [&] { return mInFlight.empty() || mWorkerDone; });
    out->works.clear();
    out->baseBlocks.clear();
    for (PendingWork& pending : mPending) {
        const int32_t baseOffset = static_cast<int32_t>(out->baseBlocks.size());
        for (auto& block : pending.baseBlocks) out->baseBlocks.push_back(std::move(block));
        for (auto& block : pending.work.input.buffers) {
            for (auto& ref : block.blocks) {
                if (ref.index >= 0) ref.index += baseOffset;
            }
        }
        out->works.push_back(std::move(pending.work));
    }
    mPending.clear();
    mDropped.clear();
    return ScopedAStatus::ok();
}

ScopedAStatus MesaAvcComponent::drain(bool /*withEos*/) {
    // There is no pipe-video drain operation until the backend is available.
    // Returning OK here would make Codec2 wait for output that can never be
    // produced and leaves the component permanently stuck.
    return notSupported();
}

ScopedAStatus MesaAvcComponent::createBlockPool(const IComponent::BlockPoolAllocator& /*allocator*/,
                                                 IComponent::BlockPool* out) {
    out->blockPoolId = mNextBlockPoolId++;
    out->configurable = mCfg;
    return ScopedAStatus::ok();
}

ScopedAStatus MesaAvcComponent::destroyBlockPool(int64_t /*blockPoolId*/) {
    return ScopedAStatus::ok();
}

ScopedAStatus MesaAvcComponent::configureVideoTunnel(int32_t /*avSyncHwId*/, NativeHandle* /*out*/) {
    return notSupported();
}

ScopedAStatus MesaAvcComponent::connectToInputSurface(
        const std::shared_ptr<IInputSurface>& inputSurface,
        std::shared_ptr<IInputSurfaceConnection>* out) {
    if (!inputSurface || !out || !mStarted) return badState();
    // The AIDL input-surface path delivers GraphicBuffer-backed WorkBundles
    // through asInputSink(). Keep the connection alive even though the
    // component has no codec-specific drain operation.
    *out = ndk::SharedRefBase::make<MesaInputSurfaceConnection>();
    return ScopedAStatus::ok();
}

ScopedAStatus MesaAvcComponent::asInputSink(std::shared_ptr<IInputSink>* out) {
    if (!out) return badState();
    ALOGI("asInputSink called");
    *out = ndk::SharedRefBase::make<MesaInputSink>(this);
    return ScopedAStatus::ok();
}

}  // namespace gki
