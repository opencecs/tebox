/*
 * Minimal HIDL audio HAL for the Android 12 Mystic GSI bring-up.
 *
 * The GSI's audioserver (libaudiohal@7.1.so) only has the HIDL audio HAL
 * client: android.hardware.audio@7.1::IDevicesFactory plus
 * android.hardware.audio.effect@7.0::IEffectsFactory. It has no AIDL audio
 * path, so the AIDL stub used by the Android 16 variant is never seen.
 *
 * This GSI is Unisoc/SPRD-customized: AudioPolicyService only loads
 * libaudiopolicymanagercustom.so and has no fallback, so createAudioPolicyManager()
 * returning null crashes audioserver. The SPRD manager's initialize() opens the
 * primary output and input streams eagerly while it collects the available
 * devices; if either open fails, the configured default output device
 * (speaker) never becomes "reachable", initialize() returns -ENODEV and the
 * policy manager is never created. So both streams must open successfully.
 *
 * There is no audio hardware in the QEMU bring-up, so the streams are silent
 * endpoints: the output accepts and discards everything written through the
 * FMQ transport, the input returns silence. That is enough for AudioFlinger
 * to create its playback/record threads and for the policy manager to come up.
 */

#include <android/hardware/audio/7.1/IDevice.h>
#include <android/hardware/audio/7.1/IDevicesFactory.h>
#include <android/hardware/audio/effect/7.0/IEffectsFactory.h>
#include <android/hardware/audio/effect/7.0/types.h>

#include <fmq/MessageQueue.h>

#include <hidl/HidlTransportSupport.h>
#include <log/log.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <atomic>
#include <vector>

// The vendor libfmq.so was built before the Android 13 headers added the
// message overload of details::check(); the FMQ template code compiled here
// references it, so supply it. The one-argument overload, logError() and
// errorWriteLog() are all exported by the shipped libfmq.so.
namespace android {
namespace hardware {
namespace details {
void check(bool exp, const char* message) {
    if (!exp) {
        ALOGE("FMQ check failed: %s", message != nullptr ? message : "");
        abort();
    }
}
}  // namespace details
}  // namespace hardware
}  // namespace android

namespace {

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::MessageQueue;
using ::android::sp;

namespace audio7_0 = ::android::hardware::audio::V7_0;
namespace audio7_1 = ::android::hardware::audio::V7_1;
namespace common7_0 = ::android::hardware::audio::common::V7_0;
namespace effect7_0 = ::android::hardware::audio::effect::V7_0;

using Result = audio7_0::Result;

using WriteCommand = audio7_0::IStreamOut::WriteCommand;
using WriteStatus = audio7_0::IStreamOut::WriteStatus;
using ReadParameters = audio7_0::IStreamIn::ReadParameters;
using ReadStatus = audio7_0::IStreamIn::ReadStatus;

using CommandMQ = MessageQueue<WriteCommand, kSynchronizedReadWrite>;
using DataMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;
using StatusMQ = MessageQueue<WriteStatus, kSynchronizedReadWrite>;
using ReadCommandMQ = MessageQueue<ReadParameters, kSynchronizedReadWrite>;
using ReadStatusMQ = MessageQueue<ReadStatus, kSynchronizedReadWrite>;

// Polling period of the stream worker threads. The FMQ blocking helpers use
// the event flag word, so this is only an upper bound on how long a worker
// can sit idle before re-checking the stop flag.
constexpr int64_t kPollNs = 200 * 1000 * 1000;

constexpr uint32_t kLatencyMs = 20;
constexpr uint32_t kDefaultSampleRate = 48000;
constexpr uint32_t kDefaultFrameCount = 960;  // 20 ms at 48 kHz

size_t bytesPerSample(const hidl_string& format) {
    const char* f = format.c_str();
    if (strstr(f, "PCM_8_BIT")) return 1;
    if (strstr(f, "PCM_24_BIT_PACKED")) return 3;
    if (strstr(f, "PCM_32_BIT") || strstr(f, "PCM_FLOAT")) return 4;
    if (strstr(f, "PCM_24_BIT")) return 4;  // unpacked, 32-bit container
    return 2;
}

size_t channelCount(const hidl_string& mask) {
    const char* m = mask.c_str();
    if (strstr(m, "MONO")) return 1;
    if (strstr(m, "STEREO")) return 2;
    if (strstr(m, "QUAD")) return 4;
    if (strstr(m, "5POINT1") || strstr(m, "6POINT1")) return 6;
    if (strstr(m, "7POINT1")) return 8;
    return 2;
}

common7_0::AudioConfig effectiveConfig(const common7_0::AudioConfig& config) {
    common7_0::AudioConfig out = config;
    if (out.base.format.empty()) out.base.format = "AUDIO_FORMAT_PCM_16_BIT";
    if (out.base.sampleRateHz == 0) out.base.sampleRateHz = kDefaultSampleRate;
    if (out.base.channelMask.empty()) out.base.channelMask = "AUDIO_CHANNEL_OUT_STEREO";
    if (out.frameCount == 0) out.frameCount = kDefaultFrameCount;
    return out;
}

common7_0::DeviceAddress effectiveDevice(const common7_0::DeviceAddress& device,
                                         const char* fallbackType) {
    common7_0::DeviceAddress out = device;
    if (out.deviceType.empty()) out.deviceType = fallbackType;
    return out;
}

// ---------------------------------------------------------------------------
// Output stream: accepts audio through the FMQ transport and discards it.
// ---------------------------------------------------------------------------

class StreamOut : public audio7_1::IStreamOut {
  public:
    StreamOut(const common7_0::DeviceAddress& device, const common7_0::AudioConfig& config)
        : mDevice(effectiveDevice(device, "AUDIO_DEVICE_OUT_SPEAKER")),
          mConfig(effectiveConfig(config)) {
        mFrameSize = bytesPerSample(mConfig.base.format) *
                     channelCount(mConfig.base.channelMask);
        mFrameCount = mConfig.frameCount;
        mBufferSize = mFrameSize * mFrameCount;
    }

