#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_MAPPER_V3_0_IMAPPER_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_MAPPER_V3_0_IMAPPER_H

#include <android/hardware/graphics/common/1.2/types.h>
#include <android/hardware/graphics/mapper/3.0/types.h>
#include <android/hidl/base/1.0/IBase.h>

#include <android/hidl/manager/1.0/IServiceNotification.h>

#include <hidl/HidlSupport.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <utils/NativeHandle.h>
#include <utils/misc.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V3_0 {

struct IMapper : public ::android::hidl::base::V1_0::IBase {
    /**
     * Type tag for use in template logic that indicates this is a 'pure' class.
     */
    typedef android::hardware::details::i_tag _hidl_tag;

    /**
     * Fully qualified interface name: "android.hardware.graphics.mapper@3.0::IMapper"
     */
    static const char* descriptor;

    // Forward declaration for forward reference support:
    struct BufferDescriptorInfo;
    struct Rect;

    struct BufferDescriptorInfo final {
        uint32_t width __attribute__ ((aligned(4)));
        uint32_t height __attribute__ ((aligned(4)));
        uint32_t layerCount __attribute__ ((aligned(4)));
        ::android::hardware::graphics::common::V1_2::PixelFormat format __attribute__ ((aligned(4)));
        ::android::hardware::hidl_bitfield<::android::hardware::graphics::common::V1_2::BufferUsage> usage __attribute__ ((aligned(8)));
    };

