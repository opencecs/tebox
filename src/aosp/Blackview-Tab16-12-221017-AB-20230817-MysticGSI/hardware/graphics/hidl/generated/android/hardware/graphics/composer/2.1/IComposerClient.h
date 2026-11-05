#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_ICOMPOSERCLIENT_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_ICOMPOSERCLIENT_H

#include <android/hardware/graphics/common/1.0/types.h>
#include <android/hardware/graphics/composer/2.1/IComposerCallback.h>
#include <android/hardware/graphics/composer/2.1/types.h>
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
namespace composer {
namespace V2_1 {

struct IComposerClient : public ::android::hidl::base::V1_0::IBase {
    /**
     * Type tag for use in template logic that indicates this is a 'pure' class.
     */
    typedef android::hardware::details::i_tag _hidl_tag;

    /**
     * Fully qualified interface name: "android.hardware.graphics.composer@2.1::IComposerClient"
     */
    static const char* descriptor;

    // Forward declaration for forward reference support:
    enum class Attribute : int32_t;
    enum class DisplayRequest : uint32_t;
    enum class LayerRequest : uint32_t;
    enum class PowerMode : int32_t;
    enum class Vsync : int32_t;
    enum class BlendMode : int32_t;
    enum class Composition : int32_t;
    enum class DisplayType : int32_t;
    enum class HandleIndex : int32_t;
    struct Rect;
    struct FRect;
    struct Color;
    enum class Command : int32_t;

    /**
     * Display attributes queryable through getDisplayAttribute.
     */
    enum class Attribute : int32_t {
        INVALID = 0,
        /**
         * Dimensions in pixels
         */
        WIDTH = 1,
        HEIGHT = 2,
        /**
         * Vsync period in nanoseconds
         */
        VSYNC_PERIOD = 3,
        /**
         * Dots per thousand inches (DPI * 1000). Scaling by 1000 allows these
         * numbers to be stored in an int32_t without losing too much
         * precision. If the DPI for a configuration is unavailable or is
         * considered unreliable, the device may return UNSUPPORTED instead.
         */
        DPI_X = 4,
        DPI_Y = 5,
    };

    /**
     * Display requests returned by getDisplayRequests.
     */
    enum class DisplayRequest : uint32_t {
        /**
         * Instructs the client to provide a new client target buffer, even if
         * no layers are marked for client composition.
         */
        FLIP_CLIENT_TARGET = 1u /* (1 << 0) */,
        /**
         * Instructs the client to write the result of client composition
         * directly into the virtual display output buffer. If any of the
         * layers are not marked as Composition::CLIENT or the given display
         * is not a virtual display, this request has no effect.
         */
        WRITE_CLIENT_TARGET_TO_OUTPUT = 2u /* (1 << 1) */,
    };

    /**
     * Layer requests returned from getDisplayRequests.
     */
    enum class LayerRequest : uint32_t {
        /**
         * The client must clear its target with transparent pixels where
         * this layer would be. The client may ignore this request if the
         * layer must be blended.
         */
        CLEAR_CLIENT_TARGET = 1u /* (1 << 0) */,
    };

    /**
     * Power modes for use with setPowerMode.
     */
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
    };

    /**
     * Vsync values passed to setVsyncEnabled.
     */
    enum class Vsync : int32_t {
        INVALID = 0,
        /**
         * Enable vsync.
         */
        ENABLE = 1,
        /**
         * Disable vsync.
         */
        DISABLE = 2,
    };

    /**
     * Blend modes, settable per layer.
     */
    enum class BlendMode : int32_t {
        INVALID = 0,
        /**
         * colorOut = colorSrc
         */
        NONE = 1,
        /**
         * colorOut = colorSrc + colorDst * (1 - alphaSrc)
         */
        PREMULTIPLIED = 2,
        /**
         * colorOut = colorSrc * alphaSrc + colorDst * (1 - alphaSrc)
         */
        COVERAGE = 3,
    };

    /**
     * Possible composition types for a given layer.
     */
    enum class Composition : int32_t {
        INVALID = 0,
        /**
         * The client must composite this layer into the client target buffer
         * (provided to the device through setClientTarget).
         * 
         * The device must not request any composition type changes for layers
         * of this type.
         */
        CLIENT = 1,
        /**
         * The device must handle the composition of this layer through a
         * hardware overlay or other similar means.
         * 
         * Upon validateDisplay, the device may request a change from this
         * type to CLIENT.
         */
        DEVICE = 2,
        /**
         * The device must render this layer using the color set through
         * setLayerColor. If this functionality is not supported on a layer
         * that the client sets to SOLID_COLOR, the device must request that
         * the composition type of that layer is changed to CLIENT upon the
         * next call to validateDisplay.
         * 
         * Upon validateDisplay, the device may request a change from this
         * type to CLIENT.
         */
        SOLID_COLOR = 3,
        /**
         * Similar to DEVICE, but the position of this layer may also be set
         * asynchronously through setCursorPosition. If this functionality is
         * not supported on a layer that the client sets to CURSOR, the device
         * must request that the composition type of that layer is changed to
         * CLIENT upon the next call to validateDisplay.
         * 
         * Upon validateDisplay, the device may request a change from this
         * type to either DEVICE or CLIENT.  Changing to DEVICE will prevent
         * the use of setCursorPosition but still permit the device to
         * composite the layer.
         */
        CURSOR = 4,
        /**
         * The device must handle the composition of this layer, as well as
         * its buffer updates and content synchronization. Only supported on
         * devices which provide Capability::SIDEBAND_STREAM.
         * 
         * Upon validateDisplay, the device may request a change from this
         * type to either DEVICE or CLIENT, but it is unlikely that content
         * will display correctly in these cases.
         */
        SIDEBAND = 5,
    };

    /**
     * Display types returned by getDisplayType.
     */
    enum class DisplayType : int32_t {
        INVALID = 0,
        /**
         * All physical displays, including both internal displays and
         * hotpluggable external displays.
         */
        PHYSICAL = 1,
        /**
         * Virtual displays created by createVirtualDisplay.
         */
        VIRTUAL = 2,
    };

    /**
     * Special index values (always negative) for command queue commands.
     */
    enum class HandleIndex : int32_t {
        /**
         * No handle
         */
        EMPTY = -1 /* (-1) */,
        /**
         * Use cached handle
         */
        CACHED = -2 /* (-2) */,
    };

    struct Rect final {
        int32_t left __attribute__ ((aligned(4)));
        int32_t top __attribute__ ((aligned(4)));
        int32_t right __attribute__ ((aligned(4)));
        int32_t bottom __attribute__ ((aligned(4)));
    };

