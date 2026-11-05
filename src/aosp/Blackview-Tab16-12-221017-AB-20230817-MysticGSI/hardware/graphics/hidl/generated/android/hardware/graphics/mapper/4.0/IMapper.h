#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_MAPPER_V4_0_IMAPPER_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_MAPPER_V4_0_IMAPPER_H

#include <android/hardware/graphics/common/1.2/types.h>
#include <android/hardware/graphics/mapper/4.0/types.h>
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
namespace V4_0 {

struct IMapper : public ::android::hidl::base::V1_0::IBase {
    /**
     * Type tag for use in template logic that indicates this is a 'pure' class.
     */
    typedef android::hardware::details::i_tag _hidl_tag;

    /**
     * Fully qualified interface name: "android.hardware.graphics.mapper@4.0::IMapper"
     */
    static const char* descriptor;

    // Forward declaration for forward reference support:
    struct BufferDescriptorInfo;
    struct Rect;
    struct MetadataType;
    struct MetadataTypeDescription;
    struct MetadataDump;
    struct BufferDump;

    struct BufferDescriptorInfo final {
        ::android::hardware::hidl_string name __attribute__ ((aligned(8)));
        uint32_t width __attribute__ ((aligned(4)));
        uint32_t height __attribute__ ((aligned(4)));
        uint32_t layerCount __attribute__ ((aligned(4)));
        ::android::hardware::graphics::common::V1_2::PixelFormat format __attribute__ ((aligned(4)));
        ::android::hardware::hidl_bitfield<::android::hardware::graphics::common::V1_2::BufferUsage> usage __attribute__ ((aligned(8)));
        uint64_t reservedSize __attribute__ ((aligned(8)));
    };

    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo, name) == 0, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo, width) == 16, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo, height) == 20, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo, layerCount) == 24, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo, format) == 28, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo, usage) == 32, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo, reservedSize) == 40, "wrong offset");
    static_assert(sizeof(::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo) == 48, "wrong size");
    static_assert(__alignof(::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo) == 8, "wrong alignment");

    struct Rect final {
        int32_t left __attribute__ ((aligned(4)));
        int32_t top __attribute__ ((aligned(4)));
        int32_t width __attribute__ ((aligned(4)));
        int32_t height __attribute__ ((aligned(4)));
    };

    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::Rect, left) == 0, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::Rect, top) == 4, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::Rect, width) == 8, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::Rect, height) == 12, "wrong offset");
    static_assert(sizeof(::android::hardware::graphics::mapper::V4_0::IMapper::Rect) == 16, "wrong size");
    static_assert(__alignof(::android::hardware::graphics::mapper::V4_0::IMapper::Rect) == 4, "wrong alignment");

    /**
     * Description for get(...), set(...) and getFromBufferDescriptorInfo(...)
     * 
     * ------------ Overview -----------------------------------
     * Gralloc 4 adds support for getting and setting buffer metadata on a buffer.
     * 
     * To get buffer metadata, the client passes in a buffer handle and a token that
     * represents the type of buffer metadata they would like to get. IMapper returns
     * a byte stream that contains the buffer metadata. To set the buffer metadata, the
     * client passes in a buffer handle and a token that represents the type of buffer
     * metadata they would like to set and a byte stream that contains the buffer metadata
     * they are setting.
     * 
     * Buffer metadata is global for a buffer. When the metadata is set on the buffer
     * in a process, the updated metadata should be available to all other processes.
     * Please see "Storing and Propagating Metadata" below for more details.
     * 
     * The getter and setter functions have been optimized for easy vendor extension.
     * They do not require a formal HIDL extension to add support for getting and setting
     * vendor defined buffer metadata. In order to allow easy extension, the types used
     * here are not typical HIDL types. See "Buffer Metadata Token" and
     * "Buffer Metadata Stream" below for more details.
     * 
     * ------------ Storing and Propagating Metadata -----------
     * Buffer metadata must be global. Any changes to the metadata must be propagated
     * to all other processes immediately. Vendors may chose how they would like support
     * this functionality.
     * 
     * We recommend supporting this functionality by allocating an extra page of shared
     * memory and storing it in the buffer's native_handle_t. The buffer metadata can
     * be stored in the extra page of shared memory. Set operations are automatically
     * propagated to all other processes.
     * 
     * ------------ Buffer Metadata Synchronization ------------
     * There are no explicit buffer metadata synchronization primitives. Many devices
     * before gralloc 4 already support getting and setting of global buffer metadata
     * with no explicit synchronization primitives. Adding synchronization primitives
     * would just add unnecessary complexity.
     * 
     * The general rule is if a process has permission to write to a buffer, they
     * have permission to write to the buffer's metadata. If a process has permission
     * to read from a buffer, they have permission to read the buffer's metadata.
     * 
     * There is one exception to this rule. Fences CANNOT be used to protect a buffer's
     * metadata. A process should finish writing to a buffer's metadata before
     * sending the buffer to another process that will read or write to the buffer.
     * This exception is needed because sometimes userspace needs to read the
     * buffer's metadata before the buffer's contents are ready.
     * 
     * As a simple example: an app renders to a buffer and then displays the buffer.
     * In this example when the app renders to the buffer, both the buffer and its
     * metadata need to be updated. The app's process queues up its work on the GPU
     * and gets back an acquire fence. The app's process must update the buffer's
     * metadata before enqueuing the buffer to SurfaceFlinger. The app process CANNOT
     * update the buffer's metadata after enqueuing the buffer. When HardwareComposer
     * receives the buffer, it is immediately safe to read the buffer's metadata
     * and use it to program the display driver. To read the buffer's contents,
     * display driver must still wait on the acquire fence.
     * 
     * ------------ Buffer Metadata Token ----------------------
     * In order to allow arbitrary vendor defined metadata, we could not use a
     * HIDL enum as the buffer metadata token. Extending a HIDL enum requires a full
     * HIDL extension. We also could not use a simple non-HIDL enum because vendor
     * defined enums from different vendors could collide. Instead we have defined
     * a struct that has a string representing the enum type and an int that
     * represents the enum value. The string protects different enum values from
     * colliding.
     * 
     * The token struct (MetadataType) is defined as a HIDL struct since it
     * is passed into a HIDL function. The standard buffer metadata types are NOT
     * defined as a HIDL enum because it would have required a new IMapper version
     * just to add future standard buffer metadata types. By putting the enum in the
     * stable AIDL (hardware/interfaces/graphics/common/aidl/android/hardware/
     * graphics/common/StandardMetadataType.aidl), vendors will be able to optionally
     * choose to support future standard buffer metadata types without upgrading
     * HIDL versions. For more information see the description of "struct MetadataType".
     * 
     * ------------ Buffer Metadata Stream ---------------------
     * The buffer metadata is get and set as a byte stream (vec<uint8_t>). By getting
     * and setting buffer metadata as a byte stream, vendors can use the standard
     * getters and setter functions defined here. Vendors do NOT need to add their own
     * getters and setter functions for each new type of buffer metadata.
     * 
     * Converting buffer metadata into a byte stream can be non-trivial. For the standard
     * buffer metadata types defined in StandardMetadataType.aidl, there are also
     * support functions that will encode the buffer metadata into a byte stream
     * and decode the buffer metadata from a byte stream. We STRONGLY recommend using
     * these support functions. The framework will use them when getting and setting
     * metadata. The support functions are defined in
     * frameworks/native/libs/gralloc/types/include/gralloctypes/Gralloc4.h.
     * 
     * 
     * MetadataType represents the different types of buffer metadata that could be
     * associated with a buffer. It is used by IMapper to help get and set buffer metadata
     * on the buffer's native handle.
     * 
     * Standard buffer metadata will have the name field set to
     * "android.hardware.graphics.common.StandardMetadataType" and will contain values
     * from StandardMetadataType.aidl.
     * 
     * This struct should be "extended" by devices that use a proprietary or non-standard
     * buffer metadata. To extend the struct, first create a custom @VendorStability vendor
     * AIDL interface that defines the new type(s) you would like to support. Set the
     * struct's name field to the custom aidl interface's name
     * (eg. "vendor.mycompanyname.graphics.common.MetadataType"). Set the struct's value
     * field to the custom @VendorStabilty vendor AIDL interface.
     * 
     * Each company should create their own StandardMetadataType.aidl extension. The name
     * field prevents values from different companies from colliding.
     */
    struct MetadataType final {
        ::android::hardware::hidl_string name __attribute__ ((aligned(8)));
        int64_t value __attribute__ ((aligned(8)));
    };

    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType, name) == 0, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType, value) == 16, "wrong offset");
    static_assert(sizeof(::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType) == 24, "wrong size");
    static_assert(__alignof(::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType) == 8, "wrong alignment");

    struct MetadataTypeDescription final {
        ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType metadataType __attribute__ ((aligned(8)));
        ::android::hardware::hidl_string description __attribute__ ((aligned(8)));
        bool isGettable __attribute__ ((aligned(1)));
        bool isSettable __attribute__ ((aligned(1)));
    };

    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::MetadataTypeDescription, metadataType) == 0, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::MetadataTypeDescription, description) == 24, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::MetadataTypeDescription, isGettable) == 40, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::MetadataTypeDescription, isSettable) == 41, "wrong offset");
    static_assert(sizeof(::android::hardware::graphics::mapper::V4_0::IMapper::MetadataTypeDescription) == 48, "wrong size");
    static_assert(__alignof(::android::hardware::graphics::mapper::V4_0::IMapper::MetadataTypeDescription) == 8, "wrong alignment");

    struct MetadataDump final {
        ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType metadataType __attribute__ ((aligned(8)));
        ::android::hardware::hidl_vec<uint8_t> metadata __attribute__ ((aligned(8)));
    };

    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::MetadataDump, metadataType) == 0, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::MetadataDump, metadata) == 24, "wrong offset");
    static_assert(sizeof(::android::hardware::graphics::mapper::V4_0::IMapper::MetadataDump) == 40, "wrong size");
    static_assert(__alignof(::android::hardware::graphics::mapper::V4_0::IMapper::MetadataDump) == 8, "wrong alignment");

    struct BufferDump final {
        ::android::hardware::hidl_vec<::android::hardware::graphics::mapper::V4_0::IMapper::MetadataDump> metadataDump __attribute__ ((aligned(8)));
    };

    static_assert(offsetof(::android::hardware::graphics::mapper::V4_0::IMapper::BufferDump, metadataDump) == 0, "wrong offset");
    static_assert(sizeof(::android::hardware::graphics::mapper::V4_0::IMapper::BufferDump) == 16, "wrong size");
    static_assert(__alignof(::android::hardware::graphics::mapper::V4_0::IMapper::BufferDump) == 8, "wrong alignment");

    /**
     * Returns whether this object's implementation is outside of the current process.
     */
    virtual bool isRemote() const override { return false; }

    /**
     * Return callback for createDescriptor
     */
    using createDescriptor_cb = std::function<void(::android::hardware::graphics::mapper::V4_0::Error error, const ::android::hardware::hidl_vec<uint8_t>& descriptor)>;
    /**
     * Creates a buffer descriptor. The descriptor can be used with IAllocator
     * to allocate buffers.
     * 
     * Since the buffer descriptor fully describes a buffer, any device
     * dependent or device independent checks must be performed here whenever
     * possible. When layered buffers are not supported, this function must
     * return `UNSUPPORTED` if `description.layers` is great than 1. This
     * function may return `UNSUPPORTED` if `description.reservedSize` is
     * larger than a page.
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
    virtual ::android::hardware::Return<void> createDescriptor(const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& description, createDescriptor_cb _hidl_cb) = 0;

    /**
     * Return callback for importBuffer
     */
    using importBuffer_cb = std::function<void(::android::hardware::graphics::mapper::V4_0::Error error, void* buffer)>;
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
    virtual ::android::hardware::Return<::android::hardware::graphics::mapper::V4_0::Error> freeBuffer(void* buffer) = 0;

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
    virtual ::android::hardware::Return<::android::hardware::graphics::mapper::V4_0::Error> validateBufferSize(void* buffer, const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& description, uint32_t stride) = 0;

    /**
     * Return callback for getTransportSize
     */
    using getTransportSize_cb = std::function<void(::android::hardware::graphics::mapper::V4_0::Error error, uint32_t numFds, uint32_t numInts)>;
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
    using lock_cb = std::function<void(::android::hardware::graphics::mapper::V4_0::Error error, void* data)>;
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
     * An accessRegion of all-zeros means the entire buffer. That is, it is
     * equivalent to '(0,0)-(buffer width, buffer height)'.
     * 
     * This function can lock both single-planar and multi-planar formats. The caller
     * should use get() to get information about the buffer they are locking.
     * get() can be used to get information about the planes, offsets, stride,
     * etc.
     * 
     * This function must also work on buffers with
     * `AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_*` if supported by the device, as well
     * as with any other formats requested by multimedia codecs when they are
     * configured with a flexible-YUV-compatible color format.
     * 
     * On success, @p data must be filled with a pointer to the locked buffer
     * memory. This address will represent the top-left corner of the entire
     * buffer, even if @p accessRegion does not begin at the top-left corner.
     * 
     * The locked buffer must adhere to the format requested at allocation time
     * in the BufferDescriptorInfo.
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
     *       is incompatible with the buffer. Also if the @p accessRegion is
     *       outside the bounds of the buffer or the accessRegion is invalid.
     *     - `NO_RESOURCES` if the buffer cannot be locked at this time. Note
     *       that locking may succeed at a later time.
     * @return data CPU-accessible pointer to the buffer data.
     */
    virtual ::android::hardware::Return<void> lock(void* buffer, uint64_t cpuUsage, const ::android::hardware::graphics::mapper::V4_0::IMapper::Rect& accessRegion, const ::android::hardware::hidl_handle& acquireFence, lock_cb _hidl_cb) = 0;

    /**
     * Return callback for unlock
     */
    using unlock_cb = std::function<void(::android::hardware::graphics::mapper::V4_0::Error error, const ::android::hardware::hidl_handle& releaseFence)>;
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
     * Return callback for flushLockedBuffer
     */
    using flushLockedBuffer_cb = std::function<void(::android::hardware::graphics::mapper::V4_0::Error error, const ::android::hardware::hidl_handle& releaseFence)>;
    /**
     * Flushes the contents of a locked buffer.
     * 
     * This function flushes the CPUs caches for the range of all the buffer's
     * planes and metadata. This should behave similarly to unlock() except the
     * buffer should remain mapped to the CPU.
     * 
     * The client is still responsible for calling unlock() when it is done
     * with all CPU accesses to the buffer.
     * 
     * If non-CPU blocks are simultaneously writing the buffer, the locked
     * copy should still be flushed but what happens is undefined except that
     * it should not cause any crashes.
     * 
     * @param buffer Buffer to flush.
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the buffer is invalid or not locked.
     * @return releaseFence Handle containing a file descriptor referring to a
     *     sync fence object. The sync fence object will be signaled when the
     *     mapper has completed any pending work. @p releaseFence may be an
     *     empty fence.
     */
    virtual ::android::hardware::Return<void> flushLockedBuffer(void* buffer, flushLockedBuffer_cb _hidl_cb) = 0;

    /**
     * Rereads the contents of a locked buffer.
     * 
     * This should fetch the most recent copy of the locked buffer.
     * 
     * It may reread locked copies of the buffer in other processes.
     * 
     * The client is still responsible for calling unlock() when it is done
     * with all CPU accesses to the buffer.
     * 
     * @param buffer Buffer to reread.
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the buffer is invalid or not locked.
     *     - `NO_RESOURCES` if the buffer cannot be reread at this time. Note
     *       that rereading may succeed at a later time.
     */
    virtual ::android::hardware::Return<::android::hardware::graphics::mapper::V4_0::Error> rereadLockedBuffer(void* buffer) = 0;

    /**
     * Return callback for isSupported
     */
    using isSupported_cb = std::function<void(::android::hardware::graphics::mapper::V4_0::Error error, bool supported)>;
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
    virtual ::android::hardware::Return<void> isSupported(const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& description, isSupported_cb _hidl_cb) = 0;

    /**
     * Return callback for get
     */
    using get_cb = std::function<void(::android::hardware::graphics::mapper::V4_0::Error error, const ::android::hardware::hidl_vec<uint8_t>& metadata)>;
    /**
     * Gets the buffer metadata for a given MetadataType.
     * 
     * Buffer metadata can be changed after allocation so clients should avoid "caching"
     * the buffer metadata. For example, if the video resolution changes and the buffers
     * are not reallocated, several buffer metadata values may change without warning.
     * Clients should not expect the values to be constant. They should requery them every
     * frame. The only exception is buffer metadata that is determined at allocation
     * time. For StandardMetadataType values, only BUFFER_ID, NAME, WIDTH,
     * HEIGHT, LAYER_COUNT, PIXEL_FORMAT_REQUESTED and USAGE are safe to cache because
     * they are determined at allocation time.
     * 
     * @param buffer Buffer containing desired metadata
     * @param metadataType MetadataType for the metadata value being queried
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the raw handle is invalid.
     *     - `NO_RESOURCES` if the get cannot be fullfilled due to unavailability of
     *        resources.
     *     - `UNSUPPORTED` when metadataType is unknown/unsupported.
     *        IMapper must support getting all StandardMetadataType.aidl values defined
     *        at the time the device first launches.
     * @return metadata Vector of bytes representing the buffer metadata associated with
     *  the MetadataType.
     */
    virtual ::android::hardware::Return<void> get(void* buffer, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& metadataType, get_cb _hidl_cb) = 0;

    /**
     * Sets the global value for a given MetadataType.
     * 
     * Metadata fields are not required to be settable. This function can
     * return Error::UNSUPPORTED whenever it doesn't support setting a
     * particular Metadata field.
     * 
     * The framework may attempt to set the following StandardMetadataType
     * values: DATASPACE, SMPTE2086, CTA861_3, SMPTE2094_40 and BLEND_MODE.
     * We strongly encourage everyone to support setting as many of those fields as
     * possible. If a device's Composer implementation supports a field, it should be
     * supported here. Over time these metadata fields will be moved out of
     * Composer/BufferQueue/etc. and into the buffer's Metadata fields.
     * If a device's IMapper doesn't support setting those Metadata fields,
     * eventually the device may not longer be able to support these fields.
     * 
     * @param buffer Buffer receiving desired metadata
     * @param metadataType MetadataType for the metadata value being set
     * @param metadata Vector of bytes representing the value associated with
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the raw handle is invalid.
     *     - `BAD_VALUE` when the field is constant and can never be set (such as
     *       BUFFER_ID, NAME, WIDTH, HEIGHT, LAYER_COUNT, PIXEL_FORMAT_REQUESTED and
     *       USAGE)
     *     - `NO_RESOURCES` if the set cannot be fullfilled due to unavailability of
     *        resources.
     *     - `UNSUPPORTED` when metadataType is unknown/unsupported or setting
     *       it is unsupported. Unsupported should also be returned if the metadata
     *       is malformed.
     */
    virtual ::android::hardware::Return<::android::hardware::graphics::mapper::V4_0::Error> set(void* buffer, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& metadataType, const ::android::hardware::hidl_vec<uint8_t>& metadata) = 0;

    /**
     * Return callback for getFromBufferDescriptorInfo
     */
    using getFromBufferDescriptorInfo_cb = std::function<void(::android::hardware::graphics::mapper::V4_0::Error error, const ::android::hardware::hidl_vec<uint8_t>& metadata)>;
    /**
     * Given a BufferDescriptorInfo, gets the starting value of a given
     * MetadataType. This can be used to query basic information about a buffer
     * before the buffer is allocated.
     * 
     * @param description Attributes of the descriptor.
     * @param metadataType MetadataType for the metadata value being queried
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_VALUE` if any of the specified BufferDescriptorInfo attributes
     *       are invalid.
     *     - `NO_RESOURCES` if the get cannot be fullfilled due to unavailability of
     *       resources.
     *     - `UNSUPPORTED` when any of the description attributes are unsupported or
     *       if the metadataType is unknown/unsupported. This should also be
     *       returned if the requested metadata is not defined until a buffer has been
     *       allocated.
     * @return metadata Vector of bytes representing the value associated with
     *  the MetadataType value.
     */
    virtual ::android::hardware::Return<void> getFromBufferDescriptorInfo(const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& description, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& metadataType, getFromBufferDescriptorInfo_cb _hidl_cb) = 0;

    /**
     * Return callback for listSupportedMetadataTypes
     */
    using listSupportedMetadataTypes_cb = std::function<void(::android::hardware::graphics::mapper::V4_0::Error error, const ::android::hardware::hidl_vec<::android::hardware::graphics::mapper::V4_0::IMapper::MetadataTypeDescription>& descriptions)>;
    /**
     * Lists all the MetadataTypes supported by IMapper as well as a description
     * of each supported MetadataType. For StandardMetadataTypes, the description
     * string can be left empty.
     * 
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `NO_RESOURCES` if the get cannot be fullfilled due to unavailability of
     *       resources.
     * @return descriptions Vector of MetadataTypeDescriptions that represent the
     *  MetadataTypes supported by the device.
     */
    virtual ::android::hardware::Return<void> listSupportedMetadataTypes(listSupportedMetadataTypes_cb _hidl_cb) = 0;

    /**
     * Return callback for dumpBuffer
     */
    using dumpBuffer_cb = std::function<void(::android::hardware::graphics::mapper::V4_0::Error error, const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDump& bufferDump)>;
    /**
     * Dumps a buffer's metadata.
     * 
     * @param buffer Buffer that is being dumped
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the raw handle is invalid.
     *     - `NO_RESOURCES` if the get cannot be fullfilled due to unavailability of
     *       resources.
     * @return bufferDump Struct representing the metadata being dumped
     */
    virtual ::android::hardware::Return<void> dumpBuffer(void* buffer, dumpBuffer_cb _hidl_cb) = 0;

    /**
     * Return callback for dumpBuffers
     */
    using dumpBuffers_cb = std::function<void(::android::hardware::graphics::mapper::V4_0::Error error, const ::android::hardware::hidl_vec<::android::hardware::graphics::mapper::V4_0::IMapper::BufferDump>& bufferDumps)>;
    /**
     * Dumps the metadata for all the buffers in the current process.
     * 
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `NO_RESOURCES` if the get cannot be fullfilled due to unavailability of
     *       resources.
     * @return bufferDumps Vector of structs representing the buffers being dumped
     */
    virtual ::android::hardware::Return<void> dumpBuffers(dumpBuffers_cb _hidl_cb) = 0;

    /**
     * Return callback for getReservedRegion
     */
    using getReservedRegion_cb = std::function<void(::android::hardware::graphics::mapper::V4_0::Error error, void* reservedRegion, uint64_t reservedSize)>;
    /**
     * Returns the region of shared memory associated with the buffer that is
     * reserved for client use.
     * 
     * The shared memory may be allocated from any shared memory allocator.
     * The shared memory must be CPU-accessible and virtually contiguous. The
     * starting address must be word-aligned.
     * 
     * This function may only be called after importBuffer() has been called by the
     * client. The reserved region must remain accessible until freeBuffer() has
     * been called. After freeBuffer() has been called, the client must not access
     * the reserved region.
     * 
     * This reserved memory may be used in future versions of Android to
     * help clients implement backwards compatible features without requiring
     * IAllocator/IMapper updates.
     * 
     * @param buffer Imported buffer handle.
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the buffer is invalid.
     * @return reservedRegion CPU-accessible pointer to the reserved region
     * @return reservedSize the size of the reservedRegion that was requested
     *    in the BufferDescriptorInfo.
     */
    virtual ::android::hardware::Return<void> getReservedRegion(void* buffer, getReservedRegion_cb _hidl_cb) = 0;

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
    static ::android::hardware::Return<::android::sp<::android::hardware::graphics::mapper::V4_0::IMapper>> castFrom(const ::android::sp<::android::hardware::graphics::mapper::V4_0::IMapper>& parent, bool emitError = false);
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::android::hardware::graphics::mapper::V4_0::IMapper>> castFrom(const ::android::sp<::android::hidl::base::V1_0::IBase>& parent, bool emitError = false);

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

