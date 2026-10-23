/*
 * Soft graphics HAL stub for GKI+QEMU bring-up (NDK AIDL).
 * Registers IComposer (composer3 v3) + IAllocator (v2).
 * Forces client composition; vsync + hotplug unblocks SurfaceFlinger.
 */

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/log.h>

#include <aidl/android/hardware/common/NativeHandle.h>
#include <aidl/android/hardware/graphics/allocator/BnAllocator.h>
#include <aidl/android/hardware/graphics/allocator/BufferDescriptorInfo.h>
#include <aidl/android/hardware/graphics/common/BufferUsage.h>
#include <aidl/android/hardware/graphics/common/DisplayHotplugEvent.h>
#include <aidl/android/hardware/graphics/common/PixelFormat.h>
#include <aidl/android/hardware/graphics/common/Transform.h>
#include <aidl/android/hardware/graphics/composer3/BnComposer.h>
#include <aidl/android/hardware/graphics/composer3/BnComposerClient.h>
#include <aidl/android/hardware/graphics/composer3/Capability.h>
#include <aidl/android/hardware/graphics/composer3/ChangedCompositionLayer.h>
#include <aidl/android/hardware/graphics/composer3/ChangedCompositionTypes.h>
#include <aidl/android/hardware/graphics/composer3/ClockMonotonicTimestamp.h>
#include <aidl/android/hardware/graphics/composer3/ColorMode.h>
#include <aidl/android/hardware/graphics/composer3/CommandResultPayload.h>
#include <aidl/android/hardware/graphics/composer3/Composition.h>
#include <aidl/android/hardware/graphics/composer3/ContentType.h>
#include <aidl/android/hardware/graphics/composer3/DisplayAttribute.h>
#include <aidl/android/hardware/graphics/composer3/DisplayCapability.h>
#include <aidl/android/hardware/graphics/composer3/DisplayCommand.h>
#include <aidl/android/hardware/graphics/composer3/DisplayConfiguration.h>
#include <aidl/android/hardware/graphics/composer3/DisplayConnectionType.h>
#include <aidl/android/hardware/graphics/composer3/DisplayIdentification.h>
#include <aidl/android/hardware/graphics/composer3/HdrCapabilities.h>
#include <aidl/android/hardware/graphics/composer3/IComposerCallback.h>
#include <aidl/android/hardware/graphics/composer3/OutputType.h>
#include <aidl/android/hardware/graphics/composer3/OverlayProperties.h>
#include <aidl/android/hardware/graphics/composer3/PowerMode.h>
#include <aidl/android/hardware/graphics/composer3/PresentFence.h>
#include <aidl/android/hardware/graphics/composer3/RenderIntent.h>
#include <aidl/android/hardware/graphics/composer3/ScreenPartStatus.h>
#include <aidl/android/hardware/graphics/composer3/VsyncPeriodChangeConstraints.h>
#include <aidl/android/hardware/graphics/composer3/VsyncPeriodChangeTimeline.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include <poll.h>
#include <linux/dma-buf.h>
#include <sys/stat.h>
#include "buffer_format.h"

#include <fcntl.h>
#include <errno.h>
#include <linux/ashmem.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/system_properties.h>

#include <drm/drm.h>
#include <drm/drm_mode.h>

#define LOG_TAG "graphics-stub"

#define ALOGI(...)                                                                 \
    do {                                                                           \
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__);               \
        fprintf(stderr, "graphics-stub I: ");                                      \
        fprintf(stderr, __VA_ARGS__);                                              \
        fprintf(stderr, "\n");                                                     \
    } while (0)
#define ALOGE(...)                                                                 \
    do {                                                                           \
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);              \
        fprintf(stderr, "graphics-stub E: ");                                      \
        fprintf(stderr, __VA_ARGS__);                                              \
        fprintf(stderr, "\n");                                                     \
    } while (0)

using namespace aidl::android::hardware::graphics::composer3;
using namespace aidl::android::hardware::graphics::allocator;
using aidl::android::hardware::common::NativeHandle;
using aidl::android::hardware::graphics::common::BufferUsage;
using aidl::android::hardware::graphics::common::DisplayHotplugEvent;
using aidl::android::hardware::graphics::common::PixelFormat;
using aidl::android::hardware::graphics::common::Transform;
using ndk::ScopedAStatus;
using ndk::ScopedFileDescriptor;
using ndk::SharedRefBase;