    static_assert(offsetof(::android::hardware::graphics::composer::V2_1::IComposerClient::Rect, left) == 0, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::composer::V2_1::IComposerClient::Rect, top) == 4, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::composer::V2_1::IComposerClient::Rect, right) == 8, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::composer::V2_1::IComposerClient::Rect, bottom) == 12, "wrong offset");
    static_assert(sizeof(::android::hardware::graphics::composer::V2_1::IComposerClient::Rect) == 16, "wrong size");
    static_assert(__alignof(::android::hardware::graphics::composer::V2_1::IComposerClient::Rect) == 4, "wrong alignment");

    struct FRect final {
        float left __attribute__ ((aligned(4)));
        float top __attribute__ ((aligned(4)));
        float right __attribute__ ((aligned(4)));
        float bottom __attribute__ ((aligned(4)));
    };

    static_assert(offsetof(::android::hardware::graphics::composer::V2_1::IComposerClient::FRect, left) == 0, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::composer::V2_1::IComposerClient::FRect, top) == 4, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::composer::V2_1::IComposerClient::FRect, right) == 8, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::composer::V2_1::IComposerClient::FRect, bottom) == 12, "wrong offset");
    static_assert(sizeof(::android::hardware::graphics::composer::V2_1::IComposerClient::FRect) == 16, "wrong size");
    static_assert(__alignof(::android::hardware::graphics::composer::V2_1::IComposerClient::FRect) == 4, "wrong alignment");

    struct Color final {
        uint8_t r __attribute__ ((aligned(1)));
        uint8_t g __attribute__ ((aligned(1)));
        uint8_t b __attribute__ ((aligned(1)));
        uint8_t a __attribute__ ((aligned(1)));
    };

    static_assert(offsetof(::android::hardware::graphics::composer::V2_1::IComposerClient::Color, r) == 0, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::composer::V2_1::IComposerClient::Color, g) == 1, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::composer::V2_1::IComposerClient::Color, b) == 2, "wrong offset");
    static_assert(offsetof(::android::hardware::graphics::composer::V2_1::IComposerClient::Color, a) == 3, "wrong offset");
    static_assert(sizeof(::android::hardware::graphics::composer::V2_1::IComposerClient::Color) == 4, "wrong size");
    static_assert(__alignof(::android::hardware::graphics::composer::V2_1::IComposerClient::Color) == 1, "wrong alignment");

    /**
     * SELECT_DISPLAY has this pseudo prototype
     * 
     *   selectDisplay(Display display);
     * 
     * Selects the current display implied by all other commands.
     * 
     * @param display is the newly selected display.
     * 
     * 
     * SELECT_LAYER has this pseudo prototype
     * 
     *   selectLayer(Layer layer);
     * 
     * Selects the current layer implied by all implicit layer commands.
     * 
     * @param layer is the newly selected layer.
     * 
     * 
     * SET_ERROR has this pseudo prototype
     * 
     *   setError(uint32_t location, Error error);
     * 
     * Indicates an error generated by a command.
     * 
     * @param location is the offset of the command in the input command
     *        message queue.
     * @param error is the error generated by the command.
     * 
     * 
     * SET_CHANGED_COMPOSITION_TYPES has this pseudo prototype
     * 
     *   setChangedCompositionTypes(vec<Layer> layers,
     *                              vec<Composition> types);
     * 
     * Sets the layers for which the device requires a different composition
     * type than had been set prior to the last call to VALIDATE_DISPLAY. The
     * client must either update its state with these types and call
     * ACCEPT_DISPLAY_CHANGES, or must set new types and attempt to validate
     * the display again.
     * 
     * @param layers is an array of layer handles.
     * @param types is an array of composition types, each corresponding to
     *         an element of layers.
     * 
     * 
     * SET_DISPLAY_REQUESTS has this pseudo prototype
     * 
     *   setDisplayRequests(uint32_t displayRequestMask,
     *                      vec<Layer> layers,
     *                      vec<uint32_t> layerRequestMasks);
     * 
     * Sets the display requests and the layer requests required for the last
     * validated configuration.
     * 
     * Display requests provide information about how the client must handle
     * the client target. Layer requests provide information about how the
     * client must handle an individual layer.
     * 
     * @param displayRequestMask is the display requests for the current
     *        validated state.
     * @param layers is an array of layers which all have at least one
     *        request.
     * @param layerRequestMasks is the requests corresponding to each element
     *        of layers.
     * 
     * 
     * SET_PRESENT_FENCE has this pseudo prototype
     * 
     *   setPresentFence(int32_t presentFenceIndex);
     * 
     * Sets the present fence as a result of PRESENT_DISPLAY. For physical
     * displays, this fence must be signaled at the vsync when the result
     * of composition of this frame starts to appear (for video-mode panels)
     * or starts to transfer to panel memory (for command-mode panels). For
     * virtual displays, this fence must be signaled when writes to the output
     * buffer have completed and it is safe to read from it.
     * 
     * @param presentFenceIndex is an index into outHandles array.
     * 
     * 
     * SET_RELEASE_FENCES has this pseudo prototype
     * 
     *   setReleaseFences(vec<Layer> layers,
     *                    vec<int32_t> releaseFenceIndices);
     * 
     * Sets the release fences for device layers on this display which will
     * receive new buffer contents this frame.
     * 
     * A release fence is a file descriptor referring to a sync fence object
     * which must be signaled after the device has finished reading from the
     * buffer presented in the prior frame. This indicates that it is safe to
     * start writing to the buffer again. If a given layer's fence is not
     * returned from this function, it must be assumed that the buffer
     * presented on the previous frame is ready to be written.
     * 
     * The fences returned by this function must be unique for each layer
     * (even if they point to the same underlying sync object).
     * 
     * @param layers is an array of layer handles.
     * @param releaseFenceIndices are indices into outHandles array, each
     *        corresponding to an element of layers.
     * 
     * 
     * SET_COLOR_TRANSFORM has this pseudo prototype
     * 
     *   setColorTransform(float[16] matrix,
     *                     ColorTransform hint);
     * 
     * Sets a color transform which will be applied after composition.
     * 
     * If hint is not ColorTransform::ARBITRARY, then the device may use the
     * hint to apply the desired color transform instead of using the color
     * matrix directly.
     * 
     * If the device is not capable of either using the hint or the matrix to
     * apply the desired color transform, it must force all layers to client
     * composition during VALIDATE_DISPLAY.
     * 
     * If IComposer::Capability::SKIP_CLIENT_COLOR_TRANSFORM is present, then
     * the client must never apply the color transform during client
     * composition, even if all layers are being composed by the client.
     * 
     * The matrix provided is an affine color transformation of the following
     * form:
     * 
     * |r.r r.g r.b 0|
     * |g.r g.g g.b 0|
     * |b.r b.g b.b 0|
     * |Tr  Tg  Tb  1|
     * 
     * This matrix must be provided in row-major form:
     * 
     * {r.r, r.g, r.b, 0, g.r, ...}.
     * 
     * Given a matrix of this form and an input color [R_in, G_in, B_in], the
     * output color [R_out, G_out, B_out] will be:
     * 
     * R_out = R_in * r.r + G_in * g.r + B_in * b.r + Tr
     * G_out = R_in * r.g + G_in * g.g + B_in * b.g + Tg
     * B_out = R_in * r.b + G_in * g.b + B_in * b.b + Tb
     * 
     * @param matrix is a 4x4 transform matrix (16 floats) as described above.
     * @param hint is a hint value which may be used instead of the given
     *        matrix unless it is ColorTransform::ARBITRARY.
     * 
     * 
     * SET_CLIENT_TARGET has this pseudo prototype
     * 
     *   setClientTarget(uint32_t targetSlot,
     *                   int32_t targetIndex,
     *                   int32_t acquireFenceIndex,
     *                   Dataspace dataspace,
     *                   vec<Rect> damage);
     * 
     * Sets the buffer handle which will receive the output of client
     * composition.  Layers marked as Composition::CLIENT must be composited
     * into this buffer prior to the call to PRESENT_DISPLAY, and layers not
     * marked as Composition::CLIENT must be composited with this buffer by
     * the device.
     * 
     * The buffer handle provided may be empty if no layers are being
     * composited by the client. This must not result in an error (unless an
     * invalid display handle is also provided).
     * 
     * Also provides a file descriptor referring to an acquire sync fence
     * object, which must be signaled when it is safe to read from the client
     * target buffer.  If it is already safe to read from this buffer, an
     * empty handle may be passed instead.
     * 
     * For more about dataspaces, see SET_LAYER_DATASPACE.
     * 
     * The damage parameter describes a surface damage region as defined in
     * the description of SET_LAYER_SURFACE_DAMAGE.
     * 
     * Will be called before PRESENT_DISPLAY if any of the layers are marked
     * as Composition::CLIENT. If no layers are so marked, then it is not
     * necessary to call this function. It is not necessary to call
     * validateDisplay after changing the target through this function.
     * 
     * @param targetSlot is the client target buffer slot to use.
     * @param targetIndex is an index into inHandles for the new target
     *        buffer.
     * @param acquireFenceIndex is an index into inHandles for a sync fence
     *        file descriptor as described above.
     * @param dataspace is the dataspace of the buffer, as described in
     *        setLayerDataspace.
     * @param damage is the surface damage region.
     * 
     * 
     * SET_OUTPUT_BUFFER has this pseudo prototype
     * 
     *   setOutputBuffer(uint32_t bufferSlot,
     *                   int32_t bufferIndex,
     *                   int32_t releaseFenceIndex);
     * 
     * Sets the output buffer for a virtual display. That is, the buffer to
     * which the composition result will be written.
     * 
     * Also provides a file descriptor referring to a release sync fence
     * object, which must be signaled when it is safe to write to the output
     * buffer. If it is already safe to write to the output buffer, an empty
     * handle may be passed instead.
     * 
     * Must be called at least once before PRESENT_DISPLAY, but does not have
     * any interaction with layer state or display validation.
     * 
     * @param bufferSlot is the new output buffer.
     * @param bufferIndex is the new output buffer.
     * @param releaseFenceIndex is a sync fence file descriptor as described
     *        above.
     * 
     * 
     * VALIDATE_DISPLAY has this pseudo prototype
     * 
     *   validateDisplay();
     * 
     * Instructs the device to inspect all of the layer state and determine if
     * there are any composition type changes necessary before presenting the
     * display. Permitted changes are described in the definition of
     * Composition above.
     * 
     * 
     * ACCEPT_DISPLAY_CHANGES has this pseudo prototype
     * 
     *   acceptDisplayChanges();
     * 
     * Accepts the changes required by the device from the previous
     * validateDisplay call (which may be queried using
     * getChangedCompositionTypes) and revalidates the display. This function
     * is equivalent to requesting the changed types from
     * getChangedCompositionTypes, setting those types on the corresponding
     * layers, and then calling validateDisplay again.
     * 
     * After this call it must be valid to present this display. Calling this
     * after validateDisplay returns 0 changes must succeed with NONE, but
     * must have no other effect.
     * 
     * 
     * PRESENT_DISPLAY has this pseudo prototype
     * 
     *   presentDisplay();
     * 
     * Presents the current display contents on the screen (or in the case of
     * virtual displays, into the output buffer).
     * 
     * Prior to calling this function, the display must be successfully
     * validated with validateDisplay. Note that setLayerBuffer and
     * setLayerSurfaceDamage specifically do not count as layer state, so if
     * there are no other changes to the layer state (or to the buffer's
     * properties as described in setLayerBuffer), then it is safe to call
     * this function without first validating the display.
     * 
     * 
     * SET_LAYER_CURSOR_POSITION has this pseudo prototype
     * 
     *   setLayerCursorPosition(int32_t x, int32_t y);
     * 
     * Asynchronously sets the position of a cursor layer.
     * 
     * Prior to validateDisplay, a layer may be marked as Composition::CURSOR.
     * If validation succeeds (i.e., the device does not request a composition
     * change for that layer), then once a buffer has been set for the layer
     * and it has been presented, its position may be set by this function at
     * any time between presentDisplay and any subsequent validateDisplay
     * calls for this display.
     * 
     * Once validateDisplay is called, this function must not be called again
     * until the validate/present sequence is completed.
     * 
     * May be called from any thread so long as it is not interleaved with the
     * validate/present sequence as described above.
     * 
     * @param layer is the layer to which the position is set.
     * @param x is the new x coordinate (in pixels from the left of the
     *        screen).
     * @param y is the new y coordinate (in pixels from the top of the
     *        screen).
     * 
     * 
     * SET_LAYER_BUFFER has this pseudo prototype
     * 
     *   setLayerBuffer(uint32_t bufferSlot,
     *                  int32_t bufferIndex,
     *                  int32_t acquireFenceIndex);
     * 
     * Sets the buffer handle to be displayed for this layer. If the buffer
     * properties set at allocation time (width, height, format, and usage)
     * have not changed since the previous frame, it is not necessary to call
     * validateDisplay before calling presentDisplay unless new state needs to
     * be validated in the interim.
     * 
     * Also provides a file descriptor referring to an acquire sync fence
     * object, which must be signaled when it is safe to read from the given
     * buffer. If it is already safe to read from the buffer, an empty handle
     * may be passed instead.
     * 
     * This function must return NONE and have no other effect if called for a
     * layer with a composition type of Composition::SOLID_COLOR (because it
     * has no buffer) or Composition::SIDEBAND or Composition::CLIENT (because
     * synchronization and buffer updates for these layers are handled
     * elsewhere).
     * 
     * @param layer is the layer to which the buffer is set.
     * @param bufferSlot is the buffer slot to use.
     * @param bufferIndex is the buffer handle to set.
     * @param acquireFenceIndex is a sync fence file descriptor as described above.
     * 
     * 
     * SET_LAYER_SURFACE_DAMAGE has this pseudo prototype
     * 
     *   setLayerSurfaceDamage(vec<Rect> damage);
     * 
     * Provides the region of the source buffer which has been modified since
     * the last frame. This region does not need to be validated before
     * calling presentDisplay.
     * 
     * Once set through this function, the damage region remains the same
     * until a subsequent call to this function.
     * 
     * If damage is non-empty, then it may be assumed that any portion of the
     * source buffer not covered by one of the rects has not been modified
     * this frame. If damage is empty, then the whole source buffer must be
     * treated as if it has been modified.
     * 
     * If the layer's contents are not modified relative to the prior frame,
     * damage must contain exactly one empty rect([0, 0, 0, 0]).
     * 
     * The damage rects are relative to the pre-transformed buffer, and their
     * origin is the top-left corner. They must not exceed the dimensions of
     * the latched buffer.
     * 
     * @param layer is the layer to which the damage region is set.
     * @param damage is the new surface damage region.
     * 
     * 
     * SET_LAYER_BLEND_MODE has this pseudo prototype
     * 
     *   setLayerBlendMode(BlendMode mode)
     * 
     * Sets the blend mode of the given layer.
     * 
     * @param mode is the new blend mode.
     * 
     * 
     * SET_LAYER_COLOR has this pseudo prototype
     * 
     *   setLayerColor(Color color);
     * 
     * Sets the color of the given layer. If the composition type of the layer
     * is not Composition::SOLID_COLOR, this call must succeed and have no
     * other effect.
     * 
     * @param color is the new color.
     * 
     * 
     * SET_LAYER_COMPOSITION_TYPE has this pseudo prototype
     * 
     *   setLayerCompositionType(Composition type);
     * 
     * Sets the desired composition type of the given layer. During
     * validateDisplay, the device may request changes to the composition
     * types of any of the layers as described in the definition of
     * Composition above.
     * 
     * @param type is the new composition type.
     * 
     * 
     * SET_LAYER_DATASPACE has this pseudo prototype
     * 
     *   setLayerDataspace(Dataspace dataspace);
     * 
     * Sets the dataspace of the layer.
     * 
     * The dataspace provides more information about how to interpret the buffer
     * or solid color, such as the encoding standard and color transform.
     * 
     * See the values of Dataspace for more information.
     * 
     * @param dataspace is the new dataspace.
     * 
     * 
     * SET_LAYER_DISPLAY_FRAME has this pseudo prototype
     * 
     *   setLayerDisplayFrame(Rect frame);
     * 
     * Sets the display frame (the portion of the display covered by a layer)
     * of the given layer. This frame must not exceed the display dimensions.
     * 
     * @param frame is the new display frame.
     * 
     * 
     * SET_LAYER_PLANE_ALPHA has this pseudo prototype
     * 
     *   setLayerPlaneAlpha(float alpha);
     * 
     * Sets an alpha value (a floating point value in the range [0.0, 1.0])
     * which will be applied to the whole layer. It can be conceptualized as a
     * preprocessing step which applies the following function:
     *   if (blendMode == BlendMode::PREMULTIPLIED)
     *       out.rgb = in.rgb * planeAlpha
     *   out.a = in.a * planeAlpha
     * 
     * If the device does not support this operation on a layer which is
     * marked Composition::DEVICE, it must request a composition type change
     * to Composition::CLIENT upon the next validateDisplay call.
     * 
     * @param alpha is the plane alpha value to apply.
     * 
     * 
     * SET_LAYER_SIDEBAND_STREAM has this pseudo prototype
     * 
     *   setLayerSidebandStream(int32_t streamIndex)
     * 
     * Sets the sideband stream for this layer. If the composition type of the
     * given layer is not Composition::SIDEBAND, this call must succeed and
     * have no other effect.
     * 
     * @param streamIndex is the new sideband stream.
     * 
     * 
     * SET_LAYER_SOURCE_CROP has this pseudo prototype
     * 
     *   setLayerSourceCrop(FRect crop);
     * 
     * Sets the source crop (the portion of the source buffer which will fill
     * the display frame) of the given layer. This crop rectangle must not
     * exceed the dimensions of the latched buffer.
     * 
     * If the device is not capable of supporting a true float source crop
     * (i.e., it will truncate or round the floats to integers), it must set
     * this layer to Composition::CLIENT when crop is non-integral for the
     * most accurate rendering.
     * 
     * If the device cannot support float source crops, but still wants to
     * handle the layer, it must use the following code (or similar) to
     * convert to an integer crop:
     *   intCrop.left = (int) ceilf(crop.left);
     *   intCrop.top = (int) ceilf(crop.top);
     *   intCrop.right = (int) floorf(crop.right);
     *   intCrop.bottom = (int) floorf(crop.bottom);
     * 
     * @param crop is the new source crop.
     * 
     * 
     * SET_LAYER_TRANSFORM has this pseudo prototype
     * 
     * Sets the transform (rotation/flip) of the given layer.
     * 
     *   setLayerTransform(Transform transform);
     * 
     * @param transform is the new transform.
     * 
     * 
     * SET_LAYER_VISIBLE_REGION has this pseudo prototype
     * 
     *   setLayerVisibleRegion(vec<Rect> visible);
     * 
     * Specifies the portion of the layer that is visible, including portions
     * under translucent areas of other layers. The region is in screen space,
     * and must not exceed the dimensions of the screen.
     * 
     * @param visible is the new visible region, in screen space.
     * 
     * 
     * SET_LAYER_Z_ORDER has this pseudo prototype
     * 
     *   setLayerZOrder(uint32_t z);
     * 
     * Sets the desired Z order (height) of the given layer. A layer with a
     * greater Z value occludes a layer with a lesser Z value.
     * 
     * @param z is the new Z order.
     */
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
    static ::android::hardware::Return<::android::sp<::android::hardware::graphics::composer::V2_1::IComposerClient>> castFrom(const ::android::sp<::android::hardware::graphics::composer::V2_1::IComposerClient>& parent, bool emitError = false);
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::android::hardware::graphics::composer::V2_1::IComposerClient>> castFrom(const ::android::sp<::android::hidl::base::V1_0::IBase>& parent, bool emitError = false);

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
static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute o);

constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const int32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute rhs) {
    return static_cast<int32_t>(lhs | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | rhs);
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const int32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute rhs) {
    return static_cast<int32_t>(lhs & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & rhs);
}
constexpr int32_t &operator|=(int32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute e) {
    v |= static_cast<int32_t>(e);
    return v;
}
constexpr int32_t &operator&=(int32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute e) {
    v &= static_cast<int32_t>(e);
    return v;
}

template<typename>
static inline std::string toString(uint32_t o);
static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest o);

constexpr uint32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest rhs) {
    return static_cast<uint32_t>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}
constexpr uint32_t operator|(const uint32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest rhs) {
    return static_cast<uint32_t>(lhs | static_cast<uint32_t>(rhs));
}
constexpr uint32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest lhs, const uint32_t rhs) {
    return static_cast<uint32_t>(static_cast<uint32_t>(lhs) | rhs);
}
constexpr uint32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest rhs) {
    return static_cast<uint32_t>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}
constexpr uint32_t operator&(const uint32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest rhs) {
    return static_cast<uint32_t>(lhs & static_cast<uint32_t>(rhs));
}
constexpr uint32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest lhs, const uint32_t rhs) {
    return static_cast<uint32_t>(static_cast<uint32_t>(lhs) & rhs);
}
constexpr uint32_t &operator|=(uint32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest e) {
    v |= static_cast<uint32_t>(e);
    return v;
}
constexpr uint32_t &operator&=(uint32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest e) {
    v &= static_cast<uint32_t>(e);
    return v;
}

