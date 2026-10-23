// QEMU camera provider (AIDL, LEGACY hardware level, one back camera).
//
// Frames come from the virtio-serial port "tebox.camera": tebox-cast writes
// NV12 frames that cast viewers upload (protocol in
// src/service/cast/include/cast/protocol.h). Without a viewer camera the HAL
// shows a moving test pattern. A paced worker fills RGBA preview buffers,
// NV12 (YUV_420_888) buffers and JPEG (BLOB) buffers at up to 30 fps.
#include <aidl/android/hardware/camera/provider/BnCameraProvider.h>
#include <aidl/android/hardware/camera/device/BnCameraDevice.h>
#include <aidl/android/hardware/camera/device/BnCameraDeviceSession.h>
#include <aidl/android/hardware/camera/device/CameraBlob.h>
#include <aidl/android/hardware/camera/device/HalStream.h>
#include <aidl/android/hardware/camera/device/CaptureResult.h>
#include <aidl/android/hardware/camera/device/BufferStatus.h>
#include <aidl/android/hardware/camera/device/ErrorMsg.h>
#include <aidl/android/hardware/camera/device/NotifyMsg.h>
#include <aidl/android/hardware/camera/device/ShutterMsg.h>
#include <aidl/android/hardware/camera/device/StreamType.h>
#include <aidl/android/hardware/graphics/common/BufferUsage.h>
#include <aidl/android/hardware/graphics/common/PixelFormat.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/log.h>
#include <fmq/AidlMessageQueue.h>
#include <system/camera_metadata.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <dirent.h>
#include <drm/drm.h>
#include <drm/virtgpu_drm.h>
#include <fcntl.h>
#include <fstream>
#include <map>
#include <mutex>
#include <poll.h>
#include <sys/ioctl.h>
#include <string>
#include <sys/mman.h>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <vector>

#include "jpeg_encoder.h"

using aidl::android::hardware::camera::provider::BnCameraProvider;
using aidl::android::hardware::camera::provider::CameraIdAndStreamCombination;
using aidl::android::hardware::camera::provider::ConcurrentCameraIdCombination;
using aidl::android::hardware::camera::provider::ICameraProviderCallback;
using aidl::android::hardware::camera::common::VendorTagSection;
using aidl::android::hardware::camera::device::BnCameraDeviceSession;
using aidl::android::hardware::camera::device::BufferStatus;
using aidl::android::hardware::camera::device::CameraBlob;
using aidl::android::hardware::camera::device::CameraBlobId;
using aidl::android::hardware::camera::device::CameraMetadata;
using aidl::android::hardware::camera::device::CaptureRequest;
using aidl::android::hardware::camera::device::CaptureResult;
using aidl::android::hardware::camera::device::ErrorCode;
using aidl::android::hardware::camera::device::ErrorMsg;
using aidl::android::hardware::camera::device::HalStream;
using aidl::android::hardware::camera::device::ICameraDevice;
using aidl::android::hardware::camera::device::ICameraDeviceCallback;
using aidl::android::hardware::camera::device::ICameraDeviceSession;
using aidl::android::hardware::camera::device::NotifyMsg;
using aidl::android::hardware::camera::device::RequestTemplate;
using aidl::android::hardware::camera::device::ShutterMsg;
using aidl::android::hardware::camera::device::StreamBuffer;
using aidl::android::hardware::camera::device::StreamConfiguration;
using aidl::android::hardware::camera::common::CameraResourceCost;
using aidl::android::hardware::common::NativeHandle;
using aidl::android::hardware::common::fmq::MQDescriptor;
using aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using aidl::android::hardware::graphics::common::BufferUsage;
using aidl::android::hardware::graphics::common::PixelFormat;
using CameraQueue = android::AidlMessageQueue<int8_t, SynchronizedReadWrite>;
using Clock = std::chrono::steady_clock;

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "QemuCamera", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "QemuCamera", __VA_ARGS__)

namespace {

// Sensor = what tebox-cast delivers (CAST_CAMERA_WIDTH x HEIGHT, NV12).
constexpr int kSensorW = 1280, kSensorH = 720;
constexpr uint32_t kCameraMagic = 0x4d414354;  // 'TCAM'
constexpr int64_t kFrameNs = 33333333;         // 30 fps
constexpr int32_t kJpegMaxSize = kSensorW * kSensorH * 3 / 2 + int32_t(sizeof(CameraBlob));
constexpr int kFormatBlob = 33, kFormatYuv = 35, kFormatImpl = 34;
const int32_t kSizes[][2] = {{1280, 720}, {640, 480}, {320, 240}};

int64_t bootNs() {
    timespec ts{};
    clock_gettime(CLOCK_BOOTTIME, &ts);
    return int64_t(ts.tv_sec) * 1000000000 + ts.tv_nsec;
}

// ---------------------------------------------------------------------------
// camera_metadata helpers

class Meta {
  public:
    explicit Meta(size_t entries = 128, size_t data = 4096)
        : m_(allocate_camera_metadata(entries, data)) {}
    explicit Meta(const std::vector<uint8_t>& blob) {
        const auto* src = reinterpret_cast<const camera_metadata_t*>(blob.data());
        size_t size = blob.size();
        if (!blob.empty() && validate_camera_metadata_structure(src, &size) == 0)
            m_ = allocate_copy_camera_metadata_checked(src, blob.size());
        if (!m_) m_ = allocate_camera_metadata(64, 1024);
    }
    ~Meta() { if (m_) free_camera_metadata(m_); }
    Meta(const Meta&) = delete;
    Meta& operator=(const Meta&) = delete;
    Meta(Meta&& o) noexcept : m_(o.m_), tags_(std::move(o.tags_)) { o.m_ = nullptr; }

