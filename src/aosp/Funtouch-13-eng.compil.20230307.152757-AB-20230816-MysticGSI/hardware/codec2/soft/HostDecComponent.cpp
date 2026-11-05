#include "HostDecComponent.h"

#include "ComponentInterface.h"
#include "Configurable.h"

#include <aidl/android/hardware/media/c2/FrameData.h>
#include <aidl/android/hardware/media/c2/Status.h>
#include <aidl/android/hardware/media/c2/Work.h>
#include <aidl/android/hardware/media/c2/WorkBundle.h>
#include <android/hardware_buffer_aidl.h>
#include <android/log.h>
#include <cutils/native_handle.h>
#include <drm/drm.h>
#include <drm/virtgpu_drm.h>
#include <sys/system_properties.h>

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstring>
#include <map>

#undef LOG_TAG
#define LOG_TAG "tebox-vdec"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


using aidl::android::hardware::common::NativeHandle;
using aidl::android::hardware::HardwareBuffer;
using aidl::android::hardware::media::c2::BaseBlock;
using aidl::android::hardware::media::c2::Block;
using aidl::android::hardware::media::c2::Buffer;
using aidl::android::hardware::media::c2::FrameData;
using aidl::android::hardware::media::c2::IComponent;
using aidl::android::hardware::media::c2::IComponentInterface;
using aidl::android::hardware::media::c2::IComponentListener;
using aidl::android::hardware::media::c2::IGraphicBufferAllocator;
using aidl::android::hardware::media::c2::IInputSink;
using aidl::android::hardware::media::c2::IInputSurface;
using aidl::android::hardware::media::c2::IInputSurfaceConnection;
using aidl::android::hardware::media::c2::Params;
using aidl::android::hardware::media::c2::Status;
using aidl::android::hardware::media::c2::Work;
using aidl::android::hardware::media::c2::WorkBundle;
using ndk::ScopedAStatus;

namespace gki {

namespace {

// ---- CAST_VDEC_* (src/service/cast/include/cast/protocol.h) ----
constexpr uint32_t kVdecMagic = 0x43454456u;  // 'VDEC'
enum : uint32_t {
    kVdecConfig = 1, kVdecInput = 2, kVdecOutput = 3, kVdecFlush = 4, kVdecClose = 5,
    kVdecPicture = 16, kVdecFlushed = 17,
};
constexpr uint32_t kVdecFlagEos = 1, kVdecFlagConfig = 2;

struct __attribute__((packed)) VdecHdr { uint32_t magic, type, size; };
// Every payload starts with the session id (several decoders share the port).
struct __attribute__((packed)) VdecConfig { uint32_t session, codec, width, height; };
struct __attribute__((packed)) VdecInput {
    uint32_t session;
    uint64_t index;
    int64_t pts_us;
    uint32_t flags;
};
struct __attribute__((packed)) VdecOutput {
    uint32_t session;
    uint64_t id;
    uint32_t resource_id, width, height;
};
struct __attribute__((packed)) VdecPicture {
    uint32_t session;
    uint64_t id, index;
    int64_t pts_us;
    int32_t status;
    uint32_t flags, width, height;
};

// Output buffers lent to the host at a time; also the advertised output delay.
constexpr size_t kTargets = 8;
// Works the decoder may hold without a picture (reordering) before the
// oldest is completed empty (a frame that is never displayed).
constexpr size_t kMaxAwaiting = 32;
constexpr int kAllocWaitMs = 1000;
constexpr int kFlushWaitMs = 1000;

// The virtio-serial port has one opener at a time and every component lives
// in this process: the port is opened once and shared. Each decoder is a
// session; one reader thread hands the host's replies to their sessions.
std::mutex gPortWrite;  // whole messages
std::mutex gPortMu;     // guards the state below
int gPortFd = -1;
uint32_t gNextSession = 1;

int openPort() {
    DIR* dir = opendir("/sys/class/virtio-ports");
    if (!dir) return -1;
    int fd = -1;
    while (dirent* e = readdir(dir)) {
        if (e->d_name[0] == '.') continue;
        char path[256], name[64] = "";
        snprintf(path, sizeof(path), "/sys/class/virtio-ports/%s/name", e->d_name);
        FILE* f = fopen(path, "re");
        if (!f) continue;
        if (fgets(name, sizeof(name), f)) name[strcspn(name, "\n")] = 0;
        fclose(f);
        if (strcmp(name, "tebox.vdec") == 0) {
            snprintf(path, sizeof(path), "/dev/%s", e->d_name);
            fd = open(path, O_RDWR | O_CLOEXEC);
            break;
        }
    }
    closedir(dir);
    return fd;
}

bool writeFull(int fd, const void* data, size_t size) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    while (size) {
        ssize_t w = write(fd, p, size);
        if (w < 0 && errno == EINTR) continue;
        if (w <= 0) return false;
        p += w;
        size -= static_cast<size_t>(w);
    }
    return true;
}

bool readFull(int fd, void* data, size_t size) {
    uint8_t* p = static_cast<uint8_t*>(data);
    while (size) {
        ssize_t r = read(fd, p, size);
        if (r < 0 && errno == EINTR) continue;
        if (r <= 0) return false;
        p += r;
        size -= static_cast<size_t>(r);
    }
    return true;
}

bool send(uint32_t type, const void* a, uint32_t asize, const void* b = nullptr,
          uint32_t bsize = 0) {
    std::lock_guard<std::mutex> lock(gPortWrite);
    int fd;
    {
        std::lock_guard<std::mutex> state(gPortMu);
        if (gPortFd < 0) return false;
        fd = gPortFd;
    }
    VdecHdr hdr = {kVdecMagic, type, asize + bsize};
    return writeFull(fd, &hdr, sizeof(hdr)) && (!asize || writeFull(fd, a, asize)) &&
           (!bsize || writeFull(fd, b, bsize));
}

// virtio-gpu resource id behind a dma-buf (our allocator's gfxstream buffers).
uint32_t resourceIdFromFd(int dmabuf) {
    static int render = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC);
    if (render < 0) return 0;
    drm_prime_handle prime = {};
    prime.fd = dmabuf;
    if (ioctl(render, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime)) return 0;
    drm_virtgpu_resource_info info = {};
    info.bo_handle = prime.handle;
    const int ret = ioctl(render, DRM_IOCTL_VIRTGPU_RESOURCE_INFO, &info);
    drm_gem_close close_handle = {};
    close_handle.handle = prime.handle;
    ioctl(render, DRM_IOCTL_GEM_CLOSE, &close_handle);
    return ret ? 0 : info.res_handle;
}