template<typename>
static inline std::string toString(uint32_t o);
static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest o);

constexpr uint32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest rhs) {
    return static_cast<uint32_t>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}
constexpr uint32_t operator|(const uint32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest rhs) {
    return static_cast<uint32_t>(lhs | static_cast<uint32_t>(rhs));
}
constexpr uint32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest lhs, const uint32_t rhs) {
    return static_cast<uint32_t>(static_cast<uint32_t>(lhs) | rhs);
}
constexpr uint32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest rhs) {
    return static_cast<uint32_t>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}
constexpr uint32_t operator&(const uint32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest rhs) {
    return static_cast<uint32_t>(lhs & static_cast<uint32_t>(rhs));
}
constexpr uint32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest lhs, const uint32_t rhs) {
    return static_cast<uint32_t>(static_cast<uint32_t>(lhs) & rhs);
}
constexpr uint32_t &operator|=(uint32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest e) {
    v |= static_cast<uint32_t>(e);
    return v;
}
constexpr uint32_t &operator&=(uint32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest e) {
    v &= static_cast<uint32_t>(e);
    return v;
}

template<typename>
static inline std::string toString(int32_t o);
static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode o);

constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const int32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode rhs) {
    return static_cast<int32_t>(lhs | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | rhs);
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const int32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode rhs) {
    return static_cast<int32_t>(lhs & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & rhs);
}
constexpr int32_t &operator|=(int32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode e) {
    v |= static_cast<int32_t>(e);
    return v;
}
constexpr int32_t &operator&=(int32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode e) {
    v &= static_cast<int32_t>(e);
    return v;
}