    static_assert(offsetof(::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo, width) == 0, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo, height) == 4, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo, layerCount) == 8, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo, format) == 12, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo, usage) == 16, "wrong offset");
    static_assert(sizeof(::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo) == 24, "wrong size");
    static_assert(__alignof(::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo) == 8, "wrong alignment");

    struct Rect final {
        int32_t left __attribute__ ((aligned(4)));
        int32_t top __attribute__ ((aligned(4)));
        int32_t width __attribute__ ((aligned(4)));
        int32_t height __attribute__ ((aligned(4)));
    };

    static_assert(offsetof(::android::hardware::graphics::mapper::V3_0::IMapper::Rect, left) == 0, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V3_0::IMapper::Rect, top) == 4, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V3_0::IMapper::Rect, width) == 8, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V3_0::IMapper::Rect, height) == 12, "wrong offset");
    static_assert(sizeof(::android::hardware::graphics::mapper::V3_0::IMapper::Rect) == 16, "wrong size");
    static_assert(__alignof(::android::hardware::graphics::mapper::V3_0::IMapper::Rect) == 4, "wrong alignment");

    /**
     * Returns whether this object's implementation is outside of the current process.
     */
    virtual bool isRemote() const override { return false; }

    /**
     * Return callback for createDescriptor
     */
    using createDescriptor_cb = std::function<void(::android::hardware::graphics::mapper::V3_0::Error error, const ::android::hardware::hidl_vec<uint32_t>& descriptor)>;
    /**
     * Creates a buffer descriptor. The descriptor can be used with IAllocator
     * to allocate buffers.
     * 
     * Since the buffer descriptor fully describes a buffer, any device
     * dependent or device independent checks must be performed here whenever
     * possible. Specifically, when layered buffers are not supported, this
     * function must return `UNSUPPORTED` if `description.layers` is great than
     * 1.
     * 
     * @param description Attributes of the descriptor.
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_VALUE` if any of the specified attributes are invalid or
     *       inconsistent.
     *     - `NO_RESOURCES` if the creation cannot be fullfilled due to
     *       unavailability of resources.
     *     - `UNSUPPORTED` when any of the specified attributes are not
     *       supported.
     * @return descriptor Newly created buffer descriptor.
     */
    virtual ::android::hardware::Return<void> createDescriptor(const ::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo& description, createDescriptor_cb _hidl_cb) = 0;

    /**
     * Return callback for importBuffer
     */
    using importBuffer_cb = std::function<void(::android::hardware::graphics::mapper::V3_0::Error error, void* buffer)>;
    /**
     * Imports a raw buffer handle to create an imported buffer handle for use
     * with the rest of the mapper or with other in-process libraries.
     * 
     * A buffer handle is considered raw when it is cloned (e.g., with
     * `native_handle_clone()`) from another buffer handle locally, or when it
     * is received from another HAL server/client or another process. A raw
     * buffer handle must not be used to access the underlying graphic
     * buffer. It must be imported to create an imported handle first.
     * 
     * This function must at least validate the raw handle before creating the
     * imported handle. It must also support importing the same raw handle
     * multiple times to create multiple imported handles. The imported handle
     * must be considered valid everywhere in the process, including in
     * another instance of the mapper.
     * 
     * Because of passthrough HALs, a raw buffer handle received from a HAL
     * may actually have been imported in the process. importBuffer() must treat
     * such a handle as if it is raw and must not return `BAD_BUFFER`. The
     * returned handle is independent from the input handle as usual, and
     * freeBuffer() must be called on it when it is no longer needed.
     * 
     * @param rawHandle Raw buffer handle to import.
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the raw handle is invalid.
     *     - `NO_RESOURCES` if the raw handle cannot be imported due to
     *       unavailability of resources.
     * @return buffer Imported buffer handle that has the type
     *     `buffer_handle_t` which is a handle type.
     */
    virtual ::android::hardware::Return<void> importBuffer(const ::android::hardware::hidl_handle& rawHandle, importBuffer_cb _hidl_cb) = 0;

    /**
     * Frees a buffer handle. Buffer handles returned by importBuffer() must be
     * freed with this function when no longer needed.
     * 
     * This function must free up all resources allocated by importBuffer() for
     * the imported handle. For example, if the imported handle was created
     * with `native_handle_create()`, this function must call
     * `native_handle_close()` and `native_handle_delete()`.
     * 
     * @param buffer Imported buffer handle.
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the buffer is invalid.
     */
    virtual ::android::hardware::Return<::android::hardware::graphics::mapper::V3_0::Error> freeBuffer(void* buffer) = 0;

    /**
     * Validates that the buffer can be safely accessed by a caller who assumes
     * the specified @p description and @p stride. This must at least validate
     * that the buffer size is large enough. Validating the buffer against
     * individual buffer attributes is optional.
     * 
     * @param buffer Buffer to validate against.
     * @param description Attributes of the buffer.
     * @param stride Stride returned by IAllocator::allocate().
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the buffer is invalid.
     *     - `BAD_VALUE` if the buffer cannot be safely accessed.
     */
    virtual ::android::hardware::Return<::android::hardware::graphics::mapper::V3_0::Error> validateBufferSize(void* buffer, const ::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo& description, uint32_t stride) = 0;

    /**
     * Return callback for getTransportSize
     */
    using getTransportSize_cb = std::function<void(::android::hardware::graphics::mapper::V3_0::Error error, uint32_t numFds, uint32_t numInts)>;
    /**
     * Calculates the transport size of a buffer. An imported buffer handle is a
     * raw buffer handle with the process-local runtime data appended. This
     * function, for example, allows a caller to omit the process-local runtime
     * data at the tail when serializing the imported buffer handle.
     * 
     * Note that a client might or might not omit the process-local runtime data
     * when sending an imported buffer handle. The mapper must support both
     * cases on the receiving end.
     * 
     * @param buffer Buffer to get the transport size from.
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the buffer is invalid.
     * @return numFds The number of file descriptors needed for transport.
     * @return numInts The number of integers needed for transport.
     */
    virtual ::android::hardware::Return<void> getTransportSize(void* buffer, getTransportSize_cb _hidl_cb) = 0;

    /**
     * Return callback for lock
     */
    using lock_cb = std::function<void(::android::hardware::graphics::mapper::V3_0::Error error, void* data, int32_t bytesPerPixel, int32_t bytesPerStride)>;
    /**
     * Locks the given buffer for the specified CPU usage.
     * 
     * Locking the same buffer simultaneously from multiple threads is
     * permitted, but if any of the threads attempt to lock the buffer for
     * writing, the behavior is undefined, except that it must not cause
     * process termination or block the client indefinitely. Leaving the
     * buffer content in an indeterminate state or returning an error are both
     * acceptable.
     * 
     * 1D buffers (width = size in bytes, height = 1, pixel_format = BLOB) must
     * "lock in place". The buffers must be directly accessible via mapping.
     * 
     * The client must not modify the content of the buffer outside of
     * @p accessRegion, and the device need not guarantee that content outside
     * of @p accessRegion is valid for reading. The result of reading or writing
     * outside of @p accessRegion is undefined, except that it must not cause
     * process termination.
     * 
     * On success, @p data must be filled with a pointer to the locked buffer
     * memory. This address will represent the top-left corner of the entire
     * buffer, even if @p accessRegion does not begin at the top-left corner.
     * 
     * On success, bytesPerPixel must contain the number of bytes per pixel in
     * the buffer. If the bytesPerPixel is unknown or variable, a value of -1
     * should be returned. bytesPerStride must contain the bytes per stride of
     * the buffer. If the bytesPerStride is unknown or variable, a value of -1
     * should be returned.
     * 
     * @param buffer Buffer to lock.
     * @param cpuUsage CPU usage flags to request. See +ndk
     *     libnativewindow#AHardwareBuffer_UsageFlags for possible values.
     * @param accessRegion Portion of the buffer that the client intends to
     *     access.
     * @param acquireFence Handle containing a file descriptor referring to a
     *     sync fence object, which will be signaled when it is safe for the
     *     mapper to lock the buffer. @p acquireFence may be an empty fence if
     *     it is already safe to lock.
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the buffer is invalid or is incompatible with this
     *       function.
     *     - `BAD_VALUE` if @p cpuUsage is 0, contains non-CPU usage flags, or
     *       is incompatible with the buffer.
     *     - `NO_RESOURCES` if the buffer cannot be locked at this time. Note
     *       that locking may succeed at a later time.
     * @return data CPU-accessible pointer to the buffer data.
     * @return bytesPerPixel the number of bytes per pixel in the buffer
     * @return bytesPerStride the number of bytes per stride of the buffer
     */
    virtual ::android::hardware::Return<void> lock(void* buffer, uint64_t cpuUsage, const ::android::hardware::graphics::mapper::V3_0::IMapper::Rect& accessRegion, const ::android::hardware::hidl_handle& acquireFence, lock_cb _hidl_cb) = 0;

    /**
     * Return callback for lockYCbCr
     */
    using lockYCbCr_cb = std::function<void(::android::hardware::graphics::mapper::V3_0::Error error, const ::android::hardware::graphics::mapper::V3_0::YCbCrLayout& layout)>;
    /**
     * Locks a YCbCr buffer for the specified CPU usage.
     * 
     * This is largely the same as lock(), except that instead of returning a
     * pointer directly to the buffer data, it returns a `YCbCrLayout` struct
     * describing how to access the data planes.
     * 
     * This function must work on buffers with
     * `AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_*` if supported by the device, as well
     * as with any other formats requested by multimedia codecs when they are
     * configured with a flexible-YUV-compatible color format.
     * 
     * @param buffer Buffer to lock.
     * @param cpuUsage CPU usage flags to request. See +ndk
     *     libnativewindow#AHardwareBuffer_UsageFlags for possible values.
     * @param accessRegion Portion of the buffer that the client intends to
     *     access.
     * @param acquireFence Handle containing a file descriptor referring to a
     *     sync fence object, which will be signaled when it is safe for the
     *     mapper to lock the buffer. @p acquireFence may be empty if it is
     *     already safe to lock.
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the buffer is invalid or is incompatible with this
     *       function.
     *     - `BAD_VALUE` if @p cpuUsage is 0, contains non-CPU usage flags, or
     *       is incompatible with the buffer.
     *     - `NO_RESOURCES` if the buffer cannot be locked at this time. Note
     *       that locking may succeed at a later time.
     * @return layout Data layout of the locked buffer.
     */
    virtual ::android::hardware::Return<void> lockYCbCr(void* buffer, uint64_t cpuUsage, const ::android::hardware::graphics::mapper::V3_0::IMapper::Rect& accessRegion, const ::android::hardware::hidl_handle& acquireFence, lockYCbCr_cb _hidl_cb) = 0;

    /**
     * Return callback for unlock
     */
    using unlock_cb = std::function<void(::android::hardware::graphics::mapper::V3_0::Error error, const ::android::hardware::hidl_handle& releaseFence)>;
    /**
     * Unlocks a buffer to indicate all CPU accesses to the buffer have
     * completed.
     * 
     * @param buffer Buffer to unlock.
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the buffer is invalid or not locked.
     * @return releaseFence Handle containing a file descriptor referring to a
     *     sync fence object. The sync fence object will be signaled when the
     *     mapper has completed any pending work. @p releaseFence may be an
     *     empty fence.
     */
    virtual ::android::hardware::Return<void> unlock(void* buffer, unlock_cb _hidl_cb) = 0;

    /**
     * Return callback for isSupported
     */
    using isSupported_cb = std::function<void(::android::hardware::graphics::mapper::V3_0::Error error, bool supported)>;
    /**
     * Test whether the given BufferDescriptorInfo is allocatable.
     * 
     * If this function returns true, it means that a buffer with the given
     * description can be allocated on this implementation, unless resource
     * exhaustion occurs. If this function returns false, it means that the
     * allocation of the given description will never succeed.
     * 
     * @param description the description of the buffer
     * @return supported whether the description is supported
     */
    virtual ::android::hardware::Return<void> isSupported(const ::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo& description, isSupported_cb _hidl_cb) = 0;

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
    static ::android::hardware::Return<::android::sp<::android::hardware::graphics::mapper::V3_0::IMapper>> castFrom(const ::android::sp<::android::hardware::graphics::mapper::V3_0::IMapper>& parent, bool emitError = false);
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::android::hardware::graphics::mapper::V3_0::IMapper>> castFrom(const ::android::sp<::android::hidl::base::V1_0::IBase>& parent, bool emitError = false);

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
    static ::android::sp<IMapper> tryGetService(const std::string &serviceName="default", bool getStub=false);
    /**
     * Deprecated. See tryGetService(std::string, bool)
     */
    static ::android::sp<IMapper> tryGetService(const char serviceName[], bool getStub=false)  { std::string str(serviceName ? serviceName : "");      return tryGetService(str, getStub); }
    /**
     * Deprecated. See tryGetService(std::string, bool)
     */
    static ::android::sp<IMapper> tryGetService(const ::android::hardware::hidl_string& serviceName, bool getStub=false)  { std::string str(serviceName.c_str());      return tryGetService(str, getStub); }
    /**
     * Calls tryGetService("default", bool). This is the recommended instance name for singleton services.
     */
    static ::android::sp<IMapper> tryGetService(bool getStub) { return tryGetService("default", getStub); }
    /**
     * This gets the service of this type with the specified instance name. If the
     * service is not in the VINTF manifest on a Trebilized device, this will return
     * nullptr. If the service is not available, this will wait for the service to
     * become available. If the service is a lazy service, this will start the service
     * and return when it becomes available. If getStub is true, this will try to
     * return an unwrapped passthrough implementation in the same process. This is
     * useful when getting an implementation from the same partition/compilation group.
     */
    static ::android::sp<IMapper> getService(const std::string &serviceName="default", bool getStub=false);
    /**
     * Deprecated. See getService(std::string, bool)
     */
    static ::android::sp<IMapper> getService(const char serviceName[], bool getStub=false)  { std::string str(serviceName ? serviceName : "");      return getService(str, getStub); }
    /**
     * Deprecated. See getService(std::string, bool)
     */
    static ::android::sp<IMapper> getService(const ::android::hardware::hidl_string& serviceName, bool getStub=false)  { std::string str(serviceName.c_str());      return getService(str, getStub); }
    /**
     * Calls getService("default", bool). This is the recommended instance name for singleton services.
     */
    static ::android::sp<IMapper> getService(bool getStub) { return getService("default", getStub); }
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

