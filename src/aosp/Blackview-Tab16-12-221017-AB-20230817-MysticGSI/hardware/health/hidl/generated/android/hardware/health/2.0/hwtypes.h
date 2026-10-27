#ifndef HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_0_HWTYPES_H
#define HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_0_HWTYPES_H

#include <android/hardware/health/2.0/types.h>

#include <android/hardware/health/1.0/hwtypes.h>

#include <hidl/Status.h>
#include <hwbinder/IBinder.h>
#include <hwbinder/Parcel.h>

namespace android {
namespace hardware {
namespace health {
namespace V2_0 {
::android::status_t readEmbeddedFromParcel(
        const ::android::hardware::health::V2_0::StorageAttribute &obj,
        const ::android::hardware::Parcel &parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t writeEmbeddedToParcel(
        const ::android::hardware::health::V2_0::StorageAttribute &obj,
        ::android::hardware::Parcel *parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t readEmbeddedFromParcel(
        const ::android::hardware::health::V2_0::StorageInfo &obj,
        const ::android::hardware::Parcel &parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t writeEmbeddedToParcel(
        const ::android::hardware::health::V2_0::StorageInfo &obj,
        ::android::hardware::Parcel *parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t readEmbeddedFromParcel(
        const ::android::hardware::health::V2_0::DiskStats &obj,
        const ::android::hardware::Parcel &parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t writeEmbeddedToParcel(
        const ::android::hardware::health::V2_0::DiskStats &obj,
        ::android::hardware::Parcel *parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t readEmbeddedFromParcel(
        const ::android::hardware::health::V2_0::HealthInfo &obj,
        const ::android::hardware::Parcel &parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t writeEmbeddedToParcel(
        const ::android::hardware::health::V2_0::HealthInfo &obj,
        ::android::hardware::Parcel *parcel,
        size_t parentHandle,
        size_t parentOffset);

}  // namespace V2_0
}  // namespace health
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_0_HWTYPES_H