    ~StreamOut() override { stopWriter(); }

    const common7_0::AudioConfig& config() const { return mConfig; }

    Return<uint64_t> getFrameSize() override { return mFrameSize; }
    Return<uint64_t> getFrameCount() override { return mFrameCount; }
    Return<uint64_t> getBufferSize() override { return mBufferSize; }

    Return<void> getSupportedProfiles(getSupportedProfiles_cb cb) override {
        common7_0::AudioProfile profile;
        profile.format = "AUDIO_FORMAT_PCM_16_BIT";
        profile.sampleRates.resize(2);
        profile.sampleRates[0] = 44100;
        profile.sampleRates[1] = 48000;
        profile.channelMasks.resize(1);
        profile.channelMasks[0] = "AUDIO_CHANNEL_OUT_STEREO";
        hidl_vec<common7_0::AudioProfile> profiles;
        profiles.resize(1);
        profiles[0] = profile;
        cb(Result::OK, profiles);
        return Void();
    }

    Return<void> getAudioProperties(getAudioProperties_cb cb) override {
        cb(Result::OK, mConfig.base);
        return Void();
    }

    Return<Result> setAudioProperties(
            const common7_0::AudioConfigBaseOptional& config) override {
        (void)config;
        return Result::NOT_SUPPORTED;
    }

    Return<Result> addEffect(uint64_t effectId) override {
        (void)effectId;
        return Result::NOT_SUPPORTED;
    }

    Return<Result> removeEffect(uint64_t effectId) override {
        (void)effectId;
        return Result::NOT_SUPPORTED;
    }

    Return<Result> standby() override { return Result::OK; }

    Return<void> getDevices(getDevices_cb cb) override {
        hidl_vec<common7_0::DeviceAddress> devices;
        devices.resize(1);
        devices[0] = mDevice;
        cb(Result::OK, devices);
        return Void();
    }

    Return<Result> setDevices(const hidl_vec<common7_0::DeviceAddress>& devices) override {
        if (devices.size() > 0) mDevice = devices[0];
        return Result::OK;
    }

    Return<Result> setHwAvSync(uint32_t hwAvSync) override {
        (void)hwAvSync;
        return Result::NOT_SUPPORTED;
    }

    Return<void> getParameters(const hidl_vec<audio7_0::ParameterValue>& context,
                               const hidl_vec<hidl_string>& keys,
                               getParameters_cb cb) override {
        (void)context;
        hidl_vec<audio7_0::ParameterValue> values;
        values.resize(keys.size());
        for (size_t i = 0; i < keys.size(); ++i) {
            values[i].key = keys[i];
            values[i].value = "";
        }
        cb(Result::OK, values);
        return Void();
    }

    Return<Result> setParameters(const hidl_vec<audio7_0::ParameterValue>& context,
                                 const hidl_vec<audio7_0::ParameterValue>& parameters) override {
        (void)context;
        (void)parameters;
        return Result::OK;
    }

    Return<Result> start() override { return Result::OK; }
    Return<Result> stop() override { return Result::OK; }

    Return<void> createMmapBuffer(int32_t minSizeFrames, createMmapBuffer_cb cb) override {
        (void)minSizeFrames;
        cb(Result::NOT_SUPPORTED, audio7_0::MmapBufferInfo());
        return Void();
    }

    Return<void> getMmapPosition(getMmapPosition_cb cb) override {
        cb(Result::NOT_SUPPORTED, audio7_0::MmapPosition());
        return Void();
    }

    Return<Result> close() override {
        stopWriter();
        return Result::OK;
    }

    Return<uint32_t> getLatency() override { return kLatencyMs; }

    Return<Result> setVolume(float left, float right) override {
        (void)left;
        (void)right;
        return Result::OK;
    }

    Return<Result> updateSourceMetadata(
            const common7_0::SourceMetadata& sourceMetadata) override {
        (void)sourceMetadata;
        return Result::OK;
    }

