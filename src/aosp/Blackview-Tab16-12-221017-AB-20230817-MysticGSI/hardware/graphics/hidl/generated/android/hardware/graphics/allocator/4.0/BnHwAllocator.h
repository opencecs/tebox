#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_ALLOCATOR_V4_0_BNHWALLOCATOR_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_ALLOCATOR_V4_0_BNHWALLOCATOR_H

#include <android/hardware/graphics/allocator/4.0/IHwAllocator.h>

namespace android {
namespace hardware {
namespace graphics {
namespace allocator {
namespace V4_0 {

struct BnHwAllocator : public ::android::hidl::base::V1_0::BnHwBase {
    explicit BnHwAllocator(const ::android::sp<IAllocator> &_hidl_impl);
    explicit BnHwAllocator(const ::android::sp<IAllocator> &_hidl_impl, const std::string& HidlInstrumentor_package, const std::string& HidlInstrumentor_interface);

    virtual ~BnHwAllocator();

    ::android::status_t onTransact(
            uint32_t _hidl_code,
            const ::android::hardware::Parcel &_hidl_data,
            ::android::hardware::Parcel *_hidl_reply,
            uint32_t _hidl_flags = 0,
            TransactCallback _hidl_cb = nullptr) override;


    /**
     * The pure class is what this class wraps.
     */
    typedef IAllocator Pure;

    /**
     * Type tag for use in template logic that indicates this is a 'native' class.
     */
    typedef android::hardware::details::bnhw_tag _hidl_tag;

    ::android::sp<IAllocator> getImpl() { return _hidl_mImpl; }
    // Methods from ::android::hardware::graphics::allocator::V4_0::IAllocator follow.
    static ::android::status_t _hidl_allocate(
            ::android::hidl::base::V1_0::BnHwBase* _hidl_this,
            const ::android::hardware::Parcel &_hidl_data,
            ::android::hardware::Parcel *_hidl_reply,
            TransactCallback _hidl_cb);



private:
    // Methods from ::android::hardware::graphics::allocator::V4_0::IAllocator follow.

    // Methods from ::android::hidl::base::V1_0::IBase follow.
    ::android::hardware::Return<void> ping();
    using getDebugInfo_cb = ::android::hidl::base::V1_0::IBase::getDebugInfo_cb;
    ::android::hardware::Return<void> getDebugInfo(getDebugInfo_cb _hidl_cb);

    ::android::sp<IAllocator> _hidl_mImpl;
};

}  // namespace V4_0
}  // namespace allocator
}  // namespace graphics
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_ALLOCATOR_V4_0_BNHWALLOCATOR_H