template<typename>
static inline std::string toString(int32_t o);
static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync o);

constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const int32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync rhs) {
    return static_cast<int32_t>(lhs | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | rhs);
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const int32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync rhs) {
    return static_cast<int32_t>(lhs & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & rhs);
}
constexpr int32_t &operator|=(int32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync e) {
    v |= static_cast<int32_t>(e);
    return v;
}
constexpr int32_t &operator&=(int32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync e) {
    v &= static_cast<int32_t>(e);
    return v;
}

template<typename>
static inline std::string toString(int32_t o);
static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode o);

constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const int32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode rhs) {
    return static_cast<int32_t>(lhs | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | rhs);
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const int32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode rhs) {
    return static_cast<int32_t>(lhs & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & rhs);
}
constexpr int32_t &operator|=(int32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode e) {
    v |= static_cast<int32_t>(e);
    return v;
}
constexpr int32_t &operator&=(int32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode e) {
    v &= static_cast<int32_t>(e);
    return v;
}

template<typename>
static inline std::string toString(int32_t o);
static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::Composition o);

constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const int32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition rhs) {
    return static_cast<int32_t>(lhs | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | rhs);
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const int32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition rhs) {
    return static_cast<int32_t>(lhs & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & rhs);
}
constexpr int32_t &operator|=(int32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition e) {
    v |= static_cast<int32_t>(e);
    return v;
}
constexpr int32_t &operator&=(int32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition e) {
    v &= static_cast<int32_t>(e);
    return v;
}

template<typename>
static inline std::string toString(int32_t o);
static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType o);

constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const int32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType rhs) {
    return static_cast<int32_t>(lhs | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | rhs);
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const int32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType rhs) {
    return static_cast<int32_t>(lhs & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & rhs);
}
constexpr int32_t &operator|=(int32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType e) {
    v |= static_cast<int32_t>(e);
    return v;
}
constexpr int32_t &operator&=(int32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType e) {
    v &= static_cast<int32_t>(e);
    return v;
}

template<typename>
static inline std::string toString(int32_t o);
static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex o);

constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const int32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex rhs) {
    return static_cast<int32_t>(lhs | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | rhs);
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const int32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex rhs) {
    return static_cast<int32_t>(lhs & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & rhs);
}
constexpr int32_t &operator|=(int32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex e) {
    v |= static_cast<int32_t>(e);
    return v;
}
constexpr int32_t &operator&=(int32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex e) {
    v &= static_cast<int32_t>(e);
    return v;
}

static inline std::string toString(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Rect& o);
static inline bool operator==(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Rect& lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Rect& rhs);
static inline bool operator!=(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Rect& lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Rect& rhs);

static inline std::string toString(const ::android::hardware::graphics::composer::V2_1::IComposerClient::FRect& o);
static inline bool operator==(const ::android::hardware::graphics::composer::V2_1::IComposerClient::FRect& lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::FRect& rhs);
static inline bool operator!=(const ::android::hardware::graphics::composer::V2_1::IComposerClient::FRect& lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::FRect& rhs);

static inline std::string toString(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Color& o);
static inline bool operator==(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Color& lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Color& rhs);
static inline bool operator!=(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Color& lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Color& rhs);

template<typename>
static inline std::string toString(int32_t o);
static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::Command o);

constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Command lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Command rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const int32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Command rhs) {
    return static_cast<int32_t>(lhs | static_cast<int32_t>(rhs));
}
constexpr int32_t operator|(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Command lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) | rhs);
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Command lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Command rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const int32_t lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Command rhs) {
    return static_cast<int32_t>(lhs & static_cast<int32_t>(rhs));
}
constexpr int32_t operator&(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Command lhs, const int32_t rhs) {
    return static_cast<int32_t>(static_cast<int32_t>(lhs) & rhs);
}
constexpr int32_t &operator|=(int32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Command e) {
    v |= static_cast<int32_t>(e);
    return v;
}
constexpr int32_t &operator&=(int32_t& v, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Command e) {
    v &= static_cast<int32_t>(e);
    return v;
}

static inline std::string toString(const ::android::sp<::android::hardware::graphics::composer::V2_1::IComposerClient>& o);

//
// type header definitions for package
//

template<>
inline std::string toString<::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute>(int32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::INVALID) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::INVALID)) {
        os += (first ? "" : " | ");
        os += "INVALID";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::INVALID;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::WIDTH) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::WIDTH)) {
        os += (first ? "" : " | ");
        os += "WIDTH";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::WIDTH;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::HEIGHT) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::HEIGHT)) {
        os += (first ? "" : " | ");
        os += "HEIGHT";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::HEIGHT;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::VSYNC_PERIOD) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::VSYNC_PERIOD)) {
        os += (first ? "" : " | ");
        os += "VSYNC_PERIOD";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::VSYNC_PERIOD;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::DPI_X) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::DPI_X)) {
        os += (first ? "" : " | ");
        os += "DPI_X";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::DPI_X;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::DPI_Y) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::DPI_Y)) {
        os += (first ? "" : " | ");
        os += "DPI_Y";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::DPI_Y;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::INVALID) {
        return "INVALID";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::WIDTH) {
        return "WIDTH";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::HEIGHT) {
        return "HEIGHT";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::VSYNC_PERIOD) {
        return "VSYNC_PERIOD";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::DPI_X) {
        return "DPI_X";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::DPI_Y) {
        return "DPI_Y";
    }
    std::string os;
    os += toHexString(static_cast<int32_t>(o));
    return os;
}

template<>
inline std::string toString<::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest>(uint32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest::FLIP_CLIENT_TARGET) == static_cast<uint32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest::FLIP_CLIENT_TARGET)) {
        os += (first ? "" : " | ");
        os += "FLIP_CLIENT_TARGET";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest::FLIP_CLIENT_TARGET;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest::WRITE_CLIENT_TARGET_TO_OUTPUT) == static_cast<uint32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest::WRITE_CLIENT_TARGET_TO_OUTPUT)) {
        os += (first ? "" : " | ");
        os += "WRITE_CLIENT_TARGET_TO_OUTPUT";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest::WRITE_CLIENT_TARGET_TO_OUTPUT;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest::FLIP_CLIENT_TARGET) {
        return "FLIP_CLIENT_TARGET";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest::WRITE_CLIENT_TARGET_TO_OUTPUT) {
        return "WRITE_CLIENT_TARGET_TO_OUTPUT";
    }
    std::string os;
    os += toHexString(static_cast<uint32_t>(o));
    return os;
}