    Return<void> prepareForWriting(uint32_t frameSize, uint32_t framesCount,
                                   prepareForWriting_cb cb) override {
        if (frameSize == 0 || framesCount == 0) {
            ALOGW("prepareForWriting: bad frameSize=%u framesCount=%u", frameSize, framesCount);
            cb(Result::INVALID_ARGUMENTS, ::android::hardware::MQDescriptorSync<WriteCommand>(),
               ::android::hardware::MQDescriptorSync<uint8_t>(),
               ::android::hardware::MQDescriptorSync<WriteStatus>(), 0);
            return Void();
        }
        if (mCommandMQ != nullptr) {
            ALOGW("prepareForWriting: already prepared");
            cb(Result::INVALID_STATE, ::android::hardware::MQDescriptorSync<WriteCommand>(),
               ::android::hardware::MQDescriptorSync<uint8_t>(),
               ::android::hardware::MQDescriptorSync<WriteStatus>(), 0);
            return Void();
        }
        mFrameSize = frameSize;
        mFrameCount = framesCount;
        mBufferSize = static_cast<uint64_t>(frameSize) * framesCount;
        mCommandMQ.reset(new CommandMQ(framesCount, true /*configureEventFlagWord*/));
        mDataMQ.reset(new DataMQ(mBufferSize, true /*configureEventFlagWord*/));
        mStatusMQ.reset(new StatusMQ(framesCount, true /*configureEventFlagWord*/));
        if (!mCommandMQ->isValid() || !mDataMQ->isValid() || !mStatusMQ->isValid()) {
            ALOGE("prepareForWriting: FMQ allocation failed");
            mCommandMQ.reset();
            mDataMQ.reset();
            mStatusMQ.reset();
            cb(Result::INVALID_ARGUMENTS, ::android::hardware::MQDescriptorSync<WriteCommand>(),
               ::android::hardware::MQDescriptorSync<uint8_t>(),
               ::android::hardware::MQDescriptorSync<WriteStatus>(), 0);
            return Void();
        }
        mStop.store(false);
        mThreadTid.store(0);
        if (pthread_create(&mWriterThread, nullptr, &StreamOut::writerTrampoline, this) != 0) {
            ALOGE("prepareForWriting: cannot start writer thread");
            mCommandMQ.reset();
            mDataMQ.reset();
            mStatusMQ.reset();
            cb(Result::INVALID_ARGUMENTS, ::android::hardware::MQDescriptorSync<WriteCommand>(),
               ::android::hardware::MQDescriptorSync<uint8_t>(),
               ::android::hardware::MQDescriptorSync<WriteStatus>(), 0);
            return Void();
        }
        while (mThreadTid.load() == 0) usleep(1000);
        ALOGI("prepareForWriting: frameSize=%u framesCount=%u tid=%d", frameSize, framesCount,
              mThreadTid.load());
        cb(Result::OK, *mCommandMQ->getDesc(), *mDataMQ->getDesc(), *mStatusMQ->getDesc(),
           mThreadTid.load());
        return Void();
    }

    Return<void> getRenderPosition(getRenderPosition_cb cb) override {
        cb(Result::OK, static_cast<uint32_t>(mFramesWritten.load()));
        return Void();
    }

    Return<void> getNextWriteTimestamp(getNextWriteTimestamp_cb cb) override {
        cb(Result::NOT_SUPPORTED, 0);
        return Void();
    }

    Return<Result> setCallback(const sp<audio7_0::IStreamOutCallback>& callback) override {
        (void)callback;
        return Result::NOT_SUPPORTED;
    }

    Return<Result> clearCallback() override { return Result::NOT_SUPPORTED; }

    Return<Result> setEventCallback(
            const sp<audio7_0::IStreamOutEventCallback>& callback) override {
        (void)callback;
        return Result::NOT_SUPPORTED;
    }

    Return<void> supportsPauseAndResume(supportsPauseAndResume_cb cb) override {
        cb(false, false);
        return Void();
    }

    Return<Result> pause() override { return Result::NOT_SUPPORTED; }
    Return<Result> resume() override { return Result::NOT_SUPPORTED; }
    Return<bool> supportsDrain() override { return false; }

    Return<Result> drain(audio7_0::AudioDrain type) override {
        (void)type;
        return Result::NOT_SUPPORTED;
    }

    Return<Result> flush() override { return Result::OK; }

    Return<void> getPresentationPosition(getPresentationPosition_cb cb) override {
        audio7_0::TimeSpec ts;
        ts.tvSec = 0;
        ts.tvNSec = 0;
        cb(Result::OK, mFramesWritten.load(), ts);
        return Void();
    }

    Return<Result> selectPresentation(int32_t presentationId, int32_t programId) override {
        (void)presentationId;
        (void)programId;
        return Result::NOT_SUPPORTED;
    }

    Return<void> getDualMonoMode(getDualMonoMode_cb cb) override {
        cb(Result::NOT_SUPPORTED, audio7_0::DualMonoMode::OFF);
        return Void();
    }

    Return<Result> setDualMonoMode(audio7_0::DualMonoMode mode) override {
        (void)mode;
        return Result::NOT_SUPPORTED;
    }

    Return<void> getAudioDescriptionMixLevel(getAudioDescriptionMixLevel_cb cb) override {
        cb(Result::NOT_SUPPORTED, 0.0f);
        return Void();
    }

    Return<Result> setAudioDescriptionMixLevel(float leveldB) override {
        (void)leveldB;
        return Result::NOT_SUPPORTED;
    }

    Return<void> getPlaybackRateParameters(getPlaybackRateParameters_cb cb) override {
        cb(Result::NOT_SUPPORTED, audio7_0::PlaybackRate());
        return Void();
    }

    Return<Result> setPlaybackRateParameters(
            const audio7_0::PlaybackRate& playbackRate) override {
        (void)playbackRate;
        return Result::NOT_SUPPORTED;
    }