// AHardwareBuffer_getNativeHandle (LLNDK, vndk/hardware_buffer.h; not in the
// NDK stubs): the gralloc handle, whose first fd is the dma-buf.
const native_handle_t* nativeHandle(const AHardwareBuffer* buffer) {
    using Fn = const native_handle_t* (*)(const AHardwareBuffer*);
    static Fn fn = reinterpret_cast<Fn>(dlsym(RTLD_DEFAULT, "AHardwareBuffer_getNativeHandle"));
    return fn ? fn(buffer) : nullptr;
}

ScopedAStatus badState() { return ScopedAStatus::fromServiceSpecificError(Status::BAD_STATE); }

bool waitFd(int fd, int timeoutMs) {
    if (fd < 0) return true;
    pollfd p = {fd, POLLIN, 0};
    int r;
    do {
        r = poll(&p, 1, timeoutMs);
    } while (r < 0 && errno == EINTR);
    return r > 0;
}

// ---- Codec2 params (C2Param layout: u32 size, u32 index, payload) ----
void appendParam(std::vector<uint8_t>* blob, uint32_t index, const void* payload, size_t size) {
    const uint32_t total = static_cast<uint32_t>(8 + size);
    const size_t at = blob->size();
    blob->resize(at + ((total + 7u) & ~7u), 0);
    memcpy(blob->data() + at, &total, 4);
    memcpy(blob->data() + at + 4, &index, 4);
    memcpy(blob->data() + at + 8, payload, size);
}

// C2Hal_RangeInfo (codec2/common/BufferTypes.h): the bytes of a linear block.
constexpr uint32_t kRangeInfoIndex = 0xE0000000u;
// C2Hal_RectInfo: the crop of a graphic block.
constexpr uint32_t kRectInfoIndex = 0xE0000001u;
// C2StreamPictureSizeInfo::output, C2StreamCropRectInfo::output.
constexpr uint32_t kPictureSizeOut = 0xD2001800u;
constexpr uint32_t kCropRectOut = 0xD2001801u;

bool findParam(const std::vector<uint8_t>& blob, uint32_t index, void* out, size_t size) {
    for (size_t at = 0; at + 8 <= blob.size();) {
        uint32_t psize = 0, pindex = 0;
        memcpy(&psize, blob.data() + at, 4);
        memcpy(&pindex, blob.data() + at + 4, 4);
        if (psize < 8 || at + psize > blob.size()) break;
        if (pindex == index && psize >= 8 + size) {
            memcpy(out, blob.data() + at + 8, size);
            return true;
        }
        at += (psize + 7u) & ~7u;
    }
    return false;
}