    template <class T>
    void set(uint32_t tag, std::initializer_list<T> values) {
        set(tag, values.begin(), values.size(), sizeof(T));
    }
    void set(uint32_t tag, const void* data, size_t count, size_t elem) {
        static const size_t kSize[] = {1, 4, 4, 8, 8, 8};  // byte int32 float int64 double rational
        int type = get_camera_metadata_tag_type(tag);
        if (type < 0 || type >= int(sizeof(kSize) / sizeof(kSize[0])) || kSize[type] != elem) {
            LOGE("metadata tag %#x: element size %zu does not match its type", tag, elem);
            return;
        }
        camera_metadata_entry_t e;
        int r;
        if (find_camera_metadata_entry(m_, tag, &e) == 0)
            r = update_camera_metadata_entry(m_, e.index, data, count, nullptr);
        else
            r = add_camera_metadata_entry(m_, tag, data, count);
        if (r != 0) {
            // Grow and retry once.
            camera_metadata_t* bigger = allocate_camera_metadata(
                get_camera_metadata_entry_capacity(m_) * 2,
                get_camera_metadata_data_capacity(m_) * 2 + count * elem);
            append_camera_metadata(bigger, m_);
            free_camera_metadata(m_);
            m_ = bigger;
            if (find_camera_metadata_entry(m_, tag, &e) == 0)
                r = update_camera_metadata_entry(m_, e.index, data, count, nullptr);
            else
                r = add_camera_metadata_entry(m_, tag, data, count);
            if (r != 0) LOGE("metadata tag %#x failed", tag);
        }
        tags_.push_back(int32_t(tag));
    }
    bool get(uint32_t tag, camera_metadata_ro_entry_t* e) const {
        return find_camera_metadata_ro_entry(m_, tag, e) == 0;
    }
    uint8_t u8(uint32_t tag, uint8_t fallback) const {
        camera_metadata_ro_entry_t e;
        return get(tag, &e) && e.count ? e.data.u8[0] : fallback;
    }
    int32_t i32(uint32_t tag, int32_t fallback) const {
        camera_metadata_ro_entry_t e;
        return get(tag, &e) && e.count ? e.data.i32[0] : fallback;
    }
    const std::vector<int32_t>& tags() const { return tags_; }
    CameraMetadata toAidl() {
        sort_camera_metadata(m_);
        CameraMetadata out;
        const auto* p = reinterpret_cast<const uint8_t*>(m_);
        out.metadata.assign(p, p + get_camera_metadata_size(m_));
        return out;
    }
    const camera_metadata_t* raw() const { return m_; }

