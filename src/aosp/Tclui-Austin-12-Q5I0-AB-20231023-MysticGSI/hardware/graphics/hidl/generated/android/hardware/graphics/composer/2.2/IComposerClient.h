#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_2_ICOMPOSERCLIENT_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_2_ICOMPOSERCLIENT_H

#include <android/hardware/graphics/common/1.1/types.h>
#include <android/hardware/graphics/composer/2.1/IComposerClient.h>
#include <android/hardware/graphics/composer/2.1/types.h>

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
namespace V2_2 {

struct IComposerClient : public ::android::hardware::graphics::composer::V2_1::IComposerClient {
    /**
     * Type tag for use in template logic that indicates this is a 'pure' class.
     */
    typedef android::hardware::details::i_tag _hidl_tag;

    /**
     * Fully qualified interface name: "android.hardware.graphics.composer@2.2::IComposerClient"
     */
    static const char* descriptor;

    // Forward declaration for forward reference support:
    enum class PowerMode : int32_t;
    enum class PerFrameMetadataKey : int32_t;
    struct PerFrameMetadata;
    struct FloatColor;
    enum class Command : int32_t;

    enum class PowerMode : int32_t {
        /**
         * The display is fully off (blanked).
         */
        OFF = 0,
        /**
         * These are optional low power modes. getDozeSupport may be called to
         * determine whether a given display supports these modes.
         * 
         * 
         * The display is turned on and configured in a low power state that
         * is suitable for presenting ambient information to the user,
         * possibly with lower fidelity than ON, but with greater efficiency.
         */
        DOZE = 1,
        /**
         * The display is configured as in DOZE but may stop applying display
         * updates from the client. This is effectively a hint to the device
         * that drawing to the display has been suspended and that the the
         * device must remain on in a low power state and continue
         * displaying its current contents indefinitely until the power mode
         * changes.
         * 
         * This mode may also be used as a signal to enable hardware-based
         * doze functionality. In this case, the device is free to take over
         * the display and manage it autonomously to implement a low power
         * always-on display.
         */
        DOZE_SUSPEND = 3,
        /**
         * The display is fully on.
         */
        ON = 2,
        /**
         * The display is configured as in ON but may stop applying display
         * updates from the client. This is effectively a hint to the device
         * that drawing to the display has been suspended and that the the
         * device must remain on and continue displaying its current contents
         * indefinitely until the power mode changes.
         * 
         * This mode may also be used as a signal to enable hardware-based
         * functionality to take over the display and manage it autonomously
         * to implement a low power always-on display.
         */
        ON_SUSPEND = 4,
    };

    /**
     * Following enums define keys for metadata defined by SMPTE ST 2086:2014
     * and CTA 861.3.
     */
    enum class PerFrameMetadataKey : int32_t {
        /**
         * SMPTE ST 2084:2014.
         * Coordinates defined in CIE 1931 xy chromaticity space
         * 
         * 
         * SMPTE ST 2084:2014
         */
        DISPLAY_RED_PRIMARY_X = 0,
        /**
         * SMPTE ST 2084:2014
         */
        DISPLAY_RED_PRIMARY_Y = 1 /* (::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey.DISPLAY_RED_PRIMARY_X implicitly + 1) */,
        /**
         * SMPTE ST 2084:2014
         */
        DISPLAY_GREEN_PRIMARY_X = 2 /* (::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey.DISPLAY_RED_PRIMARY_Y implicitly + 1) */,
        /**
         * SMPTE ST 2084:2014
         */
        DISPLAY_GREEN_PRIMARY_Y = 3 /* (::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey.DISPLAY_GREEN_PRIMARY_X implicitly + 1) */,
        /**
         * SMPTE ST 2084:2014
         */
        DISPLAY_BLUE_PRIMARY_X = 4 /* (::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey.DISPLAY_GREEN_PRIMARY_Y implicitly + 1) */,
        /**
         * SMPTE ST 2084:2014
         */
        DISPLAY_BLUE_PRIMARY_Y = 5 /* (::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey.DISPLAY_BLUE_PRIMARY_X implicitly + 1) */,
        /**
         * SMPTE ST 2084:2014
         */
        WHITE_POINT_X = 6 /* (::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey.DISPLAY_BLUE_PRIMARY_Y implicitly + 1) */,
        /**
         * SMPTE ST 2084:2014
         */
        WHITE_POINT_Y = 7 /* (::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey.WHITE_POINT_X implicitly + 1) */,
        /**
         * SMPTE ST 2084:2014.
         * Units: nits
         * max as defined by ST 2048: 10,000 nits
         */
        MAX_LUMINANCE = 8 /* (::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey.WHITE_POINT_Y implicitly + 1) */,
        /**
         * SMPTE ST 2084:2014
         */
        MIN_LUMINANCE = 9 /* (::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey.MAX_LUMINANCE implicitly + 1) */,
        /**
         * CTA 861.3
         */
        MAX_CONTENT_LIGHT_LEVEL = 10 /* (::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey.MIN_LUMINANCE implicitly + 1) */,
        /**
         * CTA 861.3
         */
        MAX_FRAME_AVERAGE_LIGHT_LEVEL = 11 /* (::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey.MAX_CONTENT_LIGHT_LEVEL implicitly + 1) */,
    };

    struct PerFrameMetadata final {
        ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey key __attribute__ ((aligned(4)));
        float value __attribute__ ((aligned(4)));
    };

    static_assert(offsetof(::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadata, key) == 0, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadata, value) == 4, "wrong offset");
    static_assert(sizeof(::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadata) == 8, "wrong size");
    static_assert(__alignof(::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadata) == 4, "wrong alignment");

    struct FloatColor final {
        float r __attribute__ ((aligned(4)));
        float g __attribute__ ((aligned(4)));
        float b __attribute__ ((aligned(4)));
        float a __attribute__ ((aligned(4)));
    };