template<>
inline std::string toString<::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest>(uint32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest::CLEAR_CLIENT_TARGET) == static_cast<uint32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest::CLEAR_CLIENT_TARGET)) {
        os += (first ? "" : " | ");
        os += "CLEAR_CLIENT_TARGET";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest::CLEAR_CLIENT_TARGET;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest::CLEAR_CLIENT_TARGET) {
        return "CLEAR_CLIENT_TARGET";
    }
    std::string os;
    os += toHexString(static_cast<uint32_t>(o));
    return os;
}

template<>
inline std::string toString<::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode>(int32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::OFF) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::OFF)) {
        os += (first ? "" : " | ");
        os += "OFF";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::OFF;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::DOZE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::DOZE)) {
        os += (first ? "" : " | ");
        os += "DOZE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::DOZE;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::DOZE_SUSPEND) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::DOZE_SUSPEND)) {
        os += (first ? "" : " | ");
        os += "DOZE_SUSPEND";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::DOZE_SUSPEND;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::ON) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::ON)) {
        os += (first ? "" : " | ");
        os += "ON";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::ON;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::OFF) {
        return "OFF";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::DOZE) {
        return "DOZE";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::DOZE_SUSPEND) {
        return "DOZE_SUSPEND";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::ON) {
        return "ON";
    }
    std::string os;
    os += toHexString(static_cast<int32_t>(o));
    return os;
}

template<>
inline std::string toString<::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync>(int32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync::INVALID) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync::INVALID)) {
        os += (first ? "" : " | ");
        os += "INVALID";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync::INVALID;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync::ENABLE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync::ENABLE)) {
        os += (first ? "" : " | ");
        os += "ENABLE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync::ENABLE;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync::DISABLE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync::DISABLE)) {
        os += (first ? "" : " | ");
        os += "DISABLE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync::DISABLE;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync::INVALID) {
        return "INVALID";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync::ENABLE) {
        return "ENABLE";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync::DISABLE) {
        return "DISABLE";
    }
    std::string os;
    os += toHexString(static_cast<int32_t>(o));
    return os;
}

template<>
inline std::string toString<::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode>(int32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::INVALID) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::INVALID)) {
        os += (first ? "" : " | ");
        os += "INVALID";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::INVALID;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::NONE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::NONE)) {
        os += (first ? "" : " | ");
        os += "NONE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::NONE;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::PREMULTIPLIED) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::PREMULTIPLIED)) {
        os += (first ? "" : " | ");
        os += "PREMULTIPLIED";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::PREMULTIPLIED;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::COVERAGE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::COVERAGE)) {
        os += (first ? "" : " | ");
        os += "COVERAGE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::COVERAGE;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::INVALID) {
        return "INVALID";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::NONE) {
        return "NONE";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::PREMULTIPLIED) {
        return "PREMULTIPLIED";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::COVERAGE) {
        return "COVERAGE";
    }
    std::string os;
    os += toHexString(static_cast<int32_t>(o));
    return os;
}

template<>
inline std::string toString<::android::hardware::graphics::composer::V2_1::IComposerClient::Composition>(int32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::graphics::composer::V2_1::IComposerClient::Composition> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::INVALID) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::INVALID)) {
        os += (first ? "" : " | ");
        os += "INVALID";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::INVALID;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::CLIENT) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::CLIENT)) {
        os += (first ? "" : " | ");
        os += "CLIENT";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::CLIENT;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::DEVICE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::DEVICE)) {
        os += (first ? "" : " | ");
        os += "DEVICE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::DEVICE;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::SOLID_COLOR) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::SOLID_COLOR)) {
        os += (first ? "" : " | ");
        os += "SOLID_COLOR";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::SOLID_COLOR;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::CURSOR) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::CURSOR)) {
        os += (first ? "" : " | ");
        os += "CURSOR";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::CURSOR;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::SIDEBAND) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::SIDEBAND)) {
        os += (first ? "" : " | ");
        os += "SIDEBAND";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::SIDEBAND;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::Composition o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::INVALID) {
        return "INVALID";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::CLIENT) {
        return "CLIENT";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::DEVICE) {
        return "DEVICE";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::SOLID_COLOR) {
        return "SOLID_COLOR";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::CURSOR) {
        return "CURSOR";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::SIDEBAND) {
        return "SIDEBAND";
    }
    std::string os;
    os += toHexString(static_cast<int32_t>(o));
    return os;
}

template<>
inline std::string toString<::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType>(int32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType::INVALID) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType::INVALID)) {
        os += (first ? "" : " | ");
        os += "INVALID";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType::INVALID;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType::PHYSICAL) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType::PHYSICAL)) {
        os += (first ? "" : " | ");
        os += "PHYSICAL";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType::PHYSICAL;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType::VIRTUAL) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType::VIRTUAL)) {
        os += (first ? "" : " | ");
        os += "VIRTUAL";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType::VIRTUAL;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType::INVALID) {
        return "INVALID";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType::PHYSICAL) {
        return "PHYSICAL";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType::VIRTUAL) {
        return "VIRTUAL";
    }
    std::string os;
    os += toHexString(static_cast<int32_t>(o));
    return os;
}

template<>
inline std::string toString<::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex>(int32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex::EMPTY) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex::EMPTY)) {
        os += (first ? "" : " | ");
        os += "EMPTY";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex::EMPTY;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex::CACHED) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex::CACHED)) {
        os += (first ? "" : " | ");
        os += "CACHED";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex::CACHED;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex::EMPTY) {
        return "EMPTY";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex::CACHED) {
        return "CACHED";
    }
    std::string os;
    os += toHexString(static_cast<int32_t>(o));
    return os;
}

static inline std::string toString(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Rect& o) {
    using ::android::hardware::toString;
    std::string os;
    os += "{";
    os += ".left = ";
    os += ::android::hardware::toString(o.left);
    os += ", .top = ";
    os += ::android::hardware::toString(o.top);
    os += ", .right = ";
    os += ::android::hardware::toString(o.right);
    os += ", .bottom = ";
    os += ::android::hardware::toString(o.bottom);
    os += "}"; return os;
}

static inline bool operator==(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Rect& lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Rect& rhs) {
    if (lhs.left != rhs.left) {
        return false;
    }
    if (lhs.top != rhs.top) {
        return false;
    }
    if (lhs.right != rhs.right) {
        return false;
    }
    if (lhs.bottom != rhs.bottom) {
        return false;
    }
    return true;
}

static inline bool operator!=(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Rect& lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Rect& rhs){
    return !(lhs == rhs);
}