  private:
    camera_metadata_t* m_ = nullptr;
    std::vector<int32_t> tags_;
};

// Keys a request may carry; results echo them plus the dynamic ones below.
const int32_t kRequestKeys[] = {
    ANDROID_COLOR_CORRECTION_ABERRATION_MODE, ANDROID_CONTROL_AE_ANTIBANDING_MODE,
    ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, ANDROID_CONTROL_AE_LOCK, ANDROID_CONTROL_AE_MODE,
    ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER, ANDROID_CONTROL_AE_TARGET_FPS_RANGE,
    ANDROID_CONTROL_AF_MODE, ANDROID_CONTROL_AF_TRIGGER, ANDROID_CONTROL_AWB_LOCK,
    ANDROID_CONTROL_AWB_MODE, ANDROID_CONTROL_CAPTURE_INTENT, ANDROID_CONTROL_EFFECT_MODE,
    ANDROID_CONTROL_MODE, ANDROID_CONTROL_SCENE_MODE, ANDROID_CONTROL_VIDEO_STABILIZATION_MODE,
    ANDROID_FLASH_MODE, ANDROID_JPEG_GPS_COORDINATES, ANDROID_JPEG_GPS_PROCESSING_METHOD,
    ANDROID_JPEG_GPS_TIMESTAMP, ANDROID_JPEG_ORIENTATION, ANDROID_JPEG_QUALITY,
    ANDROID_JPEG_THUMBNAIL_QUALITY, ANDROID_JPEG_THUMBNAIL_SIZE, ANDROID_LENS_FOCAL_LENGTH,
    ANDROID_LENS_OPTICAL_STABILIZATION_MODE, ANDROID_NOISE_REDUCTION_MODE,
    ANDROID_SCALER_CROP_REGION, ANDROID_STATISTICS_FACE_DETECT_MODE,
};
const int32_t kDynamicKeys[] = {
    ANDROID_CONTROL_AE_STATE, ANDROID_CONTROL_AF_STATE, ANDROID_CONTROL_AWB_STATE,
    ANDROID_FLASH_STATE, ANDROID_LENS_STATE, ANDROID_REQUEST_PIPELINE_DEPTH,
    ANDROID_SENSOR_TIMESTAMP, ANDROID_SENSOR_EXPOSURE_TIME, ANDROID_SENSOR_FRAME_DURATION,
    ANDROID_SENSOR_SENSITIVITY, ANDROID_STATISTICS_SCENE_FLICKER,
    ANDROID_SENSOR_ROLLING_SHUTTER_SKEW,
};

CameraMetadata makeCharacteristics() {
    Meta m(160, 8192);
    m.set<uint8_t>(ANDROID_LENS_FACING, {ANDROID_LENS_FACING_BACK});
    m.set<int32_t>(ANDROID_SENSOR_ORIENTATION, {90});
    m.set<int32_t>(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, {0, 0, kSensorW, kSensorH});
    m.set<int32_t>(ANDROID_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE, {0, 0, kSensorW, kSensorH});
    m.set<int32_t>(ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE, {kSensorW, kSensorH});
    m.set<float>(ANDROID_SENSOR_INFO_PHYSICAL_SIZE, {3.2f, 1.8f});
    m.set<uint8_t>(ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE, {ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME});
    m.set<int64_t>(ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE, {100000, 66666666});
    m.set<int64_t>(ANDROID_SENSOR_INFO_MAX_FRAME_DURATION, {66666666});
    m.set<int32_t>(ANDROID_SENSOR_INFO_SENSITIVITY_RANGE, {100, 800});
    m.set<int32_t>(ANDROID_SENSOR_AVAILABLE_TEST_PATTERN_MODES, {ANDROID_SENSOR_TEST_PATTERN_MODE_OFF});

    m.set<uint8_t>(ANDROID_REQUEST_AVAILABLE_CAPABILITIES,
                   {ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE});
    m.set<int32_t>(ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS, {0, 3, 1});
    m.set<int32_t>(ANDROID_REQUEST_MAX_NUM_INPUT_STREAMS, {0});
    m.set<int32_t>(ANDROID_REQUEST_PARTIAL_RESULT_COUNT, {1});
    m.set<uint8_t>(ANDROID_REQUEST_PIPELINE_MAX_DEPTH, {4});
    m.set<uint8_t>(ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL,
                   {ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY});
    m.set<int32_t>(ANDROID_SYNC_MAX_LATENCY, {ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL});

    std::vector<int32_t> configs;
    std::vector<int64_t> durations, stalls;
    for (int format : {kFormatImpl, kFormatYuv, kFormatBlob}) {
        for (const auto& s : kSizes) {
            configs.insert(configs.end(), {format, s[0], s[1],
                           ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT});
            durations.insert(durations.end(), {format, s[0], s[1], kFrameNs});
            stalls.insert(stalls.end(), {format, s[0], s[1], format == kFormatBlob ? kFrameNs : 0});
        }
    }
    m.set(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, configs.data(), configs.size(), 4);
    m.set(ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS, durations.data(), durations.size(), 8);
    m.set(ANDROID_SCALER_AVAILABLE_STALL_DURATIONS, stalls.data(), stalls.size(), 8);
    m.set<float>(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM, {1.0f});
    m.set<uint8_t>(ANDROID_SCALER_CROPPING_TYPE, {ANDROID_SCALER_CROPPING_TYPE_CENTER_ONLY});

    m.set<uint8_t>(ANDROID_CONTROL_AVAILABLE_MODES, {ANDROID_CONTROL_MODE_AUTO});
    m.set<uint8_t>(ANDROID_CONTROL_AE_AVAILABLE_MODES, {ANDROID_CONTROL_AE_MODE_ON});
    m.set<uint8_t>(ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES,
                   {ANDROID_CONTROL_AE_ANTIBANDING_MODE_OFF, ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO});
    m.set<int32_t>(ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES, {15, 30, 30, 30});
    m.set<int32_t>(ANDROID_CONTROL_AE_COMPENSATION_RANGE, {0, 0});
    m.set<camera_metadata_rational_t>(ANDROID_CONTROL_AE_COMPENSATION_STEP, {{1, 3}});
    m.set<uint8_t>(ANDROID_CONTROL_AE_LOCK_AVAILABLE, {ANDROID_CONTROL_AE_LOCK_AVAILABLE_FALSE});
    m.set<uint8_t>(ANDROID_CONTROL_AF_AVAILABLE_MODES, {ANDROID_CONTROL_AF_MODE_OFF});
    m.set<uint8_t>(ANDROID_CONTROL_AWB_AVAILABLE_MODES, {ANDROID_CONTROL_AWB_MODE_AUTO});
    m.set<uint8_t>(ANDROID_CONTROL_AWB_LOCK_AVAILABLE, {ANDROID_CONTROL_AWB_LOCK_AVAILABLE_FALSE});
    m.set<uint8_t>(ANDROID_CONTROL_AVAILABLE_EFFECTS, {ANDROID_CONTROL_EFFECT_MODE_OFF});
    m.set<uint8_t>(ANDROID_CONTROL_AVAILABLE_SCENE_MODES, {ANDROID_CONTROL_SCENE_MODE_DISABLED});
    m.set<uint8_t>(ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES,
                   {ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF});
    m.set<int32_t>(ANDROID_CONTROL_MAX_REGIONS, {0, 0, 0});

    m.set<uint8_t>(ANDROID_FLASH_INFO_AVAILABLE, {ANDROID_FLASH_INFO_AVAILABLE_FALSE});
    m.set<uint8_t>(ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES,
                   {ANDROID_NOISE_REDUCTION_MODE_OFF});
    m.set<uint8_t>(ANDROID_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES,
                   {ANDROID_COLOR_CORRECTION_ABERRATION_MODE_OFF});
    m.set<int32_t>(ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES, {0, 0});
    m.set<int32_t>(ANDROID_JPEG_MAX_SIZE, {kJpegMaxSize});
    m.set<float>(ANDROID_LENS_INFO_AVAILABLE_APERTURES, {2.0f});
    m.set<float>(ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES, {0.0f});
    m.set<float>(ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS, {3.5f});
    m.set<uint8_t>(ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION,
                   {ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF});
    m.set<uint8_t>(ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION,
                   {ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED});
    m.set<float>(ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE, {0.0f});
    m.set<uint8_t>(ANDROID_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES,
                   {ANDROID_STATISTICS_FACE_DETECT_MODE_OFF});
    m.set<int32_t>(ANDROID_STATISTICS_INFO_MAX_FACE_COUNT, {0});
    m.set<uint8_t>(ANDROID_STATISTICS_INFO_AVAILABLE_LENS_SHADING_MAP_MODES,
                   {ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_OFF});

    std::vector<int32_t> result(std::begin(kRequestKeys), std::end(kRequestKeys));
    result.insert(result.end(), std::begin(kDynamicKeys), std::end(kDynamicKeys));
    m.set(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS, kRequestKeys, std::size(kRequestKeys), 4);
    m.set(ANDROID_REQUEST_AVAILABLE_RESULT_KEYS, result.data(), result.size(), 4);
    std::vector<int32_t> chars = m.tags();
    chars.push_back(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS);
    m.set(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS, chars.data(), chars.size(), 4);
    return m.toAidl();
}

CameraMetadata makeDefaultRequest(RequestTemplate type) {
    Meta m(48, 1024);
    uint8_t intent = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
    switch (type) {
        case RequestTemplate::STILL_CAPTURE: intent = ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE; break;
        case RequestTemplate::VIDEO_RECORD: intent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD; break;
        case RequestTemplate::VIDEO_SNAPSHOT: intent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT; break;
        case RequestTemplate::ZERO_SHUTTER_LAG: intent = ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG; break;
        case RequestTemplate::MANUAL: intent = ANDROID_CONTROL_CAPTURE_INTENT_MANUAL; break;
        default: break;
    }
    const bool video = type == RequestTemplate::VIDEO_RECORD || type == RequestTemplate::VIDEO_SNAPSHOT;
    m.set<uint8_t>(ANDROID_CONTROL_CAPTURE_INTENT, {intent});
    m.set<uint8_t>(ANDROID_CONTROL_MODE, {ANDROID_CONTROL_MODE_AUTO});
    m.set<uint8_t>(ANDROID_CONTROL_AE_MODE, {ANDROID_CONTROL_AE_MODE_ON});
    m.set<uint8_t>(ANDROID_CONTROL_AE_LOCK, {ANDROID_CONTROL_AE_LOCK_OFF});
    m.set<uint8_t>(ANDROID_CONTROL_AE_ANTIBANDING_MODE, {ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO});
    m.set<int32_t>(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, {0});
    m.set<uint8_t>(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER, {ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE});
    m.set<int32_t>(ANDROID_CONTROL_AE_TARGET_FPS_RANGE, {video ? 30 : 15, 30});
    m.set<uint8_t>(ANDROID_CONTROL_AF_MODE, {ANDROID_CONTROL_AF_MODE_OFF});
    m.set<uint8_t>(ANDROID_CONTROL_AF_TRIGGER, {ANDROID_CONTROL_AF_TRIGGER_IDLE});
    m.set<uint8_t>(ANDROID_CONTROL_AWB_MODE, {ANDROID_CONTROL_AWB_MODE_AUTO});
    m.set<uint8_t>(ANDROID_CONTROL_AWB_LOCK, {ANDROID_CONTROL_AWB_LOCK_OFF});
    m.set<uint8_t>(ANDROID_CONTROL_EFFECT_MODE, {ANDROID_CONTROL_EFFECT_MODE_OFF});
    m.set<uint8_t>(ANDROID_CONTROL_SCENE_MODE, {ANDROID_CONTROL_SCENE_MODE_DISABLED});
    m.set<uint8_t>(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, {ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF});
    m.set<uint8_t>(ANDROID_COLOR_CORRECTION_ABERRATION_MODE, {ANDROID_COLOR_CORRECTION_ABERRATION_MODE_OFF});
    m.set<uint8_t>(ANDROID_FLASH_MODE, {ANDROID_FLASH_MODE_OFF});
    m.set<uint8_t>(ANDROID_JPEG_QUALITY, {90});
    m.set<uint8_t>(ANDROID_JPEG_THUMBNAIL_QUALITY, {90});
    m.set<int32_t>(ANDROID_JPEG_THUMBNAIL_SIZE, {0, 0});
    m.set<int32_t>(ANDROID_JPEG_ORIENTATION, {0});
    m.set<float>(ANDROID_LENS_FOCAL_LENGTH, {3.5f});
    m.set<uint8_t>(ANDROID_LENS_OPTICAL_STABILIZATION_MODE, {ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF});
    m.set<uint8_t>(ANDROID_NOISE_REDUCTION_MODE, {ANDROID_NOISE_REDUCTION_MODE_OFF});
    m.set<int32_t>(ANDROID_SCALER_CROP_REGION, {0, 0, kSensorW, kSensorH});
    m.set<uint8_t>(ANDROID_STATISTICS_FACE_DETECT_MODE, {ANDROID_STATISTICS_FACE_DETECT_MODE_OFF});
    return m.toAidl();
}

// ---------------------------------------------------------------------------
// Frame source: latest NV12 frame from the virtio port, or a test pattern.

struct Frame {
    std::vector<uint8_t> nv12;  // kSensorW x kSensorH, Y then interleaved CbCr
};

class FrameSource {
  public:
    static FrameSource& get() {
        static FrameSource* s = new FrameSource();  // lives for the process
        return *s;
    }
    std::shared_ptr<const Frame> latest() {
        std::lock_guard lock(mutex_);
        if (latest_ && Clock::now() - received_ < std::chrono::milliseconds(1500))
            return latest_;
        return pattern();
    }

