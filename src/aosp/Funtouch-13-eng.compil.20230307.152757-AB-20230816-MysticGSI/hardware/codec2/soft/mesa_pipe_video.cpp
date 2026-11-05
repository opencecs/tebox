#include "mesa_pipe_video.h"

#include <android/log.h>

#include <dirent.h>
#include <dlfcn.h>
#include <sys/system_properties.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include <drm/virtgpu_drm.h>

#include "frontend/drm_driver.h"
#include "frontend/winsys_handle.h"
#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "pipe/p_video_codec.h"
#include "pipe/p_video_enums.h"
#include "virtio-gpu/virgl_hw.h"
#include "virtio-gpu/virglrenderer_hw.h"
#include "util/u_inlines.h"

#define LOG_TAG "mesa-pipe-video"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace gki {
namespace mesa_video {
namespace {

constexpr const char* kRenderNode = "/dev/dri/renderD128";
constexpr const char* kDriCandidates[] = {
        "/vendor/lib64/dri/virtio_gpu_dri.so",
        "/vendor/lib64/dri/virtio-gpu_dri.so",
        "/vendor/lib64/dri/libgallium_dri.so",
};

// Keep the wire values tied to the same Mesa headers used by the DRI build.
// Entry-point values are UNKNOWN=0, BITSTREAM=1, ... ENCODE=4; the old
// hand-written 0/1 mapping silently classified the host encoder as decode.
constexpr uint32_t kPipeAvcBaseline = PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE;
constexpr uint32_t kPipeAvcMain = PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN;
constexpr uint32_t kPipeAvcHigh = PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH;
constexpr uint32_t kPipeEntrypointBitstream = PIPE_VIDEO_ENTRYPOINT_BITSTREAM;
constexpr uint32_t kPipeEntrypointEncode = PIPE_VIDEO_ENTRYPOINT_ENCODE;

bool envForce() {
    const char* v = getenv("GKI_MESA_VIDEO_FORCE");
    return v && v[0] == '1' && v[1] == '\0';
}

int openRenderFd() {
    return open(kRenderNode, O_RDWR | O_CLOEXEC);
}

bool readVirglCaps(int fd, union virgl_caps* caps) {
    memset(caps, 0, sizeof(*caps));
    drm_virtgpu_get_caps args = {};
    args.cap_set_id = VIRGL_RENDERER_CAPSET_VIRGL2;
    args.cap_set_ver = 2;
    args.addr = reinterpret_cast<uint64_t>(caps);
    args.size = sizeof(*caps);
    if (ioctl(fd, DRM_IOCTL_VIRTGPU_GET_CAPS, &args) == 0) {
        return true;
    }
    // Fall back to capset 1 size if host is older.
    memset(caps, 0, sizeof(*caps));
    args.cap_set_id = VIRGL_RENDERER_CAPSET_VIRGL;
    args.cap_set_ver = 1;
    args.size = sizeof(struct virgl_caps_v1);
    return ioctl(fd, DRM_IOCTL_VIRTGPU_GET_CAPS, &args) == 0;
}

bool profileIsAvc(uint32_t profile) {
    return profile >= kPipeAvcBaseline && profile <= 16 /* HIGH444 */;
}

Caps probeCapsLocked() {
    Caps out;
    if (envForce()) ALOGI("GKI_MESA_VIDEO_FORCE is ignored unless the backend probes successfully");

    int fd = openRenderFd();
    if (fd < 0) {
        ALOGW("open %s failed: %s", kRenderNode, strerror(errno));
        return out;
    }

    uint64_t has3d = 0;
    drm_virtgpu_getparam gp = {VIRTGPU_PARAM_3D_FEATURES,
                               reinterpret_cast<uint64_t>(&has3d)};
    if (ioctl(fd, DRM_IOCTL_VIRTGPU_GETPARAM, &gp) != 0 || !has3d) {
        ALOGW("virtio-gpu 3D features unavailable");
        ::close(fd);
        return out;
    }

    union virgl_caps caps;
    if (!readVirglCaps(fd, &caps)) {
        ALOGW("VIRTGPU_GET_CAPS failed: %s", strerror(errno));
        ::close(fd);
        return out;
    }
    ::close(fd);

    const uint32_t n = caps.v2.num_video_caps;
    if (n == 0 || n > 32) {
        ALOGI("VirGL video caps unavailable (num_video_caps=%u)", n);
        return out;
    }

    for (uint32_t i = 0; i < n; ++i) {
        const virgl_video_caps& vc = caps.v2.video_caps[i];
        if (!profileIsAvc(vc.profile)) continue;
        if (vc.entrypoint == kPipeEntrypointBitstream) {
            out.avc_decode = true;
        } else if (vc.entrypoint == kPipeEntrypointEncode) {
            out.avc_encode = true;
        }
        if (vc.max_width > out.max_width) out.max_width = vc.max_width;
        if (vc.max_height > out.max_height) out.max_height = vc.max_height;
    }
    ALOGI("VirGL video: avc_dec=%d avc_enc=%d max=%ux%u (n=%u)",
          out.avc_decode ? 1 : 0, out.avc_encode ? 1 : 0, out.max_width,
          out.max_height, n);
    return out;
}

std::mutex gCapsMu;
bool gCapsInit = false;
Caps gCaps;

// ---- Host encoder (gfxstream backend) ----
// No VirGL video without VirGL: frames are encoded by tebox-cast on the host.
// We send each frame's virtio-gpu resource id over the virtio-serial port
// tebox.venc; QEMU reads that gfxstream ColorBuffer for the host hardware
// encoder and the H.264 comes back on the same port (CAST_VENC_* in
// src/service/cast/include/cast/protocol.h, mirrored here). Every payload
// starts with the session id: several encoders share the port.
constexpr uint32_t kVencMagic = 0x434e4556u;  // 'VENC'
enum : uint32_t { kVencConfig = 1, kVencFrame = 2, kVencClose = 3, kVencBitstream = 16 };
constexpr uint32_t kVencFlagKey = 1;

struct __attribute__((packed)) VencHdr { uint32_t magic, type, size; };
struct __attribute__((packed)) VencConfig {
    uint32_t session, width, height, bitrate, fps, gop_sec;
};
struct __attribute__((packed)) VencFrame {
    uint32_t session;
    uint64_t index;
    int64_t pts_us;
    uint32_t resource_id, width, height, format, flags;
};
struct __attribute__((packed)) VencOut {
    uint32_t session;
    uint64_t index;
    int64_t pts_us;
    int32_t status;
    uint32_t flags;
};

bool gfxstreamGpu() {
    char value[PROP_VALUE_MAX] = "";
    __system_property_get("ro.boot.tebox.gpu", value);
    return strcmp(value, "gfxstream") == 0;
}

// tebox.venc is a virtio-serial port with one opener at a time, while every
// encoder lives in this process: the port is opened once and shared. Each
// session has an id; one reader thread files the host's replies by session,
// so concurrent encoders (two screen-cast clients) do not disturb each other.
struct VencReply {
    uint64_t index;
    int32_t status;
    std::vector<uint8_t> data;
};
std::mutex gVencWrite;  // whole messages
std::mutex gVencMu;     // guards the state below
std::condition_variable gVencCv;
int gVencFd = -1;
uint32_t gVencNextSession = 1;
std::map<uint32_t, std::deque<VencReply>> gVencReplies;  // open sessions

int openVencPort() {
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
        if (strcmp(name, "tebox.venc") == 0) {
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

// |fd| is the port the session was opened on: a reopened port (after an
// error) is not that session's any more, and a closed fd number may be reused.
bool vencSend(int fd, uint32_t type, const void* payload, uint32_t size) {
    VencHdr hdr = {kVencMagic, type, size};
    std::lock_guard<std::mutex> lock(gVencWrite);
    {
        std::lock_guard<std::mutex> state(gVencMu);
        if (fd < 0 || fd != gVencFd) return false;
    }
    return writeFull(fd, &hdr, sizeof(hdr)) && (!size || writeFull(fd, payload, size));
}

// Files every reply under its session. On a port error (QEMU or tebox-cast
// went away) every waiting session fails and the port is reopened later.
void vencReaderLoop(int fd) {
    for (;;) {
        VencHdr hdr;
        VencOut out;
        if (!readFull(fd, &hdr, sizeof(hdr)) || hdr.magic != kVencMagic ||
            hdr.type != kVencBitstream || hdr.size < sizeof(out) || hdr.size > (16u << 20) ||
            !readFull(fd, &out, sizeof(out)))
            break;
        VencReply reply = {out.index, out.status, std::vector<uint8_t>(hdr.size - sizeof(out))};
        if (!reply.data.empty() && !readFull(fd, reply.data.data(), reply.data.size())) break;
        std::lock_guard<std::mutex> lock(gVencMu);
        auto it = gVencReplies.find(out.session);
        if (it != gVencReplies.end()) it->second.push_back(std::move(reply));
        gVencCv.notify_all();
    }
    ALOGE("host encoder: tebox.venc read failed");
    std::lock_guard<std::mutex> write(gVencWrite);  // no send is using |fd|
    std::lock_guard<std::mutex> lock(gVencMu);
    for (auto& [session, replies] : gVencReplies) replies.push_back({UINT64_MAX, -EIO, {}});
    if (gVencFd == fd) gVencFd = -1;
    ::close(fd);
    gVencCv.notify_all();
}

// Opens the port (and its reader) if needed; returns the fd or -1.
int vencPortLocked() {
    if (gVencFd >= 0) return gVencFd;
    gVencFd = openVencPort();
    if (gVencFd >= 0) std::thread(vencReaderLoop, gVencFd).detach();
    return gVencFd;
}

// virtio-gpu resource id behind a dma-buf (our allocator's gfxstream buffers).
uint32_t resourceIdFromFd(int dmabuf) {
    static int render = open(kRenderNode, O_RDWR | O_CLOEXEC);
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

}  // namespace

const Caps& queryCaps() {
    if (gfxstreamGpu()) {
        static const Caps kHostCaps = {false, true, 4096, 4096};
        return kHostCaps;
    }
    std::lock_guard<std::mutex> lock(gCapsMu);
    // The HAL is started in parallel with virtio-gpu.  Do not permanently
    // cache an empty capset observed before renderD128 is ready.
    if (!gCapsInit || (!gCaps.avc_decode && !gCaps.avc_encode)) {
        Caps fresh = probeCapsLocked();
        if (fresh.avc_decode || fresh.avc_encode || !gCapsInit)
            gCaps = fresh;
        gCapsInit = true;
    }
    return gCaps;
}

bool hasAvcDecode() { return queryCaps().avc_decode; }
bool hasAvcEncode() { return queryCaps().avc_encode; }
bool forceVideoEnabled() { return envForce(); }

bool backendAvailable(Entrypoint ep) {
    // The encoder path below has a complete NV12 -> pipe_video_codec bridge.
    // Decoder submission still needs an AVC parameter-set/slice parser, so do
    // not publish a decoder that would accept work and then return ENOSYS.
    if (ep == Entrypoint::kDecode) return false;
    if (gfxstreamGpu()) {
        // tebox-cast connects to the port at any time; the port existing is
        // what makes this guest offer the encoder.
        const int fd = openVencPort();
        if (fd >= 0) ::close(fd);
        ALOGI("AVC encode via host (tebox.venc): %s", fd >= 0 ? "ok" : "no port");
        return fd >= 0;
    }
    Session probe;
    const bool ok = probe.open(ep, Profile::kAvcMain, 640, 360);
    FILE* trace = fopen("/data/local/tmp/mesa-c2-probe.log", "a");
    if (trace) {
        fprintf(trace, "backend ep=%d result=%d force=%d\\n", static_cast<int>(ep),
                ok ? 1 : 0, forceVideoEnabled() ? 1 : 0);
        fclose(trace);
    }
    ALOGI("AVC encode backend probe: %s", ok ? "ok" : "failed");
    return ok;
}

Session::~Session() { close(); }

bool Session::ensurePipe() {
    if (mPipeOk) return true;
    setenv("LIBGL_DRIVERS_PATH", "/vendor/lib64/dri", 0);
    setenv("GBM_BACKENDS_PATH", "/vendor/lib64/gbm", 0);

    for (const char* path : kDriCandidates) {
        void* lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
        if (!lib) {
            fprintf(stderr, "mesa-pipe-video: dlopen %s failed: %s\\n", path, dlerror());
            continue;
        }
        // Android's Mesa DRI target statically links the pipe loader.  Its
        // exported descriptor is named after the driver, while the standalone
        // pipe-loader .so uses the generic driver_descriptor name.
        using DescriptorFn = const drm_driver_descriptor* (*)();
        auto get_descriptor = reinterpret_cast<DescriptorFn>(
                dlsym(lib, "gki_virtio_gpu_driver_descriptor"));
        auto* dd = get_descriptor ? get_descriptor() : nullptr;
        if (!dd) dd = reinterpret_cast<const drm_driver_descriptor*>(
                dlsym(lib, "driver_descriptor"));
        if (!dd) {
            dd = reinterpret_cast<const drm_driver_descriptor*>(
                    dlsym(lib, "virtio_gpu_driver_descriptor"));
        }
        if (!dd || !dd->create_screen) {
            fprintf(stderr, "mesa-pipe-video: no virtio descriptor in %s (%s)\\n", path,
                    dlerror() ? dlerror() : "missing create_screen");
            dlclose(lib);
            continue;
        }
        int fd = openRenderFd();
        if (fd < 0) {
            fprintf(stderr, "mesa-pipe-video: open render node failed: %s\\n", strerror(errno));
            dlclose(lib);
            continue;
        }
        // pipe_screen_config is optional; pass nullptr.
        pipe_screen* screen = dd->create_screen(fd, nullptr);
        // create_screen takes ownership of fd semantics vary; keep dri lib.
        if (!screen) {
            fprintf(stderr, "mesa-pipe-video: create_screen failed for %s\\n", path);
            ::close(fd);
            dlclose(lib);
            continue;
        }
        mDriLib = lib;
        mCreateExternalBuffer = reinterpret_cast<CreateExternalBufferFn>(
                dlsym(lib, "gki_virgl_video_create_buffer_from_resource"));
        ALOGI("external video buffer helper=%p", reinterpret_cast<void*>(mCreateExternalBuffer));
        mScreen = screen;
        mContext = mScreen->context_create(mScreen, nullptr, 0);
        if (!mContext) {
            mScreen->destroy(mScreen);
            mScreen = nullptr;
            ::close(fd);
            dlclose(lib);
            continue;
        }
        mPipeOk = true;
        ALOGI("pipe_screen via %s (%s)", path, dd->driver_name ? dd->driver_name : "?");
        return true;
    }
    ALOGW("unable to create pipe_screen from dri (video ops limited)");
    return false;
}

bool Session::createCodec() {
    if (!mContext || !mScreen || !mContext->create_video_codec) return false;
    pipe_video_codec templ = {};
    templ.profile = PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN;
    if (mProfile == Profile::kAvcBaseline)
        templ.profile = PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE;
    else if (mProfile == Profile::kAvcHigh)
        templ.profile = PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH;
    templ.entrypoint = mEp == Entrypoint::kEncode ? PIPE_VIDEO_ENTRYPOINT_ENCODE
                                                  : PIPE_VIDEO_ENTRYPOINT_BITSTREAM;
    templ.chroma_format = PIPE_VIDEO_CHROMA_FORMAT_420;
    templ.width = mWidth;
    templ.height = mHeight;
    templ.max_references = PIPE_H264_MAX_REFERENCES;
    templ.expect_chunked_decode = false;
    if (mScreen->get_video_param && !forceVideoEnabled() &&
        !mScreen->get_video_param(mScreen, templ.profile, templ.entrypoint,
                                  PIPE_VIDEO_CAP_SUPPORTED)) {
        ALOGW("Gallium rejected AVC profile=%d entrypoint=%d", templ.profile, templ.entrypoint);
        return false;
    }
    mCodec = mContext->create_video_codec(mContext, &templ);
    if (!mCodec) {
        ALOGW("Gallium create_video_codec failed for entrypoint=%d", templ.entrypoint);
        return false;
    }
    if (mEp == Entrypoint::kEncode) {
        pipe_video_buffer templ_buf = {};
        templ_buf.buffer_format = PIPE_FORMAT_NV12;
        templ_buf.width = mWidth;
        templ_buf.height = mHeight;
        templ_buf.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
        templ_buf.contiguous_planes = true;
        mSource = mContext->create_video_buffer(mContext, &templ_buf);
        if (!mSource) {
            ALOGW("Gallium create_video_buffer failed");
            mCodec->destroy(mCodec);
            mCodec = nullptr;
            return false;
        }
        mBitstream = pipe_buffer_create(mScreen, PIPE_BIND_CUSTOM, PIPE_USAGE_STAGING,
                                        static_cast<unsigned>(mWidth * mHeight * 2));
        bool ringOk = mBitstream != nullptr;
        mBitstreams[0] = mBitstream;
        for (unsigned i = 1; ringOk && i < kBitstreams; ++i) {
            mBitstreams[i] = pipe_buffer_create(mScreen, PIPE_BIND_CUSTOM, PIPE_USAGE_STAGING,
                                                static_cast<unsigned>(mWidth * mHeight * 2));
            ringOk = mBitstreams[i] != nullptr;
        }
        if (!ringOk) {
            for (unsigned i = 1; i < kBitstreams; ++i)
                pipe_resource_reference(&mBitstreams[i], nullptr);
            mBitstreams[0] = nullptr;
            pipe_resource_reference(&mBitstream, nullptr);
        }
        if (!mBitstream) {
            mSource->destroy(mSource);
            mSource = nullptr;
            mCodec->destroy(mCodec);
            mCodec = nullptr;
            return false;
        }
    }
    return true;
}

bool Session::open(Entrypoint ep, Profile profile, uint32_t width, uint32_t height) {
    close();
    const Caps& caps = queryCaps();
    if (ep == Entrypoint::kDecode && !caps.avc_decode) return false;
    if (ep == Entrypoint::kEncode && !caps.avc_encode && !forceVideoEnabled()) return false;
    if (width == 0 || height == 0) return false;
    if (caps.max_width && width > caps.max_width) return false;
    if (caps.max_height && height > caps.max_height) return false;

    mEp = ep;
    mProfile = profile;
    mVisibleWidth = width;
    mVisibleHeight = height;
    // Keep the resource dimensions identical to the producer's graphic
    // buffer.  The H.264 picture descriptor below carries the macroblock
    // ceiling, while importing a padded 1088-wide resource for a 1080-wide
    // HardwareBuffer makes direct dmabuf encoding unsafe and forces a costly
    // CPU copy.
    mWidth = width;
    mHeight = height;
    if (ep == Entrypoint::kEncode && gfxstreamGpu()) {
        int fd;
        uint32_t session;
        {
            std::lock_guard<std::mutex> lock(gVencMu);
            fd = vencPortLocked();
            session = gVencNextSession++;
            if (fd >= 0) gVencReplies[session];
        }
        const VencConfig cfg = {session, width, height, 8000000, 30, 2};
        if (fd < 0 || !vencSend(fd, kVencConfig, &cfg, sizeof(cfg))) {
            ALOGE("host encoder: tebox.venc unavailable (%s)", strerror(errno));
            std::lock_guard<std::mutex> lock(gVencMu);
            gVencReplies.erase(session);
            return false;
        }
        mHostFd = fd;
        mHostSession = session;
        mHostIndex = 0;
        mHost = true;
        mOpen = true;
        mFrameIndex = 0;
        mSubmitted = 0;
        ALOGI("session open enc %ux%u via host encoder", width, height);
        return true;
    }
    ensurePipe();  // best-effort
    if (!createCodec()) return false;
    mOpen = true;
    mGotCsd = false;
    mFrameIndex = 0;
    mSubmitted = 0;
    ALOGI("session open %s %ux%u pipe=%d",
          ep == Entrypoint::kEncode ? "enc" : "dec", width, height,
          mPipeOk ? 1 : 0);
    return true;
}

void Session::close() {
    if (mHost) {
        // The port stays open for the other sessions.
        vencSend(mHostFd, kVencClose, &mHostSession, sizeof(mHostSession));
        {
            std::lock_guard<std::mutex> lock(gVencMu);
            gVencReplies.erase(mHostSession);
        }
        gVencCv.notify_all();
        mHostFd = -1;
        mHost = false;
        mOpen = false;
        return;
    }
    if (mTarget) {
        mTarget->destroy(mTarget);
        mTarget = nullptr;
    }
    if (mSource) {
        mSource->destroy(mSource);
        mSource = nullptr;
    }
    for (unsigned i = 1; i < kBitstreams; ++i)
        pipe_resource_reference(&mBitstreams[i], nullptr);
    mBitstreams[0] = nullptr;
    mNextBitstream = 0;
    if (mBitstream) {
        pipe_resource_reference(&mBitstream, nullptr);
    }
    if (mCodec) {
        mCodec->destroy(mCodec);
        mCodec = nullptr;
    }
    if (mContext) {
        mContext->destroy(mContext);
        mContext = nullptr;
    }
    if (mScreen) {
        mScreen->destroy(mScreen);
        mScreen = nullptr;
    }
    if (mDriLib) {
        // Keep dri mapped for process lifetime of HAL; avoid unload races.
        mDriLib = nullptr;
    }
    mPipeOk = false;
    mOpen = false;
}

void Session::flush() {
    if (mHost) return;  // host frames in flight still get collected
    mFrameIndex = 0;
    mSubmitted = 0;
}

int Session::decode(const uint8_t* bitstream, size_t size, int64_t ptsUs,
                    std::vector<uint8_t>* outNv12, int64_t* outPtsUs, bool* outEos) {
    if (!mOpen || mEp != Entrypoint::kDecode || !outNv12) return -EINVAL;
    (void)bitstream;
    (void)size;
    (void)ptsUs;
    // Pipe video decode path requires pipe_video_codec::decode_bitstream.
    // Until the Gallium context bridge is linked, report no output yet.
    if (outEos) *outEos = false;
    if (outPtsUs) *outPtsUs = ptsUs;
    outNv12->clear();
    ALOGW("decode: pipe_video_codec not bridged yet (size=%zu)", size);
    return -ENOSYS;
}

int Session::encode(const uint8_t* nv12, size_t nv12Size, int64_t ptsUs, bool keyframe,
                    std::vector<uint8_t>* outBs, int64_t* outPtsUs, bool* outEos) {
    if (!mOpen || mEp != Entrypoint::kEncode || !outBs) return -EINVAL;
    const bool external = mSourceExternal;
    const bool synthetic = nv12Size == 0 && !external;
    const size_t visibleY = static_cast<size_t>(mVisibleWidth) * mVisibleHeight;
    const size_t visibleNeed = visibleY * 3 / 2;
    if (!synthetic && !external && (!nv12 || nv12Size < visibleNeed)) return -EINVAL;
    const uint8_t* frame = nv12;
    std::vector<uint8_t> padded;
    if (!synthetic && !external && (mWidth != mVisibleWidth || mHeight != mVisibleHeight)) {
        const size_t codedY = static_cast<size_t>(mWidth) * mHeight;
        padded.assign(codedY * 3 / 2, 128);
        std::fill(padded.begin(), padded.begin() + codedY, 16);
        for (uint32_t y = 0; y < mVisibleHeight; ++y) {
            memcpy(padded.data() + static_cast<size_t>(y) * mWidth,
                   nv12 + static_cast<size_t>(y) * mVisibleWidth, mVisibleWidth);
        }
        const uint8_t* srcUv = nv12 + visibleY;
        uint8_t* dstUv = padded.data() + codedY;
        for (uint32_t y = 0; y < (mVisibleHeight + 1) / 2; ++y) {
            memcpy(dstUv + static_cast<size_t>(y) * mWidth,
                   srcUv + static_cast<size_t>(y) * mVisibleWidth, mVisibleWidth);
        }
        frame = padded.data();
        nv12Size = padded.size();
    }
    if (outEos) *outEos = false;
    if (outPtsUs) *outPtsUs = ptsUs;
    outBs->clear();
    if (!mCodec || !mContext || !mSource || !mBitstream) return -ENODEV;

    pipe_resource* planes[3] = {};
    if (!external) {
      if (!mSource->get_resources) return -ENOSYS;
      mSource->get_resources(mSource, planes);
      if (!planes[0] || !planes[1]) return -EIO;
    }
    pipe_box ybox = {0, static_cast<int32_t>(mWidth), 0, static_cast<int32_t>(mHeight), 0, 1};
    // NV12's interleaved UV plane is R8G8 and therefore half as many texels
    // wide as the luma plane (each texel still contains two bytes).
    pipe_box uvbox = {0, static_cast<int32_t>((mWidth + 1) / 2), 0,
                      static_cast<int32_t>((mHeight + 1) / 2), 0, 1};
    auto uploadPlane = [&](pipe_resource* resource, const pipe_box& box, const uint8_t* src,
                           unsigned rowBytes, const char* name) {
        (void)name;
        mContext->texture_subdata(mContext, resource, 0, 0, &box, src, rowBytes,
                                  static_cast<uintptr_t>(rowBytes) * box.height);
        return true;
    };
    if (external) {
        // The imported dmabuf is already the source video buffer.  Do not
        // map or upload it; the host reads the resource directly.
    } else if (synthetic) {
        // Use the same upload path as real frames.  clear_texture on the
        // R8/R8G8 video resources is not guaranteed to materialize before
        // the host readback and can leave both planes at zero (green video).
        std::vector<uint8_t> black(static_cast<size_t>(mWidth) * mHeight * 3 / 2, 128);
        std::fill(black.begin(), black.begin() + static_cast<size_t>(mWidth) * mHeight, 16);
        uploadPlane(planes[0], ybox, black.data(), mWidth, "Y");
        uploadPlane(planes[1], uvbox, black.data() + static_cast<size_t>(mWidth) * mHeight,
                    mWidth, "UV");
    } else {
        uploadPlane(planes[0], ybox, frame, mWidth, "Y");
        uploadPlane(planes[1], uvbox, frame + mWidth * mHeight, mWidth, "UV");
    }
    // The virgl video backend needs the upload visible before begin_frame;
    // keep this flush for correctness. Queue bounding below removes stale
    // frames without starving the host encoder.
    if (mContext->flush) mContext->flush(mContext, nullptr, PIPE_FLUSH_HINT_FINISH);

    pipe_h264_enc_picture_desc desc = {};
    desc.base.profile = PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN;
    desc.base.entry_point = PIPE_VIDEO_ENTRYPOINT_ENCODE;
    desc.base.input_format = PIPE_FORMAT_NV12;
    desc.base.output_format = PIPE_FORMAT_R8_UNORM;
    desc.seq.profile_idc = 77;
    desc.seq.level_idc = 41;
    desc.seq.bit_depth_luma_minus8 = 0;
    desc.seq.bit_depth_chroma_minus8 = 0;
    desc.seq.log2_max_frame_num_minus4 = 4;
    desc.seq.log2_max_pic_order_cnt_lsb_minus4 = 4;
    desc.seq.pic_order_cnt_type = 0;
    desc.seq.pic_width_in_mbs_minus1 = (mWidth + 15) / 16 - 1;
    desc.seq.pic_height_in_map_units_minus1 = (mHeight + 15) / 16 - 1;
    desc.seq.max_num_ref_frames = 0;
    desc.seq.max_dec_frame_buffering = 0;
    desc.seq.vui_parameters_present_flag = 0;
    desc.seq.num_units_in_tick = 1;
    desc.seq.time_scale = 60;
    desc.intra_idr_period = 1;
    desc.ip_period = 1;
    desc.gop_size = 1;
    desc.picture_type = (keyframe || mSubmitted == 0)
                                ? PIPE_H2645_ENC_PICTURE_TYPE_IDR
                                : PIPE_H2645_ENC_PICTURE_TYPE_P;
    desc.slice.slice_type = desc.picture_type == PIPE_H2645_ENC_PICTURE_TYPE_P
                                    ? PIPE_H264_SLICE_TYPE_P
                                    : PIPE_H264_SLICE_TYPE_I;
    desc.slice.num_ref_idx_l0_active_minus1 = 0;
    desc.pic_ctrl.nal_unit_type = desc.picture_type == PIPE_H2645_ENC_PICTURE_TYPE_IDR ? 5 : 1;
    desc.pic_ctrl.nal_ref_idc = 3;
    desc.pic_ctrl.enc_cabac_enable = 1;
    desc.pic_ctrl.deblocking_filter_control_present_flag = 1;
    desc.init_qp = 26;
    desc.quant_i_frames = 26;
    desc.quant_p_frames = 26;
    desc.num_slice_descriptors = 1;
    desc.slices_descriptors[0].macroblock_address = 0;
    desc.slices_descriptors[0].num_macroblocks = ((mWidth + 15) / 16) * ((mHeight + 15) / 16);
    desc.slices_descriptors[0].slice_type = static_cast<pipe_h264_slice_type>(desc.slice.slice_type);

    void* feedback = nullptr;
    pipe_resource* bitstream = mBitstream;
    if (mTicketOut && mBitstreams[0]) bitstream = mBitstreams[mNextBitstream++ % kBitstreams];
    mCodec->begin_frame(mCodec, mSource, &desc.base);
    mCodec->encode_bitstream(mCodec, mSource, bitstream, &feedback);
    const int endRc = mCodec->end_frame(mCodec, mSource, &desc.base);
    if (endRc != 0) {
        ALOGW("encode end_frame failed rc=%d frame=%u", endRc, mFrameIndex);
        return -EIO;
    }
    if (mTicketOut) {
        // Pipelined: hand back a fence instead of waiting for the host.
        mContext->flush(mContext, &mTicketOut->fence, 0);
        mTicketOut->feedback = feedback;
        mTicketOut->bitstream = bitstream;
        mTicketOut->ptsUs = ptsUs;
        mTicketOut->index = mSubmitted++;
        return 0;
    }
    ++mSubmitted;
    if (mCodec->flush) mCodec->flush(mCodec);
    const unsigned size = pollFeedback(feedback, nullptr);
    if (size == 0 || size > mBitstream->width0) {
        ALOGW("encode feedback invalid size=%u capacity=%u feedback=%p frame=%u", size,
              mBitstream->width0, feedback, mFrameIndex);
        return -EIO;
    }
    // Only advance after a successful coded frame so a failed GPU import
    // cannot turn the next real IDR into a config-less P frame.
    ++mFrameIndex;
    outBs->resize(size);
    pipe_buffer_read(mContext, mBitstream, 0, size, outBs->data());
    if (outPtsUs) *outPtsUs = ptsUs;
    return static_cast<int>(size);
}

int Session::encodeGraphic(int fd, uint32_t width, uint32_t height, uint32_t stride,
                           uint32_t format, int64_t ptsUs, bool keyframe,
                           std::vector<uint8_t>* outBs, int64_t* outPtsUs, bool* outEos) {
    if (!mOpen || mEp != Entrypoint::kEncode || fd < 0 || !mScreen ||
        !mCreateExternalBuffer || !outBs || !width || !height || stride < width) {
        ALOGW("encodeGraphic invalid open=%d ep=%d fd=%d screen=%p helper=%p out=%p %ux%u stride=%u",
              mOpen ? 1 : 0, static_cast<int>(mEp), fd, mScreen,
              reinterpret_cast<void*>(mCreateExternalBuffer), outBs, width, height, stride);
        return -EINVAL;
    }
    // The encoder descriptor is created for the configured coded dimensions.
    // Importing a differently-sized resource and then submitting it with that
    // descriptor is rejected by some backends and can dereference stale plane
    // views in others.  Let the caller use the linear path for such frames.
    if (width != mVisibleWidth || height != mVisibleHeight ||
        width != mWidth || height != mHeight) {
        ALOGW("encodeGraphic dimensions %ux%u do not match coded %ux%u", width, height,
              mWidth, mHeight);
        return -ENOTSUP;
    }
    if (format != 1 /* AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM */) {
        ALOGW("encodeGraphic unsupported format=%u", format);
        return -ENOTSUP;
    }
    pipe_resource templ = {};
    templ.target = PIPE_TEXTURE_2D;
    templ.format = PIPE_FORMAT_R8G8B8A8_UNORM;
    templ.width0 = width;
    templ.height0 = height;
    templ.depth0 = 1;
    templ.array_size = 1;
    templ.bind = PIPE_BIND_SAMPLER_VIEW;
    templ.usage = PIPE_USAGE_DEFAULT;
    winsys_handle whandle = {};
    whandle.type = WINSYS_HANDLE_TYPE_FD;
    whandle.handle = static_cast<unsigned>(fd);
    whandle.stride = stride * 4;
    pipe_resource* imported = mScreen->resource_from_handle
            ? mScreen->resource_from_handle(mScreen, &templ, &whandle, 0)
            : nullptr;
    if (!imported) {
        ALOGW("encodeGraphic resource_from_handle failed fd=%d", fd);
        return -EIO;
    }
    pipe_video_buffer* previousSource = mSource;
    pipe_video_buffer* externalSource = mCreateExternalBuffer(
            mContext, imported, PIPE_FORMAT_R8G8B8A8_UNORM, width, height);
    if (!externalSource) {
        ALOGW("encodeGraphic external buffer creation failed");
        pipe_resource_reference(&imported, nullptr);
        return -EIO;
    }
    mSource = externalSource;
    mSourceExternal = true;
    int rc = encode(nullptr, 0, ptsUs, keyframe, outBs, outPtsUs, outEos);
    mSourceExternal = false;
    mSource->destroy(mSource);
    mSource = previousSource;
    return rc;
}

// The host encoder completes asynchronously and writes the coded data, then
// the feedback, into guest memory; poll the feedback once the submission's
// fence has signalled.  get_feedback() reports 0 bytes until then.
unsigned Session::pollFeedback(void* feedback, std::unique_lock<std::mutex>* lock) {
    constexpr int kTimeoutUs = 500000;
    unsigned size = 0;
    for (int waited = 0; mCodec && mCodec->get_feedback && feedback; waited += 200) {
        mCodec->get_feedback(mCodec, feedback, &size, nullptr);
        if (size || waited >= kTimeoutUs) break;
        if (lock) lock->unlock();
        usleep(200);
        if (lock) lock->lock();
    }
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
    return size;
}

int Session::submitHost(int fd, uint32_t width, uint32_t height, uint32_t format,
                        int64_t ptsUs, bool keyframe, EncodeTicket* ticket) {
    const uint32_t resource = resourceIdFromFd(fd);
    if (!resource) return -EBADF;
    VencFrame frame = {mHostSession, mHostIndex++, ptsUs, resource, width, height, format,
                       keyframe ? kVencFlagKey : 0u};
    if (mHostFd < 0 || !vencSend(mHostFd, kVencFrame, &frame, sizeof(frame))) return -EIO;
    *ticket = EncodeTicket{};
    ticket->ptsUs = ptsUs;
    ticket->index = frame.index;
    ++mSubmitted;
    return 0;
}

int Session::submitGraphic(int fd, uint32_t width, uint32_t height, uint32_t stride,
                           uint32_t format, int64_t ptsUs, bool keyframe, EncodeTicket* ticket) {
    if (!ticket) return -EINVAL;
    if (mHost) return submitHost(fd, width, height, format, ptsUs, keyframe, ticket);
    std::lock_guard<std::mutex> lock(mPipeLock);
    *ticket = EncodeTicket{};
    mTicketOut = ticket;
    std::vector<uint8_t> unused;
    // The host reads the imported surface while processing the encode
    // command, so encodeGraphic() may release it right after submission.
    const int rc = encodeGraphic(fd, width, height, stride, format, ptsUs, keyframe, &unused,
                                 nullptr, nullptr);
    mTicketOut = nullptr;
    if (rc < 0 && ticket->fence) mScreen->fence_reference(mScreen, &ticket->fence, nullptr);
    return rc;
}

int Session::collect(EncodeTicket* ticket, std::vector<uint8_t>* outBs) {
    if (mHost) {
        if (!ticket || !outBs) return -EINVAL;
        outBs->clear();
        // Replies come one per frame, in submission order per session.
        std::unique_lock<std::mutex> lock(gVencMu);
        for (;;) {
            auto it = gVencReplies.find(mHostSession);
            if (it == gVencReplies.end()) return -EIO;  // closed meanwhile
            if (it->second.empty()) {
                if (!gVencCv.wait_for(lock, std::chrono::seconds(5),
                                      [&] {
                                          auto q = gVencReplies.find(mHostSession);
                                          return q == gVencReplies.end() || !q->second.empty();
                                      }))
                    return -ETIMEDOUT;
                continue;
            }
            VencReply reply = std::move(it->second.front());
            it->second.pop_front();
            if (reply.index < ticket->index) continue;  // reply to an abandoned frame
            if (reply.status < 0) return reply.status;
            ++mFrameIndex;
            *outBs = std::move(reply.data);
            return static_cast<int>(outBs->size());
        }
    }
    if (!ticket || !outBs || !ticket->bitstream) return -EINVAL;
    outBs->clear();
    // Wait outside the lock so the submit thread can queue the next frame.
    if (ticket->fence && mScreen)
        mScreen->fence_finish(mScreen, nullptr, ticket->fence, ~0ull);
    std::unique_lock<std::mutex> lock(mPipeLock);
    if (ticket->fence) mScreen->fence_reference(mScreen, &ticket->fence, nullptr);
    if (!mCodec || !mContext) return -ENODEV;
    const unsigned size = pollFeedback(ticket->feedback, &lock);
    if (size == 0 || size > ticket->bitstream->width0) {
        ALOGW("encode feedback invalid size=%u capacity=%u frame=%llu", size,
              ticket->bitstream->width0, static_cast<unsigned long long>(ticket->index));
        return -EIO;
    }
    ++mFrameIndex;
    outBs->resize(size);
    pipe_buffer_read(mContext, ticket->bitstream, 0, size, outBs->data());
    return static_cast<int>(size);
}

int Session::drain(std::vector<uint8_t>* out, int64_t* outPtsUs, bool* outEos) {
    if (out) out->clear();
    if (outPtsUs) *outPtsUs = 0;
    if (outEos) *outEos = true;
    return 0;
}

}  // namespace mesa_video
}  // namespace gki
