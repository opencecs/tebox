#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_BSCOMPOSERCLIENT_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_BSCOMPOSERCLIENT_H

#include <android-base/macros.h>
#include <cutils/trace.h>
#include <future>
#include <android/hardware/graphics/composer/2.4/IComposerClient.h>

#include <hidl/HidlPassthroughSupport.h>
#include <hidl/TaskRunner.h>
namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_4 {

struct BsComposerClient : IComposerClient, ::android::hardware::details::HidlInstrumentor {
    explicit BsComposerClient(const ::android::sp<IComposerClient> impl);

    /**
     * The pure class is what this class wraps.
     */
    typedef IComposerClient Pure;

    typedef android::hardware::details::bs_tag _hidl_tag;

    // Methods from ::android::hardware::graphics::composer::V2_1::IComposerClient follow.
    ::android::hardware::Return<void> registerCallback(const ::android::sp<::android::hardware::graphics::composer::V2_1::IComposerCallback>& callback) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::registerCallback::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&callback);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "registerCallback", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::sp<::android::hardware::graphics::composer::V2_1::IComposerCallback> _hidl_wrapped_callback;
        if (callback != nullptr && !callback->isRemote()) {
            _hidl_wrapped_callback = ::android::hardware::details::wrapPassthrough(callback);
            if (_hidl_wrapped_callback == nullptr) {
                return ::android::hardware::Status::fromExceptionCode(
                        ::android::hardware::Status::EX_TRANSACTION_FAILED,
                        "Cannot wrap passthrough interface.");
            }
        } else {
            _hidl_wrapped_callback = callback;
        }

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->registerCallback(_hidl_wrapped_callback);

        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "registerCallback", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<uint32_t> getMaxVirtualDisplayCount() override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getMaxVirtualDisplayCount::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getMaxVirtualDisplayCount", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getMaxVirtualDisplayCount();

        uint32_t _hidl_out_count = _hidl_return;
        (void) _hidl_out_count;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_count);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getMaxVirtualDisplayCount", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> createVirtualDisplay(uint32_t width, uint32_t height, ::android::hardware::graphics::common::V1_0::PixelFormat formatHint, uint32_t outputBufferSlotCount, createVirtualDisplay_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::createVirtualDisplay::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&width);
            _hidl_args.push_back((void *)&height);
            _hidl_args.push_back((void *)&formatHint);
            _hidl_args.push_back((void *)&outputBufferSlotCount);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "createVirtualDisplay", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->createVirtualDisplay(width, height, formatHint, outputBufferSlotCount, [&](const auto &_hidl_out_error, const auto &_hidl_out_display, const auto &_hidl_out_format) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_display);
                _hidl_args.push_back((void *)&_hidl_out_format);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "createVirtualDisplay", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_display, _hidl_out_format);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> destroyVirtualDisplay(uint64_t display) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::destroyVirtualDisplay::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "destroyVirtualDisplay", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->destroyVirtualDisplay(display);

        ::android::hardware::graphics::composer::V2_1::Error _hidl_out_error = _hidl_return;
        (void) _hidl_out_error;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_error);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "destroyVirtualDisplay", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> createLayer(uint64_t display, uint32_t bufferSlotCount, createLayer_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::createLayer::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&bufferSlotCount);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "createLayer", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->createLayer(display, bufferSlotCount, [&](const auto &_hidl_out_error, const auto &_hidl_out_layer) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_layer);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "createLayer", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_layer);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> destroyLayer(uint64_t display, uint64_t layer) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::destroyLayer::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&layer);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "destroyLayer", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->destroyLayer(display, layer);

        ::android::hardware::graphics::composer::V2_1::Error _hidl_out_error = _hidl_return;
        (void) _hidl_out_error;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_error);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "destroyLayer", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getActiveConfig(uint64_t display, getActiveConfig_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getActiveConfig::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getActiveConfig", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getActiveConfig(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_config) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_config);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getActiveConfig", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_config);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> getClientTargetSupport(uint64_t display, uint32_t width, uint32_t height, ::android::hardware::graphics::common::V1_0::PixelFormat format, ::android::hardware::graphics::common::V1_0::Dataspace dataspace) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getClientTargetSupport::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&width);
            _hidl_args.push_back((void *)&height);
            _hidl_args.push_back((void *)&format);
            _hidl_args.push_back((void *)&dataspace);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getClientTargetSupport", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getClientTargetSupport(display, width, height, format, dataspace);

        ::android::hardware::graphics::composer::V2_1::Error _hidl_out_error = _hidl_return;
        (void) _hidl_out_error;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_error);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getClientTargetSupport", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getColorModes(uint64_t display, getColorModes_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getColorModes::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getColorModes", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getColorModes(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_modes) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_modes);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getColorModes", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_modes);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getDisplayAttribute(uint64_t display, uint32_t config, ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute attribute, getDisplayAttribute_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getDisplayAttribute::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&config);
            _hidl_args.push_back((void *)&attribute);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getDisplayAttribute", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getDisplayAttribute(display, config, attribute, [&](const auto &_hidl_out_error, const auto &_hidl_out_value) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_value);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getDisplayAttribute", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_value);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getDisplayConfigs(uint64_t display, getDisplayConfigs_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getDisplayConfigs::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getDisplayConfigs", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getDisplayConfigs(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_configs) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_configs);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getDisplayConfigs", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_configs);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getDisplayName(uint64_t display, getDisplayName_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getDisplayName::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getDisplayName", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getDisplayName(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_name) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_name);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getDisplayName", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_name);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getDisplayType(uint64_t display, getDisplayType_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getDisplayType::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getDisplayType", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getDisplayType(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_type) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_type);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getDisplayType", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_type);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getDozeSupport(uint64_t display, getDozeSupport_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getDozeSupport::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getDozeSupport", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getDozeSupport(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_support) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_support);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getDozeSupport", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_support);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getHdrCapabilities(uint64_t display, getHdrCapabilities_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getHdrCapabilities::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getHdrCapabilities", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getHdrCapabilities(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_types, const auto &_hidl_out_maxLuminance, const auto &_hidl_out_maxAverageLuminance, const auto &_hidl_out_minLuminance) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_types);
                _hidl_args.push_back((void *)&_hidl_out_maxLuminance);
                _hidl_args.push_back((void *)&_hidl_out_maxAverageLuminance);
                _hidl_args.push_back((void *)&_hidl_out_minLuminance);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getHdrCapabilities", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_types, _hidl_out_maxLuminance, _hidl_out_maxAverageLuminance, _hidl_out_minLuminance);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setClientTargetSlotCount(uint64_t display, uint32_t clientTargetSlotCount) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::setClientTargetSlotCount::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&clientTargetSlotCount);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "setClientTargetSlotCount", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->setClientTargetSlotCount(display, clientTargetSlotCount);

        ::android::hardware::graphics::composer::V2_1::Error _hidl_out_error = _hidl_return;
        (void) _hidl_out_error;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_error);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "setClientTargetSlotCount", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setActiveConfig(uint64_t display, uint32_t config) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::setActiveConfig::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&config);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "setActiveConfig", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->setActiveConfig(display, config);

        ::android::hardware::graphics::composer::V2_1::Error _hidl_out_error = _hidl_return;
        (void) _hidl_out_error;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_error);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "setActiveConfig", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setColorMode(uint64_t display, ::android::hardware::graphics::common::V1_0::ColorMode mode) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::setColorMode::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&mode);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "setColorMode", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->setColorMode(display, mode);

        ::android::hardware::graphics::composer::V2_1::Error _hidl_out_error = _hidl_return;
        (void) _hidl_out_error;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_error);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "setColorMode", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setPowerMode(uint64_t display, ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode mode) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::setPowerMode::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&mode);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "setPowerMode", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->setPowerMode(display, mode);

        ::android::hardware::graphics::composer::V2_1::Error _hidl_out_error = _hidl_return;
        (void) _hidl_out_error;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_error);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "setPowerMode", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setVsyncEnabled(uint64_t display, ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync enabled) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::setVsyncEnabled::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&enabled);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "setVsyncEnabled", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->setVsyncEnabled(display, enabled);

        ::android::hardware::graphics::composer::V2_1::Error _hidl_out_error = _hidl_return;
        (void) _hidl_out_error;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_error);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "setVsyncEnabled", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setInputCommandQueue(const ::android::hardware::MQDescriptorSync<uint32_t>& descriptor) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::setInputCommandQueue::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&descriptor);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "setInputCommandQueue", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->setInputCommandQueue(descriptor);

        ::android::hardware::graphics::composer::V2_1::Error _hidl_out_error = _hidl_return;
        (void) _hidl_out_error;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_error);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "setInputCommandQueue", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getOutputCommandQueue(getOutputCommandQueue_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getOutputCommandQueue::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getOutputCommandQueue", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getOutputCommandQueue([&](const auto &_hidl_out_error, const auto &_hidl_out_descriptor) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_descriptor);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "getOutputCommandQueue", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_descriptor);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> executeCommands(uint32_t inLength, const ::android::hardware::hidl_vec<::android::hardware::hidl_handle>& inHandles, executeCommands_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::executeCommands::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&inLength);
            _hidl_args.push_back((void *)&inHandles);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.1", "IComposerClient", "executeCommands", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->executeCommands(inLength, inHandles, [&](const auto &_hidl_out_error, const auto &_hidl_out_outQueueChanged, const auto &_hidl_out_outLength, const auto &_hidl_out_outHandles) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_outQueueChanged);
                _hidl_args.push_back((void *)&_hidl_out_outLength);
                _hidl_args.push_back((void *)&_hidl_out_outHandles);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.1", "IComposerClient", "executeCommands", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_outQueueChanged, _hidl_out_outLength, _hidl_out_outHandles);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }

    // Methods from ::android::hardware::graphics::composer::V2_2::IComposerClient follow.
    ::android::hardware::Return<void> getPerFrameMetadataKeys(uint64_t display, getPerFrameMetadataKeys_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getPerFrameMetadataKeys::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.2", "IComposerClient", "getPerFrameMetadataKeys", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getPerFrameMetadataKeys(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_keys) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_keys);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.2", "IComposerClient", "getPerFrameMetadataKeys", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_keys);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getReadbackBufferAttributes(uint64_t display, getReadbackBufferAttributes_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getReadbackBufferAttributes::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.2", "IComposerClient", "getReadbackBufferAttributes", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getReadbackBufferAttributes(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_format, const auto &_hidl_out_dataspace) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_format);
                _hidl_args.push_back((void *)&_hidl_out_dataspace);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.2", "IComposerClient", "getReadbackBufferAttributes", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_format, _hidl_out_dataspace);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getReadbackBufferFence(uint64_t display, getReadbackBufferFence_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getReadbackBufferFence::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.2", "IComposerClient", "getReadbackBufferFence", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getReadbackBufferFence(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_acquireFence) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_acquireFence);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.2", "IComposerClient", "getReadbackBufferFence", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_acquireFence);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setReadbackBuffer(uint64_t display, const ::android::hardware::hidl_handle& buffer, const ::android::hardware::hidl_handle& releaseFence) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::setReadbackBuffer::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&buffer);
            _hidl_args.push_back((void *)&releaseFence);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.2", "IComposerClient", "setReadbackBuffer", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->setReadbackBuffer(display, buffer, releaseFence);

        ::android::hardware::graphics::composer::V2_1::Error _hidl_out_error = _hidl_return;
        (void) _hidl_out_error;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_error);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.2", "IComposerClient", "setReadbackBuffer", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> createVirtualDisplay_2_2(uint32_t width, uint32_t height, ::android::hardware::graphics::common::V1_1::PixelFormat formatHint, uint32_t outputBufferSlotCount, createVirtualDisplay_2_2_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::createVirtualDisplay_2_2::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&width);
            _hidl_args.push_back((void *)&height);
            _hidl_args.push_back((void *)&formatHint);
            _hidl_args.push_back((void *)&outputBufferSlotCount);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.2", "IComposerClient", "createVirtualDisplay_2_2", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->createVirtualDisplay_2_2(width, height, formatHint, outputBufferSlotCount, [&](const auto &_hidl_out_error, const auto &_hidl_out_display, const auto &_hidl_out_format) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_display);
                _hidl_args.push_back((void *)&_hidl_out_format);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.2", "IComposerClient", "createVirtualDisplay_2_2", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_display, _hidl_out_format);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> getClientTargetSupport_2_2(uint64_t display, uint32_t width, uint32_t height, ::android::hardware::graphics::common::V1_1::PixelFormat format, ::android::hardware::graphics::common::V1_1::Dataspace dataspace) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getClientTargetSupport_2_2::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&width);
            _hidl_args.push_back((void *)&height);
            _hidl_args.push_back((void *)&format);
            _hidl_args.push_back((void *)&dataspace);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.2", "IComposerClient", "getClientTargetSupport_2_2", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getClientTargetSupport_2_2(display, width, height, format, dataspace);

        ::android::hardware::graphics::composer::V2_1::Error _hidl_out_error = _hidl_return;
        (void) _hidl_out_error;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_error);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.2", "IComposerClient", "getClientTargetSupport_2_2", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setPowerMode_2_2(uint64_t display, ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode mode) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::setPowerMode_2_2::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&mode);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.2", "IComposerClient", "setPowerMode_2_2", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->setPowerMode_2_2(display, mode);

        ::android::hardware::graphics::composer::V2_1::Error _hidl_out_error = _hidl_return;
        (void) _hidl_out_error;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_error);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.2", "IComposerClient", "setPowerMode_2_2", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getColorModes_2_2(uint64_t display, getColorModes_2_2_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getColorModes_2_2::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.2", "IComposerClient", "getColorModes_2_2", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getColorModes_2_2(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_modes) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_modes);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.2", "IComposerClient", "getColorModes_2_2", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_modes);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getRenderIntents(uint64_t display, ::android::hardware::graphics::common::V1_1::ColorMode mode, getRenderIntents_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getRenderIntents::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&mode);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.2", "IComposerClient", "getRenderIntents", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getRenderIntents(display, mode, [&](const auto &_hidl_out_error, const auto &_hidl_out_intents) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_intents);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.2", "IComposerClient", "getRenderIntents", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_intents);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setColorMode_2_2(uint64_t display, ::android::hardware::graphics::common::V1_1::ColorMode mode, ::android::hardware::graphics::common::V1_1::RenderIntent intent) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::setColorMode_2_2::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&mode);
            _hidl_args.push_back((void *)&intent);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.2", "IComposerClient", "setColorMode_2_2", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->setColorMode_2_2(display, mode, intent);

        ::android::hardware::graphics::composer::V2_1::Error _hidl_out_error = _hidl_return;
        (void) _hidl_out_error;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_error);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.2", "IComposerClient", "setColorMode_2_2", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getDataspaceSaturationMatrix(::android::hardware::graphics::common::V1_1::Dataspace dataspace, getDataspaceSaturationMatrix_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getDataspaceSaturationMatrix::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&dataspace);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.2", "IComposerClient", "getDataspaceSaturationMatrix", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getDataspaceSaturationMatrix(dataspace, [&](const auto &_hidl_out_error, const auto &_hidl_out_matrix) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_matrix);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.2", "IComposerClient", "getDataspaceSaturationMatrix", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_matrix);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> executeCommands_2_2(uint32_t inLength, const ::android::hardware::hidl_vec<::android::hardware::hidl_handle>& inHandles, executeCommands_2_2_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::executeCommands_2_2::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&inLength);
            _hidl_args.push_back((void *)&inHandles);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.2", "IComposerClient", "executeCommands_2_2", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->executeCommands_2_2(inLength, inHandles, [&](const auto &_hidl_out_error, const auto &_hidl_out_outQueueChanged, const auto &_hidl_out_outLength, const auto &_hidl_out_outHandles) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_outQueueChanged);
                _hidl_args.push_back((void *)&_hidl_out_outLength);
                _hidl_args.push_back((void *)&_hidl_out_outHandles);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.2", "IComposerClient", "executeCommands_2_2", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_outQueueChanged, _hidl_out_outLength, _hidl_out_outHandles);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }

    // Methods from ::android::hardware::graphics::composer::V2_3::IComposerClient follow.
    ::android::hardware::Return<void> getDisplayIdentificationData(uint64_t display, getDisplayIdentificationData_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getDisplayIdentificationData::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getDisplayIdentificationData", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getDisplayIdentificationData(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_port, const auto &_hidl_out_data) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_port);
                _hidl_args.push_back((void *)&_hidl_out_data);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getDisplayIdentificationData", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_port, _hidl_out_data);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getReadbackBufferAttributes_2_3(uint64_t display, getReadbackBufferAttributes_2_3_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getReadbackBufferAttributes_2_3::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getReadbackBufferAttributes_2_3", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getReadbackBufferAttributes_2_3(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_format, const auto &_hidl_out_dataspace) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_format);
                _hidl_args.push_back((void *)&_hidl_out_dataspace);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getReadbackBufferAttributes_2_3", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_format, _hidl_out_dataspace);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> getClientTargetSupport_2_3(uint64_t display, uint32_t width, uint32_t height, ::android::hardware::graphics::common::V1_2::PixelFormat format, ::android::hardware::graphics::common::V1_2::Dataspace dataspace) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getClientTargetSupport_2_3::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&width);
            _hidl_args.push_back((void *)&height);
            _hidl_args.push_back((void *)&format);
            _hidl_args.push_back((void *)&dataspace);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getClientTargetSupport_2_3", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getClientTargetSupport_2_3(display, width, height, format, dataspace);

        ::android::hardware::graphics::composer::V2_1::Error _hidl_out_error = _hidl_return;
        (void) _hidl_out_error;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_error);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getClientTargetSupport_2_3", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getDisplayedContentSamplingAttributes(uint64_t display, getDisplayedContentSamplingAttributes_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getDisplayedContentSamplingAttributes::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getDisplayedContentSamplingAttributes", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getDisplayedContentSamplingAttributes(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_format, const auto &_hidl_out_dataspace, const auto &_hidl_out_componentMask) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_format);
                _hidl_args.push_back((void *)&_hidl_out_dataspace);
                _hidl_args.push_back((void *)&_hidl_out_componentMask);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getDisplayedContentSamplingAttributes", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_format, _hidl_out_dataspace, _hidl_out_componentMask);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setDisplayedContentSamplingEnabled(uint64_t display, ::android::hardware::graphics::composer::V2_3::IComposerClient::DisplayedContentSampling enable, ::android::hardware::hidl_bitfield<::android::hardware::graphics::composer::V2_3::IComposerClient::FormatColorComponent> componentMask, uint64_t maxFrames) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::setDisplayedContentSamplingEnabled::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&enable);
            _hidl_args.push_back((void *)&componentMask);
            _hidl_args.push_back((void *)&maxFrames);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.3", "IComposerClient", "setDisplayedContentSamplingEnabled", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->setDisplayedContentSamplingEnabled(display, enable, componentMask, maxFrames);

        ::android::hardware::graphics::composer::V2_1::Error _hidl_out_error = _hidl_return;
        (void) _hidl_out_error;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_error);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.3", "IComposerClient", "setDisplayedContentSamplingEnabled", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getDisplayedContentSample(uint64_t display, uint64_t maxFrames, uint64_t timestamp, getDisplayedContentSample_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getDisplayedContentSample::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&maxFrames);
            _hidl_args.push_back((void *)&timestamp);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getDisplayedContentSample", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getDisplayedContentSample(display, maxFrames, timestamp, [&](const auto &_hidl_out_error, const auto &_hidl_out_frameCount, const auto &_hidl_out_sampleComponent0, const auto &_hidl_out_sampleComponent1, const auto &_hidl_out_sampleComponent2, const auto &_hidl_out_sampleComponent3) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_frameCount);
                _hidl_args.push_back((void *)&_hidl_out_sampleComponent0);
                _hidl_args.push_back((void *)&_hidl_out_sampleComponent1);
                _hidl_args.push_back((void *)&_hidl_out_sampleComponent2);
                _hidl_args.push_back((void *)&_hidl_out_sampleComponent3);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getDisplayedContentSample", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_frameCount, _hidl_out_sampleComponent0, _hidl_out_sampleComponent1, _hidl_out_sampleComponent2, _hidl_out_sampleComponent3);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> executeCommands_2_3(uint32_t inLength, const ::android::hardware::hidl_vec<::android::hardware::hidl_handle>& inHandles, executeCommands_2_3_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::executeCommands_2_3::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&inLength);
            _hidl_args.push_back((void *)&inHandles);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.3", "IComposerClient", "executeCommands_2_3", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->executeCommands_2_3(inLength, inHandles, [&](const auto &_hidl_out_error, const auto &_hidl_out_outQueueChanged, const auto &_hidl_out_outLength, const auto &_hidl_out_outHandles) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_outQueueChanged);
                _hidl_args.push_back((void *)&_hidl_out_outLength);
                _hidl_args.push_back((void *)&_hidl_out_outHandles);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.3", "IComposerClient", "executeCommands_2_3", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_outQueueChanged, _hidl_out_outLength, _hidl_out_outHandles);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getRenderIntents_2_3(uint64_t display, ::android::hardware::graphics::common::V1_2::ColorMode mode, getRenderIntents_2_3_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getRenderIntents_2_3::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&mode);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getRenderIntents_2_3", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getRenderIntents_2_3(display, mode, [&](const auto &_hidl_out_error, const auto &_hidl_out_intents) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_intents);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getRenderIntents_2_3", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_intents);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getColorModes_2_3(uint64_t display, getColorModes_2_3_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getColorModes_2_3::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getColorModes_2_3", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getColorModes_2_3(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_modes) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_modes);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getColorModes_2_3", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_modes);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setColorMode_2_3(uint64_t display, ::android::hardware::graphics::common::V1_2::ColorMode mode, ::android::hardware::graphics::common::V1_1::RenderIntent intent) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::setColorMode_2_3::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&mode);
            _hidl_args.push_back((void *)&intent);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.3", "IComposerClient", "setColorMode_2_3", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->setColorMode_2_3(display, mode, intent);

        ::android::hardware::graphics::composer::V2_1::Error _hidl_out_error = _hidl_return;
        (void) _hidl_out_error;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_error);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.3", "IComposerClient", "setColorMode_2_3", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getDisplayCapabilities(uint64_t display, getDisplayCapabilities_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getDisplayCapabilities::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getDisplayCapabilities", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getDisplayCapabilities(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_capabilities) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_capabilities);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getDisplayCapabilities", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_capabilities);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getPerFrameMetadataKeys_2_3(uint64_t display, getPerFrameMetadataKeys_2_3_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getPerFrameMetadataKeys_2_3::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getPerFrameMetadataKeys_2_3", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getPerFrameMetadataKeys_2_3(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_keys) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_keys);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getPerFrameMetadataKeys_2_3", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_keys);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getHdrCapabilities_2_3(uint64_t display, getHdrCapabilities_2_3_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getHdrCapabilities_2_3::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getHdrCapabilities_2_3", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getHdrCapabilities_2_3(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_types, const auto &_hidl_out_maxLuminance, const auto &_hidl_out_maxAverageLuminance, const auto &_hidl_out_minLuminance) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_types);
                _hidl_args.push_back((void *)&_hidl_out_maxLuminance);
                _hidl_args.push_back((void *)&_hidl_out_maxAverageLuminance);
                _hidl_args.push_back((void *)&_hidl_out_minLuminance);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getHdrCapabilities_2_3", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_types, _hidl_out_maxLuminance, _hidl_out_maxAverageLuminance, _hidl_out_minLuminance);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getDisplayBrightnessSupport(uint64_t display, getDisplayBrightnessSupport_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getDisplayBrightnessSupport::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getDisplayBrightnessSupport", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getDisplayBrightnessSupport(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_support) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_support);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.3", "IComposerClient", "getDisplayBrightnessSupport", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_support);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setDisplayBrightness(uint64_t display, float brightness) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::setDisplayBrightness::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&brightness);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.3", "IComposerClient", "setDisplayBrightness", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->setDisplayBrightness(display, brightness);

        ::android::hardware::graphics::composer::V2_1::Error _hidl_out_error = _hidl_return;
        (void) _hidl_out_error;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_error);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.3", "IComposerClient", "setDisplayBrightness", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }

    // Methods from ::android::hardware::graphics::composer::V2_4::IComposerClient follow.
    ::android::hardware::Return<void> registerCallback_2_4(const ::android::sp<::android::hardware::graphics::composer::V2_4::IComposerCallback>& callback) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::registerCallback_2_4::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&callback);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.4", "IComposerClient", "registerCallback_2_4", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::sp<::android::hardware::graphics::composer::V2_4::IComposerCallback> _hidl_wrapped_callback;
        if (callback != nullptr && !callback->isRemote()) {
            _hidl_wrapped_callback = ::android::hardware::details::wrapPassthrough(callback);
            if (_hidl_wrapped_callback == nullptr) {
                return ::android::hardware::Status::fromExceptionCode(
                        ::android::hardware::Status::EX_TRANSACTION_FAILED,
                        "Cannot wrap passthrough interface.");
            }
        } else {
            _hidl_wrapped_callback = callback;
        }

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->registerCallback_2_4(_hidl_wrapped_callback);

        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.4", "IComposerClient", "registerCallback_2_4", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getDisplayCapabilities_2_4(uint64_t display, getDisplayCapabilities_2_4_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getDisplayCapabilities_2_4::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.4", "IComposerClient", "getDisplayCapabilities_2_4", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getDisplayCapabilities_2_4(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_capabilities) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_capabilities);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.4", "IComposerClient", "getDisplayCapabilities_2_4", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_capabilities);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getDisplayConnectionType(uint64_t display, getDisplayConnectionType_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getDisplayConnectionType::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.4", "IComposerClient", "getDisplayConnectionType", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getDisplayConnectionType(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_type) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_type);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.4", "IComposerClient", "getDisplayConnectionType", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_type);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getDisplayAttribute_2_4(uint64_t display, uint32_t config, ::android::hardware::graphics::composer::V2_4::IComposerClient::Attribute attribute, getDisplayAttribute_2_4_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getDisplayAttribute_2_4::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&config);
            _hidl_args.push_back((void *)&attribute);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.4", "IComposerClient", "getDisplayAttribute_2_4", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getDisplayAttribute_2_4(display, config, attribute, [&](const auto &_hidl_out_error, const auto &_hidl_out_value) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_value);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.4", "IComposerClient", "getDisplayAttribute_2_4", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_value);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getDisplayVsyncPeriod(uint64_t display, getDisplayVsyncPeriod_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getDisplayVsyncPeriod::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.4", "IComposerClient", "getDisplayVsyncPeriod", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getDisplayVsyncPeriod(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_vsyncPeriodNanos) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_vsyncPeriodNanos);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.4", "IComposerClient", "getDisplayVsyncPeriod", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_vsyncPeriodNanos);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> setActiveConfigWithConstraints(uint64_t display, uint32_t config, const ::android::hardware::graphics::composer::V2_4::IComposerClient::VsyncPeriodChangeConstraints& vsyncPeriodChangeConstraints, setActiveConfigWithConstraints_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::setActiveConfigWithConstraints::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&config);
            _hidl_args.push_back((void *)&vsyncPeriodChangeConstraints);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.4", "IComposerClient", "setActiveConfigWithConstraints", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->setActiveConfigWithConstraints(display, config, vsyncPeriodChangeConstraints, [&](const auto &_hidl_out_error, const auto &_hidl_out_timeline) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_timeline);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.4", "IComposerClient", "setActiveConfigWithConstraints", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_timeline);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_4::Error> setAutoLowLatencyMode(uint64_t display, bool on) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::setAutoLowLatencyMode::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&on);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.4", "IComposerClient", "setAutoLowLatencyMode", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->setAutoLowLatencyMode(display, on);

        ::android::hardware::graphics::composer::V2_4::Error _hidl_out_error = _hidl_return;
        (void) _hidl_out_error;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_error);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.4", "IComposerClient", "setAutoLowLatencyMode", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getSupportedContentTypes(uint64_t display, getSupportedContentTypes_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getSupportedContentTypes::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.4", "IComposerClient", "getSupportedContentTypes", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getSupportedContentTypes(display, [&](const auto &_hidl_out_error, const auto &_hidl_out_supportedContentTypes) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_supportedContentTypes);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.4", "IComposerClient", "getSupportedContentTypes", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_supportedContentTypes);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<::android::hardware::graphics::composer::V2_4::Error> setContentType(uint64_t display, ::android::hardware::graphics::composer::V2_4::IComposerClient::ContentType type) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::setContentType::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&display);
            _hidl_args.push_back((void *)&type);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.4", "IComposerClient", "setContentType", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->setContentType(display, type);

        ::android::hardware::graphics::composer::V2_4::Error _hidl_out_error = _hidl_return;
        (void) _hidl_out_error;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_error);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.4", "IComposerClient", "setContentType", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getLayerGenericMetadataKeys(getLayerGenericMetadataKeys_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getLayerGenericMetadataKeys::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hardware.graphics.composer", "2.4", "IComposerClient", "getLayerGenericMetadataKeys", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getLayerGenericMetadataKeys([&](const auto &_hidl_out_error, const auto &_hidl_out_keys) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_error);
                _hidl_args.push_back((void *)&_hidl_out_keys);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hardware.graphics.composer", "2.4", "IComposerClient", "getLayerGenericMetadataKeys", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_error, _hidl_out_keys);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }

    // Methods from ::android::hidl::base::V1_0::IBase follow.
    ::android::hardware::Return<void> interfaceChain(interfaceChain_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::interfaceChain::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hidl.base", "1.0", "IBase", "interfaceChain", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->interfaceChain([&](const auto &_hidl_out_descriptors) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_descriptors);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hidl.base", "1.0", "IBase", "interfaceChain", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_descriptors);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> debug(const ::android::hardware::hidl_handle& fd, const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& options) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::debug::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&fd);
            _hidl_args.push_back((void *)&options);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hidl.base", "1.0", "IBase", "debug", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->debug(fd, options);

        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hidl.base", "1.0", "IBase", "debug", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> interfaceDescriptor(interfaceDescriptor_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::interfaceDescriptor::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hidl.base", "1.0", "IBase", "interfaceDescriptor", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->interfaceDescriptor([&](const auto &_hidl_out_descriptor) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_descriptor);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hidl.base", "1.0", "IBase", "interfaceDescriptor", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_descriptor);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getHashChain(getHashChain_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getHashChain::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hidl.base", "1.0", "IBase", "getHashChain", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getHashChain([&](const auto &_hidl_out_hashchain) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_hashchain);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hidl.base", "1.0", "IBase", "getHashChain", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_hashchain);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> setHALInstrumentation() override {
        configureInstrumentation();
        return ::android::hardware::Void();
    }

    ::android::hardware::Return<bool> linkToDeath(const ::android::sp<::android::hardware::hidl_death_recipient>& recipient, uint64_t cookie) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::linkToDeath::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&recipient);
            _hidl_args.push_back((void *)&cookie);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hidl.base", "1.0", "IBase", "linkToDeath", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->linkToDeath(recipient, cookie);

        bool _hidl_out_success = _hidl_return;
        (void) _hidl_out_success;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_success);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hidl.base", "1.0", "IBase", "linkToDeath", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> ping() override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::ping::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hidl.base", "1.0", "IBase", "ping", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->ping();

        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hidl.base", "1.0", "IBase", "ping", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> getDebugInfo(getDebugInfo_cb _hidl_cb) override {
        if (_hidl_cb == nullptr) {
            return ::android::hardware::Status::fromExceptionCode(
                    ::android::hardware::Status::EX_ILLEGAL_ARGUMENT,
                    "Null synchronous callback passed.");
        }

        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::getDebugInfo::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hidl.base", "1.0", "IBase", "getDebugInfo", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->getDebugInfo([&](const auto &_hidl_out_info) {
            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                _hidl_args.push_back((void *)&_hidl_out_info);
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hidl.base", "1.0", "IBase", "getDebugInfo", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

            _hidl_cb(_hidl_out_info);
        });

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }
    ::android::hardware::Return<void> notifySyspropsChanged() override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::notifySyspropsChanged::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hidl.base", "1.0", "IBase", "notifySyspropsChanged", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = addOnewayTask([mImpl = this->mImpl
        #ifdef __ANDROID_DEBUGGABLE__
        , mEnableInstrumentation = this->mEnableInstrumentation, mInstrumentationCallbacks = this->mInstrumentationCallbacks
        #endif // __ANDROID_DEBUGGABLE__
        ] {
            mImpl->notifySyspropsChanged();

            atrace_end(ATRACE_TAG_HAL);
            #ifdef __ANDROID_DEBUGGABLE__
            if (UNLIKELY(mEnableInstrumentation)) {
                std::vector<void *> _hidl_args;
                for (const auto &callback: mInstrumentationCallbacks) {
                    callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hidl.base", "1.0", "IBase", "notifySyspropsChanged", &_hidl_args);
                }
            }
            #endif // __ANDROID_DEBUGGABLE__

        });
        return _hidl_return;
    }
    ::android::hardware::Return<bool> unlinkToDeath(const ::android::sp<::android::hardware::hidl_death_recipient>& recipient) override {
        atrace_begin(ATRACE_TAG_HAL, "HIDL::IComposerClient::unlinkToDeath::passthrough");
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&recipient);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_ENTRY, "android.hidl.base", "1.0", "IBase", "unlinkToDeath", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        ::android::hardware::Status _hidl_error = ::android::hardware::Status::ok();
        auto _hidl_return = mImpl->unlinkToDeath(recipient);

        bool _hidl_out_success = _hidl_return;
        (void) _hidl_out_success;
        atrace_end(ATRACE_TAG_HAL);
        #ifdef __ANDROID_DEBUGGABLE__
        if (UNLIKELY(mEnableInstrumentation)) {
            std::vector<void *> _hidl_args;
            _hidl_args.push_back((void *)&_hidl_out_success);
            for (const auto &callback: mInstrumentationCallbacks) {
                callback(InstrumentationEvent::PASSTHROUGH_EXIT, "android.hidl.base", "1.0", "IBase", "unlinkToDeath", &_hidl_args);
            }
        }
        #endif // __ANDROID_DEBUGGABLE__

        if (!_hidl_error.isOk()) return _hidl_error;
        return _hidl_return;
    }

private:
    const ::android::sp<IComposerClient> mImpl;
    ::android::hardware::details::TaskRunner mOnewayQueue;

    ::android::hardware::Return<void> addOnewayTask(std::function<void(void)>);

};

}  // namespace V2_4
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_BSCOMPOSERCLIENT_H
