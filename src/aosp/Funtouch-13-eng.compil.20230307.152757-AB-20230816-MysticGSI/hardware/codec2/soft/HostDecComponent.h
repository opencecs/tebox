#pragma once

#include <aidl/android/hardware/media/c2/BnComponent.h>
#include <aidl/android/hardware/media/c2/IComponentListener.h>
#include <aidl/android/hardware/media/c2/IGraphicBufferAllocator.h>

#include <android/hardware_buffer.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace gki {

class ComponentInterface;
class Configurable;

// Video decoder backed by the host hardware decoder (gfxstream backend).
// The bitstream goes to tebox-cast over the virtio-serial port tebox.vdec;
// output buffers are RGBA virtio-gpu resources that QEMU fills on the GPU
// (CAST_VDEC_* in src/service/cast/include/cast/protocol.h, mirrored in the
// .cpp). No picture is decoded or copied in the guest.
class HostDecComponent : public aidl::android::hardware::media::c2::BnComponent {
public:
    HostDecComponent(std::string name, uint32_t codec, std::shared_ptr<ComponentInterface> intf,
                     std::shared_ptr<Configurable> cfg,
                     std::shared_ptr<aidl::android::hardware::media::c2::IComponentListener>
                             listener);
    ~HostDecComponent() override;

    // True when the guest has a tebox.vdec port (the host decodes).
    static bool available();

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
    // An output buffer lent to the host.
    struct Target {
        AHardwareBuffer* buffer = nullptr;  // one reference held
        uint64_t bufferId = 0;              // AHardwareBuffer_getId, for deallocate()
        bool fromPool = false;              // from the client's allocator
        uint32_t width = 0, height = 0;
    };

    void workerLoop();
    static void portReader(int fd);
    bool provideTargets();
    bool allocateTarget(Target* target);
    void releaseTarget(Target* target, bool returnToPool);
    void onPicture(const void* payload);
    void finishWork(PendingWork* pending, Target* target, uint32_t width, uint32_t height,
                    bool eos);
    void finishStale(uint64_t upToIndex);
    void emit(std::vector<PendingWork>* works);
    void closeSession();

    const std::string mName;
    const uint32_t mCodec;  // V4L2 fourcc
    std::shared_ptr<ComponentInterface> mIntf;
    std::shared_ptr<Configurable> mCfg;
    std::shared_ptr<aidl::android::hardware::media::c2::IComponentListener> mListener;

    std::mutex mLock;  // guards everything below
    std::condition_variable mCv;
    std::deque<PendingWork> mQueued;              // not yet sent to the host
    std::map<uint64_t, PendingWork> mAwaiting;    // sent, waiting for a picture
    std::map<uint64_t, Target> mTargets;          // lent to the host, by id
    std::shared_ptr<aidl::android::hardware::media::c2::IGraphicBufferAllocator> mIgba;
    int mWaitableFd = -1;
    std::vector<uint8_t> mCsd;                    // prepended to the next frame
    std::vector<uint8_t> mSavedCsd;               // the stream's codec config
    uint64_t mNextTargetId = 1;
    uint32_t mWidth = 0, mHeight = 0;             // last picture size sent out
    bool mFlushed = false;
    bool mStarted = false;
    bool mRunning = false;
    bool mHostFailed = false;
    uint32_t mSession = 0;                        // id on the shared tebox.vdec port
    std::thread mWorker;
    int64_t mNextBlockPoolId = 1;
};

}  // namespace gki