  private:
    FrameSource() { std::thread([this] { run(); }).detach(); }

    static int openPort() {
        DIR* dir = opendir("/sys/class/virtio-ports");
        if (!dir) return -1;
        int fd = -1;
        while (dirent* e = readdir(dir)) {
            if (e->d_name[0] == '.') continue;
            std::ifstream f(std::string("/sys/class/virtio-ports/") + e->d_name + "/name");
            std::string n;
            if (std::getline(f, n) && n == "tebox.camera") {
                fd = ::open((std::string("/dev/") + e->d_name).c_str(), O_RDONLY | O_CLOEXEC);
                if (fd >= 0) LOGI("camera frames from /dev/%s", e->d_name);
                else LOGE("open /dev/%s: %s", e->d_name, strerror(errno));
                break;
            }
        }
        closedir(dir);
        return fd;
    }
    static bool readFull(int fd, void* p, size_t n) {
        auto* b = static_cast<uint8_t*>(p);
        while (n > 0) {
            ssize_t r = ::read(fd, b, n);
            if (r < 0 && errno == EINTR) continue;
            if (r <= 0) return false;
            b += r;
            n -= size_t(r);
        }
        return true;
    }
    void run() {
        struct Header {
            uint32_t magic, format, width, height, size, reserved;
            uint64_t pts;
        } __attribute__((packed));
        for (;;) {
            int fd = openPort();
            if (fd < 0) {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
            Header h;
            // A virtio port read returns 0 while the host side is closed:
            // back off and keep the port open.
            while (true) {
                if (!readFull(fd, &h, sizeof(h))) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    continue;
                }
                if (h.magic != kCameraMagic || h.format != 1 || h.width != uint32_t(kSensorW) ||
                    h.height != uint32_t(kSensorH) || h.size != uint32_t(kSensorW * kSensorH * 3 / 2)) {
                    LOGE("bad camera frame header (magic %#x %ux%u); resyncing", h.magic, h.width, h.height);
                    // Drop whatever is buffered and wait for the next header.
                    uint8_t junk[4096];
                    while (::read(fd, junk, sizeof(junk)) == sizeof(junk)) {}
                    continue;
                }
                auto f = std::make_shared<Frame>();
                f->nv12.resize(h.size);
                if (!readFull(fd, f->nv12.data(), h.size)) continue;
                std::lock_guard lock(mutex_);
                if (!latest_) LOGI("first camera frame from the viewer");
                latest_ = std::move(f);
                received_ = Clock::now();
            }
        }
    }
    // Colour bars with a moving block: shows the pipeline is alive.
    std::shared_ptr<const Frame> pattern() {
        auto f = std::make_shared<Frame>();
        f->nv12.resize(size_t(kSensorW) * kSensorH * 3 / 2);
        static const uint8_t bars[8][3] = {{235, 128, 128}, {210, 16, 146}, {170, 166, 16},
                                           {145, 54, 34},   {106, 202, 222}, {81, 90, 240},
                                           {41, 240, 110},  {16, 128, 128}};
        const int t = int((bootNs() / 16000000) % (kSensorW - 160));
        uint8_t* y = f->nv12.data();
        uint8_t* uv = y + kSensorW * kSensorH;
        for (int r = 0; r < kSensorH; r++)
            for (int c = 0; c < kSensorW; c++) {
                const bool box = c >= t && c < t + 160 && r >= 280 && r < 440;
                y[r * kSensorW + c] = box ? 235 : bars[c * 8 / kSensorW][0];
            }
        for (int r = 0; r < kSensorH / 2; r++)
            for (int c = 0; c < kSensorW / 2; c++) {
                const bool box = c * 2 >= t && c * 2 < t + 160 && r * 2 >= 280 && r * 2 < 440;
                uv[r * kSensorW + c * 2] = box ? 128 : bars[c * 16 / kSensorW][1];
                uv[r * kSensorW + c * 2 + 1] = box ? 128 : bars[c * 16 / kSensorW][2];
            }
        return f;
    }