    Return<Result> setLatencyMode(audio7_1::LatencyMode mode) override {
        (void)mode;
        return Result::NOT_SUPPORTED;
    }

    Return<void> getRecommendedLatencyModes(getRecommendedLatencyModes_cb cb) override {
        cb(Result::OK, hidl_vec<audio7_1::LatencyMode>());
        return Void();
    }

    Return<Result> setLatencyModeCallback(
            const sp<audio7_1::IStreamOutLatencyModeCallback>& callback) override {
        (void)callback;
        return Result::NOT_SUPPORTED;
    }

  private:
    static void* writerTrampoline(void* arg) {
        static_cast<StreamOut*>(arg)->writerLoop();
        return nullptr;
    }

    void stopWriter() {
        mStop.store(true);
        if (mWriterThread != 0) {
            pthread_join(mWriterThread, nullptr);
            mWriterThread = 0;
        }
    }

    void writerLoop() {
        mThreadTid.store(::gettid());
        pthread_setname_np(pthread_self(), "audiohal_out");
        std::vector<uint8_t> scratch(mDataMQ->getQuantumCount());
        bool loggedFirstWrite = false;
        while (!mStop.load()) {
            WriteCommand cmd;
            if (!mCommandMQ->readBlocking(&cmd, 1, kPollNs)) continue;
            WriteStatus status;
            status.replyTo = cmd;
            status.retval = Result::OK;
            switch (cmd) {
                case WriteCommand::WRITE: {
                    // The client writes the whole buffer to the data queue in one
                    // operation, right after the command. Wait for the first byte,
                    // then drain whatever arrived with it.
                    uint8_t first;
                    if (!mDataMQ->readBlocking(&first, 1, 1000 * 1000 * 1000LL)) {
                        ALOGW("writer: WRITE command without data");
                        status.retval = Result::INVALID_STATE;
                        status.reply.written = 0;
                        break;
                    }
                    size_t rest = mDataMQ->availableToRead();
                    if (rest > scratch.size()) rest = scratch.size();
                    size_t got = rest > 0 ? mDataMQ->read(scratch.data(), rest) : 0;
                    size_t total = 1 + got;
                    mFramesWritten.fetch_add(total / mFrameSize);
                    status.reply.written = total;
                    if (!loggedFirstWrite) {
                        ALOGI("writer: first buffer of %zu bytes accepted", total);
                        loggedFirstWrite = true;
                    }
                    break;
                }
                case WriteCommand::GET_PRESENTATION_POSITION:
                    status.reply.presentationPosition.frames = mFramesWritten.load();
                    status.reply.presentationPosition.timeStamp.tvSec = 0;
                    status.reply.presentationPosition.timeStamp.tvNSec = 0;
                    break;
                case WriteCommand::GET_LATENCY:
                    status.reply.latencyMs = kLatencyMs;
                    break;
                default:
                    status.retval = Result::INVALID_ARGUMENTS;
                    break;
            }
            while (!mStop.load() && !mStatusMQ->writeBlocking(&status, 1, kPollNs)) {
            }
        }
    }

    common7_0::DeviceAddress mDevice;
    common7_0::AudioConfig mConfig;
    uint64_t mFrameSize = 4;
    uint64_t mFrameCount = kDefaultFrameCount;
    uint64_t mBufferSize = 4 * kDefaultFrameCount;

    std::unique_ptr<CommandMQ> mCommandMQ;
    std::unique_ptr<DataMQ> mDataMQ;
    std::unique_ptr<StatusMQ> mStatusMQ;
    pthread_t mWriterThread = 0;
    std::atomic<int32_t> mThreadTid{0};
    std::atomic<bool> mStop{false};
    std::atomic<uint64_t> mFramesWritten{0};
};

// ---------------------------------------------------------------------------
// Input stream: returns silence through the FMQ transport.
// ---------------------------------------------------------------------------

class StreamIn : public audio7_0::IStreamIn {
  public:
    StreamIn(const common7_0::DeviceAddress& device, const common7_0::AudioConfig& config)
        : mDevice(effectiveDevice(device, "AUDIO_DEVICE_IN_BUILTIN_MIC")),
          mConfig(effectiveConfig(config)) {
        if (mConfig.base.channelMask.empty() ||
            mConfig.base.channelMask == hidl_string("AUDIO_CHANNEL_OUT_STEREO")) {
            mConfig.base.channelMask = "AUDIO_CHANNEL_IN_STEREO";
        }
        mFrameSize = bytesPerSample(mConfig.base.format) *
                     channelCount(mConfig.base.channelMask);
        mFrameCount = mConfig.frameCount;
        mBufferSize = mFrameSize * mFrameCount;
    }

    ~StreamIn() override { stopReader(); }

    const common7_0::AudioConfig& config() const { return mConfig; }

    Return<uint64_t> getFrameSize() override { return mFrameSize; }
    Return<uint64_t> getFrameCount() override { return mFrameCount; }
    Return<uint64_t> getBufferSize() override { return mBufferSize; }

