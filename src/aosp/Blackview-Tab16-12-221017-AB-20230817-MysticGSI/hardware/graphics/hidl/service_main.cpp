/*
 * HIDL graphics service for the TCLUI (Android 12) GSI: composer 2.4 and
 * allocator 4.0 share one hwbinder process, mirroring the AIDL stub.
 */

#define LOG_TAG "tebox-hidl-service"

#include <android-base/logging.h>
#include <android/hardware/graphics/allocator/4.0/IAllocator.h>
#include <android/hardware/graphics/composer/2.4/IComposer.h>
#include <composer-hal/2.4/Composer.h>
#include <hidl/HidlTransportSupport.h>
#include <log/log.h>

#include <cstdio>
#include <memory>

#include "composer_hal.h"

namespace android {
namespace hardware {
namespace graphics {
namespace allocator {
namespace V4_0 {

sp<IAllocator> createAllocator();

}  // namespace V4_0
}  // namespace allocator
}  // namespace graphics
}  // namespace hardware
}  // namespace android

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::graphics::allocator::V4_0::createAllocator;
using android::hardware::graphics::allocator::V4_0::IAllocator;
using android::hardware::graphics::composer::V2_4::IComposer;
using android::hardware::graphics::composer::V2_4::hal::Composer;
using android::hardware::graphics::composer::V2_4::hal::TeBoxComposerHal;

// The service runs before logd is useful to us, so send libbase's LOG()
// output to stderr; init's stdio_to_kmsg option forwards it to the kernel
// log where the QEMU serial console shows it.
static void trace(const char* what) {
    std::fprintf(stderr, "tebox-hidl-service: %s\n", what);
    std::fflush(stderr);
}

int main() {
    android::base::SetLogger(android::base::StderrLogger);
    trace("start");

    configureRpcThreadpool(4, true /* callerWillJoin */);
    trace("rpc threadpool ready");

    auto composer = Composer::create(std::make_unique<TeBoxComposerHal>());
    if (composer == nullptr) {
        trace("failed to create composer 2.4");
        return 1;
    }
    trace("composer 2.4 created");
    android::status_t status = composer->registerAsService();
    if (status != android::OK) {
        std::fprintf(stderr, "tebox-hidl-service: register composer failed: %d\n", status);
        return 1;
    }
    trace("composer 2.4 registered");

    android::sp<IAllocator> allocator = createAllocator();
    status = allocator->registerAsService();
    if (status != android::OK) {
        std::fprintf(stderr, "tebox-hidl-service: register allocator failed: %d\n", status);
        return 1;
    }
    trace("allocator 4.0 registered");

    joinRpcThreadpool();
    return 0;
}
