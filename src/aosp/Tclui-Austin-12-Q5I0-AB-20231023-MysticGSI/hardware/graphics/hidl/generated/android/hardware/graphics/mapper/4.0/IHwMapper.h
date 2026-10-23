#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_MAPPER_V4_0_IHWMAPPER_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_MAPPER_V4_0_IHWMAPPER_H

#include <android/hardware/graphics/mapper/4.0/IMapper.h>

#include <android/hardware/graphics/common/1.2/hwtypes.h>
#include <android/hardware/graphics/mapper/4.0/hwtypes.h>
#include <android/hidl/base/1.0/BnHwBase.h>
#include <android/hidl/base/1.0/BpHwBase.h>

#include <hidl/Status.h>
#include <hwbinder/IBinder.h>
#include <hwbinder/Parcel.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V4_0 {
::android::status_t readEmbeddedFromParcel(
        const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo &obj,
        const ::android::hardware::Parcel &parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t writeEmbeddedToParcel(
        const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo &obj,
        ::android::hardware::Parcel *parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t readEmbeddedFromParcel(
        const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType &obj,
        const ::android::hardware::Parcel &parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t writeEmbeddedToParcel(
        const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType &obj,
        ::android::hardware::Parcel *parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t readEmbeddedFromParcel(
        const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataTypeDescription &obj,
        const ::android::hardware::Parcel &parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t writeEmbeddedToParcel(
        const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataTypeDescription &obj,
        ::android::hardware::Parcel *parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t readEmbeddedFromParcel(
        const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataDump &obj,
        const ::android::hardware::Parcel &parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t writeEmbeddedToParcel(
        const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataDump &obj,
        ::android::hardware::Parcel *parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t readEmbeddedFromParcel(
        const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDump &obj,
        const ::android::hardware::Parcel &parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t writeEmbeddedToParcel(
        const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDump &obj,
        ::android::hardware::Parcel *parcel,
        size_t parentHandle,
        size_t parentOffset);

}  // namespace V4_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_MAPPER_V4_0_IHWMAPPER_H