    std::mutex mutex_;
    std::shared_ptr<const Frame> latest_;
    Clock::time_point received_;
};

// Centre-crop the sensor frame to the target aspect and sample nearest.
struct Mapper {
    int x0, y0, cw, ch, w, h;
    Mapper(int tw, int th) : w(tw), h(th) {
        cw = kSensorW;
        ch = kSensorH;
        if (int64_t(cw) * th > int64_t(ch) * tw) cw = int(int64_t(ch) * tw / th);
        else ch = int(int64_t(cw) * th / tw);
        x0 = (kSensorW - cw) / 2;
        y0 = (kSensorH - ch) / 2;
    }
    int sx(int x) const { return x0 + x * cw / w; }
    int sy(int y) const { return y0 + y * ch / h; }
};

inline uint8_t clamp8(int v) { return uint8_t(v < 0 ? 0 : v > 255 ? 255 : v); }

void fillRgba(const Frame& f, uint8_t* dst, int w, int h, int stridePx, bool bgra) {
    const uint8_t* Y = f.nv12.data();
    const uint8_t* UV = Y + kSensorW * kSensorH;
    Mapper m(w, h);
    for (int r = 0; r < h; r++) {
        const int sy = m.sy(r);
        uint8_t* row = dst + size_t(r) * stridePx * 4;
        for (int c = 0; c < w; c++) {
            const int sx = m.sx(c);
            // BT.601 limited range (what swscale's NV12 output is).
            const int yy = (Y[sy * kSensorW + sx] - 16) * 298;
            const int u = UV[(sy / 2) * kSensorW + (sx & ~1)] - 128;
            const int v = UV[(sy / 2) * kSensorW + (sx & ~1) + 1] - 128;
            const uint8_t R = clamp8((yy + 409 * v + 128) >> 8);
            const uint8_t G = clamp8((yy - 100 * u - 208 * v + 128) >> 8);
            const uint8_t B = clamp8((yy + 516 * u + 128) >> 8);
            row[c * 4 + 0] = bgra ? B : R;
            row[c * 4 + 1] = G;
            row[c * 4 + 2] = bgra ? R : B;
            row[c * 4 + 3] = 255;
        }
    }
}

// fullRange: expand to JFIF's 0..255 YCbCr (for JPEG).
void fillNv12(const Frame& f, uint8_t* y, uint8_t* uv, int w, int h, int stride, bool fullRange) {
    const uint8_t* Y = f.nv12.data();
    const uint8_t* UV = Y + kSensorW * kSensorH;
    Mapper m(w, h);
    for (int r = 0; r < h; r++) {
        const int sy = m.sy(r);
        for (int c = 0; c < w; c++) {
            const int v = Y[sy * kSensorW + m.sx(c)];
            y[size_t(r) * stride + c] = fullRange ? clamp8((v - 16) * 255 / 219) : uint8_t(v);
        }
    }
    for (int r = 0; r < h / 2; r++) {
        const int sy = m.sy(r * 2) / 2;
        for (int c = 0; c < w / 2; c++) {
            const int sx = m.sx(c * 2) & ~1;
            for (int k = 0; k < 2; k++) {
                const int v = UV[sy * kSensorW + sx + k];
                uv[size_t(r) * stride + c * 2 + k] =
                    fullRange ? clamp8((v - 128) * 255 / 224 + 128) : uint8_t(v);
            }
        }
    }
}

// VirGL buffers written through a CPU mapping must be uploaded to the host
// resource before a GPU consumer (preview SurfaceTexture) samples them.
void transferToHost(int dmabuf, int width, int height) {
    static int drm = ::open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC);
    if (drm < 0) return;
    drm_prime_handle prime{};
    prime.fd = dmabuf;
    if (ioctl(drm, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime) != 0) return;  // not a GPU buffer
    drm_virtgpu_3d_transfer_to_host t{};
    t.bo_handle = prime.handle;
    t.box.w = uint32_t(width);
    t.box.h = uint32_t(height);
    t.box.d = 1;
    if (ioctl(drm, DRM_IOCTL_VIRTGPU_TRANSFER_TO_HOST, &t) != 0)
        LOGE("transfer to host: %s", strerror(errno));
    drm_gem_close close{};
    close.handle = prime.handle;
    ioctl(drm, DRM_IOCTL_GEM_CLOSE, &close);
}