    static_assert(offsetof(::android::hardware::graphics::composer::V2_2::IComposerClient::FloatColor, r) == 0, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::composer::V2_2::IComposerClient::FloatColor, g) == 4, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::composer::V2_2::IComposerClient::FloatColor, b) == 8, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::composer::V2_2::IComposerClient::FloatColor, a) == 12, "wrong offset");
    static_assert(sizeof(::android::hardware::graphics::composer::V2_2::IComposerClient::FloatColor) == 16, "wrong size");
    static_assert(__alignof(::android::hardware::graphics::composer::V2_2::IComposerClient::FloatColor) == 4, "wrong alignment");

    enum class Command : int32_t {
        LENGTH_MASK = 65535 /* 0xffff */,
        OPCODE_SHIFT = 16,
        OPCODE_MASK = -65536 /* (0xffff << OPCODE_SHIFT) */,
        /**
         * special commands
         */
        SELECT_DISPLAY = 0 /* (0x000 << OPCODE_SHIFT) */,
        SELECT_LAYER = 65536 /* (0x001 << OPCODE_SHIFT) */,
        /**
         * value commands (for return values)
         */
        SET_ERROR = 16777216 /* (0x100 << OPCODE_SHIFT) */,
        SET_CHANGED_COMPOSITION_TYPES = 16842752 /* (0x101 << OPCODE_SHIFT) */,
        SET_DISPLAY_REQUESTS = 16908288 /* (0x102 << OPCODE_SHIFT) */,
        SET_PRESENT_FENCE = 16973824 /* (0x103 << OPCODE_SHIFT) */,
        SET_RELEASE_FENCES = 17039360 /* (0x104 << OPCODE_SHIFT) */,
        /**
         * display commands
         */
        SET_COLOR_TRANSFORM = 33554432 /* (0x200 << OPCODE_SHIFT) */,
        SET_CLIENT_TARGET = 33619968 /* (0x201 << OPCODE_SHIFT) */,
        SET_OUTPUT_BUFFER = 33685504 /* (0x202 << OPCODE_SHIFT) */,
        VALIDATE_DISPLAY = 33751040 /* (0x203 << OPCODE_SHIFT) */,
        ACCEPT_DISPLAY_CHANGES = 33816576 /* (0x204 << OPCODE_SHIFT) */,
        PRESENT_DISPLAY = 33882112 /* (0x205 << OPCODE_SHIFT) */,
        PRESENT_OR_VALIDATE_DISPLAY = 33947648 /* (0x206 << OPCODE_SHIFT) */,
        /**
         * layer commands (VALIDATE_DISPLAY not required)
         */
        SET_LAYER_CURSOR_POSITION = 50331648 /* (0x300 << OPCODE_SHIFT) */,
        SET_LAYER_BUFFER = 50397184 /* (0x301 << OPCODE_SHIFT) */,
        SET_LAYER_SURFACE_DAMAGE = 50462720 /* (0x302 << OPCODE_SHIFT) */,
        /**
         * layer state commands (VALIDATE_DISPLAY required)
         */
        SET_LAYER_BLEND_MODE = 67108864 /* (0x400 << OPCODE_SHIFT) */,
        SET_LAYER_COLOR = 67174400 /* (0x401 << OPCODE_SHIFT) */,
        SET_LAYER_COMPOSITION_TYPE = 67239936 /* (0x402 << OPCODE_SHIFT) */,
        SET_LAYER_DATASPACE = 67305472 /* (0x403 << OPCODE_SHIFT) */,
        SET_LAYER_DISPLAY_FRAME = 67371008 /* (0x404 << OPCODE_SHIFT) */,
        SET_LAYER_PLANE_ALPHA = 67436544 /* (0x405 << OPCODE_SHIFT) */,
        SET_LAYER_SIDEBAND_STREAM = 67502080 /* (0x406 << OPCODE_SHIFT) */,
        SET_LAYER_SOURCE_CROP = 67567616 /* (0x407 << OPCODE_SHIFT) */,
        SET_LAYER_TRANSFORM = 67633152 /* (0x408 << OPCODE_SHIFT) */,
        SET_LAYER_VISIBLE_REGION = 67698688 /* (0x409 << OPCODE_SHIFT) */,
        SET_LAYER_Z_ORDER = 67764224 /* (0x40a << OPCODE_SHIFT) */,
        SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT = 67829760 /* (0x40b << OPCODE_SHIFT) */,
        /**
         * SET_LAYER_PER_FRAME_METADATA has this pseudo prototype
         * 
         *   setLayerPerFrameMetadata(Display display, Layer layer,
         *                            vec<PerFrameMetadata> data);
         * 
         * Sets the PerFrameMetadata for the display. This metadata must be used
         * by the implementation to better tone map content to that display.
         * 
         * This is a method that may be called every frame. Thus it's
         * implemented using buffered transport.
         * SET_LAYER_PER_FRAME_METADATA is the command used by the buffered transport
         * mechanism.
         */
        SET_LAYER_PER_FRAME_METADATA = 50528256 /* (0x303 << @2.1::IComposerClient.Command:OPCODE_SHIFT) */,
        /**
         * SET_LAYER_COLOR has this pseudo prototype
         * 
         *   setLayerColor(FloatColor color);
         * 
         * Sets the color of the given layer. If the composition type of the layer
         * is not Composition::SOLID_COLOR, this call must succeed and have no
         * other effect.
         * 
         * @param color is the new color using float type.
         */
        SET_LAYER_FLOAT_COLOR = 67895296 /* (0x40c << @2.1::IComposerClient.Command:OPCODE_SHIFT) */,
    };

    /**
     * Returns whether this object's implementation is outside of the current process.
     */
    virtual bool isRemote() const override { return false; }

    // @entry @callflow(next="*")
    /**
     * Provides a IComposerCallback object for the device to call.
     * 
     * This function must be called only once.
     * 
     * @param callback is the IComposerCallback object.
     */
    virtual ::android::hardware::Return<void> registerCallback(const ::android::sp<::android::hardware::graphics::composer::V2_1::IComposerCallback>& callback) = 0;

    // @callflow(next="*")
    /**
     * Returns the maximum number of virtual displays supported by this device
     * (which may be 0). The client must not attempt to create more than this
     * many virtual displays on this device. This number must not change for
     * the lifetime of the device.
     * 
     * @return count is the maximum number of virtual displays supported.
     */
    virtual ::android::hardware::Return<uint32_t> getMaxVirtualDisplayCount() = 0;

    /**
     * Return callback for createVirtualDisplay
     */
    using createVirtualDisplay_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, uint64_t display, ::android::hardware::graphics::common::V1_0::PixelFormat format)>;
    // @callflow(next="*")
    /**
     * Creates a new virtual display with the given width and height. The
     * format passed into this function is the default format requested by the
     * consumer of the virtual display output buffers.
     * 
     * The display must be assumed to be on from the time the first frame is
     * presented until the display is destroyed.
     * 
     * @param width is the width in pixels.
     * @param height is the height in pixels.
     * @param formatHint is the default output buffer format selected by
     *        the consumer.
     * @param outputBufferSlotCount is the number of output buffer slots to be
     *        reserved.
     * @return error is NONE upon success. Otherwise,
     *         UNSUPPORTED when the width or height is too large for the
     *                     device to be able to create a virtual display.
     *         NO_RESOURCES when the device is unable to create a new virtual
     *                      display at this time.
     * @return display is the newly-created virtual display.
     * @return format is the format of the buffer the device will produce.
     */
    virtual ::android::hardware::Return<void> createVirtualDisplay(uint32_t width, uint32_t height, ::android::hardware::graphics::common::V1_0::PixelFormat formatHint, uint32_t outputBufferSlotCount, createVirtualDisplay_cb _hidl_cb) = 0;

    // @callflow(next="*")
    /**
     * Destroys a virtual display. After this call all resources consumed by
     * this display may be freed by the device and any operations performed on
     * this display must fail.
     * 
     * @param display is the virtual display to destroy.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     *         BAD_PARAMETER when the display handle which was passed in does
     *                       not refer to a virtual display.
     */
    virtual ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> destroyVirtualDisplay(uint64_t display) = 0;

    /**
     * Return callback for createLayer
     */
    using createLayer_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, uint64_t layer)>;
    // @callflow(next="*")
    /**
     * Creates a new layer on the given display.
     * 
     * @param display is the display on which to create the layer.
     * @param bufferSlotCount is the number of buffer slot to be reserved.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     *         NO_RESOURCES when the device was unable to create a layer this
     *                      time.
     * @return layer is the handle of the new layer.
     */
    virtual ::android::hardware::Return<void> createLayer(uint64_t display, uint32_t bufferSlotCount, createLayer_cb _hidl_cb) = 0;

    // @callflow(next="*")
    /**
     * Destroys the given layer.
     * 
     * @param display is the display on which the layer was created.
     * @param layer is the layer to destroy.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     *         BAD_LAYER when an invalid layer handle was passed in.
     */
    virtual ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> destroyLayer(uint64_t display, uint64_t layer) = 0;

    /**
     * Return callback for getActiveConfig
     */
    using getActiveConfig_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, uint32_t config)>;
    // @callflow(next="*")
    /**
     * Retrieves which display configuration is currently active.
     * 
     * If no display configuration is currently active, this function must
     * return BAD_CONFIG. It is the responsibility of the client to call
     * setActiveConfig with a valid configuration before attempting to present
     * anything on the display.
     * 
     * @param display is the display to which the active config is queried.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     *         BAD_CONFIG when no configuration is currently active.
     * @return config is the currently active display configuration.
     */
    virtual ::android::hardware::Return<void> getActiveConfig(uint64_t display, getActiveConfig_cb _hidl_cb) = 0;

    // @callflow(next="*")
    /**
     * Returns whether a client target with the given properties can be
     * handled by the device.
     * 
     * This function must return true for a client target with width and
     * height equal to the active display configuration dimensions,
     * PixelFormat::RGBA_8888, and Dataspace::UNKNOWN. It is not required to
     * return true for any other configuration.
     * 
     * @param display is the display to query.
     * @param width is the client target width in pixels.
     * @param height is the client target height in pixels.
     * @param format is the client target format.
     * @param dataspace is the client target dataspace, as described in
     *        setLayerDataspace.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     *         UNSUPPORTED when the given configuration is not supported.
     */
    virtual ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> getClientTargetSupport(uint64_t display, uint32_t width, uint32_t height, ::android::hardware::graphics::common::V1_0::PixelFormat format, ::android::hardware::graphics::common::V1_0::Dataspace dataspace) = 0;

    /**
     * Return callback for getColorModes
     */
    using getColorModes_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, const ::android::hardware::hidl_vec<::android::hardware::graphics::common::V1_0::ColorMode>& modes)>;
    // @callflow(next="*")
    /**
     * Returns the color modes supported on this display.
     * 
     * All devices must support at least ColorMode::NATIVE.
     * 
     * @param display is the display to query.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     * @return modes is an array of color modes.
     */
    virtual ::android::hardware::Return<void> getColorModes(uint64_t display, getColorModes_cb _hidl_cb) = 0;

    /**
     * Return callback for getDisplayAttribute
     */
    using getDisplayAttribute_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, int32_t value)>;
    // @callflow(next="*")
    /**
     * Returns a display attribute value for a particular display
     * configuration.
     * 
     * @param display is the display to query.
     * @param config is the display configuration for which to return
     *        attribute values.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     *         BAD_CONFIG when config does not name a valid configuration for
     *                    this display.
     *         BAD_PARAMETER when attribute is unrecognized.
     *         UNSUPPORTED when attribute cannot be queried for the config.
     * @return value is the value of the attribute.
     */
    virtual ::android::hardware::Return<void> getDisplayAttribute(uint64_t display, uint32_t config, ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute attribute, getDisplayAttribute_cb _hidl_cb) = 0;

    /**
     * Return callback for getDisplayConfigs
     */
    using getDisplayConfigs_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, const ::android::hardware::hidl_vec<uint32_t>& configs)>;
    // @callflow(next="*")
    /**
     * Returns handles for all of the valid display configurations on this
     * display.
     * 
     * @param display is the display to query.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     * @return configs is an array of configuration handles.
     */
    virtual ::android::hardware::Return<void> getDisplayConfigs(uint64_t display, getDisplayConfigs_cb _hidl_cb) = 0;

    /**
     * Return callback for getDisplayName
     */
    using getDisplayName_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, const ::android::hardware::hidl_string& name)>;
    // @callflow(next="*")
    /**
     * Returns a human-readable version of the display's name.
     * 
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     * @return name is the name of the display.
     */
    virtual ::android::hardware::Return<void> getDisplayName(uint64_t display, getDisplayName_cb _hidl_cb) = 0;

    /**
     * Return callback for getDisplayType
     */
    using getDisplayType_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType type)>;
    // @callflow(next="*")
    /**
     * Returns whether the given display is a physical or virtual display.
     * 
     * @param display is the display to query.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     * @return type is the type of the display.
     */
    virtual ::android::hardware::Return<void> getDisplayType(uint64_t display, getDisplayType_cb _hidl_cb) = 0;

    /**
     * Return callback for getDozeSupport
     */
    using getDozeSupport_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, bool support)>;
    // @callflow(next="*")
    /**
     * Returns whether the given display supports PowerMode::DOZE and
     * PowerMode::DOZE_SUSPEND. DOZE_SUSPEND may not provide any benefit over
     * DOZE (see the definition of PowerMode for more information), but if
     * both DOZE and DOZE_SUSPEND are no different from PowerMode::ON, the
     * device must not claim support.
     * 
     * @param display is the display to query.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     * @return support is true only when the display supports doze modes.
     */
    virtual ::android::hardware::Return<void> getDozeSupport(uint64_t display, getDozeSupport_cb _hidl_cb) = 0;

    /**
     * Return callback for getHdrCapabilities
     */
    using getHdrCapabilities_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, const ::android::hardware::hidl_vec<::android::hardware::graphics::common::V1_0::Hdr>& types, float maxLuminance, float maxAverageLuminance, float minLuminance)>;
    // @callflow(next="*")
    /**
     * Returns the high dynamic range (HDR) capabilities of the given display,
     * which are invariant with regard to the active configuration.
     * 
     * Displays which are not HDR-capable must return no types.
     * 
     * @param display is the display to query.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     * @return types is an array of HDR types, may have 0 elements if the
     *         display is not HDR-capable.
     * @return maxLuminance is the desired content maximum luminance for this
     *         display in cd/m^2.
     * @return maxAverageLuminance - the desired content maximum frame-average
     *         luminance for this display in cd/m^2.
     * @return minLuminance is the desired content minimum luminance for this
     *         display in cd/m^2.
     */
    virtual ::android::hardware::Return<void> getHdrCapabilities(uint64_t display, getHdrCapabilities_cb _hidl_cb) = 0;

    // @callflow(next="*")
    /**
     * Set the number of client target slots to be reserved.
     * 
     * @param display is the display to which the slots are reserved.
     * @param clientTargetSlotCount is the slot count for client targets.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     *         NO_RESOURCES when unable to reserve the slots.
     */
    virtual ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setClientTargetSlotCount(uint64_t display, uint32_t clientTargetSlotCount) = 0;

    // @callflow(next="*")
    /**
     * Sets the active configuration for this display. Upon returning, the
     * given display configuration must be active and remain so until either
     * this function is called again or the display is disconnected.
     * 
     * @param display is the display to which the active config is set.
     * @param config is the new display configuration.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     *         BAD_CONFIG when the configuration handle passed in is not valid
     *                    for this display.
     */
    virtual ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setActiveConfig(uint64_t display, uint32_t config) = 0;

    // @callflow(next="*")
    /**
     * Sets the color mode of the given display.
     * 
     * Upon returning from this function, the color mode change must have
     * fully taken effect.
     * 
     * All devices must support at least ColorMode::NATIVE, and displays are
     * assumed to be in this mode upon hotplug.
     * 
     * @param display is the display to which the color mode is set.
     * @param mode is the mode to set to.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     *         BAD_PARAMETER when mode is not a valid color mode.
     *         UNSUPPORTED when mode is not supported on this display.
     */
    virtual ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setColorMode(uint64_t display, ::android::hardware::graphics::common::V1_0::ColorMode mode) = 0;

    // @callflow(next="*")
    /**
     * Sets the power mode of the given display. The transition must be
     * complete when this function returns. It is valid to call this function
     * multiple times with the same power mode.
     * 
     * All displays must support PowerMode::ON and PowerMode::OFF.  Whether a
     * display supports PowerMode::DOZE or PowerMode::DOZE_SUSPEND may be
     * queried using getDozeSupport.
     * 
     * @param display is the display to which the power mode is set.
     * @param mode is the new power mode.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     *         BAD_PARAMETER when mode was not a valid power mode.
     *         UNSUPPORTED when mode is not supported on this display.
     */
    virtual ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setPowerMode(uint64_t display, ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode mode) = 0;

    // @callflow(next="*")
    /**
     * Enables or disables the vsync signal for the given display. Virtual
     * displays never generate vsync callbacks, and any attempt to enable
     * vsync for a virtual display though this function must succeed and have
     * no other effect.
     * 
     * @param display is the display to which the vsync mode is set.
     * @param enabled indicates whether to enable or disable vsync
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     *         BAD_PARAMETER when enabled was an invalid value.
     */
    virtual ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setVsyncEnabled(uint64_t display, ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync enabled) = 0;

    // @callflow(next="*")
    /**
     * Sets the input command message queue.
     * 
     * @param descriptor is the descriptor of the input command message queue.
     * @return error is NONE upon success. Otherwise,
     *         NO_RESOURCES when failed to set the queue temporarily.
     */
    virtual ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setInputCommandQueue(const ::android::hardware::MQDescriptorSync<uint32_t>& descriptor) = 0;

    /**
     * Return callback for getOutputCommandQueue
     */
    using getOutputCommandQueue_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, const ::android::hardware::MQDescriptorSync<uint32_t>& descriptor)>;
    // @callflow(next="*")
    /**
     * Gets the output command message queue.
     * 
     * This function must only be called inside executeCommands closure.
     * 
     * @return error is NONE upon success. Otherwise,
     *         NO_RESOURCES when failed to get the queue temporarily.
     * @return descriptor is the descriptor of the output command queue.
     */
    virtual ::android::hardware::Return<void> getOutputCommandQueue(getOutputCommandQueue_cb _hidl_cb) = 0;

    /**
     * Return callback for executeCommands
     */
    using executeCommands_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, bool outQueueChanged, uint32_t outLength, const ::android::hardware::hidl_vec<::android::hardware::hidl_handle>& outHandles)>;
    // @callflow(next="*")
    /**
     * Executes commands from the input command message queue. Return values
     * generated by the input commands are written to the output command
     * message queue in the form of value commands.
     * 
     * @param inLength is the length of input commands.
     * @param inHandles is an array of handles referenced by the input
     *        commands.
     * @return error is NONE upon success. Otherwise,
     *         BAD_PARAMETER when inLength is not equal to the length of
     *                       commands in the input command message queue.
     *         NO_RESOURCES when the output command message queue was not
     *                      properly drained.
     * @param outQueueChanged indicates whether the output command message
     *        queue has changed.
     * @param outLength is the length of output commands.
     * @param outHandles is an array of handles referenced by the output
     *        commands.
     */
    virtual ::android::hardware::Return<void> executeCommands(uint32_t inLength, const ::android::hardware::hidl_vec<::android::hardware::hidl_handle>& inHandles, executeCommands_cb _hidl_cb) = 0;

    /**
     * Return callback for getPerFrameMetadataKeys
     */
    using getPerFrameMetadataKeys_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, const ::android::hardware::hidl_vec<::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey>& keys)>;
    /**
     * Returns the PerFrameMetadataKeys that are supported by this device.
     * 
     * @param display is the display on which to create the layer.
     * @return keys is the vector of PerFrameMetadataKey keys that are
     *        supported by this device.
     * @return error is NONE upon success. Otherwise,
     *         UNSUPPORTED if not supported on underlying HAL
     */
    virtual ::android::hardware::Return<void> getPerFrameMetadataKeys(uint64_t display, getPerFrameMetadataKeys_cb _hidl_cb) = 0;

    /**
     * Return callback for getReadbackBufferAttributes
     */
    using getReadbackBufferAttributes_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, ::android::hardware::graphics::common::V1_1::PixelFormat format, ::android::hardware::graphics::common::V1_1::Dataspace dataspace)>;
    /**
     * getReadbackBufferAttributes
     * Returns the format which should be used when allocating a buffer for use by
     * device readback as well as the dataspace in which its contents should be
     * interpreted.
     * 
     * The width and height of this buffer must be those of the currently-active
     * display configuration, and the usage flags must consist of the following:
     *   BufferUsage::CPU_READ | BufferUsage::GPU_TEXTURE |
     *   BufferUsage::COMPOSER_OUTPUT
     * 
     * The format and dataspace provided must be sufficient such that if a
     * correctly-configured buffer is passed into setReadbackBuffer, filled by
     * the device, and then displayed by the client as a full-screen buffer, the
     * output of the display remains the same (subject to the note about protected
     * content in the description of setReadbackBuffer).
     * 
     * If the active configuration or color mode of this display has changed
     * since a previous call to this function, it must be called again prior to
     * setting a readback buffer such that the returned format and dataspace can
     * be updated accordingly.
     * 
     * Parameters:
     * @param display - the display on which to create the layer.
     * 
     * @return format - the format the client should use when allocating a device
     *       readback buffer
     * @return dataspace - the dataspace to use when interpreting the
     *       contents of a device readback buffer
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     *         UNSUPPORTED if not supported on underlying HAL
     * 
     * See also:
     *   setReadbackBuffer
     *   getReadbackBufferFence
     */
    virtual ::android::hardware::Return<void> getReadbackBufferAttributes(uint64_t display, getReadbackBufferAttributes_cb _hidl_cb) = 0;

    /**
     * Return callback for getReadbackBufferFence
     */
    using getReadbackBufferFence_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, const ::android::hardware::hidl_handle& acquireFence)>;
    /**
     * getReadbackBufferFence
     * Returns an acquire sync fence file descriptor which must signal when the
     * buffer provided to setReadbackBuffer has been filled by the device and is
     * safe for the client to read.
     * 
     * If it is already safe to read from this buffer, -1 may be returned instead.
     * The client takes ownership of this file descriptor and is responsible for
     * closing it when it is no longer needed.
     * 
     * This function must be called immediately after the composition cycle being
     * captured into the readback buffer. The complete ordering of a readback buffer
     * capture is as follows:
     * 
     *   getReadbackBufferAttributes
     *   // Readback buffer is allocated
     *   // Many frames may pass
     * 
     *   setReadbackBuffer
     *   validateDisplay
     *   presentDisplay
     *   getReadbackBufferFence
     *   // Implicitly wait on the acquire fence before accessing the buffer
     * 
     * Parameters:
     * @param display - the display on which to create the layer.
     * 
     * @return acquireFence - a sync fence file descriptor as described above; pointer
     *       must be non-NULL
     * @return error - is HWC2_ERROR_NONE or one of the following errors:
     *         BAD_DISPLAY - an invalid display handle was passed in
     *         NO_RESOURCES - the readback operation was successful, but
     *                        resulted in a different validate result than would
     *                        have occurred without readback
     *         UNSUPPORTED - the readback operation was unsuccessful because of
     *                       resource constraints, the presence of protected
     *                       content, or other reasons; -1 must be returned for
     *                       acquireFence
     * 
     * See also:
     *   getReadbackBufferAttributes
     *   setReadbackBuffer
     */
    virtual ::android::hardware::Return<void> getReadbackBufferFence(uint64_t display, getReadbackBufferFence_cb _hidl_cb) = 0;

    /**
     * setReadbackBuffer
     * Sets the readback buffer to be filled with the contents of the next
     * composition performed for this display (i.e., the contents present at the
     * time of the next validateDisplay/presentDisplay cycle).
     * 
     * This buffer must have been allocated as described in
     * getReadbackBufferAttributes and is in the dataspace provided by the same.
     * 
     * If there is hardware protected content on the display at the time of the next
     * composition, the area of the readback buffer covered by such content must be
     * completely black. Any areas of the buffer not covered by such content may
     * optionally be black as well.
     * 
     * The release fence file descriptor provided works identically to the one
     * described for setOutputBuffer.
     * 
     * This function must not be called between any call to validateDisplay and a
     * subsequent call to presentDisplay.
     * 
     * Parameters:
     * @param display - the display on which to create the layer.
     * @param buffer - the new readback buffer
     * @param releaseFence - a sync fence file descriptor as described in setOutputBuffer
     * 
     * @return error - is HWC2_ERROR_NONE or one of the following errors:
     *   HWC2_ERROR_BAD_DISPLAY - an invalid display handle was passed in
     *   HWC2_ERROR_BAD_PARAMETER - the new readback buffer handle was invalid
     * 
     * See also:
     *   getReadbackBufferAttributes
     *   getReadbackBufferFence
     */
    virtual ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setReadbackBuffer(uint64_t display, const ::android::hardware::hidl_handle& buffer, const ::android::hardware::hidl_handle& releaseFence) = 0;

    /**
     * Return callback for createVirtualDisplay_2_2
     */
    using createVirtualDisplay_2_2_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, uint64_t display, ::android::hardware::graphics::common::V1_1::PixelFormat format)>;
    // @callflow(next="*")
    /**
     * createVirtualDisplay_2_2
     * Creates a new virtual display with the given width and height. The
     * format passed into this function is the default format requested by the
     * consumer of the virtual display output buffers.
     * 
     * The display must be assumed to be on from the time the first frame is
     * presented until the display is destroyed.
     * 
     * @param width is the width in pixels.
     * @param height is the height in pixels.
     * @param formatHint is the default output buffer format selected by
     *        the consumer.
     * @param outputBufferSlotCount is the number of output buffer slots to be
     *        reserved.
     * @return error is NONE upon success. Otherwise,
     *         UNSUPPORTED when the width or height is too large for the
     *                     device to be able to create a virtual display.
     *         NO_RESOURCES when the device is unable to create a new virtual
     *                      display at this time.
     * @return display is the newly-created virtual display.
     * @return format is the format of the buffer the device will produce.
     */
    virtual ::android::hardware::Return<void> createVirtualDisplay_2_2(uint32_t width, uint32_t height, ::android::hardware::graphics::common::V1_1::PixelFormat formatHint, uint32_t outputBufferSlotCount, createVirtualDisplay_2_2_cb _hidl_cb) = 0;

    // @callflow(next="*")
    /**
     * getClientTargetSupport_2_2
     * Returns whether a client target with the given properties can be
     * handled by the device.
     * 
     * This function must return true for a client target with width and
     * height equal to the active display configuration dimensions,
     * PixelFormat::RGBA_8888, and Dataspace::UNKNOWN. It is not required to
     * return true for any other configuration.
     * 
     * @param display is the display to query.
     * @param width is the client target width in pixels.
     * @param height is the client target height in pixels.
     * @param format is the client target format.
     * @param dataspace is the client target dataspace, as described in
     *        setLayerDataspace.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     *         UNSUPPORTED when the given configuration is not supported.
     */
    virtual ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> getClientTargetSupport_2_2(uint64_t display, uint32_t width, uint32_t height, ::android::hardware::graphics::common::V1_1::PixelFormat format, ::android::hardware::graphics::common::V1_1::Dataspace dataspace) = 0;

    /**
     * setPowerMode_2_2
     * Sets the power mode of the given display. The transition must be
     * complete when this function returns. It is valid to call this function
     * multiple times with the same power mode.
     * 
     * All displays must support PowerMode::ON and PowerMode::OFF.  Whether a
     * display supports PowerMode::DOZE or PowerMode::DOZE_SUSPEND may be
     * queried using getDozeSupport.
     * 
     * @param display is the display to which the power mode is set.
     * @param mode is the new power mode.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     *         BAD_PARAMETER when mode was not a valid power mode.
     *         UNSUPPORTED when mode is not supported on this display.
     */
    virtual ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setPowerMode_2_2(uint64_t display, ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode mode) = 0;

    /**
     * Return callback for getColorModes_2_2
     */
    using getColorModes_2_2_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, const ::android::hardware::hidl_vec<::android::hardware::graphics::common::V1_1::ColorMode>& modes)>;
    /**
     * Returns the color modes supported on this display.
     * 
     * All devices must support at least ColorMode::NATIVE.
     * 
     * @param display is the display to query.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     * @return modes is an array of color modes.
     */
    virtual ::android::hardware::Return<void> getColorModes_2_2(uint64_t display, getColorModes_2_2_cb _hidl_cb) = 0;

    /**
     * Return callback for getRenderIntents
     */
    using getRenderIntents_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, const ::android::hardware::hidl_vec<::android::hardware::graphics::common::V1_1::RenderIntent>& intents)>;
    /**
     * Returns the render intents supported by the specified display and color
     * mode.
     * 
     * For SDR color modes, RenderIntent::COLORIMETRIC must be supported. For
     * HDR color modes, RenderIntent::TONE_MAP_COLORIMETRIC must be supported.
     * 
     * @param display is the display to query.
     * @param mode is the color mode to query.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     *         BAD_PARAMETER when an invalid color mode was passed in.
     * @return intents is an array of render intents.
     */
    virtual ::android::hardware::Return<void> getRenderIntents(uint64_t display, ::android::hardware::graphics::common::V1_1::ColorMode mode, getRenderIntents_cb _hidl_cb) = 0;

    /**
     * Sets the color mode and render intent of the given display.
     * 
     * The color mode and render intent change must take effect on next
     * presentDisplay.
     * 
     * All devices must support at least ColorMode::NATIVE and
     * RenderIntent::COLORIMETRIC, and displays are assumed to be in this mode
     * upon hotplug.
     * 
     * @param display is the display to which the color mode is set.
     * @param mode is the color mode to set to.
     * @param intent is the render intent to set to.
     * @return error is NONE upon success. Otherwise,
     *         BAD_DISPLAY when an invalid display handle was passed in.
     *         BAD_PARAMETER when mode or intent is invalid
     *         UNSUPPORTED when mode or intent is not supported on this
     *                     display.
     */
    virtual ::android::hardware::Return<::android::hardware::graphics::composer::V2_1::Error> setColorMode_2_2(uint64_t display, ::android::hardware::graphics::common::V1_1::ColorMode mode, ::android::hardware::graphics::common::V1_1::RenderIntent intent) = 0;

    /**
     * Return callback for getDataspaceSaturationMatrix
     */
    using getDataspaceSaturationMatrix_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, const ::android::hardware::hidl_array<float, 4, 4>& matrix)>;
    /**
     * Returns the saturation matrix of the specified legacy dataspace.
     * 
     * The saturation matrix can be used to approximate the legacy dataspace
     * saturation transform. It is to be applied on linear pixel values like
     * this:
     * 
     *   (in GLSL)
     *   linearSrgb = saturationMatrix * linearSrgb;
     * 
     * @param dataspace must be Dataspace::SRGB_LINEAR.
     * @return error is NONE upon success. Otherwise,
     *         BAD_PARAMETER when an invalid dataspace was passed in.
     * @return matrix is the 4x4 column-major matrix used to approximate the
     *         legacy dataspace saturation operation. The last row must be
     *         [0.0, 0.0, 0.0, 1.0].
     */
    virtual ::android::hardware::Return<void> getDataspaceSaturationMatrix(::android::hardware::graphics::common::V1_1::Dataspace dataspace, getDataspaceSaturationMatrix_cb _hidl_cb) = 0;

    /**
     * Return callback for executeCommands_2_2
     */
    using executeCommands_2_2_cb = std::function<void(::android::hardware::graphics::composer::V2_1::Error error, bool outQueueChanged, uint32_t outLength, const ::android::hardware::hidl_vec<::android::hardware::hidl_handle>& outHandles)>;
    /**
     * Executes commands from the input command message queue. Return values
     * generated by the input commands are written to the output command
     * message queue in the form of value commands.
     * 
     * @param inLength is the length of input commands.
     * @param inHandles is an array of handles referenced by the input
     *        commands.
     * @return error is NONE upon success. Otherwise,
     *         BAD_PARAMETER when inLength is not equal to the length of
     *                       commands in the input command message queue.
     *         NO_RESOURCES when the output command message queue was not
     *                      properly drained.
     * @param outQueueChanged indicates whether the output command message
     *        queue has changed.
     * @param outLength is the length of output commands.
     * @param outHandles is an array of handles referenced by the output
     *        commands.
     */
    virtual ::android::hardware::Return<void> executeCommands_2_2(uint32_t inLength, const ::android::hardware::hidl_vec<::android::hardware::hidl_handle>& inHandles, executeCommands_2_2_cb _hidl_cb) = 0;

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
    static ::android::hardware::Return<::android::sp<::android::hardware::graphics::composer::V2_2::IComposerClient>> castFrom(const ::android::sp<::android::hardware::graphics::composer::V2_2::IComposerClient>& parent, bool emitError = false);
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::android::hardware::graphics::composer::V2_2::IComposerClient>> castFrom(const ::android::sp<::android::hardware::graphics::composer::V2_1::IComposerClient>& parent, bool emitError = false);
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::android::hardware::graphics::composer::V2_2::IComposerClient>> castFrom(const ::android::sp<::android::hidl::base::V1_0::IBase>& parent, bool emitError = false);

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
    static ::android::sp<IComposerClient> tryGetService(const std::string &serviceName="default", bool getStub=false);
    /**
     * Deprecated. See tryGetService(std::string, bool)
     */
    static ::android::sp<IComposerClient> tryGetService(const char serviceName[], bool getStub=false)  { std::string str(serviceName ? serviceName : "");      return tryGetService(str, getStub); }
    /**
     * Deprecated. See tryGetService(std::string, bool)
     */
    static ::android::sp<IComposerClient> tryGetService(const ::android::hardware::hidl_string& serviceName, bool getStub=false)  { std::string str(serviceName.c_str());      return tryGetService(str, getStub); }
    /**
     * Calls tryGetService("default", bool). This is the recommended instance name for singleton services.
     */
    static ::android::sp<IComposerClient> tryGetService(bool getStub) { return tryGetService("default", getStub); }
    /**
     * This gets the service of this type with the specified instance name. If the
     * service is not in the VINTF manifest on a Trebilized device, this will return
     * nullptr. If the service is not available, this will wait for the service to
     * become available. If the service is a lazy service, this will start the service
     * and return when it becomes available. If getStub is true, this will try to
     * return an unwrapped passthrough implementation in the same process. This is
     * useful when getting an implementation from the same partition/compilation group.
     */
    static ::android::sp<IComposerClient> getService(const std::string &serviceName="default", bool getStub=false);
    /**
     * Deprecated. See getService(std::string, bool)
     */
    static ::android::sp<IComposerClient> getService(const char serviceName[], bool getStub=false)  { std::string str(serviceName ? serviceName : "");      return getService(str, getStub); }
    /**
     * Deprecated. See getService(std::string, bool)
     */
    static ::android::sp<IComposerClient> getService(const ::android::hardware::hidl_string& serviceName, bool getStub=false)  { std::string str(serviceName.c_str());      return getService(str, getStub); }
    /**
     * Calls getService("default", bool). This is the recommended instance name for singleton services.
     */
    static ::android::sp<IComposerClient> getService(bool getStub) { return getService("default", getStub); }
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

