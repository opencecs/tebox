#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_MAPPER_V4_0_BPHWMAPPER_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_MAPPER_V4_0_BPHWMAPPER_H

#include <hidl/HidlTransportSupport.h>

#include <android/hardware/graphics/mapper/4.0/IHwMapper.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V4_0 {

struct BpHwMapper : public ::android::hardware::BpInterface<IMapper>, public ::android::hardware::details::HidlInstrumentor {
    explicit BpHwMapper(const ::android::sp<::android::hardware::IBinder> &_hidl_impl);

    /**
     * The pure class is what this class wraps.
     */
    typedef IMapper Pure;

    /**
     * Type tag for use in template logic that indicates this is a 'proxy' class.
     */
    typedef android::hardware::details::bphw_tag _hidl_tag;

    virtual bool isRemote() const override { return true; }

    // Methods from ::android::hardware::graphics::mapper::V4_0::IMapper follow.
    static ::android::hardware::Return<void>  _hidl_createDescriptor(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& description, createDescriptor_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_importBuffer(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, const ::android::hardware::hidl_handle& rawHandle, importBuffer_cb _hidl_cb);
    static ::android::hardware::Return<::android::hardware::graphics::mapper::V4_0::Error>  _hidl_freeBuffer(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, void* buffer);
    static ::android::hardware::Return<::android::hardware::graphics::mapper::V4_0::Error>  _hidl_validateBufferSize(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, void* buffer, const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& description, uint32_t stride);
    static ::android::hardware::Return<void>  _hidl_getTransportSize(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, void* buffer, getTransportSize_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_lock(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, void* buffer, uint64_t cpuUsage, const ::android::hardware::graphics::mapper::V4_0::IMapper::Rect& accessRegion, const ::android::hardware::hidl_handle& acquireFence, lock_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_unlock(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, void* buffer, unlock_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_flushLockedBuffer(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, void* buffer, flushLockedBuffer_cb _hidl_cb);
    static ::android::hardware::Return<::android::hardware::graphics::mapper::V4_0::Error>  _hidl_rereadLockedBuffer(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, void* buffer);
    static ::android::hardware::Return<void>  _hidl_isSupported(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& description, isSupported_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_get(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, void* buffer, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& metadataType, get_cb _hidl_cb);
    static ::android::hardware::Return<::android::hardware::graphics::mapper::V4_0::Error>  _hidl_set(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, void* buffer, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& metadataType, const ::android::hardware::hidl_vec<uint8_t>& metadata);
    static ::android::hardware::Return<void>  _hidl_getFromBufferDescriptorInfo(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& description, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& metadataType, getFromBufferDescriptorInfo_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_listSupportedMetadataTypes(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, listSupportedMetadataTypes_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_dumpBuffer(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, void* buffer, dumpBuffer_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_dumpBuffers(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, dumpBuffers_cb _hidl_cb);
    static ::android::hardware::Return<void>  _hidl_getReservedRegion(::android::hardware::IInterface* _hidl_this, ::android::hardware::details::HidlInstrumentor *_hidl_this_instrumentor, void* buffer, getReservedRegion_cb _hidl_cb);

    // Methods from ::android::hardware::graphics::mapper::V4_0::IMapper follow.
    ::android::hardware::Return<void> createDescriptor(const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& description, createDescriptor_cb _hidl_cb) override;
    ::android::hardware::Return<void> importBuffer(const ::android::hardware::hidl_handle& rawHandle, importBuffer_cb _hidl_cb) override;
    ::android::hardware::Return<::android::hardware::graphics::mapper::V4_0::Error> freeBuffer(void* buffer) override;
    ::android::hardware::Return<::android::hardware::graphics::mapper::V4_0::Error> validateBufferSize(void* buffer, const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& description, uint32_t stride) override;
    ::android::hardware::Return<void> getTransportSize(void* buffer, getTransportSize_cb _hidl_cb) override;
    ::android::hardware::Return<void> lock(void* buffer, uint64_t cpuUsage, const ::android::hardware::graphics::mapper::V4_0::IMapper::Rect& accessRegion, const ::android::hardware::hidl_handle& acquireFence, lock_cb _hidl_cb) override;
    ::android::hardware::Return<void> unlock(void* buffer, unlock_cb _hidl_cb) override;
    ::android::hardware::Return<void> flushLockedBuffer(void* buffer, flushLockedBuffer_cb _hidl_cb) override;
    ::android::hardware::Return<::android::hardware::graphics::mapper::V4_0::Error> rereadLockedBuffer(void* buffer) override;
    ::android::hardware::Return<void> isSupported(const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& description, isSupported_cb _hidl_cb) override;
    ::android::hardware::Return<void> get(void* buffer, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& metadataType, get_cb _hidl_cb) override;
    ::android::hardware::Return<::android::hardware::graphics::mapper::V4_0::Error> set(void* buffer, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& metadataType, const ::android::hardware::hidl_vec<uint8_t>& metadata) override;
    ::android::hardware::Return<void> getFromBufferDescriptorInfo(const ::android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& description, const ::android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& metadataType, getFromBufferDescriptorInfo_cb _hidl_cb) override;
    ::android::hardware::Return<void> listSupportedMetadataTypes(listSupportedMetadataTypes_cb _hidl_cb) override;
    ::android::hardware::Return<void> dumpBuffer(void* buffer, dumpBuffer_cb _hidl_cb) override;
    ::android::hardware::Return<void> dumpBuffers(dumpBuffers_cb _hidl_cb) override;
    ::android::hardware::Return<void> getReservedRegion(void* buffer, getReservedRegion_cb _hidl_cb) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.
    ::android::hardware::Return<void> interfaceChain(interfaceChain_cb _hidl_cb) override;
    ::android::hardware::Return<void> debug(const ::android::hardware::hidl_handle& fd, const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& options) override;
    ::android::hardware::Return<void> interfaceDescriptor(interfaceDescriptor_cb _hidl_cb) override;
    ::android::hardware::Return<void> getHashChain(getHashChain_cb _hidl_cb) override;
    ::android::hardware::Return<void> setHALInstrumentation() override;
    ::android::hardware::Return<bool> linkToDeath(const ::android::sp<::android::hardware::hidl_death_recipient>& recipient, uint64_t cookie) override;
    ::android::hardware::Return<void> ping() override;
    ::android::hardware::Return<void> getDebugInfo(getDebugInfo_cb _hidl_cb) override;
    ::android::hardware::Return<void> notifySyspropsChanged() override;
    ::android::hardware::Return<bool> unlinkToDeath(const ::android::sp<::android::hardware::hidl_death_recipient>& recipient) override;

private:
    std::mutex _hidl_mMutex;
    std::vector<::android::sp<::android::hardware::hidl_binder_death_recipient>> _hidl_mDeathRecipients;
};

}  // namespace V4_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_MAPPER_V4_0_BPHWMAPPER_H
