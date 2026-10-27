#pragma once

// Hand-written stand-in for the hidl-gen output of android.hidl.manager@1.0.
// Every generated interface header declares a registerForNotifications()
// static that mentions IServiceNotification, but the graphics service never
// registers for service notifications, so the name alone is enough. Only the
// type is referenced (inside android::sp<>), never constructed or called.

namespace android {
namespace hidl {
namespace manager {
namespace V1_0 {

class IServiceNotification;

}  // namespace V1_0
}  // namespace manager
}  // namespace hidl
}  // namespace android