template<typename>
static inline std::string toString(int32_t o);
static inline std::string toString(::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode o);

constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const int32_t lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode rhs) {
    return static_cast<int32_t>(lhs | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | rhs);
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const int32_t lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode rhs) {
    return static_cast<int32_t>(lhs & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & rhs);
}
constexpr int32_t &operator|=(int32_t& v, const ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode e) {
    v |= static_cast<int32_t>(e);
    return v;
}
constexpr int32_t &operator&=(int32_t& v, const ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode e) {
    v &= static_cast<int32_t>(e);
    return v;
}

template<typename>
static inline std::string toString(int32_t o);
static inline std::string toString(::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey o);

constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const int32_t lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey rhs) {
    return static_cast<int32_t>(lhs | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | rhs);
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const int32_t lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey rhs) {
    return static_cast<int32_t>(lhs & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & rhs);
}
constexpr int32_t &operator|=(int32_t& v, const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey e) {
    v |= static_cast<int32_t>(e);
    return v;
}
constexpr int32_t &operator&=(int32_t& v, const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey e) {
    v &= static_cast<int32_t>(e);
    return v;
}

static inline std::string toString(const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadata& o);
static inline bool operator==(const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadata& lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadata& rhs);
static inline bool operator!=(const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadata& lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadata& rhs);