static inline std::string toString(const ::android::hardware::graphics::composer::V2_1::IComposerClient::FRect& o) {
    using ::android::hardware::toString;
    std::string os;
    os += "{";
    os += ".left = ";
    os += ::android::hardware::toString(o.left);
    os += ", .top = ";
    os += ::android::hardware::toString(o.top);
    os += ", .right = ";
    os += ::android::hardware::toString(o.right);
    os += ", .bottom = ";
    os += ::android::hardware::toString(o.bottom);
    os += "}"; return os;
}

static inline bool operator==(const ::android::hardware::graphics::composer::V2_1::IComposerClient::FRect& lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::FRect& rhs) {
    if (lhs.left != rhs.left) {
        return false;
    }
    if (lhs.top != rhs.top) {
        return false;
    }
    if (lhs.right != rhs.right) {
        return false;
    }
    if (lhs.bottom != rhs.bottom) {
        return false;
    }
    return true;
}

static inline bool operator!=(const ::android::hardware::graphics::composer::V2_1::IComposerClient::FRect& lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::FRect& rhs){
    return !(lhs == rhs);
}

static inline std::string toString(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Color& o) {
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

static inline bool operator==(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Color& lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Color& rhs) {
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

static inline bool operator!=(const ::android::hardware::graphics::composer::V2_1::IComposerClient::Color& lhs, const ::android::hardware::graphics::composer::V2_1::IComposerClient::Color& rhs){
    return !(lhs == rhs);
}

template<>
inline std::string toString<::android::hardware::graphics::composer::V2_1::IComposerClient::Command>(int32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::android::hardware::graphics::composer::V2_1::IComposerClient::Command> flipped = 0;
    bool first = true;
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::LENGTH_MASK) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::LENGTH_MASK)) {
        os += (first ? "" : " | ");
        os += "LENGTH_MASK";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::LENGTH_MASK;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::OPCODE_SHIFT) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::OPCODE_SHIFT)) {
        os += (first ? "" : " | ");
        os += "OPCODE_SHIFT";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::OPCODE_SHIFT;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::OPCODE_MASK) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::OPCODE_MASK)) {
        os += (first ? "" : " | ");
        os += "OPCODE_MASK";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::OPCODE_MASK;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SELECT_DISPLAY) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SELECT_DISPLAY)) {
        os += (first ? "" : " | ");
        os += "SELECT_DISPLAY";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SELECT_DISPLAY;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SELECT_LAYER) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SELECT_LAYER)) {
        os += (first ? "" : " | ");
        os += "SELECT_LAYER";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SELECT_LAYER;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_ERROR) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_ERROR)) {
        os += (first ? "" : " | ");
        os += "SET_ERROR";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_ERROR;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_CHANGED_COMPOSITION_TYPES) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_CHANGED_COMPOSITION_TYPES)) {
        os += (first ? "" : " | ");
        os += "SET_CHANGED_COMPOSITION_TYPES";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_CHANGED_COMPOSITION_TYPES;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_DISPLAY_REQUESTS) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_DISPLAY_REQUESTS)) {
        os += (first ? "" : " | ");
        os += "SET_DISPLAY_REQUESTS";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_DISPLAY_REQUESTS;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_PRESENT_FENCE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_PRESENT_FENCE)) {
        os += (first ? "" : " | ");
        os += "SET_PRESENT_FENCE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_PRESENT_FENCE;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_RELEASE_FENCES) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_RELEASE_FENCES)) {
        os += (first ? "" : " | ");
        os += "SET_RELEASE_FENCES";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_RELEASE_FENCES;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_COLOR_TRANSFORM) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_COLOR_TRANSFORM)) {
        os += (first ? "" : " | ");
        os += "SET_COLOR_TRANSFORM";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_COLOR_TRANSFORM;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_CLIENT_TARGET) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_CLIENT_TARGET)) {
        os += (first ? "" : " | ");
        os += "SET_CLIENT_TARGET";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_CLIENT_TARGET;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_OUTPUT_BUFFER) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_OUTPUT_BUFFER)) {
        os += (first ? "" : " | ");
        os += "SET_OUTPUT_BUFFER";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_OUTPUT_BUFFER;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::VALIDATE_DISPLAY) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::VALIDATE_DISPLAY)) {
        os += (first ? "" : " | ");
        os += "VALIDATE_DISPLAY";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::VALIDATE_DISPLAY;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::ACCEPT_DISPLAY_CHANGES) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::ACCEPT_DISPLAY_CHANGES)) {
        os += (first ? "" : " | ");
        os += "ACCEPT_DISPLAY_CHANGES";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::ACCEPT_DISPLAY_CHANGES;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::PRESENT_DISPLAY) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::PRESENT_DISPLAY)) {
        os += (first ? "" : " | ");
        os += "PRESENT_DISPLAY";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::PRESENT_DISPLAY;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::PRESENT_OR_VALIDATE_DISPLAY) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::PRESENT_OR_VALIDATE_DISPLAY)) {
        os += (first ? "" : " | ");
        os += "PRESENT_OR_VALIDATE_DISPLAY";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::PRESENT_OR_VALIDATE_DISPLAY;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_CURSOR_POSITION) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_CURSOR_POSITION)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_CURSOR_POSITION";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_CURSOR_POSITION;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_BUFFER) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_BUFFER)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_BUFFER";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_BUFFER;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_SURFACE_DAMAGE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_SURFACE_DAMAGE)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_SURFACE_DAMAGE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_SURFACE_DAMAGE;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_BLEND_MODE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_BLEND_MODE)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_BLEND_MODE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_BLEND_MODE;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_COLOR) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_COLOR)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_COLOR";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_COLOR;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_COMPOSITION_TYPE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_COMPOSITION_TYPE)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_COMPOSITION_TYPE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_COMPOSITION_TYPE;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_DATASPACE) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_DATASPACE)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_DATASPACE";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_DATASPACE;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_DISPLAY_FRAME) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_DISPLAY_FRAME)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_DISPLAY_FRAME";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_DISPLAY_FRAME;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_PLANE_ALPHA) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_PLANE_ALPHA)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_PLANE_ALPHA";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_PLANE_ALPHA;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_SIDEBAND_STREAM) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_SIDEBAND_STREAM)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_SIDEBAND_STREAM";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_SIDEBAND_STREAM;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_SOURCE_CROP) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_SOURCE_CROP)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_SOURCE_CROP";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_SOURCE_CROP;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_TRANSFORM) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_TRANSFORM)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_TRANSFORM";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_TRANSFORM;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_VISIBLE_REGION) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_VISIBLE_REGION)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_VISIBLE_REGION";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_VISIBLE_REGION;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_Z_ORDER) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_Z_ORDER)) {
        os += (first ? "" : " | ");
        os += "SET_LAYER_Z_ORDER";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_Z_ORDER;
    }
    if ((o & ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT) == static_cast<int32_t>(::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT)) {
        os += (first ? "" : " | ");
        os += "SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT";
        first = false;
        flipped |= ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::android::hardware::graphics::composer::V2_1::IComposerClient::Command o) {
    using ::android::hardware::details::toHexString;
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::LENGTH_MASK) {
        return "LENGTH_MASK";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::OPCODE_SHIFT) {
        return "OPCODE_SHIFT";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::OPCODE_MASK) {
        return "OPCODE_MASK";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SELECT_DISPLAY) {
        return "SELECT_DISPLAY";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SELECT_LAYER) {
        return "SELECT_LAYER";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_ERROR) {
        return "SET_ERROR";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_CHANGED_COMPOSITION_TYPES) {
        return "SET_CHANGED_COMPOSITION_TYPES";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_DISPLAY_REQUESTS) {
        return "SET_DISPLAY_REQUESTS";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_PRESENT_FENCE) {
        return "SET_PRESENT_FENCE";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_RELEASE_FENCES) {
        return "SET_RELEASE_FENCES";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_COLOR_TRANSFORM) {
        return "SET_COLOR_TRANSFORM";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_CLIENT_TARGET) {
        return "SET_CLIENT_TARGET";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_OUTPUT_BUFFER) {
        return "SET_OUTPUT_BUFFER";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::VALIDATE_DISPLAY) {
        return "VALIDATE_DISPLAY";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::ACCEPT_DISPLAY_CHANGES) {
        return "ACCEPT_DISPLAY_CHANGES";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::PRESENT_DISPLAY) {
        return "PRESENT_DISPLAY";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::PRESENT_OR_VALIDATE_DISPLAY) {
        return "PRESENT_OR_VALIDATE_DISPLAY";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_CURSOR_POSITION) {
        return "SET_LAYER_CURSOR_POSITION";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_BUFFER) {
        return "SET_LAYER_BUFFER";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_SURFACE_DAMAGE) {
        return "SET_LAYER_SURFACE_DAMAGE";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_BLEND_MODE) {
        return "SET_LAYER_BLEND_MODE";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_COLOR) {
        return "SET_LAYER_COLOR";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_COMPOSITION_TYPE) {
        return "SET_LAYER_COMPOSITION_TYPE";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_DATASPACE) {
        return "SET_LAYER_DATASPACE";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_DISPLAY_FRAME) {
        return "SET_LAYER_DISPLAY_FRAME";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_PLANE_ALPHA) {
        return "SET_LAYER_PLANE_ALPHA";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_SIDEBAND_STREAM) {
        return "SET_LAYER_SIDEBAND_STREAM";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_SOURCE_CROP) {
        return "SET_LAYER_SOURCE_CROP";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_TRANSFORM) {
        return "SET_LAYER_TRANSFORM";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_VISIBLE_REGION) {
        return "SET_LAYER_VISIBLE_REGION";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_Z_ORDER) {
        return "SET_LAYER_Z_ORDER";
    }
    if (o == ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT) {
        return "SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT";
    }
    std::string os;
    os += toHexString(static_cast<int32_t>(o));
    return os;
}