static inline std::string toString(const ::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo& o);
static inline bool operator==(const ::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo& lhs, const ::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo& rhs);
static inline bool operator!=(const ::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo& lhs, const ::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo& rhs);

static inline std::string toString(const ::android::hardware::graphics::mapper::V3_0::IMapper::Rect& o);
static inline bool operator==(const ::android::hardware::graphics::mapper::V3_0::IMapper::Rect& lhs, const ::android::hardware::graphics::mapper::V3_0::IMapper::Rect& rhs);
static inline bool operator!=(const ::android::hardware::graphics::mapper::V3_0::IMapper::Rect& lhs, const ::android::hardware::graphics::mapper::V3_0::IMapper::Rect& rhs);

static inline std::string toString(const ::android::sp<::android::hardware::graphics::mapper::V3_0::IMapper>& o);

//
// type header definitions for package
//

static inline std::string toString(const ::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo& o) {
    using ::android::hardware::toString;
    std::string os;
    os += "{";
    os += ".width = ";
    os += ::android::hardware::toString(o.width);
    os += ", .height = ";
    os += ::android::hardware::toString(o.height);
    os += ", .layerCount = ";
    os += ::android::hardware::toString(o.layerCount);
    os += ", .format = ";
    os += ::android::hardware::graphics::common::V1_2::toString(o.format);
    os += ", .usage = ";
    os += ::android::hardware::graphics::common::V1_2::toString<::android::hardware::graphics::common::V1_2::BufferUsage>(o.usage);
    os += "}"; return os;
}

