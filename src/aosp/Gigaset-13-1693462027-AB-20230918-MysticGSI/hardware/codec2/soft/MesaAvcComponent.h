#pragma once

#include <aidl/android/hardware/media/c2/BnComponent.h>
#include <aidl/android/hardware/media/c2/IComponentListener.h>

#include "mesa_pipe_video.h"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace gki {

class ComponentInterface;
class Configurable;

enum class MesaCodecKind { kDecoder, kEncoder };

class MesaAvcComponent : public aidl::android::hardware::media::c2::BnComponent {
public:
    MesaAvcComponent(MesaCodecKind kind, std::string name,
                     std::shared_ptr<ComponentInterface> intf,
                     std::shared_ptr<Configurable> cfg,
                     std::shared_ptr<aidl::android::hardware::media::c2::IComponentListener>
                             listener);
    ~MesaAvcComponent() override;

    ndk::ScopedAStatus configureVideoTunnel(
            int32_t avSyncHwId,
            aidl::android::hardware::common::NativeHandle* _aidl_return) override;
    ndk::ScopedAStatus createBlockPool(
            const aidl::android::hardware::media::c2::IComponent::BlockPoolAllocator& allocator,
            aidl::android::hardware::media::c2::IComponent::BlockPool* _aidl_return) override;
    ndk::ScopedAStatus destroyBlockPool(int64_t blockPoolId) override;
    ndk::ScopedAStatus drain(bool withEos) override;
    ndk::ScopedAStatus flush(
            aidl::android::hardware::media::c2::WorkBundle* _aidl_return) override;
    ndk::ScopedAStatus getInterface(
            std::shared_ptr<aidl::android::hardware::media::c2::IComponentInterface>*
                    _aidl_return) override;
    ndk::ScopedAStatus queue(const aidl::android::hardware::media::c2::WorkBundle& workBundle)
            override;
    ndk::ScopedAStatus release() override;
    ndk::ScopedAStatus reset() override;
    ndk::ScopedAStatus start() override;
    ndk::ScopedAStatus stop() override;
    ndk::ScopedAStatus connectToInputSurface(
            const std::shared_ptr<aidl::android::hardware::media::c2::IInputSurface>& inputSurface,
            std::shared_ptr<aidl::android::hardware::media::c2::IInputSurfaceConnection>*
                    _aidl_return) override;
    ndk::ScopedAStatus asInputSink(
            std::shared_ptr<aidl::android::hardware::media::c2::IInputSink>* _aidl_return)
            override;

private:
    struct PendingWork {
        aidl::android::hardware::media::c2::Work work;
        std::vector<aidl::android::hardware::media::c2::BaseBlock> baseBlocks;
    };

    // Encoder pipeline: the worker submits frames, the collector completes
    // them in input order (encoded, failed or dropped).
    struct InFlight {
        enum class Kind { kEncoded, kDone, kDropped } kind = Kind::kDone;
        PendingWork pending;
        mesa_video::EncodeTicket ticket;
    };

    void workerLoop();
    void collectorLoop();
    void processWork(PendingWork* pending);
    bool submitEncode(PendingWork* pending, mesa_video::EncodeTicket* ticket);
    void finishEncode(PendingWork* pending, int rc, const std::vector<uint8_t>& bs,
                      int64_t pts, uint64_t index);
    void emitWork(PendingWork* pending);
    static void markDropped(PendingWork* pending);
    static std::vector<uint8_t> extractLinearData(
            const aidl::android::hardware::media::c2::Buffer& buf,
            const std::vector<aidl::android::hardware::media::c2::BaseBlock>& baseBlocks);

    const MesaCodecKind mKind;
    const std::string mName;
    std::shared_ptr<ComponentInterface> mIntf;
    std::shared_ptr<Configurable> mCfg;
    std::shared_ptr<aidl::android::hardware::media::c2::IComponentListener> mListener;

    std::mutex mLock;
    std::condition_variable mCv;
    std::vector<PendingWork> mPending;
    std::vector<PendingWork> mDropped;
    std::atomic<bool> mRunning{false};
    std::atomic<bool> mStarted{false};
    std::atomic<bool> mReleased{false};
    std::thread mWorker;
    std::thread mCollector;
    std::deque<InFlight> mInFlight;   // guarded by mLock
    unsigned mEncodedInFlight = 0;    // kEncoded entries in mInFlight
    bool mWorkerDone = false;
    std::condition_variable mInFlightCv;

    mesa_video::Session mSession;
    int64_t mNextBlockPoolId = 1;
};

}  // namespace gki