    Return<void> getSupportedProfiles(getSupportedProfiles_cb cb) override {
        common7_0::AudioProfile profile;
        profile.format = "AUDIO_FORMAT_PCM_16_BIT";
        profile.sampleRates.resize(2);
        profile.sampleRates[0] = 44100;
        profile.sampleRates[1] = 48000;
        profile.channelMasks.resize(1);
        profile.channelMasks[0] = "AUDIO_CHANNEL_IN_STEREO";
        hidl_vec<common7_0::AudioProfile> profiles;
        profiles.resize(1);
        profiles[0] = profile;
        cb(Result::OK, profiles);
        return Void();
    }

    Return<void> getAudioProperties(getAudioProperties_cb cb) override {
        cb(Result::OK, mConfig.base);
        return Void();
    }

    Return<Result> setAudioProperties(
            const common7_0::AudioConfigBaseOptional& config) override {
        (void)config;
        return Result::NOT_SUPPORTED;
    }

    Return<Result> addEffect(uint64_t effectId) override {
        (void)effectId;
        return Result::NOT_SUPPORTED;
    }

    Return<Result> removeEffect(uint64_t effectId) override {
        (void)effectId;
        return Result::NOT_SUPPORTED;
    }

    Return<Result> standby() override { return Result::OK; }

    Return<void> getDevices(getDevices_cb cb) override {
        hidl_vec<common7_0::DeviceAddress> devices;
        devices.resize(1);
        devices[0] = mDevice;
        cb(Result::OK, devices);
        return Void();
    }

    Return<Result> setDevices(const hidl_vec<common7_0::DeviceAddress>& devices) override {
        if (devices.size() > 0) mDevice = devices[0];
        return Result::OK;
    }

    Return<Result> setHwAvSync(uint32_t hwAvSync) override {
        (void)hwAvSync;
        return Result::NOT_SUPPORTED;
    }

    Return<void> getParameters(const hidl_vec<audio7_0::ParameterValue>& context,
                               const hidl_vec<hidl_string>& keys,
                               getParameters_cb cb) override {
        (void)context;
        hidl_vec<audio7_0::ParameterValue> values;
        values.resize(keys.size());
        for (size_t i = 0; i < keys.size(); ++i) {
            values[i].key = keys[i];
            values[i].value = "";
        }
        cb(Result::OK, values);
        return Void();
    }

    Return<Result> setParameters(const hidl_vec<audio7_0::ParameterValue>& context,
                                 const hidl_vec<audio7_0::ParameterValue>& parameters) override {
        (void)context;
        (void)parameters;
        return Result::OK;
    }

    Return<Result> start() override { return Result::OK; }
    Return<Result> stop() override { return Result::OK; }

    Return<void> createMmapBuffer(int32_t minSizeFrames, createMmapBuffer_cb cb) override {
        (void)minSizeFrames;
        cb(Result::NOT_SUPPORTED, audio7_0::MmapBufferInfo());
        return Void();
    }

    Return<void> getMmapPosition(getMmapPosition_cb cb) override {
        cb(Result::NOT_SUPPORTED, audio7_0::MmapPosition());
        return Void();
    }

    Return<Result> close() override {
        stopReader();
        return Result::OK;
    }

    Return<void> getAudioSource(getAudioSource_cb cb) override {
        cb(Result::OK, hidl_string("MIC"));
        return Void();
    }

    Return<Result> setGain(float gain) override {
        (void)gain;
        return Result::OK;
    }

    Return<Result> updateSinkMetadata(const common7_0::SinkMetadata& sinkMetadata) override {
        (void)sinkMetadata;
        return Result::OK;
    }

    Return<void> prepareForReading(uint32_t frameSize, uint32_t framesCount,
                                   prepareForReading_cb cb) override {
        if (frameSize == 0 || framesCount == 0) {
            ALOGW("prepareForReading: bad frameSize=%u framesCount=%u", frameSize, framesCount);
            cb(Result::INVALID_ARGUMENTS, ::android::hardware::MQDescriptorSync<ReadParameters>(),
               ::android::hardware::MQDescriptorSync<uint8_t>(),
               ::android::hardware::MQDescriptorSync<ReadStatus>(), 0);
            return Void();
        }
        if (mCommandMQ != nullptr) {
            ALOGW("prepareForReading: already prepared");
            cb(Result::INVALID_STATE, ::android::hardware::MQDescriptorSync<ReadParameters>(),
               ::android::hardware::MQDescriptorSync<uint8_t>(),
               ::android::hardware::MQDescriptorSync<ReadStatus>(), 0);
            return Void();
        }
        mFrameSize = frameSize;
        mFrameCount = framesCount;
        mBufferSize = static_cast<uint64_t>(frameSize) * framesCount;
        mCommandMQ.reset(new ReadCommandMQ(framesCount, true /*configureEventFlagWord*/));
        mDataMQ.reset(new DataMQ(mBufferSize, true /*configureEventFlagWord*/));
        mStatusMQ.reset(new ReadStatusMQ(framesCount, true /*configureEventFlagWord*/));
        if (!mCommandMQ->isValid() || !mDataMQ->isValid() || !mStatusMQ->isValid()) {
            ALOGE("prepareForReading: FMQ allocation failed");
            mCommandMQ.reset();
            mDataMQ.reset();
            mStatusMQ.reset();
            cb(Result::INVALID_ARGUMENTS, ::android::hardware::MQDescriptorSync<ReadParameters>(),
               ::android::hardware::MQDescriptorSync<uint8_t>(),
               ::android::hardware::MQDescriptorSync<ReadStatus>(), 0);
            return Void();
        }
        mStop.store(false);
        mThreadTid.store(0);
        if (pthread_create(&mReaderThread, nullptr, &StreamIn::readerTrampoline, this) != 0) {
            ALOGE("prepareForReading: cannot start reader thread");
            mCommandMQ.reset();
            mDataMQ.reset();
            mStatusMQ.reset();
            cb(Result::INVALID_ARGUMENTS, ::android::hardware::MQDescriptorSync<ReadParameters>(),
               ::android::hardware::MQDescriptorSync<uint8_t>(),
               ::android::hardware::MQDescriptorSync<ReadStatus>(), 0);
            return Void();
        }
        while (mThreadTid.load() == 0) usleep(1000);
        ALOGI("prepareForReading: frameSize=%u framesCount=%u tid=%d", frameSize, framesCount,
              mThreadTid.load());
        cb(Result::OK, *mCommandMQ->getDesc(), *mDataMQ->getDesc(), *mStatusMQ->getDesc(),
           mThreadTid.load());
        return Void();
    }

