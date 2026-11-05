#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_3_IHWCOMPOSERCLIENT_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_3_IHWCOMPOSERCLIENT_H

#include <android/hardware/graphics/composer/2.3/IComposerClient.h>

#include <android/hardware/graphics/common/1.1/hwtypes.h>
#include <android/hardware/graphics/common/1.2/hwtypes.h>
#include <android/hardware/graphics/composer/2.1/BnHwComposerClient.h>
#include <android/hardware/graphics/composer/2.1/BpHwComposerClient.h>
#include <android/hardware/graphics/composer/2.1/hwtypes.h>
#include <android/hardware/graphics/composer/2.2/BnHwComposerClient.h>
#include <android/hardware/graphics/composer/2.2/BpHwComposerClient.h>

#include <hidl/Status.h>
#include <hwbinder/IBinder.h>
#include <hwbinder/Parcel.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_3 {
::android::status_t readEmbeddedFromParcel(
        const ::android::hardware::graphics::composer::V2_3::IComposerClient::PerFrameMetadataBlob &obj,
        const ::android::hardware::Parcel &parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t writeEmbeddedToParcel(
        const ::android::hardware::graphics::composer::V2_3::IComposerClient::PerFrameMetadataBlob &obj,
        ::android::hardware::Parcel *parcel,
        size_t parentHandle,
        size_t parentOffset);

}  // namespace V2_3
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_3_IHWCOMPOSERCLIENT_H
