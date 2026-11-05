#ifndef HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_1_HWTYPES_H
#define HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_1_HWTYPES_H

#include <android/hardware/health/2.1/types.h>

#include <android/hardware/health/1.0/hwtypes.h>
#include <android/hardware/health/2.0/hwtypes.h>

#include <hidl/Status.h>
#include <hwbinder/IBinder.h>
#include <hwbinder/Parcel.h>

namespace android {
namespace hardware {
namespace health {
namespace V2_1 {
::android::status_t readEmbeddedFromParcel(
        const ::android::hardware::health::V2_1::HealthInfo &obj,
        const ::android::hardware::Parcel &parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t writeEmbeddedToParcel(
        const ::android::hardware::health::V2_1::HealthInfo &obj,
        ::android::hardware::Parcel *parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t readEmbeddedFromParcel(
        const ::android::hardware::health::V2_1::HealthConfig &obj,
        const ::android::hardware::Parcel &parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t writeEmbeddedToParcel(
        const ::android::hardware::health::V2_1::HealthConfig &obj,
        ::android::hardware::Parcel *parcel,
        size_t parentHandle,
        size_t parentOffset);

}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_HEALTH_V2_1_HWTYPES_H
