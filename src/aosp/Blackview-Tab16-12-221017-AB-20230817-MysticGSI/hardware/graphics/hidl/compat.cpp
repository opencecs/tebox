/*
 * FMQ compatibility shim.
 *
 * The FMQ headers come from Android 13, where details::check() takes a
 * message string. The Android 12 libfmq this image ships only exports the
 * older one-argument check(bool). The FMQ templates pulled in by
 * libhidlbase's headers instantiate check(bool, const char*), so provide
 * that overload here; a failed check is fatal either way.
 */

#define LOG_TAG "tebox-hidl-service"

#include <stdlib.h>

#include <fmq/MQDescriptorBase.h>
#include <log/log.h>

namespace android {
namespace hardware {
namespace details {

void check(bool exp, const char* message) {
    if (!exp) {
        ALOGE("HIDL/FMQ check failed: %s", message ? message : "(null)");
        abort();
    }
}

}  // namespace details
}  // namespace hardware
}  // namespace android