static inline std::string toString(const ::android::hardware::graphics::composer::V2_2::IComposerClient::FloatColor& o);
static inline bool operator==(const ::android::hardware::graphics::composer::V2_2::IComposerClient::FloatColor& lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::FloatColor& rhs);
static inline bool operator!=(const ::android::hardware::graphics::composer::V2_2::IComposerClient::FloatColor& lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::FloatColor& rhs);

template<typename>
static inline std::string toString(int32_t o);
static inline std::string toString(::android::hardware::graphics::composer::V2_2::IComposerClient::Command o);

constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_2::IComposerClient::Command lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::Command rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const int32_t lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::Command rhs) {
    return static_cast<int32_t>(lhs | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_2::IComposerClient::Command lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | rhs);
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_2::IComposerClient::Command lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::Command rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const int32_t lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::Command rhs) {
    return static_cast<int32_t>(lhs & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_2::IComposerClient::Command lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & rhs);
}
constexpr int32_t &operator|=(int32_t& v, const ::android::hardware::graphics::composer::V2_2::IComposerClient::Command e) {
    v |= static_cast<int32_t>(e);
    return v;
}
constexpr int32_t &operator&=(int32_t& v, const ::android::hardware::graphics::composer::V2_2::IComposerClient::Command e) {
    v &= static_cast<int32_t>(e);
    return v;
}

