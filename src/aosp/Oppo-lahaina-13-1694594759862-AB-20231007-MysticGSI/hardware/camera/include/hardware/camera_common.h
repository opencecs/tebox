#ifndef TEBOX_CAMERA_COMMON_H
#define TEBOX_CAMERA_COMMON_H

#include <hardware/hardware.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define CAMERA_MODULE_API_VERSION_1_0 HARDWARE_MODULE_API_VERSION(1, 0)
#define CAMERA_DEVICE_API_VERSION_1_0 HARDWARE_DEVICE_API_VERSION(1, 0)
#define CAMERA_HARDWARE_MODULE_ID "camera"

typedef struct camera_metadata camera_metadata_t;
typedef struct camera_info {
    int facing;
    int orientation;
    uint32_t device_version;
    const camera_metadata_t *static_camera_characteristics;
    size_t conflicting_devices_length;
    const char * const *conflicting_devices;
} camera_info_t;

typedef struct camera_module_callbacks {
    void (*camera_device_status_change)(const struct camera_module_callbacks *, int, int);
    void (*torch_mode_status_change)(const struct camera_module_callbacks *, const char *, int);
} camera_module_callbacks_t;

typedef struct camera_module {
    hw_module_t common;
    int (*get_number_of_cameras)(void);
    int (*get_camera_info)(int, camera_info_t *);
    int (*set_callbacks)(const camera_module_callbacks_t *);
    void (*get_vendor_tag_ops)(void *);
    int (*open_legacy)(const struct hw_module_t *, const char *, uint32_t, struct hw_device_t **);
    int (*set_torch_mode)(const char *, bool);
    int (*init)();
    void *reserved[5];
} camera_module_t;

#endif
