#pragma once

// Minimal <log/log.h> for the platform-compatible HAL build. The platform
// header tree is not available here, so map the logging macros the AOSP
// composer utilities and this HAL use onto the NDK logging entry points.

#include <android/log.h>

#ifndef LOG_TAG
#define LOG_TAG "tebox-hidl"
#endif

#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define ALOGV_IF(cond, ...) ((cond) ? (void)ALOGV(__VA_ARGS__) : (void)0)
#define ALOGD_IF(cond, ...) ((cond) ? (void)ALOGD(__VA_ARGS__) : (void)0)
#define ALOGI_IF(cond, ...) ((cond) ? (void)ALOGI(__VA_ARGS__) : (void)0)
#define ALOGW_IF(cond, ...) ((cond) ? (void)ALOGW(__VA_ARGS__) : (void)0)
#define ALOGE_IF(cond, ...) ((cond) ? (void)ALOGE(__VA_ARGS__) : (void)0)

#define LOG_ALWAYS_FATAL(...) __android_log_assert(nullptr, LOG_TAG, __VA_ARGS__)
#define LOG_ALWAYS_FATAL_IF(cond, ...) \
    ((cond) ? __android_log_assert(nullptr, LOG_TAG, __VA_ARGS__) : (void)0)