// The bytes of an input buffer: linear blocks arrive as dma-buf / ashmem
// native handles (the input allocator we ask for keeps them out of BufferPool).
bool readInput(const FrameData& frame, const std::vector<BaseBlock>& baseBlocks,
               std::vector<uint8_t>* out) {
    for (const Buffer& buffer : frame.buffers) {
        for (const Block& block : buffer.blocks) {
            if (block.index < 0 || static_cast<size_t>(block.index) >= baseBlocks.size())
                continue;
            const BaseBlock& base = baseBlocks[static_cast<size_t>(block.index)];
            if (base.getTag() != BaseBlock::nativeBlock) {
                ALOGE("input block type %d not supported", static_cast<int>(base.getTag()));
                return false;
            }
            const NativeHandle& nh = base.get<BaseBlock::nativeBlock>();
            if (nh.fds.empty()) return false;
            struct { uint32_t offset, length; } range = {0, 0};
            if (!findParam(block.meta.params, kRangeInfoIndex, &range, sizeof(range))) {
                off_t end = lseek(nh.fds[0].get(), 0, SEEK_END);
                range.length = end > 0 ? static_cast<uint32_t>(end) : 0;
            }
            if (!range.length) continue;
            const size_t mapLen = static_cast<size_t>(range.offset) + range.length;
            void* map = mmap(nullptr, mapLen, PROT_READ, MAP_SHARED, nh.fds[0].get(), 0);
            if (map == MAP_FAILED) return false;
            const uint8_t* p = static_cast<const uint8_t*>(map) + range.offset;
            out->insert(out->end(), p, p + range.length);
            munmap(map, mapLen);
        }
    }
    return true;
}

NativeHandle dupHandle(const NativeHandle& src) {
    NativeHandle out;
    out.ints = src.ints;
    for (const auto& fd : src.fds) out.fds.emplace_back(fd.get() >= 0 ? dup(fd.get()) : -1);
    return out;
}

FrameData dupFrame(const FrameData& src) {
    FrameData out;
    out.flags = src.flags;
    out.ordinal = src.ordinal;
    out.configUpdate = src.configUpdate;
    for (const Buffer& b : src.buffers) {
        Buffer copy;
        copy.info = b.info;
        for (const Block& blk : b.blocks) {
            Block c;
            c.index = blk.index;
            c.meta = blk.meta;
            c.fence = dupHandle(blk.fence);
            copy.blocks.push_back(std::move(c));
        }
        out.buffers.push_back(std::move(copy));
    }
    return out;
}

BaseBlock dupBaseBlock(const BaseBlock& src) {
    if (src.getTag() == BaseBlock::nativeBlock)
        return BaseBlock::make<BaseBlock::nativeBlock>(dupHandle(src.get<BaseBlock::nativeBlock>()));
    if (src.getTag() == BaseBlock::hwbBlock) {
        const HardwareBuffer& source = src.get<BaseBlock::hwbBlock>();
        HardwareBuffer copy;
        if (source) {
            AHardwareBuffer_acquire(source.get());
            copy.reset(source.get());
        }
        return BaseBlock::make<BaseBlock::hwbBlock>(std::move(copy));
    }
    return BaseBlock();
}

// ByteBuffer clients read YUV: CCodec converts a YUV block to the flexible
// format they get, but cannot convert RGBA. The host renders RGBA on the GPU;
// here (CPU mode only) it becomes YV12, BT.601 limited range.
AHardwareBuffer* rgbaToYv12(AHardwareBuffer* rgba, uint32_t width, uint32_t height) {
    constexpr uint32_t kFormatYv12 = 0x32315659;  // HAL_PIXEL_FORMAT_YV12
    AHardwareBuffer_Desc desc = {};
    desc.width = width;
    desc.height = height;
    desc.layers = 1;
    desc.format = kFormatYv12;
    desc.usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN;
    AHardwareBuffer* yuv = nullptr;
    if (AHardwareBuffer_allocate(&desc, &yuv) != 0) return nullptr;
    AHardwareBuffer_Desc srcDesc;
    AHardwareBuffer_describe(rgba, &srcDesc);
    void* src = nullptr;
    AHardwareBuffer_Planes planes;
    if (AHardwareBuffer_lock(rgba, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, nullptr, &src) != 0) {
        AHardwareBuffer_release(yuv);
        return nullptr;
    }
    if (AHardwareBuffer_lockPlanes(yuv, AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, -1, nullptr,
                                   &planes) != 0 || planes.planeCount < 3) {
        AHardwareBuffer_unlock(rgba, nullptr);
        AHardwareBuffer_release(yuv);
        return nullptr;
    }
    const uint8_t* in = static_cast<const uint8_t*>(src);
    const size_t inStride = static_cast<size_t>(srcDesc.stride) * 4;
    const AHardwareBuffer_Plane& py = planes.planes[0];
    const AHardwareBuffer_Plane& pu = planes.planes[1];
    const AHardwareBuffer_Plane& pv = planes.planes[2];
    for (uint32_t y = 0; y < height; ++y) {
        const uint8_t* row = in + y * inStride;
        uint8_t* out = static_cast<uint8_t*>(py.data) + y * py.rowStride;
        for (uint32_t x = 0; x < width; ++x) {
            const int r = row[4 * x], g = row[4 * x + 1], b = row[4 * x + 2];
            out[x] = static_cast<uint8_t>(((66 * r + 129 * g + 25 * b + 128) >> 8) + 16);
        }
    }
    for (uint32_t y = 0; y + 1 < height; y += 2) {
        const uint8_t* r0 = in + y * inStride;
        const uint8_t* r1 = r0 + inStride;
        uint8_t* u = static_cast<uint8_t*>(pu.data) + (y / 2) * pu.rowStride;
        uint8_t* v = static_cast<uint8_t*>(pv.data) + (y / 2) * pv.rowStride;
        for (uint32_t x = 0; x + 1 < width; x += 2) {
            const int r = r0[4 * x] + r0[4 * x + 4] + r1[4 * x] + r1[4 * x + 4];
            const int g = r0[4 * x + 1] + r0[4 * x + 5] + r1[4 * x + 1] + r1[4 * x + 5];
            const int b = r0[4 * x + 2] + r0[4 * x + 6] + r1[4 * x + 2] + r1[4 * x + 6];
            u[(x / 2) * pu.pixelStride] =
                    static_cast<uint8_t>(((-38 * r - 74 * g + 112 * b + 512) >> 10) + 128);
            v[(x / 2) * pv.pixelStride] =
                    static_cast<uint8_t>(((112 * r - 94 * g - 18 * b + 512) >> 10) + 128);
        }
    }
    AHardwareBuffer_unlock(yuv, nullptr);
    AHardwareBuffer_unlock(rgba, nullptr);
    return yuv;
}

}  // namespace