bool waitFence(const NativeHandle& fence) {
    if (fence.fds.empty()) return true;
    pollfd p{fence.fds[0].get(), POLLIN, 0};
    return poll(&p, 1, 1000) > 0;
}

// ---------------------------------------------------------------------------

class Session final : public BnCameraDeviceSession {
  public:
    explicit Session(const std::shared_ptr<ICameraDeviceCallback>& cb)
        : callback_(cb), requestQueue_(1 << 16, false), resultQueue_(1 << 16, false),
          settings_(makeDefaultRequest(RequestTemplate::PREVIEW).metadata) {
        FrameSource::get();
        worker_ = std::thread([this] { run(); });
    }
    ~Session() override { stop(); }

    ndk::ScopedAStatus close() override {
        stop();
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus configureStreams(const StreamConfiguration& config,
                                        std::vector<HalStream>* out) override {
        out->clear();
        std::lock_guard lock(mutex_);
        streams_.clear();
        for (const auto& s : config.streams) {
            HalStream h;
            h.id = s.id;
            // Concrete RGBA for IMPLEMENTATION_DEFINED: Mesa's fallback gralloc
            // derives the DRM fourcc from the buffer format and cannot import
            // format 34, so GPU consumers (preview) would fail.
            h.overrideFormat = int(s.format) == kFormatImpl ? PixelFormat::RGBA_8888 : s.format;
            h.overrideDataSpace = s.dataSpace;
            // The worker writes every buffer with the CPU; the allocator gives
            // CAMERA_OUTPUT buffers a writable mapping. CPU usage bits here
            // would make GPU consumers (preview) fail to import the buffer.
            h.producerUsage = BufferUsage::CAMERA_OUTPUT;
            h.consumerUsage = BufferUsage(0);
            h.maxBuffers = 4;
            streams_[s.id] = {int(h.overrideFormat), s.width, s.height};
            LOGI("stream %d: format %d %dx%d usage %#llx", s.id, int(s.format), s.width,
                 s.height, (long long)s.usage);
            out->push_back(h);
        }
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus configureStreamsV2(const StreamConfiguration& config,
                                          aidl::android::hardware::camera::device::ConfigureStreamsRet* out) override {
        return configureStreams(config, &out->halStreams);
    }
    ndk::ScopedAStatus constructDefaultRequestSettings(RequestTemplate type, CameraMetadata* out) override {
        *out = makeDefaultRequest(type);
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus flush() override {
        std::unique_lock lock(mutex_);
        flushing_ = true;
        cond_.notify_all();
        idle_.wait_for(lock, std::chrono::seconds(2), [this] { return queue_.empty() && !busy_; });
        flushing_ = false;
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus getCaptureRequestMetadataQueue(MQDescriptor<int8_t, SynchronizedReadWrite>* out) override {
        *out = requestQueue_.dupeDesc();
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus getCaptureResultMetadataQueue(MQDescriptor<int8_t, SynchronizedReadWrite>* out) override {
        *out = resultQueue_.dupeDesc();
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus isReconfigurationRequired(const CameraMetadata&, const CameraMetadata&, bool* out) override {
        *out = false;
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus processCaptureRequest(const std::vector<CaptureRequest>& requests,
                                             const std::vector<aidl::android::hardware::camera::device::BufferCache>& cachesToRemove,
                                             int32_t* out) override {
        std::lock_guard lock(mutex_);
        for (const auto& cache : cachesToRemove) buffers_.erase({cache.streamId, cache.bufferId});
        for (const auto& request : requests) {
            if (request.frameNumber < 3)
                LOGI("request %d: %zu buffers, settings %zu bytes inline / %lld fmq",
                     request.frameNumber, request.outputBuffers.size(),
                     request.settings.metadata.size(), (long long)request.fmqSettingsSize);
            Pending p;
            p.frameNumber = request.frameNumber;
            // Settings arrive inline, through the FMQ, or not at all (= repeat).
            if (request.fmqSettingsSize > 0) {
                std::vector<uint8_t> blob(size_t(request.fmqSettingsSize));
                if (requestQueue_.read(reinterpret_cast<int8_t*>(blob.data()), blob.size()))
                    settings_ = std::move(blob);
            } else if (!request.settings.metadata.empty()) {
                settings_ = request.settings.metadata;
            }
            p.settings = settings_;
            for (const auto& b : request.outputBuffers) {
                Output o;
                o.streamId = b.streamId;
                o.bufferId = b.bufferId;
                if (!b.buffer.fds.empty()) {
                    NativeHandle h;
                    h.ints = b.buffer.ints;
                    for (const auto& fd : b.buffer.fds) h.fds.emplace_back(dup(fd.get()));
                    buffers_[{b.streamId, b.bufferId}] = std::move(h);
                }
                if (!b.acquireFence.fds.empty())
                    o.fence.fds.emplace_back(dup(b.acquireFence.fds[0].get()));
                p.outputs.push_back(std::move(o));
            }
            queue_.push_back(std::move(p));
        }
        cond_.notify_all();
        *out = int32_t(requests.size());
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus signalStreamFlush(const std::vector<int32_t>&, int32_t) override {
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus switchToOffline(const std::vector<int32_t>&,
                                       aidl::android::hardware::camera::device::CameraOfflineSessionInfo*,
                                       std::shared_ptr<aidl::android::hardware::camera::device::ICameraOfflineSession>* out) override {
        out->reset();
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    ndk::ScopedAStatus repeatingRequestEnd(int32_t, const std::vector<int32_t>&) override {
        return ndk::ScopedAStatus::ok();
    }

  private:
    struct StreamInfo { int format, width, height; };
    struct Output {
        int32_t streamId = 0;
        int64_t bufferId = 0;
        NativeHandle fence;
    };
    struct Pending {
        int32_t frameNumber = 0;
        std::vector<uint8_t> settings;
        std::vector<Output> outputs;
    };

    void stop() {
        {
            std::lock_guard lock(mutex_);
            if (stopping_) return;
            stopping_ = true;
            cond_.notify_all();
        }
        if (worker_.joinable()) worker_.join();
        std::lock_guard lock(mutex_);
        callback_.reset();
    }

    void run() {
        auto next = Clock::now();
        for (;;) {
            Pending p;
            std::shared_ptr<ICameraDeviceCallback> cb;
            {
                std::unique_lock lock(mutex_);
                cond_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
                if (stopping_) return;
                p = std::move(queue_.front());
                queue_.pop_front();
                busy_ = true;
                cb = callback_;
                if (flushing_) next = Clock::now();
            }
            std::this_thread::sleep_until(next);
            next = std::max(next + std::chrono::nanoseconds(kFrameNs), Clock::now());
            process(p, cb);
            std::lock_guard lock(mutex_);
            busy_ = false;
            if (queue_.empty()) idle_.notify_all();
        }
    }

    bool fill(const Output& o, const Frame& frame, const Meta& settings) {
        NativeHandle h;
        StreamInfo info{};
        {
            std::lock_guard lock(mutex_);
            auto it = buffers_.find({o.streamId, o.bufferId});
            auto st = streams_.find(o.streamId);
            if (it == buffers_.end() || st == streams_.end()) return false;
            h.ints = it->second.ints;
            for (const auto& fd : it->second.fds) h.fds.emplace_back(dup(fd.get()));
            info = st->second;
        }
        if (h.fds.empty() || h.ints.size() < 4) {
            LOGE("stream %d buffer %lld: no handle", o.streamId, (long long)o.bufferId);
            return false;
        }
        if (!waitFence(o.fence)) {
            LOGE("stream %d: acquire fence timeout", o.streamId);
            return false;
        }
        // gralloc handle ints: width, height, stride (pixels), format, usage.
        const int w = h.ints[0], hgt = h.ints[1], stride = h.ints[2], format = h.ints[3];
        size_t bytes = format == kFormatYuv ? size_t(stride) * hgt * 3 / 2
                     : format == kFormatBlob ? size_t(w) * hgt
                     : size_t(stride) * hgt * 4;
        void* p = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, h.fds[0].get(), 0);
        if (p == MAP_FAILED) {
            LOGE("mmap stream %d format %d %dx%d stride %d: %s", o.streamId, format, w, hgt,
                 stride, strerror(errno));
            return false;
        }
        auto* px = static_cast<uint8_t*>(p);
        if (format == kFormatYuv) {
            fillNv12(frame, px, px + size_t(stride) * hgt, w, hgt, stride, false);
        } else if (format == kFormatBlob) {
            const int jw = info.width, jh = info.height;
            std::vector<uint8_t> tmp(size_t(jw) * jh * 3 / 2);
            fillNv12(frame, tmp.data(), tmp.data() + size_t(jw) * jh, jw, jh, jw, true);
            const auto jpg = jpeg::encodeNv12(tmp.data(), tmp.data() + size_t(jw) * jh, jw, jh, jw,
                                              settings.u8(ANDROID_JPEG_QUALITY, 90));
            if (jpg.size() + sizeof(CameraBlob) <= bytes) {
                memcpy(px, jpg.data(), jpg.size());
                // The framework finds the JPEG size in a trailer at the end.
                struct { int32_t id, size; } blob{int32_t(CameraBlobId::JPEG), int32_t(jpg.size())};
                memcpy(px + bytes - sizeof(blob), &blob, sizeof(blob));
            } else {
                LOGE("JPEG %zu bytes does not fit the %zu byte buffer", jpg.size(), bytes);
            }
        } else {
            fillRgba(frame, px, w, hgt, stride, format == 5 /* BGRA_8888 */);
        }
        munmap(p, bytes);
        if (format != kFormatYuv && format != kFormatBlob) transferToHost(h.fds[0].get(), w, hgt);
        return true;
    }

    void process(Pending& p, const std::shared_ptr<ICameraDeviceCallback>& cb) {
        if (!cb) return;
        const int64_t timestamp = bootNs();
        Meta settings(p.settings);
        const auto frame = FrameSource::get().latest();

        NotifyMsg shutter;
        shutter.set<NotifyMsg::shutter>(ShutterMsg{p.frameNumber, timestamp, timestamp});
        std::vector<NotifyMsg> notes{shutter};

        CaptureResult result;
        result.frameNumber = p.frameNumber;
        result.partialResult = 1;
        result.inputBuffer.streamId = -1;
        for (const auto& o : p.outputs) {
            StreamBuffer b;
            b.streamId = o.streamId;
            b.bufferId = o.bufferId;
            b.status = fill(o, *frame, settings) ? BufferStatus::OK : BufferStatus::ERROR;
            if (b.status == BufferStatus::ERROR) {
                NotifyMsg err;
                err.set<NotifyMsg::error>(ErrorMsg{p.frameNumber, o.streamId, ErrorCode::ERROR_BUFFER});
                notes.push_back(err);
            }
            result.outputBuffers.push_back(std::move(b));
        }

        // Echo the request settings plus the 3A and sensor state.
        Meta r(p.settings);
        r.set<int64_t>(ANDROID_SENSOR_TIMESTAMP, {timestamp});
        r.set<int64_t>(ANDROID_SENSOR_EXPOSURE_TIME, {10000000});
        r.set<int64_t>(ANDROID_SENSOR_FRAME_DURATION, {kFrameNs});
        r.set<int64_t>(ANDROID_SENSOR_ROLLING_SHUTTER_SKEW, {10000000});
        r.set<int32_t>(ANDROID_SENSOR_SENSITIVITY, {100});
        r.set<uint8_t>(ANDROID_CONTROL_AE_STATE, {ANDROID_CONTROL_AE_STATE_CONVERGED});
        r.set<uint8_t>(ANDROID_CONTROL_AF_STATE, {ANDROID_CONTROL_AF_STATE_INACTIVE});
        r.set<uint8_t>(ANDROID_CONTROL_AWB_STATE, {ANDROID_CONTROL_AWB_STATE_CONVERGED});
        r.set<uint8_t>(ANDROID_FLASH_STATE, {ANDROID_FLASH_STATE_UNAVAILABLE});
        r.set<uint8_t>(ANDROID_LENS_STATE, {ANDROID_LENS_STATE_STATIONARY});
        r.set<uint8_t>(ANDROID_REQUEST_PIPELINE_DEPTH, {2});
        r.set<uint8_t>(ANDROID_STATISTICS_SCENE_FLICKER, {ANDROID_STATISTICS_SCENE_FLICKER_NONE});
        result.result = r.toAidl();

        auto st = cb->notify(notes);
        std::vector<CaptureResult> results;
        results.push_back(std::move(result));
        auto rt = cb->processCaptureResult(results);
        if (p.frameNumber < 3 || !st.isOk() || !rt.isOk())
            LOGI("frame %d done: notify %s, result %s", p.frameNumber, st.getDescription().c_str(),
                 rt.getDescription().c_str());
    }

    std::mutex mutex_;
    std::condition_variable cond_, idle_;
    std::deque<Pending> queue_;
    std::thread worker_;
    bool stopping_ = false, flushing_ = false, busy_ = false;
    std::shared_ptr<ICameraDeviceCallback> callback_;
    CameraQueue requestQueue_, resultQueue_;
    std::vector<uint8_t> settings_;
    std::map<int32_t, StreamInfo> streams_;
    std::map<std::pair<int32_t, int64_t>, NativeHandle> buffers_;
};

bool supported(const StreamConfiguration& config) {
    for (const auto& s : config.streams) {
        const int f = int(s.format);
        if (f != kFormatImpl && f != kFormatYuv && f != kFormatBlob && f != 1 && f != 2) return false;
        bool size = false;
        for (const auto& k : kSizes) size |= s.width == k[0] && s.height == k[1];
        if (!size) return false;
    }
    return true;
}

class Device final : public aidl::android::hardware::camera::device::BnCameraDevice {
  public:
    ndk::ScopedAStatus getCameraCharacteristics(CameraMetadata* out) override {
        *out = makeCharacteristics();
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus getPhysicalCameraCharacteristics(const std::string&, CameraMetadata* out) override {
        out->metadata.clear();
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    ndk::ScopedAStatus getResourceCost(CameraResourceCost* out) override {
        out->resourceCost = 100;
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus isStreamCombinationSupported(const StreamConfiguration& config, bool* out) override {
        *out = supported(config);
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus open(const std::shared_ptr<ICameraDeviceCallback>& cb,
                            std::shared_ptr<ICameraDeviceSession>* out) override {
        LOGI("open");
        *out = ndk::SharedRefBase::make<Session>(cb);
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus openInjectionSession(const std::shared_ptr<ICameraDeviceCallback>&,
                                            std::shared_ptr<aidl::android::hardware::camera::device::ICameraInjectionSession>* out) override {
        out->reset();
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    ndk::ScopedAStatus setTorchMode(bool) override {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    ndk::ScopedAStatus turnOnTorchWithStrengthLevel(int32_t) override {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    ndk::ScopedAStatus getTorchStrengthLevel(int32_t* out) override {
        *out = 0;
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    ndk::ScopedAStatus constructDefaultRequestSettings(RequestTemplate type, CameraMetadata* out) override {
        *out = makeDefaultRequest(type);
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus isStreamCombinationWithSettingsSupported(const StreamConfiguration& config, bool* out) override {
        *out = supported(config);
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus getSessionCharacteristics(const StreamConfiguration&, CameraMetadata* out) override {
        out->metadata.clear();
        return ndk::ScopedAStatus::ok();
    }
};

class Provider final : public BnCameraProvider {
  public:
    ndk::ScopedAStatus setCallback(const std::shared_ptr<ICameraProviderCallback>& cb) override {
        callback_ = cb;
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus getVendorTags(std::vector<VendorTagSection>* out) override {
        out->clear();
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus getCameraIdList(std::vector<std::string>* out) override {
        *out = {"device@1.0/internal/0"};
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus getCameraDeviceInterface(const std::string& name,
                                                std::shared_ptr<ICameraDevice>* out) override {
        if (name != "device@1.0/internal/0")
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        *out = ndk::SharedRefBase::make<Device>();
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus notifyDeviceStateChange(int64_t) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus getConcurrentCameraIds(std::vector<ConcurrentCameraIdCombination>* out) override {
        out->clear();
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus isConcurrentStreamCombinationSupported(const std::vector<CameraIdAndStreamCombination>&,
                                                              bool* out) override {
        *out = false;
        return ndk::ScopedAStatus::ok();
    }

  private:
    std::shared_ptr<ICameraProviderCallback> callback_;
};

}  // namespace

int main() {
    LOGI("starting provider");
    ABinderProcess_setThreadPoolMaxThreadCount(4);
    auto provider = ndk::SharedRefBase::make<Provider>();
    const char* name = "android.hardware.camera.provider.ICameraProvider/internal/0";
    if (AServiceManager_addService(provider->asBinder().get(), name) != EX_NONE) return 1;
    ABinderProcess_joinThreadPool();
    return 0;
}