static inline bool operator==(const ::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo& lhs, const ::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo& rhs) {
    if (lhs.width != rhs.width) {
        return false;
    }
    if (lhs.height != rhs.height) {
        return false;
    }
    if (lhs.layerCount != rhs.layerCount) {
        return false;
    }
    if (lhs.format != rhs.format) {
        return false;
    }
    if (lhs.usage != rhs.usage) {
        return false;
    }
    return true;
}

static inline bool operator!=(const ::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo& lhs, const ::android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo& rhs){
    return !(lhs == rhs);
}

static inline std::string toString(const ::android::hardware::graphics::mapper::V3_0::IMapper::Rect& o) {
    using ::android::hardware::toString;
    std::string os;
    os += "{";
    os += ".left = ";
    os += ::android::hardware::toString(o.left);
    os += ", .top = ";
    os += ::android::hardware::toString(o.top);
    os += ", .width = ";
    os += ::android::hardware::toString(o.width);
    os += ", .height = ";
    os += ::android::hardware::toString(o.height);
    os += "}"; return os;
}

static inline bool operator==(const ::android::hardware::graphics::mapper::V3_0::IMapper::Rect& lhs, const ::android::hardware::graphics::mapper::V3_0::IMapper::Rect& rhs) {
    if (lhs.left != rhs.left) {
        return false;
    }
    if (lhs.top != rhs.top) {
        return false;
    }
    if (lhs.width != rhs.width) {
        return false;
    }
    if (lhs.height != rhs.height) {
        return false;
    }
    return true;
}

static inline bool operator!=(const ::android::hardware::graphics::mapper::V3_0::IMapper::Rect& lhs, const ::android::hardware::graphics::mapper::V3_0::IMapper::Rect& rhs){
    return !(lhs == rhs);
}

static inline std::string toString(const ::android::sp<::android::hardware::graphics::mapper::V3_0::IMapper>& o) {
    std::string os = "[class or subclass of ";
    os += ::android::hardware::graphics::mapper::V3_0::IMapper::descriptor;
    os += "]";
    os += o->isRemote() ? "@remote" : "@local";
    return os;
}


}  // namespace V3_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android

//
// global type declarations for package
//


#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_MAPPER_V3_0_IMAPPER_H
