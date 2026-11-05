/*
 * Vendor Codec2 store: Mesa / VirGL Gallium video (c2.mesa.avc.*).
 */

#include "ComponentStore.h"

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/log.h>

#define LOG_TAG "mesa-c2-service"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using aidl::android::hardware::media::c2::IComponentStore;
using ndk::SharedRefBase;

int main() {
    constexpr int kThreads = 8;
    ABinderProcess_setThreadPoolMaxThreadCount(kThreads);
    ABinderProcess_startThreadPool();

    auto store = SharedRefBase::make<gki::ComponentStore>();
    const std::string instance = std::string(IComponentStore::descriptor) + "/default";
    binder_status_t st =
            AServiceManager_addService(store->asBinder().get(), instance.c_str());
    if (st != STATUS_OK) {
        ALOGE("failed to register %s (%d)", instance.c_str(), st);
        return 1;
    }
    ALOGI("registered %s", instance.c_str());
    ABinderProcess_joinThreadPool();
    return 0;
}