static inline std::string toString(const ::android::sp<::android::hardware::graphics::composer::V2_2::IComposerClient>& o);

//
// type header definitions for package
//

template<>
inline std::string toString<::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode>(int32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::OFF) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::OFF)) {
        os += (first ? "" : " | ");
        os += "OFF";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::OFF;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::DOZE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::DOZE)) {
        os += (first ? "" : " | ");
        os += "DOZE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::DOZE;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::DOZE_SUSPEND) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::DOZE_SUSPEND)) {
        os += (first ? "" : " | ");
        os += "DOZE_SUSPEND";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::DOZE_SUSPEND;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::ON) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::ON)) {
        os += (first ? "" : " | ");
        os += "ON";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::ON;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::ON_SUSPEND) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::ON_SUSPEND)) {
        os += (first ? "" : " | ");
        os += "ON_SUSPEND";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::ON_SUSPEND;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::OFF) {
        return "OFF";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::DOZE) {
        return "DOZE";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::DOZE_SUSPEND) {
        return "DOZE_SUSPEND";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::ON) {
        return "ON";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::ON_SUSPEND) {
        return "ON_SUSPEND";
    }
    std::string os;
    os += toHexString(static_cast<int32_t>(o));
    return os;
}

template<>
inline std::string toString<::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey>(int32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_RED_PRIMARY_X) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_RED_PRIMARY_X)) {
        os += (first ? "" : " | ");
        os += "DISPLAY_RED_PRIMARY_X";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_RED_PRIMARY_X;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_RED_PRIMARY_Y) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_RED_PRIMARY_Y)) {
        os += (first ? "" : " | ");
        os += "DISPLAY_RED_PRIMARY_Y";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_RED_PRIMARY_Y;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_X) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_X)) {
        os += (first ? "" : " | ");
        os += "DISPLAY_GREEN_PRIMARY_X";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_X;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_Y) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_Y)) {
        os += (first ? "" : " | ");
        os += "DISPLAY_GREEN_PRIMARY_Y";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_Y;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_X) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_X)) {
        os += (first ? "" : " | ");
        os += "DISPLAY_BLUE_PRIMARY_X";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_X;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_Y) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_Y)) {
        os += (first ? "" : " | ");
        os += "DISPLAY_BLUE_PRIMARY_Y";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_Y;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::WHITE_POINT_X) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::WHITE_POINT_X)) {
        os += (first ? "" : " | ");
        os += "WHITE_POINT_X";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::WHITE_POINT_X;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::WHITE_POINT_Y) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::WHITE_POINT_Y)) {
        os += (first ? "" : " | ");
        os += "WHITE_POINT_Y";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::WHITE_POINT_Y;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MAX_LUMINANCE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MAX_LUMINANCE)) {
        os += (first ? "" : " | ");
        os += "MAX_LUMINANCE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MAX_LUMINANCE;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MIN_LUMINANCE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MIN_LUMINANCE)) {
        os += (first ? "" : " | ");
        os += "MIN_LUMINANCE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MIN_LUMINANCE;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MAX_CONTENT_LIGHT_LEVEL) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MAX_CONTENT_LIGHT_LEVEL)) {
        os += (first ? "" : " | ");
        os += "MAX_CONTENT_LIGHT_LEVEL";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MAX_CONTENT_LIGHT_LEVEL;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MAX_FRAME_AVERAGE_LIGHT_LEVEL) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MAX_FRAME_AVERAGE_LIGHT_LEVEL)) {
        os += (first ? "" : " | ");
        os += "MAX_FRAME_AVERAGE_LIGHT_LEVEL";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MAX_FRAME_AVERAGE_LIGHT_LEVEL;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_RED_PRIMARY_X) {
        return "DISPLAY_RED_PRIMARY_X";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_RED_PRIMARY_Y) {
        return "DISPLAY_RED_PRIMARY_Y";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_X) {
        return "DISPLAY_GREEN_PRIMARY_X";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_Y) {
        return "DISPLAY_GREEN_PRIMARY_Y";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_X) {
        return "DISPLAY_BLUE_PRIMARY_X";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_Y) {
        return "DISPLAY_BLUE_PRIMARY_Y";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::WHITE_POINT_X) {
        return "WHITE_POINT_X";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::WHITE_POINT_Y) {
        return "WHITE_POINT_Y";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MAX_LUMINANCE) {
        return "MAX_LUMINANCE";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MIN_LUMINANCE) {
        return "MIN_LUMINANCE";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MAX_CONTENT_LIGHT_LEVEL) {
        return "MAX_CONTENT_LIGHT_LEVEL";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MAX_FRAME_AVERAGE_LIGHT_LEVEL) {
        return "MAX_FRAME_AVERAGE_LIGHT_LEVEL";
    }
    std::string os;
    os += toHexString(static_cast<int32_t>(o));
    return os;
}