    Return<uint32_t> getInputFramesLost() override { return 0; }

    Return<void> getCapturePosition(getCapturePosition_cb cb) override {
        cb(Result::OK, mFramesRead.load(), 0);
        return Void();
    }

    Return<void> getActiveMicrophones(getActiveMicrophones_cb cb) override {
        cb(Result::NOT_SUPPORTED, hidl_vec<audio7_0::MicrophoneInfo>());
        return Void();
    }

    Return<Result> setMicrophoneDirection(audio7_0::MicrophoneDirection direction) override {
        (void)direction;
        return Result::NOT_SUPPORTED;
    }

    Return<Result> setMicrophoneFieldDimension(float zoom) override {
        (void)zoom;
        return Result::NOT_SUPPORTED;
    }

  private:
    static void* readerTrampoline(void* arg) {
        static_cast<StreamIn*>(arg)->readerLoop();
        return nullptr;
    }

    void stopReader() {
        mStop.store(true);
        if (mReaderThread != 0) {
            pthread_join(mReaderThread, nullptr);
            mReaderThread = 0;
        }
    }

    void readerLoop() {
        mThreadTid.store(::gettid());
        pthread_setname_np(pthread_self(), "audiohal_in");
        std::vector<uint8_t> silence(mDataMQ->getQuantumCount(), 0);
        bool loggedFirstRead = false;
        while (!mStop.load()) {
            ReadParameters cmd;
            if (!mCommandMQ->readBlocking(&cmd, 1, kPollNs)) continue;
            ReadStatus status;
            status.replyTo = cmd.command;
            status.retval = Result::OK;
            switch (cmd.command) {
                case audio7_0::IStreamIn::ReadCommand::READ: {
                    if (!loggedFirstRead) {
                        ALOGI("reader: first READ of %llu bytes",
                              static_cast<unsigned long long>(cmd.params.read));
                        loggedFirstRead = true;
                    }
                    uint64_t remaining = cmd.params.read;
                    bool ok = true;
                    while (remaining > 0 && !mStop.load()) {
                        size_t chunk = remaining < silence.size()
                                               ? static_cast<size_t>(remaining)
                                               : silence.size();
                        if (!mDataMQ->writeBlocking(silence.data(), chunk, kPollNs)) {
                            ok = false;
                            break;
                        }
                        remaining -= chunk;
                        mFramesRead.fetch_add(chunk / mFrameSize);
                    }
                    status.retval = ok ? Result::OK : Result::INVALID_STATE;
                    status.reply.read = ok ? cmd.params.read : 0;
                    break;
                }
                case audio7_0::IStreamIn::ReadCommand::GET_CAPTURE_POSITION:
                    status.reply.capturePosition.frames = mFramesRead.load();
                    status.reply.capturePosition.time = 0;
                    break;
                default:
                    status.retval = Result::INVALID_ARGUMENTS;
                    break;
            }
            while (!mStop.load() && !mStatusMQ->writeBlocking(&status, 1, kPollNs)) {
            }
        }
    }

    common7_0::DeviceAddress mDevice;
    common7_0::AudioConfig mConfig;
    uint64_t mFrameSize = 4;
    uint64_t mFrameCount = kDefaultFrameCount;
    uint64_t mBufferSize = 4 * kDefaultFrameCount;

    std::unique_ptr<ReadCommandMQ> mCommandMQ;
    std::unique_ptr<DataMQ> mDataMQ;
    std::unique_ptr<ReadStatusMQ> mStatusMQ;
    pthread_t mReaderThread = 0;
    std::atomic<int32_t> mThreadTid{0};
    std::atomic<bool> mStop{false};
    std::atomic<uint64_t> mFramesRead{0};
};

// ---------------------------------------------------------------------------
// Device
// ---------------------------------------------------------------------------

class Device : public audio7_1::IDevice {
  public:
    Return<Result> initCheck() override { return Result::OK; }

    Return<Result> setMasterVolume(float volume) override {
        (void)volume;
        return Result::NOT_SUPPORTED;
    }