// Open sessions, by id (weak: a reply never keeps a component alive).
std::map<uint32_t, std::weak_ptr<HostDecComponent>>& sessions() {
    static auto* map = new std::map<uint32_t, std::weak_ptr<HostDecComponent>>;
    return *map;
}

bool HostDecComponent::available() {
    char value[PROP_VALUE_MAX] = "";
    __system_property_get("ro.boot.tebox.gpu", value);
    if (strcmp(value, "gfxstream") != 0) return false;  // RGBA resources only there
    std::lock_guard<std::mutex> state(gPortMu);
    if (gPortFd >= 0) return true;
    const int fd = openPort();
    if (fd < 0) return false;
    gPortFd = fd;  // kept: the port has one opener at a time
    std::thread(portReader, fd).detach();
    return true;
}

// Hands every reply to its session. On a port error the open sessions fail
// (their pictures never come) and the HAL keeps the port closed.
void HostDecComponent::portReader(int fd) {
    std::vector<uint8_t> payload;
    for (;;) {
        VdecHdr hdr;
        if (!readFull(fd, &hdr, sizeof(hdr)) || hdr.magic != kVdecMagic ||
            hdr.size > (1u << 20) || hdr.size < sizeof(uint32_t))
            break;
        payload.resize(hdr.size);
        if (!readFull(fd, payload.data(), hdr.size)) break;
        uint32_t session;
        memcpy(&session, payload.data(), sizeof(session));
        std::shared_ptr<HostDecComponent> comp;
        {
            std::lock_guard<std::mutex> state(gPortMu);
            auto it = sessions().find(session);
            if (it != sessions().end()) comp = it->second.lock();
        }
        if (!comp) continue;  // closed meanwhile: a late reply
        if (hdr.type == kVdecPicture && hdr.size >= sizeof(VdecPicture)) {
            comp->onPicture(payload.data());
        } else if (hdr.type == kVdecFlushed) {
            std::lock_guard<std::mutex> lock(comp->mLock);
            comp->mFlushed = true;
            comp->mCv.notify_all();
        }
    }
    ALOGE("tebox.vdec read failed");
    std::lock_guard<std::mutex> state(gPortMu);
    if (gPortFd == fd) gPortFd = -1;
}

HostDecComponent::HostDecComponent(std::string name, uint32_t codec,
                                   std::shared_ptr<ComponentInterface> intf,
                                   std::shared_ptr<Configurable> cfg,
                                   std::shared_ptr<IComponentListener> listener)
    : mName(std::move(name)),
      mCodec(codec),
      mIntf(std::move(intf)),
      mCfg(std::move(cfg)),
      mListener(std::move(listener)) {}

HostDecComponent::~HostDecComponent() { release(); }

ScopedAStatus HostDecComponent::getInterface(std::shared_ptr<IComponentInterface>* out) {
    *out = mIntf;
    return ScopedAStatus::ok();
}