namespace {

constexpr int64_t kPrimaryDisplay = 0;
constexpr int32_t kConfigId = 0;
constexpr int32_t kWidth = 1080;
constexpr int32_t kHeight = 2400;
// Display refresh rate from androidboot.tebox.refresh_hz (default 60, 30..240).
// The vsync rate caps every app's frame rate (benchmarks render on screen), so
// boot-qemu.sh raises it where the host GPU has headroom.
static int32_t teboxVsyncPeriodNs() {
    char value[PROP_VALUE_MAX] = {};
    int hz = __system_property_get("ro.boot.tebox.refresh_hz", value) > 0 ? atoi(value) : 0;
    if (hz < 30 || hz > 240) hz = 60;
    return static_cast<int32_t>(1000000000LL / hz);
}
static const int32_t kVsyncPeriodNs = teboxVsyncPeriodNs();
constexpr int32_t kDpiX = 420000;
constexpr int32_t kDpiY = 420000;

int ashmemCreate(size_t size) {
    int fd = open("/dev/ashmem", O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        // Fallback: memfd-style via anonymous mmap file on tmpfs
        char name[] = "/dev/ashmem-stub-XXXXXX";
        fd = mkstemp(name);
        if (fd < 0) return -1;
        unlink(name);
        if (ftruncate(fd, static_cast<off_t>(size)) != 0) {
            close(fd);
            return -1;
        }
        return fd;
    }
    char name[ASHMEM_NAME_LEN] = "graphics-stub";
    ioctl(fd, ASHMEM_SET_NAME, name);
    if (ioctl(fd, ASHMEM_SET_SIZE, size) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

// Prefer virtio-gpu dumb buffers (dma-buf) so Mesa/EGL can import as textures.
// Falls back to -1 if DRM is unavailable (caller uses ashmem).
int drmDumbCreate(uint32_t width, uint32_t height, uint32_t bpp_bits, uint32_t* out_stride_px,
                  uint32_t* out_size) {
    static int drm_fd = -2;  // -2 = uninit, -1 = failed
    if (drm_fd == -2) {
        drm_fd = -1;
        for (const char* path : {"/dev/dri/card0", "/dev/dri/renderD128"}) {
            int fd = open(path, O_RDWR | O_CLOEXEC);
            if (fd < 0) continue;
            drm_mode_create_dumb probe = {};
            probe.width = 64;
            probe.height = 64;
            probe.bpp = 32;
            if (ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &probe) != 0) {
                ALOGI("DRM %s: CREATE_DUMB unsupported (%d)", path, errno);
                close(fd);
                continue;
            }
            drm_mode_destroy_dumb destroy = {};
            destroy.handle = probe.handle;
            ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
            drm_fd = fd;
            ALOGI("DRM dumb buffers via %s", path);
            break;
        }
        if (drm_fd < 0) ALOGI("DRM dumb buffers unavailable; using ashmem");
    }
    if (drm_fd < 0) return -1;

    drm_mode_create_dumb creq = {};
    // virtio-gpu accepts 32-bit dumb buffers; allocate enough linear storage
    // for the requested format, including FP16, with an integral pixel stride.
    const uint32_t bytes = bpp_bits / 8;
    const uint32_t alignment = bytes == 8 ? 8 : 12;
    creq.width = ((width * bytes + alignment - 1) / alignment) * alignment / 4;
    creq.height = height;
    creq.bpp = 32;
    if (ioctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq) != 0) {
        ALOGE("CREATE_DUMB %ux%u bpp=%u failed errno=%d", width, height, bpp_bits, errno);
        return -1;
    }

    drm_prime_handle prime = {};
    prime.handle = creq.handle;
    prime.flags = DRM_CLOEXEC | DRM_RDWR;
    if (ioctl(drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime) != 0) {
        ALOGE("PRIME_HANDLE_TO_FD failed errno=%d", errno);
        drm_mode_destroy_dumb destroy = {};
        destroy.handle = creq.handle;
        ioctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
        return -1;
    }

    // Drop GEM handle; dma-buf fd keeps the backing store alive.
    drm_mode_destroy_dumb destroy = {};
    destroy.handle = creq.handle;
    ioctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);