    Return<void> getMasterVolume(getMasterVolume_cb cb) override {
        cb(Result::NOT_SUPPORTED, 0.0f);
        return Void();
    }

    Return<Result> setMicMute(bool mute) override {
        (void)mute;
        return Result::NOT_SUPPORTED;
    }

    Return<void> getMicMute(getMicMute_cb cb) override {
        cb(Result::NOT_SUPPORTED, false);
        return Void();
    }

    Return<Result> setMasterMute(bool mute) override {
        (void)mute;
        return Result::NOT_SUPPORTED;
    }

    Return<void> getMasterMute(getMasterMute_cb cb) override {
        cb(Result::NOT_SUPPORTED, false);
        return Void();
    }

    Return<void> getInputBufferSize(const common7_0::AudioConfig& config,
                                    getInputBufferSize_cb cb) override {
        // 20 ms of stereo 16-bit at the requested rate.
        uint32_t rate = config.base.sampleRateHz ? config.base.sampleRateHz : kDefaultSampleRate;
        cb(Result::OK, static_cast<uint64_t>(4) * (rate / 50));
        return Void();
    }

    Return<void> openOutputStream(int32_t ioHandle, const common7_0::DeviceAddress& device,
                                  const common7_0::AudioConfig& config,
                                  const hidl_vec<hidl_string>& flags,
                                  const common7_0::SourceMetadata& sourceMetadata,
                                  openOutputStream_cb cb) override {
        (void)ioHandle;
        (void)flags;
        (void)sourceMetadata;
        ALOGI("openOutputStream: device=%s rate=%u format=%s", device.deviceType.c_str(),
              config.base.sampleRateHz, config.base.format.c_str());
        sp<StreamOut> stream = new StreamOut(device, config);
        sp<audio7_0::IStreamOut> out = stream;
        cb(Result::OK, out, stream->config());
        return Void();
    }

    Return<void> openInputStream(int32_t ioHandle, const common7_0::DeviceAddress& device,
                                 const common7_0::AudioConfig& config,
                                 const hidl_vec<hidl_string>& flags,
                                 const common7_0::SinkMetadata& sinkMetadata,
                                 openInputStream_cb cb) override {
        (void)ioHandle;
        (void)flags;
        (void)sinkMetadata;
        ALOGI("openInputStream: device=%s rate=%u format=%s", device.deviceType.c_str(),
              config.base.sampleRateHz, config.base.format.c_str());
        sp<StreamIn> stream = new StreamIn(device, config);
        sp<audio7_0::IStreamIn> in = stream;
        cb(Result::OK, in, stream->config());
        return Void();
    }

    Return<bool> supportsAudioPatches() override { return false; }

    Return<void> createAudioPatch(const hidl_vec<common7_0::AudioPortConfig>& sources,
                                  const hidl_vec<common7_0::AudioPortConfig>& sinks,
                                  createAudioPatch_cb cb) override {
        (void)sources;
        (void)sinks;
        cb(Result::NOT_SUPPORTED, 0);
        return Void();
    }

    Return<void> updateAudioPatch(int32_t previousPatch,
                                  const hidl_vec<common7_0::AudioPortConfig>& sources,
                                  const hidl_vec<common7_0::AudioPortConfig>& sinks,
                                  updateAudioPatch_cb cb) override {
        (void)previousPatch;
        (void)sources;
        (void)sinks;
        cb(Result::NOT_SUPPORTED, 0);
        return Void();
    }

    Return<Result> releaseAudioPatch(int32_t patch) override {
        (void)patch;
        return Result::NOT_SUPPORTED;
    }

    // The framework only asks about ports it already knows from the policy
    // configuration, so echoing the descriptor back is enough.
    Return<void> getAudioPort(const common7_0::AudioPort& port, getAudioPort_cb cb) override {
        cb(Result::OK, port);
        return Void();
    }

    Return<Result> setAudioPortConfig(const common7_0::AudioPortConfig& config) override {
        (void)config;
        return Result::OK;
    }

    Return<void> getHwAvSync(getHwAvSync_cb cb) override {
        cb(Result::NOT_SUPPORTED, 0);
        return Void();
    }

    Return<Result> setScreenState(bool turnedOn) override {
        (void)turnedOn;
        return Result::OK;
    }

    Return<void> getParameters(const hidl_vec<audio7_0::ParameterValue>& context,
                               const hidl_vec<hidl_string>& keys,
                               getParameters_cb cb) override {
        (void)context;
        // The contract says OK for an empty key list; anything else is unknown.
        cb(keys.size() == 0 ? Result::OK : Result::NOT_SUPPORTED,
           hidl_vec<audio7_0::ParameterValue>());
        return Void();
    }

    Return<Result> setParameters(const hidl_vec<audio7_0::ParameterValue>& context,
                                 const hidl_vec<audio7_0::ParameterValue>& parameters) override {
        (void)context;
        (void)parameters;
        return Result::OK;
    }

    Return<void> getMicrophones(getMicrophones_cb cb) override {
        cb(Result::NOT_SUPPORTED, hidl_vec<audio7_0::MicrophoneInfo>());
        return Void();
    }

    Return<Result> setConnectedState(const common7_0::DeviceAddress& address,
                                     bool connected) override {
        (void)address;
        (void)connected;
        return Result::OK;
    }

