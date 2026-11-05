// Soft Sensors AIDL v2 for QEMU. Exposes virtual stationary sensors.
#include "event-queue.h"
#include <aidl/android/hardware/sensors/BnSensors.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/log.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <ctime>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using namespace aidl::android::hardware::sensors;
using aidl::android::hardware::common::fmq::MQDescriptor;
using aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using ndk::ScopedAStatus;

namespace {
constexpr int32_t kAccel = 1, kGyro = 2, kLight = 3;
constexpr uint32_t kReadAndProcess = ISensors::EVENT_QUEUE_FLAG_BITS_READ_AND_PROCESS;

struct VirtualSensor {
    SensorInfo info;
    bool enabled = false;
    int64_t periodNs = 20'000'000;
    int64_t nextNs = 0;
    bool reported = false;
};

int64_t nowNs() {
    timespec ts{};
    clock_gettime(CLOCK_BOOTTIME, &ts);
    return int64_t(ts.tv_sec) * 1'000'000'000LL + ts.tv_nsec;
}

SensorInfo makeInfo(int32_t handle, SensorType type, const char* name, const char* typeName,
                   float range, float resolution, int32_t minDelayUs, int32_t flags) {
    SensorInfo out;
    out.sensorHandle = handle;
    out.name = name;
    out.vendor = "tebox QEMU";
    out.version = 1;
    out.type = type;
    out.typeAsString = typeName;
    out.maxRange = range;
    out.resolution = resolution;
    out.power = 0.1f;
    out.minDelayUs = minDelayUs;
    out.fifoReservedEventCount = 0;
    out.fifoMaxEventCount = 0;
    out.requiredPermission = "";
    out.maxDelayUs = 10'000'000;
    out.flags = flags;
    return out;
}

class QemuSensors final : public BnSensors {
  public:
    QemuSensors() {
        sensors_.push_back({makeInfo(kAccel, SensorType::ACCELEROMETER, "QEMU Accelerometer",
                                     "android.sensor.accelerometer", 78.4f, 0.001f, 10'000, 0),
                            false, 20'000'000});
        sensors_.push_back({makeInfo(kGyro, SensorType::GYROSCOPE, "QEMU Gyroscope",
                                     "android.sensor.gyroscope", 34.9f, 0.001f, 10'000, 0),
                            false, 20'000'000});
        sensors_.push_back({makeInfo(kLight, SensorType::LIGHT, "QEMU Ambient Light",
                                     "android.sensor.light", 40'000.0f, 1.0f, 100'000,
                                     static_cast<int32_t>(SensorInfo::SENSOR_FLAG_BITS_ON_CHANGE_MODE)),
                            false, 200'000'000});
    }
    ~QemuSensors() override {
        stop_.store(true);
        if (worker_.joinable()) worker_.join();
    }

    ScopedAStatus activate(int32_t handle, bool enabled) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto* sensor = find(handle);
        if (!sensor) return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        if (sensor->enabled != enabled) {
            sensor->enabled = enabled;
            sensor->nextNs = 0;
            sensor->reported = false;
        }
        return ScopedAStatus::ok();
    }
    ScopedAStatus batch(int32_t handle, int64_t period, int64_t) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto* sensor = find(handle);
        if (!sensor || period < 0) return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        const int64_t minimum = int64_t(sensor->info.minDelayUs) * 1000;
        sensor->periodNs = std::max(minimum, std::min<int64_t>(period, 10'000'000'000LL));
        return ScopedAStatus::ok();
    }
    ScopedAStatus configDirectReport(int32_t, int32_t, ISensors::RateLevel, int32_t* out) override {
        *out = ERROR_BAD_VALUE;
        return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    ScopedAStatus flush(int32_t handle) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto* sensor = find(handle);
        if (!sensor || !sensor->enabled || !queue_ || pending_.size() >= 128)
            return ScopedAStatus::fromServiceSpecificError(ERROR_BAD_VALUE);
        Event event;
        event.timestamp = nowNs();
        event.sensorHandle = handle;
        event.sensorType = SensorType::META_DATA;
        Event::EventPayload::MetaData meta;
        meta.what = Event::EventPayload::MetaData::MetaDataEventType::META_DATA_FLUSH_COMPLETE;
        event.payload.set<Event::EventPayload::Tag::meta>(meta);
        pending_.push_back(event);
        return ScopedAStatus::ok();
    }
    ScopedAStatus getSensorsList(std::vector<SensorInfo>* out) override {
        std::lock_guard<std::mutex> lock(mutex_);
        out->clear();
        for (const auto& sensor : sensors_) out->push_back(sensor.info);
        return ScopedAStatus::ok();
    }
    ScopedAStatus initialize(const MQDescriptor<Event, SynchronizedReadWrite>& descriptor,
                             const MQDescriptor<int32_t, SynchronizedReadWrite>&,
                             const std::shared_ptr<ISensorsCallback>& callback) override {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& sensor : sensors_) {
            sensor.enabled = false;
            sensor.nextNs = 0;
            sensor.reported = false;
        }
        pending_.clear();
        queue_ = std::make_unique<EventQueue<Event>>();
        if (!callback || !queue_->initialize(descriptor)) {
            queue_.reset();
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
        if (!worker_.joinable()) worker_ = std::thread(&QemuSensors::run, this);
        __android_log_print(ANDROID_LOG_INFO, "qemu-sensors", "initialized virtual sensor queue");
        return ScopedAStatus::ok();
    }
    ScopedAStatus injectSensorData(const Event&) override {
        return ScopedAStatus::fromServiceSpecificError(ERROR_BAD_VALUE);
    }
    ScopedAStatus registerDirectChannel(const ISensors::SharedMemInfo&, int32_t* out) override {
        *out = ERROR_BAD_VALUE;
        return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    ScopedAStatus setOperationMode(ISensors::OperationMode mode) override {
        if (mode != ISensors::OperationMode::NORMAL)
            return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
        return ScopedAStatus::ok();
    }
    ScopedAStatus unregisterDirectChannel(int32_t) override {
        return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

  private:
    VirtualSensor* find(int32_t handle) {
        for (auto& sensor : sensors_) if (sensor.info.sensorHandle == handle) return &sensor;
        return nullptr;
    }
    void run() {
        while (!stop_.load()) {
            int64_t sleepNs = 100'000'000;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                const int64_t timestamp = nowNs();
                while (queue_ && !pending_.empty()) {
                    if (!queue_->write(pending_.front(), kReadAndProcess)) break;
                    pending_.pop_front();
                }
                for (auto& sensor : sensors_) {
                    if (!sensor.enabled || !queue_ || !pending_.empty()) continue;
                    if (sensor.info.type == SensorType::LIGHT && sensor.reported) continue;
                    if (timestamp < sensor.nextNs) {
                        sleepNs = std::min(sleepNs, sensor.nextNs - timestamp);
                        continue;
                    }
                    Event event;
                    event.timestamp = timestamp;
                    event.sensorHandle = sensor.info.sensorHandle;
                    event.sensorType = sensor.info.type;
                    Event::EventPayload::Vec3 values{};
                    values.status = SensorStatus::ACCURACY_HIGH;
                    // The virtual phone is stationary and upright in portrait.
                    if (sensor.info.type == SensorType::ACCELEROMETER) values.y = 9.80665f;
                    if (sensor.info.type == SensorType::LIGHT)
                        event.payload.set<Event::EventPayload::Tag::scalar>(300.0f);
                    else event.payload.set<Event::EventPayload::Tag::vec3>(values);
                    if (queue_->write(event, kReadAndProcess)) {
                        sensor.reported = true;
                        sensor.nextNs = timestamp + sensor.periodNs;
                    }
                    sleepNs = std::min(sleepNs, sensor.periodNs);
                }
            }
            std::this_thread::sleep_for(std::chrono::nanoseconds(sleepNs));
        }
    }
    std::mutex mutex_;
    std::vector<VirtualSensor> sensors_;
    std::unique_ptr<EventQueue<Event>> queue_;
    std::deque<Event> pending_;
    std::atomic<bool> stop_{false};
    std::thread worker_;
};
}  // namespace

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    auto service = ndk::SharedRefBase::make<QemuSensors>();
    constexpr auto instance = "android.hardware.sensors.ISensors/default";
    auto status = AServiceManager_addService(service->asBinder().get(), instance);
    __android_log_print(status == STATUS_OK ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR,
                        "qemu-sensors", "register %s: %d", instance, status);
    if (status != STATUS_OK) return 1;
    ABinderProcess_joinThreadPool();
    return 1;
}