    uint32_t bpp_bytes = bpp_bits / 8;
    if (bpp_bytes == 0) bpp_bytes = 4;
    *out_stride_px = creq.pitch / bpp_bytes;
    *out_size = static_cast<uint32_t>(creq.size);
    return prime.fd;
}

DisplayConfiguration primaryConfig() {
    DisplayConfiguration cfg;
    cfg.configId = kConfigId;
    cfg.width = kWidth;
    cfg.height = kHeight;
    cfg.configGroup = 0;
    cfg.vsyncPeriod = kVsyncPeriodNs;
    DisplayConfiguration::Dpi dpi;
    dpi.x = 160.f;
    dpi.y = 160.f;
    cfg.dpi = dpi;
    cfg.hdrOutputType = OutputType::SDR;
    return cfg;
}

#include "gpu_buffer.h"
#include "buffer_handle.h"
#include "kms_display.h"

class StubComposerClient : public BnComposerClient {
  public:
    explicit StubComposerClient(std::shared_ptr<KmsDisplay> display) : display_(std::move(display)) {}
    ~StubComposerClient() override { stopVsync(); }

    ScopedAStatus registerCallback(
            const std::shared_ptr<IComposerCallback>& in_callback) override {
        ALOGI("registerCallback");
        {
            std::lock_guard<std::mutex> lock(mu_);
            callback_ = in_callback;
        }
        if (in_callback) {
            // Critical: SF waits for hotplug before creating the primary display.
            auto st = in_callback->onHotplug(kPrimaryDisplay, true);
            if (!st.isOk()) ALOGE("onHotplug failed: %s", st.getDescription().c_str());
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus getDisplayConfigurations(int64_t display, int32_t,
                                           std::vector<DisplayConfiguration>* out) override {
        if (display != kPrimaryDisplay) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        out->clear();
        out->push_back(primaryConfig());
        return ScopedAStatus::ok();
    }

    ScopedAStatus getDisplayConfigs(int64_t display, std::vector<int32_t>* out) override {
        if (display != kPrimaryDisplay) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        *out = {kConfigId};
        return ScopedAStatus::ok();
    }

    ScopedAStatus getDisplayAttribute(int64_t display, int32_t config,
                                      DisplayAttribute attr, int32_t* out) override {
        if (display != kPrimaryDisplay || config != kConfigId) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        switch (attr) {
            case DisplayAttribute::WIDTH:
                *out = kWidth;
                break;
            case DisplayAttribute::HEIGHT:
                *out = kHeight;
                break;
            case DisplayAttribute::VSYNC_PERIOD:
                *out = kVsyncPeriodNs;
                break;
            case DisplayAttribute::DPI_X:
                *out = kDpiX;
                break;
            case DisplayAttribute::DPI_Y:
                *out = kDpiY;
                break;
            case DisplayAttribute::CONFIG_GROUP:
                *out = 0;
                break;
            default:
                return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus getActiveConfig(int64_t display, int32_t* out) override {
        if (display != kPrimaryDisplay) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        *out = active_config_;
        return ScopedAStatus::ok();
    }

    ScopedAStatus setActiveConfig(int64_t display, int32_t config) override {
        if (display != kPrimaryDisplay || config != kConfigId) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        active_config_ = config;
        return ScopedAStatus::ok();
    }

    ScopedAStatus setActiveConfigWithConstraints(
            int64_t display, int32_t config, const VsyncPeriodChangeConstraints&,
            VsyncPeriodChangeTimeline* timeline) override {
        auto st = setActiveConfig(display, config);
        if (!st.isOk()) return st;
        timeline->newVsyncAppliedTimeNanos = 0;
        timeline->refreshRequired = false;
        timeline->refreshTimeNanos = 0;
        return ScopedAStatus::ok();
    }

    ScopedAStatus createLayer(int64_t display, int32_t, int64_t* out) override {
        if (display != kPrimaryDisplay) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        *out = next_layer_++;
        layers_[*out] = Composition::DEVICE;
        return ScopedAStatus::ok();
    }

    ScopedAStatus destroyLayer(int64_t display, int64_t layer) override {
        if (display != kPrimaryDisplay) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        layers_.erase(layer);
        return ScopedAStatus::ok();
    }

    ScopedAStatus createVirtualDisplay(int32_t, int32_t, PixelFormat, int32_t,
                                       VirtualDisplay*) override {
        return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
    }

    ScopedAStatus destroyVirtualDisplay(int64_t) override {
        return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
    }

    ScopedAStatus getMaxVirtualDisplayCount(int32_t* out) override {
        *out = 0;
        return ScopedAStatus::ok();
    }

    ScopedAStatus executeCommands(const std::vector<DisplayCommand>& commands,
                                  std::vector<CommandResultPayload>* results) override {
        results->clear();
        for (size_t index = 0; index < commands.size(); ++index) {
            const auto& cmd = commands[index];
            auto error = [&](int code) {
                CommandError e;
                e.commandIndex = static_cast<int>(index);
                e.errorCode = code;
                results->emplace_back(CommandResultPayload::make<CommandResultPayload::Tag::error>(e));
            };
            if (cmd.display != kPrimaryDisplay) {
                error(IComposerClient::EX_BAD_DISPLAY);
                continue;
            }
            for (const auto& layer : cmd.layers) {
                if (!layers_.count(layer.layer)) { error(IComposerClient::EX_BAD_LAYER); continue; }
                if (layer.composition) layers_[layer.layer] = layer.composition->composition;
            }
            if (cmd.clientTarget && !display_->setTarget(cmd.clientTarget->buffer)) {
                error(IComposerClient::EX_BAD_PARAMETER);
                continue;
            }
            if (cmd.validateDisplay || cmd.presentOrValidateDisplay) {
                ChangedCompositionTypes changed;
                changed.display = cmd.display;
                for (auto& [id, composition] : layers_) {
                    if (composition == Composition::CLIENT) continue;
                    ChangedCompositionLayer layer;
                    layer.layer = id;
                    layer.composition = Composition::CLIENT;
                    changed.layers.push_back(layer);
                    composition = Composition::CLIENT;
                }
                if (!changed.layers.empty())
                    results->emplace_back(CommandResultPayload::make<
                        CommandResultPayload::Tag::changedCompositionTypes>(std::move(changed)));
                if (cmd.presentOrValidateDisplay) {
                    PresentOrValidate result;
                    result.display = cmd.display;
                    result.result = PresentOrValidate::Result::Validated;
                    results->emplace_back(CommandResultPayload::make<
                        CommandResultPayload::Tag::presentOrValidateResult>(result));
                }
            }
            if (cmd.presentDisplay) {
                if (!display_->present()) {
                    error(IComposerClient::EX_NO_RESOURCES);
                    continue;
                }
                // Synchronous copy is complete. Omit the payload for NO_FENCE:
                // AIDL PresentFence.fence is non-nullable and cannot encode -1.

            }
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus setPowerMode(int64_t display, PowerMode mode) override {
        if (display != kPrimaryDisplay) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        ALOGI("setPowerMode %d", static_cast<int>(mode));
        power_mode_ = mode;
        return ScopedAStatus::ok();
    }

    ScopedAStatus setVsyncEnabled(int64_t display, bool enabled) override {
        if (display != kPrimaryDisplay) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        ALOGI("setVsyncEnabled %d", enabled ? 1 : 0);
        if (enabled) startVsync();
        else stopVsync();
        return ScopedAStatus::ok();
    }

    ScopedAStatus getDisplayVsyncPeriod(int64_t display, int32_t* out) override {
        if (display != kPrimaryDisplay) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        *out = kVsyncPeriodNs;
        return ScopedAStatus::ok();
    }

    ScopedAStatus getColorModes(int64_t display, std::vector<ColorMode>* out) override {
        if (display != kPrimaryDisplay) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        *out = {ColorMode::NATIVE};
        return ScopedAStatus::ok();
    }

    ScopedAStatus setColorMode(int64_t display, ColorMode, RenderIntent) override {
        if (display != kPrimaryDisplay) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus getRenderIntents(int64_t display, ColorMode,
                                   std::vector<RenderIntent>* out) override {
        if (display != kPrimaryDisplay) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        *out = {RenderIntent::COLORIMETRIC};
        return ScopedAStatus::ok();
    }

    ScopedAStatus getDisplayName(int64_t display, std::string* out) override {
        if (display != kPrimaryDisplay) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        *out = "qemu-virtio-stub";
        return ScopedAStatus::ok();
    }

    ScopedAStatus getDisplayConnectionType(int64_t display,
                                           DisplayConnectionType* out) override {
        if (display != kPrimaryDisplay) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        *out = DisplayConnectionType::INTERNAL;
        return ScopedAStatus::ok();
    }

    ScopedAStatus getDisplayCapabilities(int64_t display,
                                         std::vector<DisplayCapability>* out) override {
        if (display != kPrimaryDisplay) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        out->clear();
        return ScopedAStatus::ok();
    }

    ScopedAStatus getHdrCapabilities(int64_t display, HdrCapabilities* out) override {
        if (display != kPrimaryDisplay) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        *out = HdrCapabilities{};
        return ScopedAStatus::ok();
    }

    ScopedAStatus getSupportedContentTypes(int64_t display,
                                           std::vector<ContentType>* out) override {
        if (display != kPrimaryDisplay) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        out->clear();
        return ScopedAStatus::ok();
    }

    ScopedAStatus getDisplayPhysicalOrientation(int64_t display, Transform* out) override {
        if (display != kPrimaryDisplay) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        *out = Transform::NONE;
        return ScopedAStatus::ok();
    }

    ScopedAStatus getDisplayIdentificationData(int64_t display,
                                               DisplayIdentification* out) override {
        if (display != kPrimaryDisplay) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        out->port = 0;
        // Minimal EDID-ish blob (128 bytes of zeros is rejected; put a tiny marker).
        out->data.assign(128, 0);
        out->data[0] = 0x00;
        out->data[1] = 0xff;
        out->data[2] = 0xff;
        out->data[3] = 0xff;
        out->data[4] = 0xff;
        out->data[5] = 0xff;
        out->data[6] = 0xff;
        out->data[7] = 0x00;
        out->screenPartStatus = ScreenPartStatus::UNSUPPORTED;
        return ScopedAStatus::ok();
    }

    ScopedAStatus getDataspaceSaturationMatrix(
            ::aidl::android::hardware::graphics::common::Dataspace,
            std::vector<float>* out) override {
        // Identity 4x4
        *out = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        return ScopedAStatus::ok();
    }

    ScopedAStatus setClientTargetSlotCount(int64_t display, int32_t) override {
        if (display != kPrimaryDisplay) {
            return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus setBootDisplayConfig(int64_t display, int32_t config) override {
        return setActiveConfig(display, config);
    }

    ScopedAStatus clearBootDisplayConfig(int64_t) override { return ScopedAStatus::ok(); }

    ScopedAStatus getPreferredBootDisplayConfig(int64_t display, int32_t* out) override {
        return getActiveConfig(display, out);
    }

    ScopedAStatus notifyExpectedPresent(int64_t, const ClockMonotonicTimestamp&,
                                        int32_t) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus getOverlaySupport(OverlayProperties* out) override {
        *out = OverlayProperties{};
        return ScopedAStatus::ok();
    }

    ScopedAStatus getDisplayDecorationSupport(
            int64_t,
            std::optional<::aidl::android::hardware::graphics::common::DisplayDecorationSupport>*
                    out) override {
        out->reset();
        return ScopedAStatus::ok();
    }

    ScopedAStatus getPerFrameMetadataKeys(int64_t, std::vector<PerFrameMetadataKey>* out)
            override {
        out->clear();
        return ScopedAStatus::ok();
    }

    ScopedAStatus getMaxLayerPictureProfiles(int64_t, int32_t* out) override {
        *out = 0;
        return ScopedAStatus::ok();
    }

#include "composer_client_defaults.inc"

  private:
    void startVsync() {
        bool expected = false;
        if (!vsync_running_.compare_exchange_strong(expected, true)) return;
        vsync_thread_ = std::thread([this] {
            // A fixed CLOCK_MONOTONIC timeline: each vsync fires at an absolute
            // deadline and reports that ideal time. Relative sleeps drifted
            // (every period ran long by the wakeup latency) and leaked that
            // jitter into the timestamps, so SurfaceFlinger's vsync predictor
            // mis-scheduled frames ("Frame time is in the future").
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            int64_t next = int64_t(now.tv_sec) * 1000000000LL + now.tv_nsec + kVsyncPeriodNs;
            while (vsync_running_.load()) {
                struct timespec deadline = {static_cast<time_t>(next / 1000000000LL),
                                            static_cast<long>(next % 1000000000LL)};
                while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, nullptr) == EINTR) {
                }
                const int64_t ts = next;
                next += kVsyncPeriodNs;
                clock_gettime(CLOCK_MONOTONIC, &now);
                const int64_t nowNs = int64_t(now.tv_sec) * 1000000000LL + now.tv_nsec;
                if (nowNs > next) {  // stalled (suspend, heavy load): skip missed vsyncs
                    next += ((nowNs - next) / kVsyncPeriodNs + 1) * kVsyncPeriodNs;
                }
                std::shared_ptr<IComposerCallback> cb;
                {
                    std::lock_guard<std::mutex> lock(mu_);
                    cb = callback_;
                }
                if (cb) cb->onVsync(kPrimaryDisplay, ts, kVsyncPeriodNs);
            }
        });
    }

    void stopVsync() {
        vsync_running_.store(false);
        if (vsync_thread_.joinable()) vsync_thread_.join();
    }

    std::mutex mu_;
    std::shared_ptr<IComposerCallback> callback_;
    std::atomic<bool> vsync_running_{false};
    std::thread vsync_thread_;
    int32_t active_config_ = kConfigId;
    int64_t next_layer_ = 1;
    std::unordered_map<int64_t, Composition> layers_;
    std::shared_ptr<KmsDisplay> display_;
    PowerMode power_mode_ = PowerMode::OFF;
};

class StubComposer : public BnComposer {
    std::shared_ptr<KmsDisplay> display_ = std::make_shared<KmsDisplay>();
  public:
    StubComposer() { display_->initialize(); }
    ScopedAStatus createClient(std::shared_ptr<IComposerClient>* out) override {
        ALOGI("createClient");
        *out = SharedRefBase::make<StubComposerClient>(display_);
        return ScopedAStatus::ok();
    }

    ScopedAStatus getCapabilities(std::vector<Capability>* out) override {
        *out = {Capability::PRESENT_FENCE_IS_NOT_RELIABLE};
        return ScopedAStatus::ok();
    }
};

// Preserve the operational allocator's typed GBM/VirGL RGB path. Its current
// ELF uses GBM_BO_USE_RENDERING | GBM_BO_USE_LINEAR (0x14), exports a dma-buf,
// and destroys the BO only after obtaining that independent fd.
#include "rgb_storage_layout.h"
static int gbmCreate(uint32_t width,uint32_t height,int format,bool cpuWrite,uint32_t* stride,uint32_t* size) {
    static std::mutex mutex;std::lock_guard lock(mutex);
    pthread_once(&qemu_gbm_once,qemuInitGbm);
    const auto info=bufferFormat(format);
    if(!qemu_gbm_device || !info.bytes || format==0x21)return -1;
    const int storageWidth=teboxRgbStorageWidth(format,width,teboxRgbRowAlignment());
    if(!storageWidth)return -1;
    gbm_bo* bo=gbm_bo_create(qemu_gbm_device,storageWidth,height,info.fourcc,
                           GBM_BO_USE_RENDERING | GBM_BO_USE_LINEAR);
    if(!bo)return -1;
    uint32_t pitch=gbm_bo_get_stride(bo);int fd=-1;
    if(cpuWrite){
        // Mesa's VirGL GBM export is read-only. CPU_WRITE consumers need a
        // writable descriptor to this same typed BO, never anonymous storage.
        drm_prime_handle prime{};prime.handle=gbm_bo_get_handle(bo).u32;
        prime.flags=DRM_CLOEXEC|DRM_RDWR;prime.fd=-1;
        // Export writable on the FIRST export: DMA-BUF caches its file and
        // later exports cannot upgrade a previously read-only file's mode.
        if(!ioctl(gbm_device_get_fd(qemu_gbm_device),DRM_IOCTL_PRIME_HANDLE_TO_FD,&prime))fd=prime.fd;
    }else fd=gbm_bo_get_fd(bo);
    uint64_t length=uint64_t(pitch)*height;
    if(fd>=0 && pitch>=uint32_t(storageWidth)*info.bytes && pitch%info.bytes==0 &&
       pitch/info.bytes<=16384 && length<=UINT32_MAX &&
       (teboxRgbRowAlignment()!=64 || pitch==uint32_t(storageWidth)*info.bytes)){
        *stride=pitch/info.bytes;*size=uint32_t(length);
    }else{if(fd>=0)close(fd);fd=-1;}
    gbm_bo_destroy(bo);return fd;
}

class StubAllocator : public BnAllocator {
  public:
    ScopedAStatus allocate(const std::vector<uint8_t>&, int32_t,
                           AllocationResult*) override {
        return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    ScopedAStatus allocate2(const BufferDescriptorInfo& desc, int32_t count,
                            AllocationResult* out) override {
        if (count <= 0 || desc.width <= 0 || desc.height <= 0) {
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
        BufferDescriptorInfo effective = desc;
        if (effective.format == PixelFormat::IMPLEMENTATION_DEFINED)
            effective.format = PixelFormat::RGBA_8888;
        bool supported = false;
        isSupported(effective, &supported);
        if (!supported) return ScopedAStatus::fromServiceSpecificError(1);
        const int32_t bpp = bufferFormat(static_cast<int>(effective.format)).bytes;
        int32_t stride = effective.width;  // pixels (logical)
        size_t size = static_cast<size_t>(stride) * static_cast<size_t>(effective.height) *
                      static_cast<size_t>(bpp);
        if (static_cast<int>(effective.format) == 35)
            size = size * 3 / 2;
        if (effective.layerCount > 1) size *= static_cast<size_t>(effective.layerCount);

        out->stride = stride;
        out->buffers.clear();
        out->buffers.reserve(static_cast<size_t>(count));
        const char* backend = "ashmem";
        for (int32_t i = 0; i < count; ++i) {
            int fd = -1;
            uint32_t drm_stride = 0, drm_size = 0;
            const bool cpuOnly = static_cast<int>(effective.format) == 35 ||
                                 static_cast<int>(effective.format) == 0x21;
            const bool gfx = qemuGpuIsGfxstream() && !cpuOnly &&
                             virglFormat(static_cast<int>(effective.format));
            const bool virgl = !gfx && qemuGbmUsesVirgl() && !cpuOnly;
            if (gfx) {
                fd = virtgpuCreate(effective.width, effective.height,
                                   static_cast<int>(effective.format), bpp, &drm_stride, &drm_size);
                if (fd < 0) return ScopedAStatus::fromServiceSpecificError(1);
                drm_stride /= bpp;  // pixels, like the other backends
                backend = "virtgpu-gfxstream";
            } else if (virgl) {
                // CPU writers: CPU_WRITE_* (0xf0) or a camera HAL (CAMERA_OUTPUT).
                fd = gbmCreate(effective.width, effective.height, static_cast<int>(effective.format),
                               (static_cast<int64_t>(effective.usage) & (0xf0 | (1LL << 17))) != 0,
                               &drm_stride, &drm_size);
                if (fd < 0) return ScopedAStatus::fromServiceSpecificError(1);
                backend = "gbm-virgl";
            } else if (!cpuOnly) {
                fd = drmDumbCreate(static_cast<uint32_t>(effective.width),
                                   static_cast<uint32_t>(effective.height),
                                   static_cast<uint32_t>(bpp * 8), &drm_stride, &drm_size);
            }
            if (fd >= 0) {
                stride = static_cast<int32_t>(drm_stride);
                size = drm_size;
                if (effective.layerCount > 1) size *= static_cast<size_t>(effective.layerCount);
                if (!virgl && !gfx) backend = "drm-dumb";
            }
            if (fd < 0) {
                fd = ashmemCreate(size);
                if (fd < 0) {
                    ALOGE("ashmemCreate failed size=%zu", size);
                    return ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
                }
                backend = "ashmem";
            }
            NativeHandle h;
            h.fds.emplace_back(ndk::ScopedFileDescriptor(fd));
            if (gfx) {
                // minigbm layout: the gfxstream guest Vulkan driver reads it.
                tebox_cros_handle cros = {};
                cros.fds[0] = fd;
                for (int p = 1; p < TEBOX_CROS_MAX_FDS; ++p) cros.fds[p] = -1;
                cros.strides[0] = static_cast<uint32_t>(stride * bpp);
                cros.sizes[0] = static_cast<uint32_t>(size);
                cros.width = static_cast<uint32_t>(effective.width);
                cros.height = static_cast<uint32_t>(effective.height);
                cros.format = bufferFormat(static_cast<int>(effective.format)).fourcc;
                cros.use_flags = static_cast<uint64_t>(effective.usage);
                cros.magic = TEBOX_CROS_MAGIC;
                cros.pixel_stride = static_cast<uint32_t>(stride);
                cros.droid_format = static_cast<int32_t>(effective.format);
                cros.usage = static_cast<int32_t>(static_cast<int64_t>(effective.usage));
                cros.num_planes = 1;
                cros.total_size = size;
                const int* ints = reinterpret_cast<const int*>(&cros.fds[1]);
                h.ints.assign(ints, ints + TEBOX_CROS_NUM_INTS);
                out->buffers.push_back(std::move(h));
                continue;
            }
            // ints: width, height, stride, format, usage_lo, usage_hi (gralloc-ish)
            h.ints = {effective.width, effective.height, stride,
                      static_cast<int32_t>(effective.format),
                      static_cast<int32_t>(static_cast<int64_t>(effective.usage) & 0xffffffff),
                      static_cast<int32_t>((static_cast<int64_t>(effective.usage) >> 32) & 0xffffffff)};
            if (virgl && teboxRgbRowAlignment() == 64 && stride > effective.width)
                h.ints.push_back(kTeBoxPaddedRgb);
            out->buffers.push_back(std::move(h));
        }
        out->stride = stride;
        ALOGI("allocate2 %dx%d fmt=%d count=%d size=%zu via %s", effective.width, effective.height,
              static_cast<int>(effective.format), count, size, backend);
        return ScopedAStatus::ok();
    }

    ScopedAStatus isSupported(const BufferDescriptorInfo& desc, bool* out) override {
        const bool fmt_ok = bufferFormat(static_cast<int>(desc.format)).bytes != 0 ||
                            desc.format == PixelFormat::IMPLEMENTATION_DEFINED;
        // BLOB buffers are width bytes x 1 row (camera JPEG: several MB).
        const bool blob = static_cast<int>(desc.format) == 0x21;
        const int32_t maxWidth = blob ? (64 << 20) : 16384;
        *out = fmt_ok && desc.width > 0 && desc.height > 0 &&
               desc.width <= maxWidth && desc.height <= (blob ? 1 : 16384) && desc.layerCount == 1 &&
               !(static_cast<int64_t>(desc.usage) & (1LL << 14)) &&
               desc.additionalOptions.empty();
        return ScopedAStatus::ok();
    }

    ScopedAStatus getIMapperLibrarySuffix(std::string* out) override {
        ALOGI("getIMapperLibrarySuffix -> stub");
        *out = "stub";
        return ScopedAStatus::ok();
    }
};

bool registerSvc(const char* instance, const std::shared_ptr<ndk::ICInterface>& svc) {
    binder_status_t st = AServiceManager_addService(svc->asBinder().get(), instance);
    if (st != STATUS_OK) {
        ALOGE("addService %s failed: %d", instance, st);
        return false;
    }
    ALOGI("registered %s", instance);
    return true;
}

}  // namespace

int main() {
    char re_backend[PROP_VALUE_MAX] = "(unset)";
    char re_vk[PROP_VALUE_MAX] = "(unset)";
    __system_property_get("debug.renderengine.backend", re_backend);
    __system_property_get("debug.renderengine.vulkan", re_vk);
    ALOGI("props: debug.renderengine.backend=%s vulkan=%s", re_backend, re_vk);
    ALOGI("starting soft graphics HAL (composer3 + allocator)");
    ABinderProcess_setThreadPoolMaxThreadCount(4);
    ABinderProcess_startThreadPool();

    auto composer = SharedRefBase::make<StubComposer>();
    auto allocator = SharedRefBase::make<StubAllocator>();

    const char* composerInst = "android.hardware.graphics.composer3.IComposer/default";
    const char* allocInst = "android.hardware.graphics.allocator.IAllocator/default";

    if (!registerSvc(composerInst, composer)) return 1;
    if (!registerSvc(allocInst, allocator)) return 1;

    ABinderProcess_joinThreadPool();
    return 0;
}
