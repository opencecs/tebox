#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_MAPPER_V2_0_HWTYPES_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_MAPPER_V2_0_HWTYPES_H

#include <android/hardware/graphics/mapper/2.0/types.h>


#include <hidl/Status.h>
#include <hwbinder/IBinder.h>
#include <hwbinder/Parcel.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_0 {
::android::status_t readEmbeddedFromParcel(
        const ::android::hardware::graphics::mapper::V2_0::YCbCrLayout &obj,
        const ::android::hardware::Parcel &parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t writeEmbeddedToParcel(
        const ::android::hardware::graphics::mapper::V2_0::YCbCrLayout &obj,
        ::android::hardware::Parcel *parcel,
        size_t parentHandle,
        size_t parentOffset);

}  // namespace V2_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_MAPPER_V2_0_HWTYPES_H