    Return<Result> close() override { return Result::OK; }

    Return<Result> addDeviceEffect(int32_t device, uint64_t effectId) override {
        (void)device;
        (void)effectId;
        return Result::NOT_SUPPORTED;
    }

    Return<Result> removeDeviceEffect(int32_t device, uint64_t effectId) override {
        (void)device;
        (void)effectId;
        return Result::NOT_SUPPORTED;
    }

    Return<void> openOutputStream_7_1(int32_t ioHandle, const common7_0::DeviceAddress& device,
                                      const common7_0::AudioConfig& config,
                                      const hidl_vec<hidl_string>& flags,
                                      const common7_0::SourceMetadata& sourceMetadata,
                                      openOutputStream_7_1_cb cb) override {
        (void)ioHandle;
        (void)flags;
        (void)sourceMetadata;
        ALOGI("openOutputStream_7_1: device=%s rate=%u format=%s", device.deviceType.c_str(),
              config.base.sampleRateHz, config.base.format.c_str());
        sp<StreamOut> stream = new StreamOut(device, config);
        sp<audio7_1::IStreamOut> out = stream;
        cb(Result::OK, out, stream->config());
        return Void();
    }

    Return<Result> setConnectedState_7_1(const common7_0::AudioPort& devicePort,
                                         bool connected) override {
        (void)devicePort;
        (void)connected;
        return Result::OK;
    }
};

class DevicesFactory : public audio7_1::IDevicesFactory {
  public:
    Return<void> openDevice(const hidl_string& device, openDevice_cb cb) override {
        ALOGI("openDevice(%s)", device.c_str());
        if (strcmp(device.c_str(), "primary") == 0) {
            sp<audio7_0::IDevice> dev = new Device();
            cb(Result::OK, dev);
        } else {
            ALOGW("openDevice(%s): unsupported module", device.c_str());
            cb(Result::INVALID_ARGUMENTS, nullptr);
        }
        return Void();
    }

    Return<void> openPrimaryDevice(openPrimaryDevice_cb cb) override {
        ALOGW("openPrimaryDevice: no primary-device interface in this bring-up");
        cb(Result::INVALID_ARGUMENTS, nullptr);
        return Void();
    }

    Return<void> openDevice_7_1(const hidl_string& device, openDevice_7_1_cb cb) override {
        ALOGI("openDevice_7_1(%s)", device.c_str());
        if (strcmp(device.c_str(), "primary") == 0) {
            sp<audio7_1::IDevice> dev = new Device();
            cb(Result::OK, dev);
        } else {
            ALOGW("openDevice_7_1(%s): unsupported module", device.c_str());
            cb(Result::INVALID_ARGUMENTS, nullptr);
        }
        return Void();
    }

    Return<void> openPrimaryDevice_7_1(openPrimaryDevice_7_1_cb cb) override {
        ALOGW("openPrimaryDevice_7_1: no primary-device interface in this bring-up");
        cb(Result::INVALID_ARGUMENTS, nullptr);
        return Void();
    }
};

class EffectsFactory : public effect7_0::IEffectsFactory {
  public:
    Return<void> getAllDescriptors(getAllDescriptors_cb cb) override {
        cb(effect7_0::Result::NOT_SUPPORTED, hidl_vec<effect7_0::EffectDescriptor>());
        return Void();
    }

    Return<void> getDescriptor(const ::android::hardware::audio::common::V7_0::Uuid& uid,
                               getDescriptor_cb cb) override {
        (void)uid;
        cb(effect7_0::Result::INVALID_ARGUMENTS, effect7_0::EffectDescriptor());
        return Void();
    }

    Return<void> createEffect(const ::android::hardware::audio::common::V7_0::Uuid& uid,
                              int32_t session, int32_t ioHandle, int32_t device,
                              createEffect_cb cb) override {
        (void)uid;
        (void)session;
        (void)ioHandle;
        (void)device;
        cb(effect7_0::Result::INVALID_ARGUMENTS, nullptr, 0);
        return Void();
    }
};

}  // namespace

// hwservicemanager may not be up yet when early_hal services start, and
// registerAsService() only succeeds once it is.
template <typename T>
static ::android::status_t registerWithRetry(const sp<T>& service, const char* name) {
    ::android::status_t status = ::android::INVALID_OPERATION;
    for (int i = 0; i < 100; ++i) {
        status = service->registerAsService("default");
        if (status == ::android::OK) return status;
        usleep(100 * 1000);
    }
    ALOGE("cannot register %s: %d", name, status);
    return status;
}

int main() {
    // Two threads: one serves the audio factory, one the effect factory.
    ::android::hardware::configureRpcThreadpool(2, true /* callerWillJoin */);

    sp<audio7_1::IDevicesFactory> devices = new DevicesFactory();
    if (registerWithRetry(devices, "android.hardware.audio@7.1::IDevicesFactory") !=
        ::android::OK) {
        return 1;
    }

    sp<effect7_0::IEffectsFactory> effects = new EffectsFactory();
    if (registerWithRetry(effects, "android.hardware.audio.effect@7.0::IEffectsFactory") !=
        ::android::OK) {
        return 1;
    }

    ALOGI("HIDL audio HAL registered");
    ::android::hardware::joinRpcThreadpool();
    return 0;
}