ScopedAStatus HostDecComponent::start() {
    std::unique_lock<std::mutex> lock(mLock);
    if (mStarted) return ScopedAStatus::ok();
    if (!available()) return ScopedAStatus::fromServiceSpecificError(Status::CANNOT_DO);
    {
        std::lock_guard<std::mutex> state(gPortMu);
        mSession = gNextSession++;
        sessions()[mSession] = ref<HostDecComponent>();
    }
    const VdecConfig cfg = {mSession, mCodec, mCfg->width(), mCfg->height()};
    if (!send(kVdecConfig, &cfg, sizeof(cfg))) {
        ALOGE("%s: tebox.vdec unavailable (%s)", mName.c_str(), strerror(errno));
        std::lock_guard<std::mutex> state(gPortMu);
        sessions().erase(mSession);
        return ScopedAStatus::fromServiceSpecificError(Status::CANNOT_DO);
    }
    mStarted = mRunning = true;
    mHostFailed = false;
    mWidth = mHeight = 0;
    mWorker = std::thread([this] { workerLoop(); });
    ALOGI("%s started (%ux%u hint)", mName.c_str(), cfg.width, cfg.height);
    return ScopedAStatus::ok();
}

void HostDecComponent::closeSession() {
    std::vector<Target> targets;
    {
        std::lock_guard<std::mutex> lock(mLock);
        if (!mStarted) return;
        mRunning = false;
        mStarted = false;
        for (auto& [id, target] : mTargets) targets.push_back(target);
        mTargets.clear();
        mQueued.clear();
        mAwaiting.clear();
        mCsd.clear();
        mSavedCsd.clear();
    }
    mCv.notify_all();
    {
        std::lock_guard<std::mutex> state(gPortMu);
        sessions().erase(mSession);
    }
    send(kVdecClose, &mSession, sizeof(mSession));
    if (mWorker.joinable()) mWorker.join();
    for (Target& t : targets) releaseTarget(&t, true);
}

ScopedAStatus HostDecComponent::stop() {
    closeSession();
    return ScopedAStatus::ok();
}

ScopedAStatus HostDecComponent::reset() {
    closeSession();
    return ScopedAStatus::ok();
}

ScopedAStatus HostDecComponent::release() {
    closeSession();
    std::lock_guard<std::mutex> lock(mLock);
    mIgba.reset();
    if (mWaitableFd >= 0) close(mWaitableFd);
    mWaitableFd = -1;
    return ScopedAStatus::ok();
}

ScopedAStatus HostDecComponent::queue(const WorkBundle& bundle) {
    std::lock_guard<std::mutex> lock(mLock);
    if (!mStarted) return badState();
    // AIDL owns the incoming descriptors; duplicate what outlives this call.
    for (const Work& src : bundle.works) {
        PendingWork pending;
        pending.work.chainInfo = src.chainInfo;
        pending.work.input = dupFrame(src.input);
        pending.work.worklets.resize(1);
        if (!src.worklets.empty()) pending.work.worklets[0].componentId = src.worklets[0].componentId;
        pending.work.workletsProcessed = 0;
        pending.work.result.status = Status::OK;
        for (const BaseBlock& block : bundle.baseBlocks) pending.baseBlocks.push_back(dupBaseBlock(block));
        mQueued.push_back(std::move(pending));
    }
    mCv.notify_all();
    return ScopedAStatus::ok();
}

bool HostDecComponent::allocateTarget(Target* target) {
    const uint32_t w = mCfg->width() ? mCfg->width() : 1920;
    const uint32_t h = mCfg->height() ? mCfg->height() : 1088;
    std::shared_ptr<IGraphicBufferAllocator> igba;
    int waitable;
    {
        std::lock_guard<std::mutex> lock(mLock);
        igba = mIgba;
        waitable = mWaitableFd;
    }
    // The host renders into it on the GPU; the consumer samples or scans it.
    const uint64_t usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
                           AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER;
    AHardwareBuffer* ahb = nullptr;
    int fence = -1;
    if (igba) {
        // Surface output: buffers come from the consumer's queue.
        IGraphicBufferAllocator::Description desc;
        desc.width = static_cast<int32_t>(w);
        desc.height = static_cast<int32_t>(h);
        desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
        desc.usage = static_cast<int64_t>(usage);
        for (int attempt = 0; attempt < 2 && !ahb; ++attempt) {
            IGraphicBufferAllocator::Allocation alloc;
            ScopedAStatus st = igba->allocate(desc, &alloc);
            if (st.isOk() && alloc.buffer) {
                ahb = alloc.buffer.release();
                if (alloc.fence.get() >= 0) fence = dup(alloc.fence.get());
            } else if (st.getServiceSpecificError() == static_cast<int>(Status::BLOCKING) ||
                       st.getServiceSpecificError() == static_cast<int>(Status::TIMED_OUT)) {
                if (!waitFd(waitable, kAllocWaitMs)) return false;
            } else {
                return false;
            }
        }
        if (!ahb) return false;
        target->fromPool = true;
    } else {
        // ByteBuffer output: our own buffer, readable by the client.
        AHardwareBuffer_Desc desc = {};
        desc.width = w;
        desc.height = h;
        desc.layers = 1;
        desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
        desc.usage = usage | AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN;
        if (AHardwareBuffer_allocate(&desc, &ahb) != 0) return false;
        target->fromPool = false;
    }
    if (fence >= 0) {
        // The consumer may still read it: the host must not draw before.
        waitFd(fence, kAllocWaitMs);
        close(fence);
    }
    target->buffer = ahb;
    AHardwareBuffer_getId(ahb, &target->bufferId);
    target->width = w;
    target->height = h;
    return true;
}

