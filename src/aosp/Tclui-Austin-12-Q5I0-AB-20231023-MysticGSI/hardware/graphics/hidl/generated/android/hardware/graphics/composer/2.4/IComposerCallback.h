#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_ICOMPOSERCALLBACK_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_ICOMPOSERCALLBACK_H

#include <android/hardware/graphics/composer/2.1/IComposerCallback.h>
#include <android/hardware/graphics/composer/2.1/types.h>
#include <android/hardware/graphics/composer/2.4/types.h>

#include <android/hidl/manager/1.0/IServiceNotification.h>

#include <hidl/HidlSupport.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <utils/NativeHandle.h>
#include <utils/misc.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_4 {

struct IComposerCallback : public ::android::hardware::graphics::composer::V2_1::IComposerCallback {
    /**
     * Type tag for use in template logic that indicates this is a 'pure' class.
     */
    typedef android::hardware::details::i_tag _hidl_tag;

    /**
     * Fully qualified interface name: "android.hardware.graphics.composer@2.4::IComposerCallback"
     */
    static const char* descriptor;

    /**
     * Returns whether this object's implementation is outside of the current process.
     */
    virtual bool isRemote() const override { return false; }

    // @callflow(next="*")
    /**
     * Notifies the client that the given display has either been connected or
     * disconnected. Every active display (even a built-in physical display)
     * must trigger at least one hotplug notification, even if it only occurs
     * immediately after callback registration.
     * 
     * Displays which have been connected are assumed to be in PowerMode::OFF,
     * and the onVsync callback should not be called for a display until vsync
     * has been enabled with setVsyncEnabled.
     * 
     * The client may call back into the device while the callback is in
     * progress. The device must serialize calls to this callback such that
     * only one thread is calling it at a time.
     * 
     * @param display is the display that triggers the hotplug event.
     * @param connected indicates whether the display is connected or
     *        disconnected.
     */
    virtual ::android::hardware::Return<void> onHotplug(uint64_t display, ::android::hardware::graphics::composer::V2_1::IComposerCallback::Connection connected) = 0;

    // @callflow(next="*")
    /**
     * Notifies the client to trigger a screen refresh. This forces all layer
     * state for this display to be resent, and the display to be validated
     * and presented, even if there have been no changes.
     * 
     * This refresh will occur some time after the callback is initiated, but
     * not necessarily before it returns.  It is safe to trigger this callback
     * from other functions which call into the device.
     * 
     * @param display is the display to refresh.
     */
    virtual ::android::hardware::Return<void> onRefresh(uint64_t display) = 0;

    // @callflow(next="*")
    /**
     * Notifies the client that a vsync event has occurred. This callback must
     * only be triggered when vsync is enabled for this display (through
     * setVsyncEnabled).
     * 
     * @param display is the display which has received a vsync event
     * @param timestamp is the CLOCK_MONOTONIC time at which the vsync event
     *        occurred, in nanoseconds.
     */
    virtual ::android::hardware::Return<void> onVsync(uint64_t display, int64_t timestamp) = 0;

    /**
     * Notifies the client that a vsync event has occurred. This callback must
     * only be triggered when vsync is enabled for this display (through
     * setVsyncEnabled).
     * 
     * @param display is the display which has received a vsync event
     * @param timestamp is the CLOCK_MONOTONIC time at which the vsync event
     *        occurred, in nanoseconds.
     * @param vsyncPeriodNanos is the display vsync period in nanoseconds i.e. the next onVsync_2_4
     *        is expected to be called vsyncPeriodNanos nanoseconds after this call.
     */
    virtual ::android::hardware::Return<void> onVsync_2_4(uint64_t display, int64_t timestamp, uint32_t vsyncPeriodNanos) = 0;

    /**
     * Notifies the client that the previously reported timing for vsync period change has been
     * updated. This may occur if the composer missed the deadline for changing the vsync period
     * or the client submitted a refresh frame too late.
     * 
     * @param display is the display which vsync period change is in progress
     * @param updatedTimeline is the new timeline for the vsync period change.
     */
    virtual ::android::hardware::Return<void> onVsyncPeriodTimingChanged(uint64_t display, const ::android::hardware::graphics::composer::V2_4::VsyncPeriodChangeTimeline& updatedTimeline) = 0;

    /**
     * Notifies the client that the conditions which previously led to returning
     * SEAMLESS_NOT_POSSIBLE from setActiveConfigWithConstraints have changed and now seamless may
     * be possible. Client should retry calling setActiveConfigWithConstraints.
     * 
     * @param display is a display setActiveConfigWithConstraints previously failed with
     * SEAMLESS_NOT_POSSIBLE.
     */
    virtual ::android::hardware::Return<void> onSeamlessPossible(uint64_t display) = 0;

    /**
     * Return callback for interfaceChain
     */
    using interfaceChain_cb = std::function<void(const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& descriptors)>;
    virtual ::android::hardware::Return<void> interfaceChain(interfaceChain_cb _hidl_cb) override;

    virtual ::android::hardware::Return<void> debug(const ::android::hardware::hidl_handle& fd, const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& options) override;

    /**
     * Return callback for interfaceDescriptor
     */
    using interfaceDescriptor_cb = std::function<void(const ::android::hardware::hidl_string& descriptor)>;
    virtual ::android::hardware::Return<void> interfaceDescriptor(interfaceDescriptor_cb _hidl_cb) override;