static inline std::string toString(const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadata& o) {
    using ::android::hardware::toString;
    std::string os;
    os += "{";
    os += ".key = ";
    os += ::android::hardware::graphics::composer::V2_2::toString(o.key);
    os += ", .value = ";
    os += ::android::hardware::toString(o.value);
    os += "}"; return os;
}

static inline bool operator==(const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadata& lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadata& rhs) {
    if (lhs.key != rhs.key) {
        return false;
    }
    if (lhs.value != rhs.value) {
        return false;
    }
    return true;
}

static inline bool operator!=(const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadata& lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadata& rhs){
    return !(lhs == rhs);
}

static inline std::string toString(const ::android::hardware::graphics::composer::V2_2::IComposerClient::FloatColor& o) {
    using ::android::hardware::toString;
    std::string os;
    os += "{";
    os += ".r = ";
    os += ::android::hardware::toString(o.r);
    os += ", .g = ";
    os += ::android::hardware::toString(o.g);
    os += ", .b = ";
    os += ::android::hardware::toString(o.b);
    os += ", .a = ";
    os += ::android::hardware::toString(o.a);
    os += "}"; return os;
}

static inline bool operator==(const ::android::hardware::graphics::composer::V2_2::IComposerClient::FloatColor& lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::FloatColor& rhs) {
    if (lhs.r != rhs.r) {
        return false;
    }
    if (lhs.g != rhs.g) {
        return false;
    }
    if (lhs.b != rhs.b) {
        return false;
    }
    if (lhs.a != rhs.a) {
        return false;
    }
    return true;
}

