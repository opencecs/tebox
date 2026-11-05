#pragma once
// Minimal NDK-style AIDL Surface parcelable support for vendor Codec2 HAL.
// Full framework surface_aidl.h is not in the public NDK; we only need the
// type so IInputSurface AIDL stubs compile. createInputSurface stays OMITTED.

#include <android/binder_parcel.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

typedef struct ANativeWindow ANativeWindow;

// Placeholder parcel helpers — unused if createInputSurface is not implemented.
static inline binder_status_t ASurface_readFromParcel(const AParcel* /*parcel*/,
                                                      ANativeWindow** outWindow) {
    if (outWindow) *outWindow = nullptr;
    return STATUS_BAD_VALUE;
}

static inline binder_status_t ASurface_writeToParcel(ANativeWindow* /*window*/,
                                                     AParcel* /*parcel*/) {
    return STATUS_BAD_VALUE;
}

__END_DECLS