void HostDecComponent::releaseTarget(Target* target, bool returnToPool) {
    if (!target->buffer) return;
    if (returnToPool && target->fromPool) {
        std::shared_ptr<IGraphicBufferAllocator> igba;
        {
            std::lock_guard<std::mutex> lock(mLock);
            igba = mIgba;
        }
        bool ok = false;
        if (igba) igba->deallocate(static_cast<int64_t>(target->bufferId), &ok);
    }
    AHardwareBuffer_release(target->buffer);
    target->buffer = nullptr;
}

// Keeps kTargets output buffers with the host.
bool HostDecComponent::provideTargets() {
    for (;;) {
        {
            std::lock_guard<std::mutex> lock(mLock);
            if (!mRunning || mTargets.size() >= kTargets) return true;
        }
        Target target;
        if (!allocateTarget(&target)) return false;
        const native_handle_t* nh = nativeHandle(target.buffer);
        const uint32_t resource = nh && nh->numFds > 0 ? resourceIdFromFd(nh->data[0]) : 0;
        if (!resource) {
            ALOGE("%s: output buffer is not a virtio-gpu resource", mName.c_str());
            {
                std::lock_guard<std::mutex> lock(mLock);
                mHostFailed = true;
            }
            releaseTarget(&target, true);
            return false;
        }
        uint64_t id = 0;
        {
            std::lock_guard<std::mutex> lock(mLock);
            if (mRunning) {
                id = mNextTargetId++;
                mTargets[id] = target;
            }
        }
        if (!id) {  // stopped meanwhile (releaseTarget takes mLock itself)
            releaseTarget(&target, true);
            return true;
        }
        const VdecOutput out = {mSession, id, resource, target.width, target.height};
        if (!send(kVdecOutput, &out, sizeof(out))) return false;
    }
}

void HostDecComponent::workerLoop() {
    // Keeps the host supplied with output buffers even when no input is
    // left (it holds decoded pictures until one arrives, the last ones and
    // the end of stream included). |starved|: the consumer has every buffer
    // right now; retry shortly instead of spinning.
    bool starved = false;
    while (true) {
        PendingWork pending;
        bool haveWork = false;
        {
            std::unique_lock<std::mutex> lock(mLock);
            auto ready = [&] {
                return !mRunning || !mQueued.empty() || (!starved && mTargets.size() < kTargets);
            };
            if (starved) {
                mCv.wait_for(lock, std::chrono::milliseconds(10), ready);
            } else {
                mCv.wait(lock, ready);
            }
            if (!mRunning) return;
            if (!mQueued.empty()) {
                pending = std::move(mQueued.front());
                mQueued.pop_front();
                haveWork = true;
            }
        }
        starved = !provideTargets();
        if (!haveWork) continue;
        std::vector<uint8_t> data;
        const FrameData& in = pending.work.input;
        const bool config = (in.flags & FrameData::CODEC_CONFIG) != 0;
        const bool eos = (in.flags & FrameData::END_OF_STREAM) != 0;
        bool ok = readInput(in, pending.baseBlocks, &data);
        std::vector<PendingWork> done;
        if (ok && config) {
            // Parameter sets travel with the next frame (one access unit per
            // host buffer); the config work itself has no output.
            std::lock_guard<std::mutex> lock(mLock);
            mCsd.insert(mCsd.end(), data.begin(), data.end());
            mSavedCsd = mCsd;
            pending.work.workletsProcessed = 1;
            pending.work.worklets[0].output.ordinal = in.ordinal;
            pending.work.worklets[0].output.flags = 0;
            done.push_back(std::move(pending));
        } else if (ok && (!data.empty() || eos)) {
            const VdecInput hdr = {mSession, static_cast<uint64_t>(in.ordinal.frameIndex),
                                   in.ordinal.timestampUs, eos ? kVdecFlagEos : 0u};
            const int64_t frameIndex = in.ordinal.frameIndex;
            {
                std::lock_guard<std::mutex> lock(mLock);
                if (!mCsd.empty() && !data.empty()) {
                    data.insert(data.begin(), mCsd.begin(), mCsd.end());
                    mCsd.clear();
                }
                mAwaiting[hdr.index] = std::move(pending);
            }
            // Never hold mLock while writing: the reader needs it to drain
            // the host's replies, and the host waits for those.
            ok = send(kVdecInput, &hdr, sizeof(hdr), data.data(),
                      static_cast<uint32_t>(data.size()));
            if (ok && mListener) {
                // The bitstream is with the host: Codec2 may reuse the buffer.
                IComponentListener::InputBuffer released;
                released.frameIndex = frameIndex;
                released.arrayIndex = 0;
                mListener->onInputBuffersReleased({released});
            }
            if (!ok) {
                std::lock_guard<std::mutex> lock(mLock);
                auto it = mAwaiting.find(hdr.index);
                if (it != mAwaiting.end()) {
                    it->second.work.result.status = Status::CORRUPTED;
                    it->second.work.workletsProcessed = 1;
                    done.push_back(std::move(it->second));
                    mAwaiting.erase(it);
                }
            }
        } else {
            // Empty frame (or unreadable input): nothing to decode.
            pending.work.workletsProcessed = 1;
            pending.work.result.status = ok ? Status::OK : static_cast<int32_t>(Status::CORRUPTED);
            pending.work.worklets[0].output.ordinal = in.ordinal;
            done.push_back(std::move(pending));
        }
        emit(&done);
    }
}