static inline std::string toString(const ::android::sp<::android::hardware::graphics::composer::V2_1::IComposerClient>& o) {
    std::string os = "[class or subclass of ";
    os += ::android::hardware::graphics::composer::V2_1::IComposerClient::descriptor;
    os += "]";
    os += o->isRemote() ? "@remote" : "@local";
    return os;
}


}  // namespace V2_1
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
template<> constexpr std::array<::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute, 6> hidl_enum_values<::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute> = {
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::INVALID,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::WIDTH,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::HEIGHT,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::VSYNC_PERIOD,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::DPI_X,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Attribute::DPI_Y,
};
}  // namespace details
}  // namespace hardware
}  // namespace android

namespace android {
namespace hardware {
namespace details {
template<> constexpr std::array<::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest, 2> hidl_enum_values<::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest> = {
    ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest::FLIP_CLIENT_TARGET,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayRequest::WRITE_CLIENT_TARGET_TO_OUTPUT,
};
}  // namespace details
}  // namespace hardware
}  // namespace android

namespace android {
namespace hardware {
namespace details {
template<> constexpr std::array<::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest, 1> hidl_enum_values<::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest> = {
    ::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest::CLEAR_CLIENT_TARGET,
};
}  // namespace details
}  // namespace hardware
}  // namespace android

namespace android {
namespace hardware {
namespace details {
template<> constexpr std::array<::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode, 4> hidl_enum_values<::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode> = {
    ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::OFF,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::DOZE,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::DOZE_SUSPEND,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::PowerMode::ON,
};
}  // namespace details
}  // namespace hardware
}  // namespace android

namespace android {
namespace hardware {
namespace details {
template<> constexpr std::array<::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync, 3> hidl_enum_values<::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync> = {
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync::INVALID,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync::ENABLE,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Vsync::DISABLE,
};
}  // namespace details
}  // namespace hardware
}  // namespace android

namespace android {
namespace hardware {
namespace details {
template<> constexpr std::array<::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode, 4> hidl_enum_values<::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode> = {
    ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::INVALID,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::NONE,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::PREMULTIPLIED,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::COVERAGE,
};
}  // namespace details
}  // namespace hardware
}  // namespace android

namespace android {
namespace hardware {
namespace details {
template<> constexpr std::array<::android::hardware::graphics::composer::V2_1::IComposerClient::Composition, 6> hidl_enum_values<::android::hardware::graphics::composer::V2_1::IComposerClient::Composition> = {
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::INVALID,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::CLIENT,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::DEVICE,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::SOLID_COLOR,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::CURSOR,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::SIDEBAND,
};
}  // namespace details
}  // namespace hardware
}  // namespace android

namespace android {
namespace hardware {
namespace details {
template<> constexpr std::array<::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType, 3> hidl_enum_values<::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType> = {
    ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType::INVALID,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType::PHYSICAL,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::DisplayType::VIRTUAL,
};
}  // namespace details
}  // namespace hardware
}  // namespace android

namespace android {
namespace hardware {
namespace details {
template<> constexpr std::array<::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex, 2> hidl_enum_values<::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex> = {
    ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex::EMPTY,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex::CACHED,
};
}  // namespace details
}  // namespace hardware
}  // namespace android

namespace android {
namespace hardware {
namespace details {
template<> constexpr std::array<::android::hardware::graphics::composer::V2_1::IComposerClient::Command, 32> hidl_enum_values<::android::hardware::graphics::composer::V2_1::IComposerClient::Command> = {
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::LENGTH_MASK,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::OPCODE_SHIFT,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::OPCODE_MASK,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SELECT_DISPLAY,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SELECT_LAYER,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_ERROR,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_CHANGED_COMPOSITION_TYPES,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_DISPLAY_REQUESTS,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_PRESENT_FENCE,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_RELEASE_FENCES,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_COLOR_TRANSFORM,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_CLIENT_TARGET,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_OUTPUT_BUFFER,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::VALIDATE_DISPLAY,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::ACCEPT_DISPLAY_CHANGES,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::PRESENT_DISPLAY,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::PRESENT_OR_VALIDATE_DISPLAY,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_CURSOR_POSITION,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_BUFFER,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_SURFACE_DAMAGE,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_BLEND_MODE,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_COLOR,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_COMPOSITION_TYPE,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_DATASPACE,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_DISPLAY_FRAME,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_PLANE_ALPHA,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_SIDEBAND_STREAM,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_SOURCE_CROP,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_TRANSFORM,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_VISIBLE_REGION,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_LAYER_Z_ORDER,
    ::android::hardware::graphics::composer::V2_1::IComposerClient::Command::SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT,
};
}  // namespace details
}  // namespace hardware
}  // namespace android


#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_ICOMPOSERCLIENT_H