    /**
     * Return callback for getHashChain
     */
    using getHashChain_cb = std::function<void(const ::android::hardware::hidl_vec<::android::hardware::hidl_array<uint8_t, 32>>& hashchain)>;
    virtual ::android::hardware::Return<void> getHashChain(getHashChain_cb _hidl_cb) override;

    virtual ::android::hardware::Return<void> setHALInstrumentation() override;

    virtual ::android::hardware::Return<bool> linkToDeath(const ::android::sp<::android::hardware::hidl_death_recipient>& recipient, uint64_t cookie) override;

    virtual ::android::hardware::Return<void> ping() override;

    /**
     * Return callback for getDebugInfo
     */
    using getDebugInfo_cb = std::function<void(const ::android::hidl::base::V1_0::DebugInfo& info)>;
    virtual ::android::hardware::Return<void> getDebugInfo(getDebugInfo_cb _hidl_cb) override;

    virtual ::android::hardware::Return<void> notifySyspropsChanged() override;

    virtual ::android::hardware::Return<bool> unlinkToDeath(const ::android::sp<::android::hardware::hidl_death_recipient>& recipient) override;

    // cast static functions
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::android::hardware::graphics::composer::V2_4::IComposerCallback>> castFrom(const ::android::sp<::android::hardware::graphics::composer::V2_4::IComposerCallback>& parent, bool emitError = false);
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::android::hardware::graphics::composer::V2_4::IComposerCallback>> castFrom(const ::android::sp<::android::hardware::graphics::composer::V2_1::IComposerCallback>& parent, bool emitError = false);
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::android::hardware::graphics::composer::V2_4::IComposerCallback>> castFrom(const ::android::sp<::android::hidl::base::V1_0::IBase>& parent, bool emitError = false);

    // helper methods for interactions with the hwservicemanager
    /**
     * This gets the service of this type with the specified instance name. If the
     * service is currently not available or not in the VINTF manifest on a Trebilized
     * device, this will return nullptr. This is useful when you don't want to block
     * during device boot. If getStub is true, this will try to return an unwrapped
     * passthrough implementation in the same process. This is useful when getting an
     * implementation from the same partition/compilation group.
     * 
     * In general, prefer getService(std::string,bool)
     */
    static ::android::sp<IComposerCallback> tryGetService(const std::string &serviceName="default", bool getStub=false);
    /**
     * Deprecated. See tryGetService(std::string, bool)
     */
    static ::android::sp<IComposerCallback> tryGetService(const char serviceName[], bool getStub=false)  { std::string str(serviceName ? serviceName : "");      return tryGetService(str, getStub); }
    /**
     * Deprecated. See tryGetService(std::string, bool)
     */
    static ::android::sp<IComposerCallback> tryGetService(const ::android::hardware::hidl_string& serviceName, bool getStub=false)  { std::string str(serviceName.c_str());      return tryGetService(str, getStub); }
    /**
     * Calls tryGetService("default", bool). This is the recommended instance name for singleton services.
     */
    static ::android::sp<IComposerCallback> tryGetService(bool getStub) { return tryGetService("default", getStub); }
    /**
     * This gets the service of this type with the specified instance name. If the
     * service is not in the VINTF manifest on a Trebilized device, this will return
     * nullptr. If the service is not available, this will wait for the service to
     * become available. If the service is a lazy service, this will start the service
     * and return when it becomes available. If getStub is true, this will try to
     * return an unwrapped passthrough implementation in the same process. This is
     * useful when getting an implementation from the same partition/compilation group.
     */
    static ::android::sp<IComposerCallback> getService(const std::string &serviceName="default", bool getStub=false);
    /**
     * Deprecated. See getService(std::string, bool)
     */
    static ::android::sp<IComposerCallback> getService(const char serviceName[], bool getStub=false)  { std::string str(serviceName ? serviceName : "");      return getService(str, getStub); }
    /**
     * Deprecated. See getService(std::string, bool)
     */
    static ::android::sp<IComposerCallback> getService(const ::android::hardware::hidl_string& serviceName, bool getStub=false)  { std::string str(serviceName.c_str());      return getService(str, getStub); }
    /**
     * Calls getService("default", bool). This is the recommended instance name for singleton services.
     */
    static ::android::sp<IComposerCallback> getService(bool getStub) { return getService("default", getStub); }
    /**
     * Registers a service with the service manager. For Trebilized devices, the service
     * must also be in the VINTF manifest.
     */
    __attribute__ ((warn_unused_result))::android::status_t registerAsService(const std::string &serviceName="default");
    /**
     * Registers for notifications for when a service is registered.
     */
    static bool registerForNotifications(
            const std::string &serviceName,
            const ::android::sp<::android::hidl::manager::V1_0::IServiceNotification> &notification);
};

//
// type declarations for package
//

static inline std::string toString(const ::android::sp<::android::hardware::graphics::composer::V2_4::IComposerCallback>& o);

//
// type header definitions for package
//

static inline std::string toString(const ::android::sp<::android::hardware::graphics::composer::V2_4::IComposerCallback>& o) {
    std::string os = "[class or subclass of ";
    os += ::android::hardware::graphics::composer::V2_4::IComposerCallback::descriptor;
    os += "]";
    os += o->isRemote() ? "@remote" : "@local";
    return os;
}


}  // namespace V2_4
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android

//
// global type declarations for package
//


#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_ICOMPOSERCALLBACK_H
