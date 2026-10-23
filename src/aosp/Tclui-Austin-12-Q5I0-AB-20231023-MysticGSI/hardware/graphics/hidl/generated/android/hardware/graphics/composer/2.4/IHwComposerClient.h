#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_IHWCOMPOSERCLIENT_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_IHWCOMPOSERCLIENT_H

#include <android/hardware/graphics/composer/2.4/IComposerClient.h>

#include <android/hardware/graphics/common/1.2/hwtypes.h>
#include <android/hardware/graphics/composer/2.1/BnHwComposerClient.h>
#include <android/hardware/graphics/composer/2.1/BpHwComposerClient.h>
#include <android/hardware/graphics/composer/2.1/hwtypes.h>
#include <android/hardware/graphics/composer/2.3/BnHwComposerClient.h>
#include <android/hardware/graphics/composer/2.3/BpHwComposerClient.h>
#include <android/hardware/graphics/composer/2.4/BnHwComposerCallback.h>
#include <android/hardware/graphics/composer/2.4/BpHwComposerCallback.h>
#include <android/hardware/graphics/composer/2.4/hwtypes.h>

#include <hidl/Status.h>
#include <hwbinder/IBinder.h>
#include <hwbinder/Parcel.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_4 {
::android::status_t readEmbeddedFromParcel(
        const ::android::hardware::graphics::composer::V2_4::IComposerClient::LayerGenericMetadataKey &obj,
        const ::android::hardware::Parcel &parcel,
        size_t parentHandle,
        size_t parentOffset);

::android::status_t writeEmbeddedToParcel(
        const ::android::hardware::graphics::composer::V2_4::IComposerClient::LayerGenericMetadataKey &obj,
        ::android::hardware::Parcel *parcel,
        size_t parentHandle,
        size_t parentOffset);

}  // namespace V2_4
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_IHWCOMPOSERCLIENT_H