static inline std::string toString(const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& o);
static inline bool operator==(const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& rhs);
static inline bool operator!=(const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& rhs);

static inline std::string toString(const ::android::hardware::graphics::mapper::V4_0::IMapper::Rect& o);
static inline bool operator==(const ::android::hardware::graphics::mapper::V4_0::IMapper::Rect& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::Rect& rhs);
static inline bool operator!=(const ::android::hardware::graphics::mapper::V4_0::IMapper::Rect& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::Rect& rhs);

static inline std::string toString(const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& o);
static inline bool operator==(const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& rhs);
static inline bool operator!=(const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& rhs);

static inline std::string toString(const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataTypeDescription& o);
static inline bool operator==(const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataTypeDescription& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataTypeDescription& rhs);
static inline bool operator!=(const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataTypeDescription& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataTypeDescription& rhs);

static inline std::string toString(const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataDump& o);
static inline bool operator==(const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataDump& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataDump& rhs);
static inline bool operator!=(const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataDump& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataDump& rhs);

static inline std::string toString(const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDump& o);
static inline bool operator==(const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDump& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDump& rhs);
static inline bool operator!=(const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDump& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDump& rhs);

static inline std::string toString(const ::android::sp<::android::hardware::graphics::mapper::V4_0::IMapper>& o);