static inline bool operator!=(const ::android::hardware::graphics::composer::V2_2::IComposerClient::FloatColor& lhs, const ::android::hardware::graphics::composer::V2_2::IComposerClient::FloatColor& rhs){
    return !(lhs == rhs);
}

template<>
inline std::string toString<::android::hardware::graphics::composer::V2_2::IComposerClient::Command>(int32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::graphics::composer::V2_2::IComposerClient::Command> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::LENGTH_MASK) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::LENGTH_MASK)) {
        os += (first ? "" : " | ");
        os += "LENGTH_MASK";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::LENGTH_MASK;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::OPCODE_SHIFT) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::OPCODE_SHIFT)) {
        os += (first ? "" : " | ");
        os += "OPCODE_SHIFT";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::OPCODE_SHIFT;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::OPCODE_MASK) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::OPCODE_MASK)) {
        os += (first ? "" : " | ");
        os += "OPCODE_MASK";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::OPCODE_MASK;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SELECT_DISPLAY) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SELECT_DISPLAY)) {
        os += (first ? "" : " | ");
        os += "SELECT_DISPLAY";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SELECT_DISPLAY;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SELECT_LAYER) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SELECT_LAYER)) {
        os += (first ? "" : " | ");
        os += "SELECT_LAYER";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SELECT_LAYER;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_ERROR) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_ERROR)) {
        os += (first ? "" : " | ");
        os += "SET_ERROR";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_ERROR;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_CHANGED_COMPOSITION_TYPES) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_CHANGED_COMPOSITION_TYPES)) {
        os += (first ? "" : " | ");
        os += "SET_CHANGED_COMPOSITION_TYPES";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_CHANGED_COMPOSITION_TYPES;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_DISPLAY_REQUESTS) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_DISPLAY_REQUESTS)) {
        os += (first ? "" : " | ");
        os += "SET_DISPLAY_REQUESTS";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_DISPLAY_REQUESTS;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_PRESENT_FENCE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_PRESENT_FENCE)) {
        os += (first ? "" : " | ");
        os += "SET_PRESENT_FENCE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_PRESENT_FENCE;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_RELEASE_FENCES) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_RELEASE_FENCES)) {
        os += (first ? "" : " | ");
        os += "SET_RELEASE_FENCES";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_RELEASE_FENCES;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_COLOR_TRANSFORM) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_COLOR_TRANSFORM)) {
        os += (first ? "" : " | ");
        os += "SET_COLOR_TRANSFORM";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_COLOR_TRANSFORM;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_CLIENT_TARGET) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_CLIENT_TARGET)) {
        os += (first ? "" : " | ");
        os += "SET_CLIENT_TARGET";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_CLIENT_TARGET;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_OUTPUT_BUFFER) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_OUTPUT_BUFFER)) {
        os += (first ? "" : " | ");
        os += "SET_OUTPUT_BUFFER";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_OUTPUT_BUFFER;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::VALIDATE_DISPLAY) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::VALIDATE_DISPLAY)) {
        os += (first ? "" : " | ");
        os += "VALIDATE_DISPLAY";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::VALIDATE_DISPLAY;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::ACCEPT_DISPLAY_CHANGES) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::ACCEPT_DISPLAY_CHANGES)) {
        os += (first ? "" : " | ");
        os += "ACCEPT_DISPLAY_CHANGES";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::ACCEPT_DISPLAY_CHANGES;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::PRESENT_DISPLAY) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::PRESENT_DISPLAY)) {
        os += (first ? "" : " | ");
        os += "PRESENT_DISPLAY";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::PRESENT_DISPLAY;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::PRESENT_OR_VALIDATE_DISPLAY) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::PRESENT_OR_VALIDATE_DISPLAY)) {
        os += (first ? "" : " | ");
        os += "PRESENT_OR_VALIDATE_DISPLAY";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::PRESENT_OR_VALIDATE_DISPLAY;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_CURSOR_POSITION) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_CURSOR_POSITION)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_CURSOR_POSITION";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_CURSOR_POSITION;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_BUFFER) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_BUFFER)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_BUFFER";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_BUFFER;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_SURFACE_DAMAGE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_SURFACE_DAMAGE)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_SURFACE_DAMAGE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_SURFACE_DAMAGE;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_BLEND_MODE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_BLEND_MODE)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_BLEND_MODE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_BLEND_MODE;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_COLOR) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_COLOR)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_COLOR";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_COLOR;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_COMPOSITION_TYPE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_COMPOSITION_TYPE)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_COMPOSITION_TYPE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_COMPOSITION_TYPE;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_DATASPACE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_DATASPACE)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_DATASPACE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_DATASPACE;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_DISPLAY_FRAME) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_DISPLAY_FRAME)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_DISPLAY_FRAME";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_DISPLAY_FRAME;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_PLANE_ALPHA) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_PLANE_ALPHA)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_PLANE_ALPHA";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_PLANE_ALPHA;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_SIDEBAND_STREAM) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_SIDEBAND_STREAM)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_SIDEBAND_STREAM";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_SIDEBAND_STREAM;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_SOURCE_CROP) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_SOURCE_CROP)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_SOURCE_CROP";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_SOURCE_CROP;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_TRANSFORM) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_TRANSFORM)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_TRANSFORM";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_TRANSFORM;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_VISIBLE_REGION) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_VISIBLE_REGION)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_VISIBLE_REGION";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_VISIBLE_REGION;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_Z_ORDER) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_Z_ORDER)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_Z_ORDER";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_Z_ORDER;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT)) {
        os += (first ? "" : " | ");
        os += "SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_PER_FRAME_METADATA) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_PER_FRAME_METADATA)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_PER_FRAME_METADATA";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_PER_FRAME_METADATA;
    }
    if ((o & ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_FLOAT_COLOR) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_FLOAT_COLOR)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_FLOAT_COLOR";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_FLOAT_COLOR;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::graphics::composer::V2_2::IComposerClient::Command o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::LENGTH_MASK) {
        return "LENGTH_MASK";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::OPCODE_SHIFT) {
        return "OPCODE_SHIFT";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::OPCODE_MASK) {
        return "OPCODE_MASK";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SELECT_DISPLAY) {
        return "SELECT_DISPLAY";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SELECT_LAYER) {
        return "SELECT_LAYER";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_ERROR) {
        return "SET_ERROR";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_CHANGED_COMPOSITION_TYPES) {
        return "SET_CHANGED_COMPOSITION_TYPES";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_DISPLAY_REQUESTS) {
        return "SET_DISPLAY_REQUESTS";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_PRESENT_FENCE) {
        return "SET_PRESENT_FENCE";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_RELEASE_FENCES) {
        return "SET_RELEASE_FENCES";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_COLOR_TRANSFORM) {
        return "SET_COLOR_TRANSFORM";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_CLIENT_TARGET) {
        return "SET_CLIENT_TARGET";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_OUTPUT_BUFFER) {
        return "SET_OUTPUT_BUFFER";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::VALIDATE_DISPLAY) {
        return "VALIDATE_DISPLAY";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::ACCEPT_DISPLAY_CHANGES) {
        return "ACCEPT_DISPLAY_CHANGES";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::PRESENT_DISPLAY) {
        return "PRESENT_DISPLAY";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::PRESENT_OR_VALIDATE_DISPLAY) {
        return "PRESENT_OR_VALIDATE_DISPLAY";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_CURSOR_POSITION) {
        return "SET_LAYER_CURSOR_POSITION";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_BUFFER) {
        return "SET_LAYER_BUFFER";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_SURFACE_DAMAGE) {
        return "SET_LAYER_SURFACE_DAMAGE";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_BLEND_MODE) {
        return "SET_LAYER_BLEND_MODE";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_COLOR) {
        return "SET_LAYER_COLOR";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_COMPOSITION_TYPE) {
        return "SET_LAYER_COMPOSITION_TYPE";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_DATASPACE) {
        return "SET_LAYER_DATASPACE";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_DISPLAY_FRAME) {
        return "SET_LAYER_DISPLAY_FRAME";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_PLANE_ALPHA) {
        return "SET_LAYER_PLANE_ALPHA";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_SIDEBAND_STREAM) {
        return "SET_LAYER_SIDEBAND_STREAM";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_SOURCE_CROP) {
        return "SET_LAYER_SOURCE_CROP";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_TRANSFORM) {
        return "SET_LAYER_TRANSFORM";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_VISIBLE_REGION) {
        return "SET_LAYER_VISIBLE_REGION";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_Z_ORDER) {
        return "SET_LAYER_Z_ORDER";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT) {
        return "SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_PER_FRAME_METADATA) {
        return "SET_LAYER_PER_FRAME_METADATA";
    }
    if (o == ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_FLOAT_COLOR) {
        return "SET_LAYER_FLOAT_COLOR";
    }
    std::string os;
    os += toHexString(static_cast<int32_t>(o));
    return os;
}