void HostDecComponent::finishWork(PendingWork* pending, Target* target, uint32_t width,
                                  uint32_t height, bool eos) {
    Work& work = pending->work;
    FrameData& out = work.worklets[0].output;
    out.ordinal = work.input.ordinal;
    out.flags = eos ? FrameData::END_OF_STREAM : 0;
    work.workletsProcessed = 1;
    work.result.status = Status::OK;
    if (target && target->buffer) {
        HardwareBuffer hwb;
        hwb.reset(target->buffer);  // our reference goes to the client
        target->buffer = nullptr;
        pending->baseBlocks.clear();
        pending->baseBlocks.push_back(BaseBlock::make<BaseBlock::hwbBlock>(std::move(hwb)));
        Block block;
        block.index = 0;
        const uint32_t rect[4] = {0, 0, width, height};
        appendParam(&block.meta.params, kRectInfoIndex, rect, sizeof(rect));
        Buffer buffer;
        buffer.blocks.push_back(std::move(block));
        out.buffers.clear();
        out.buffers.push_back(std::move(buffer));
        // The visible size reaches MediaFormat (width/height, crop).
        if (width != mWidth || height != mHeight) {
            mWidth = width;
            mHeight = height;
            const uint32_t size[2] = {width, height};
            const int32_t crop[4] = {0, 0, static_cast<int32_t>(width),
                                     static_cast<int32_t>(height)};
            appendParam(&out.configUpdate.params, kPictureSizeOut, size, sizeof(size));
            appendParam(&out.configUpdate.params, kCropRectOut, crop, sizeof(crop));
        }
    } else {
        pending->baseBlocks.clear();
        out.buffers.clear();
    }
    // Input blocks were already released to the client.
    work.input.buffers.clear();
}

// Works older than |upToIndex| without a picture by now never get one.
void HostDecComponent::finishStale(uint64_t upToIndex) {
    std::vector<PendingWork> done;
    {
        std::lock_guard<std::mutex> lock(mLock);
        while (mAwaiting.size() > kMaxAwaiting && mAwaiting.begin()->first < upToIndex) {
            finishWork(&mAwaiting.begin()->second, nullptr, 0, 0, false);
            done.push_back(std::move(mAwaiting.begin()->second));
            mAwaiting.erase(mAwaiting.begin());
        }
    }
    emit(&done);
}

void HostDecComponent::onPicture(const void* payload) {
    VdecPicture pic;
    memcpy(&pic, payload, sizeof(pic));
    std::vector<PendingWork> done;
    Target undelivered;
    {
        std::lock_guard<std::mutex> lock(mLock);
        if (!mStarted) return;  // a late reply after stop()
        Target target;
        auto t = pic.id ? mTargets.find(pic.id) : mTargets.end();
        if (t != mTargets.end()) {
            target = t->second;
            mTargets.erase(t);
        }
        if (pic.flags & kVdecFlagEos) {
            // End of stream: everything still waiting completes, the last
            // work carries the EOS flag.
            while (!mAwaiting.empty()) {
                auto it = mAwaiting.begin();
                const bool last = std::next(it) == mAwaiting.end();
                const bool eosWork = (it->second.work.input.flags & FrameData::END_OF_STREAM) != 0;
                finishWork(&it->second, nullptr, 0, 0, last || eosWork);
                done.push_back(std::move(it->second));
                mAwaiting.erase(it);
            }
        } else if (pic.status < 0) {
            ALOGW("%s: picture %llu failed (%d)", mName.c_str(),
                  static_cast<unsigned long long>(pic.index), pic.status);
        } else {
            auto it = mAwaiting.find(pic.index);
            if (it != mAwaiting.end() && target.buffer && !target.fromPool) {
                // ByteBuffer mode: hand out a YUV copy, lend the RGBA again.
                Target yuv;
                yuv.buffer = rgbaToYv12(target.buffer, target.width, target.height);
                yuv.width = target.width;
                yuv.height = target.height;
                if (yuv.buffer) {
                    finishWork(&it->second, &yuv, yuv.width, yuv.height, false);
                    done.push_back(std::move(it->second));
                    mAwaiting.erase(it);
                }
            } else if (it != mAwaiting.end() && target.buffer) {
                // The host scales the visible picture into the whole buffer.
                finishWork(&it->second, &target, target.width, target.height, false);
                done.push_back(std::move(it->second));
                mAwaiting.erase(it);
            }
        }
        if (target.buffer) undelivered = target;
    }
    // Not delivered (failed, or its work is gone): back to the consumer; the
    // worker lends a fresh one with the next input.
    releaseTarget(&undelivered, true);
    emit(&done);
    if (!(pic.flags & kVdecFlagEos)) finishStale(pic.index);
    mCv.notify_all();
}