//
// type header definitions for package
//

static inline std::string toString(const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& o) {
    using ::android::hardware::toString;
    std::string os;
    os += "{";
    os += ".name = ";
    os += ::android::hardware::toString(o.name);
    os += ", .width = ";
    os += ::android::hardware::toString(o.width);
    os += ", .height = ";
    os += ::android::hardware::toString(o.height);
    os += ", .layerCount = ";
    os += ::android::hardware::toString(o.layerCount);
    os += ", .format = ";
    os += ::android::hardware::graphics::common::V1_2::toString(o.format);
    os += ", .usage = ";
    os += ::android::hardware::graphics::common::V1_2::toString<::android::hardware::graphics::common::V1_2::BufferUsage>(o.usage);
    os += ", .reservedSize = ";
    os += ::android::hardware::toString(o.reservedSize);
    os += "}"; return os;
}

static inline bool operator==(const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& rhs) {
    if (lhs.name != rhs.name) {
        return false;
    }
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
    if (lhs.reservedSize != rhs.reservedSize) {
        return false;
    }
    return true;
}

static inline bool operator!=(const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& rhs){
    return !(lhs == rhs);
}

static inline std::string toString(const ::android::hardware::graphics::mapper::V4_0::IMapper::Rect& o) {
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

static inline bool operator==(const ::android::hardware::graphics::mapper::V4_0::IMapper::Rect& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::Rect& rhs) {
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

static inline bool operator!=(const ::android::hardware::graphics::mapper::V4_0::IMapper::Rect& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::Rect& rhs){
    return !(lhs == rhs);
}

static inline std::string toString(const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& o) {
    using ::android::hardware::toString;
    std::string os;
    os += "{";
    os += ".name = ";
    os += ::android::hardware::toString(o.name);
    os += ", .value = ";
    os += ::android::hardware::toString(o.value);
    os += "}"; return os;
}

static inline bool operator==(const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& rhs) {
    if (lhs.name != rhs.name) {
        return false;
    }
    if (lhs.value != rhs.value) {
        return false;
    }
    return true;
}

static inline bool operator!=(const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& rhs){
    return !(lhs == rhs);
}

static inline std::string toString(const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataTypeDescription& o) {
    using ::android::hardware::toString;
    std::string os;
    os += "{";
    os += ".metadataType = ";
    os += ::android::hardware::graphics::mapper::V4_0::toString(o.metadataType);
    os += ", .description = ";
    os += ::android::hardware::toString(o.description);
    os += ", .isGettable = ";
    os += ::android::hardware::toString(o.isGettable);
    os += ", .isSettable = ";
    os += ::android::hardware::toString(o.isSettable);
    os += "}"; return os;
}

static inline bool operator==(const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataTypeDescription& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataTypeDescription& rhs) {
    if (lhs.metadataType != rhs.metadataType) {
        return false;
    }
    if (lhs.description != rhs.description) {
        return false;
    }
    if (lhs.isGettable != rhs.isGettable) {
        return false;
    }
    if (lhs.isSettable != rhs.isSettable) {
        return false;
    }
    return true;
}

static inline bool operator!=(const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataTypeDescription& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataTypeDescription& rhs){
    return !(lhs == rhs);
}

static inline std::string toString(const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataDump& o) {
    using ::android::hardware::toString;
    std::string os;
    os += "{";
    os += ".metadataType = ";
    os += ::android::hardware::graphics::mapper::V4_0::toString(o.metadataType);
    os += ", .metadata = ";
    os += ::android::hardware::toString(o.metadata);
    os += "}"; return os;
}

static inline bool operator==(const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataDump& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataDump& rhs) {
    if (lhs.metadataType != rhs.metadataType) {
        return false;
    }
    if (lhs.metadata != rhs.metadata) {
        return false;
    }
    return true;
}

static inline bool operator!=(const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataDump& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataDump& rhs){
    return !(lhs == rhs);
}

static inline std::string toString(const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDump& o) {
    using ::android::hardware::toString;
    std::string os;
    os += "{";
    os += ".metadataDump = ";
    os += ::android::hardware::toString(o.metadataDump);
    os += "}"; return os;
}

static inline bool operator==(const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDump& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDump& rhs) {
    if (lhs.metadataDump != rhs.metadataDump) {
        return false;
    }
    return true;
}

static inline bool operator!=(const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDump& lhs, const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDump& rhs){
    return !(lhs == rhs);
}

static inline std::string toString(const ::android::sp<::android::hardware::graphics::mapper::V4_0::IMapper>& o) {
    std::string os = "[class or subclass of ";
    os += ::android::hardware::graphics::mapper::V4_0::IMapper::descriptor;
    os += "]";
    os += o->isRemote() ? "@remote" : "@local";
    return os;
}


}  // namespace V4_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android

//
// global type declarations for package
//


#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_MAPPER_V4_0_IMAPPER_H