static inline std::string toString(const ::android::sp<::android::hardware::graphics::composer::V2_2::IComposerClient>& o) {
    std::string os = "[class or subclass of ";
    os += ::android::hardware::graphics::composer::V2_2::IComposerClient::descriptor;
    os += "]";
    os += o->isRemote() ? "@remote" : "@local";
    return os;
}


}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android

//
// global type declarations for package
//

namespace android {
namespace hardware {
namespace details {
template<> constexpr std::array<::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode, 5> hidl_enum_values<::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode> = {
    ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::OFF,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::DOZE,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::DOZE_SUSPEND,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::ON,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::ON_SUSPEND,
};
}  // namespace details
}  // namespace hardware
}  // namespace android

namespace android {
namespace hardware {
namespace details {
template<> constexpr std::array<::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey, 12> hidl_enum_values<::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey> = {
    ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_RED_PRIMARY_X,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_RED_PRIMARY_Y,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_X,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_Y,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_X,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_Y,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::WHITE_POINT_X,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::WHITE_POINT_Y,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MAX_LUMINANCE,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MIN_LUMINANCE,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MAX_CONTENT_LIGHT_LEVEL,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::PerFrameMetadataKey::MAX_FRAME_AVERAGE_LIGHT_LEVEL,
};
}  // namespace details
}  // namespace hardware
}  // namespace android

namespace android {
namespace hardware {
namespace details {
template<> constexpr std::array<::android::hardware::graphics::composer::V2_2::IComposerClient::Command, 34> hidl_enum_values<::android::hardware::graphics::composer::V2_2::IComposerClient::Command> = {
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::LENGTH_MASK,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::OPCODE_SHIFT,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::OPCODE_MASK,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SELECT_DISPLAY,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SELECT_LAYER,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_ERROR,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_CHANGED_COMPOSITION_TYPES,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_DISPLAY_REQUESTS,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_PRESENT_FENCE,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_RELEASE_FENCES,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_COLOR_TRANSFORM,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_CLIENT_TARGET,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_OUTPUT_BUFFER,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::VALIDATE_DISPLAY,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::ACCEPT_DISPLAY_CHANGES,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::PRESENT_DISPLAY,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::PRESENT_OR_VALIDATE_DISPLAY,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_CURSOR_POSITION,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_BUFFER,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_SURFACE_DAMAGE,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_BLEND_MODE,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_COLOR,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_COMPOSITION_TYPE,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_DATASPACE,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_DISPLAY_FRAME,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_PLANE_ALPHA,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_SIDEBAND_STREAM,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_SOURCE_CROP,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_TRANSFORM,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_VISIBLE_REGION,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_Z_ORDER,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_PER_FRAME_METADATA,
    ::android::hardware::graphics::composer::V2_2::IComposerClient::Command::SET_LAYER_FLOAT_COLOR,
};
}  // namespace details
}  // namespace hardware
}  // namespace android


#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_2_ICOMPOSERCLIENT_H