void HostDecComponent::emit(std::vector<PendingWork>* works) {
    if (works->empty() || !mListener) return;
    WorkBundle bundle;
    for (PendingWork& p : *works) {
        const int32_t base = static_cast<int32_t>(bundle.baseBlocks.size());
        for (Buffer& b : p.work.worklets[0].output.buffers) {
            for (Block& blk : b.blocks) blk.index += base;
        }
        for (BaseBlock& bb : p.baseBlocks) bundle.baseBlocks.push_back(std::move(bb));
        bundle.works.push_back(std::move(p.work));
    }
    works->clear();
    mListener->onWorkDone(bundle);
}

ScopedAStatus HostDecComponent::flush(WorkBundle* out) {
    std::vector<Target> targets;
    {
        std::unique_lock<std::mutex> lock(mLock);
        if (!mStarted) return badState();
        mFlushed = false;
        // Not yet sent: return as is.
        for (PendingWork& p : mQueued) {
            const int32_t base = static_cast<int32_t>(out->baseBlocks.size());
            for (Buffer& b : p.work.input.buffers) {
                for (Block& blk : b.blocks) {
                    if (blk.index >= 0) blk.index += base;
                }
            }
            for (BaseBlock& bb : p.baseBlocks) out->baseBlocks.push_back(std::move(bb));
            out->works.push_back(std::move(p.work));
        }
        mQueued.clear();
        // Streams from MP4 carry the parameter sets only in the codec
        // config: decode the first frame after a seek with them again.
        mCsd = mSavedCsd;
    }
    send(kVdecFlush, &mSession, sizeof(mSession));
    std::unique_lock<std::mutex> lock(mLock);
    mCv.wait_for(lock, std::chrono::milliseconds(kFlushWaitMs), [&] { return mFlushed; });
    // Sent but without a picture: flushed with no output.
    for (auto& [index, p] : mAwaiting) {
        finishWork(&p, nullptr, 0, 0, false);
        out->works.push_back(std::move(p.work));
    }
    mAwaiting.clear();
    // The host gave every buffer back; return them to the consumer.
    for (auto& [id, target] : mTargets) targets.push_back(target);
    mTargets.clear();
    lock.unlock();
    for (Target& t : targets) releaseTarget(&t, true);
    return ScopedAStatus::ok();
}

ScopedAStatus HostDecComponent::drain(bool /*withEos*/) {
    // MediaCodec drains through an END_OF_STREAM work, handled in queue().
    return ScopedAStatus::ok();
}

ScopedAStatus HostDecComponent::createBlockPool(const IComponent::BlockPoolAllocator& allocator,
                                                IComponent::BlockPool* out) {
    std::lock_guard<std::mutex> lock(mLock);
    if (allocator.gbAllocator && allocator.gbAllocator->igba) {
        mIgba = allocator.gbAllocator->igba;
        if (mWaitableFd >= 0) close(mWaitableFd);
        mWaitableFd = dup(allocator.gbAllocator->waitableFd.get());
    }
    out->blockPoolId = mNextBlockPoolId++;
    out->configurable = mCfg;
    return ScopedAStatus::ok();
}

ScopedAStatus HostDecComponent::destroyBlockPool(int64_t /*blockPoolId*/) {
    return ScopedAStatus::ok();
}

ScopedAStatus HostDecComponent::configureVideoTunnel(int32_t, NativeHandle*) {
    return ScopedAStatus::fromServiceSpecificError(Status::OMITTED);
}

ScopedAStatus HostDecComponent::connectToInputSurface(const std::shared_ptr<IInputSurface>&,
                                                      std::shared_ptr<IInputSurfaceConnection>*) {
    return ScopedAStatus::fromServiceSpecificError(Status::OMITTED);
}

ScopedAStatus HostDecComponent::asInputSink(std::shared_ptr<IInputSink>*) {
    return ScopedAStatus::fromServiceSpecificError(Status::OMITTED);
}

}  // namespace gki
