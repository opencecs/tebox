#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_ALLOCATOR_V4_0_BPHWALLOCATOR_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_ALLOCATOR_V4_0_BPHWALLOCATOR_H

#include <hidl/HidlTransportSupport.h>

#include <android/hardware/graphics/allocator/4.0/IHwAllocator.h>

namespace android {
namespace hardware {
namespace graphics {
namespace allocator {
namespace V4_0 {

struct BpHwAllocator : public ::android::hardware::BpInterface<IAllocator>, public ::android::hardware::details::HidlInstrumentor {
    explicit BpHwAllocator(const ::android::sp<::android::hardware::IBinder> &_hidl_impl);

    /**
     * The pure class is what this class wraps.
     */
    typedef IAllocator Pure;

    /**
     * Type tag for use in template logic that indicates this is a 'proxy' class.
     */
    typedef android::hardware::details::bphw_tag _hidl_tag;

    virtual bool isRemote() const override { return true; }

    // Methods from ::android::hardware::graphics::allocator::V4_0::IAllocator follow.
    static ::android::hardware::Return<void>  _hidl_allocate(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, const ::android::hardware::hidl_vec<uint8_t>& descriptor, uint32_t count, allocate_cb _hidl_cb);

    // Methods from ::android::hardware::graphics::allocator::V4_0::IAllocator follow.
    ::android::hardware::Return<void> allocate(const ::android::hardware::hidl_vec<uint8_t>& descriptor, uint32_t count, allocate_cb _hidl_cb) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.
    ::android::hardware::Return<void> interfaceChain(interfaceChain_cb _hidl_cb) override;
    ::android::hardware::Return<void> debug(const ::android::hardware::hidl_handle& fd, const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& options) override;
    ::android::hardware::Return<void> interfaceDescriptor(interfaceDescriptor_cb _hidl_cb) override;
    ::android::hardware::Return<void> getHashChain(getHashChain_cb _hidl_cb) override;
    ::android::hardware::Return<void> setHALInstrumentation() override;
    ::android::hardware::Return<bool> linkToDeath(const ::android::sp<::android::hardware::hidl_death_recipient>& recipient, uint64_t cookie) override;
    ::android::hardware::Return<void> ping() override;
    ::android::hardware::Return<void> getDebugInfo(getDebugInfo_cb _hidl_cb) override;
    ::android::hardware::Return<void> notifySyspropsChanged() override;
    ::android::hardware::Return<bool> unlinkToDeath(const ::android::sp<::android::hardware::hidl_death_recipient>& recipient) override;

private:
    std::mutex _hidl_mMutex;
    std::vector<::android::sp<::android::hardware::hidl_binder_death_recipient>> _hidl_mDeathRecipients;
};

}  // namespace V4_0
}  // namespace allocator
}  // namespace graphics
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_ALLOCATOR_V4_0_BPHWALLOCATOR_H
