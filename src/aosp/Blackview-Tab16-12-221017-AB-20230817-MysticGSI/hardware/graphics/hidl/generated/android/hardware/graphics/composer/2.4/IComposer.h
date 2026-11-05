#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_ICOMPOSER_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_ICOMPOSER_H

#include <android/hardware/graphics/composer/2.3/IComposer.h>
#include <android/hardware/graphics/composer/2.4/IComposerClient.h>
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

struct IComposer : public ::android::hardware::graphics::composer::V2_3::IComposer {
    /**
     * Type tag for use in template logic that indicates this is a 'pure' class.
     */
    typedef android::hardware::details::i_tag _hidl_tag;

    /**
     * Fully qualified interface name: "android.hardware.graphics.composer@2.4::IComposer"
     */
    static const char* descriptor;

    /**
     * Returns whether this object's implementation is outside of the current process.
     */
    virtual bool isRemote() const override { return false; }

    /**
     * Return callback for getCapabilities
     */
    using getCapabilities_cb = std::function<void(const ::android::hardware::hidl_vec<::android::hardware::graphics::composer::V2_1::IComposer::Capability>& capabilities)>;
    // @entry @exit @callflow(next="*")
    /**
     * Provides a list of supported capabilities (as described in the
     * definition of Capability above). This list must not change after
     * initialization.
     * 
     * @return capabilities is a list of supported capabilities.
     */
    virtual ::android::hardware::Return<void> getCapabilities(getCapabilities_cb _hidl_cb) = 0;

    /**
     * Return callback for dumpDebugInfo
     */
    using dumpDebugInfo_cb = std::function<void(const ::android::hardware::hidl_string& debugInfo)>;
    // @entry @exit @callflow(next="*")
    /**
     * Retrieves implementation-defined debug information, which will be
     * displayed during, for example, `dumpsys SurfaceFlinger`.
     * 
     * @return debugInfo is a string of debug information.
     */
    virtual ::android::hardware::Return<void> dumpDebugInfo(dumpDebugInfo_cb _hidl_cb) = 0;

    /**
     * Return callback for createClient
     */
    using createClient_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, const ::android::sp<::android::hardware::graphics::composer::V2_1::IComposerClient>& client)>;
    // @entry @callflow(next="*")
    /**
     * Creates a client of the composer. All resources created by the client
     * are owned by the client and are only visible to the client.
     * 
     * There can only be one client at any time.
     * 
     * @return error is NONE upon success. Otherwise,
     *         NO_RESOURCES when no more client can be created currently.
     * @return client is the newly created client.
     */
    virtual ::android::hardware::Return<void> createClient(createClient_cb _hidl_cb) = 0;

    /**
     * Return callback for createClient_2_3
     */
    using createClient_2_3_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, const ::android::sp<::android::hardware::graphics::composer::V2_3::IComposerClient>& client)>;
    // @entry @callflow(next="*")
    /**
     * Creates a v2.3 client of the composer. Supersedes @2.1::createClient.
     * 
     * @return error is NONE upon success. Otherwise,
     *         NO_RESOURCES when the client could not be created.
     * @return client is the newly created client.
     */
    virtual ::android::hardware::Return<void> createClient_2_3(createClient_2_3_cb _hidl_cb) = 0;

    /**
     * Return callback for createClient_2_4
     */
    using createClient_2_4_cb = std::function<void(::android::hardware::graphics::composer::V2_4::Error error, const ::android::sp<::android::hardware::graphics::composer::V2_4::IComposerClient>& client)>;
    /**
     * Creates a v2.4 client of the composer. Supersedes @2.3::createClient.
     * 
     * @return error is NONE upon success. Otherwise,
     *         NO_RESOURCES when the client could not be created.
     * @return client is the newly created client.
     */
    virtual ::android::hardware::Return<void> createClient_2_4(createClient_2_4_cb _hidl_cb) = 0;

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
    static ::android::hardware::Return<::android::sp<::android::hardware::graphics::composer::V2_4::IComposer>> castFrom(const ::android::sp<::android::hardware::graphics::composer::V2_4::IComposer>& parent, bool emitError = false);
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::android::hardware::graphics::composer::V2_4::IComposer>> castFrom(const ::android::sp<::android::hardware::graphics::composer::V2_3::IComposer>& parent, bool emitError = false);
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::android::hardware::graphics::composer::V2_4::IComposer>> castFrom(const ::android::sp<::android::hardware::graphics::composer::V2_2::IComposer>& parent, bool emitError = false);
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::android::hardware::graphics::composer::V2_4::IComposer>> castFrom(const ::android::sp<::android::hardware::graphics::composer::V2_1::IComposer>& parent, bool emitError = false);
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::android::hardware::graphics::composer::V2_4::IComposer>> castFrom(const ::android::sp<::android::hidl::base::V1_0::IBase>& parent, bool emitError = false);

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
    static ::android::sp<IComposer> tryGetService(const std::string &serviceName="default", bool getStub=false);
    /**
     * Deprecated. See tryGetService(std::string, bool)
     */
    static ::android::sp<IComposer> tryGetService(const char serviceName[], bool getStub=false)  { std::string str(serviceName ? serviceName : "");      return tryGetService(str, getStub); }
    /**
     * Deprecated. See tryGetService(std::string, bool)
     */
    static ::android::sp<IComposer> tryGetService(const ::android::hardware::hidl_string& serviceName, bool getStub=false)  { std::string str(serviceName.c_str());      return tryGetService(str, getStub); }
    /**
     * Calls tryGetService("default", bool). This is the recommended instance name for singleton services.
     */
    static ::android::sp<IComposer> tryGetService(bool getStub) { return tryGetService("default", getStub); }
    /**
     * This gets the service of this type with the specified instance name. If the
     * service is not in the VINTF manifest on a Trebilized device, this will return
     * nullptr. If the service is not available, this will wait for the service to
     * become available. If the service is a lazy service, this will start the service
     * and return when it becomes available. If getStub is true, this will try to
     * return an unwrapped passthrough implementation in the same process. This is
     * useful when getting an implementation from the same partition/compilation group.
     */
    static ::android::sp<IComposer> getService(const std::string &serviceName="default", bool getStub=false);
    /**
     * Deprecated. See getService(std::string, bool)
     */
    static ::android::sp<IComposer> getService(const char serviceName[], bool getStub=false)  { std::string str(serviceName ? serviceName : "");      return getService(str, getStub); }
    /**
     * Deprecated. See getService(std::string, bool)
     */
    static ::android::sp<IComposer> getService(const ::android::hardware::hidl_string& serviceName, bool getStub=false)  { std::string str(serviceName.c_str());      return getService(str, getStub); }
    /**
     * Calls getService("default", bool). This is the recommended instance name for singleton services.
     */
    static ::android::sp<IComposer> getService(bool getStub) { return getService("default", getStub); }
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

static inline std::string toString(const ::android::sp<::android::hardware::graphics::composer::V2_4::IComposer>& o);

//
// type header definitions for package
//

static inline std::string toString(const ::android::sp<::android::hardware::graphics::composer::V2_4::IComposer>& o) {
    std::string os = "[class or subclass of ";
    os += ::android::hardware::graphics::composer::V2_4::IComposer::descriptor;
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


#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_4_ICOMPOSER_H
