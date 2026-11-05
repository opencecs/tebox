#ifndef TEBOX_CAMERA_H
#define TEBOX_CAMERA_H

#include "camera_common.h"
#include <system/camera.h>

typedef struct camera_memory camera_memory_t;
typedef void (*camera_release_memory)(camera_memory_t *);
struct camera_memory { void *data; size_t size; void *handle; camera_release_memory release; };
typedef camera_memory_t *(*camera_request_memory)(int, size_t, unsigned int, void *);
typedef void (*camera_notify_callback)(int32_t, int32_t, int32_t, void *);
typedef void (*camera_data_callback)(int32_t, const camera_memory_t *, unsigned int,
                                     camera_frame_metadata_t *, void *);
typedef void (*camera_data_timestamp_callback)(int64_t, int32_t, const camera_memory_t *,
                                               unsigned int, void *);

struct preview_stream_ops;
typedef struct camera_device camera_device_t;
typedef struct camera_device_ops {
    int (*set_preview_window)(camera_device_t *, struct preview_stream_ops *);
    void (*set_callbacks)(camera_device_t *, camera_notify_callback, camera_data_callback,
                          camera_data_timestamp_callback, camera_request_memory, void *);
    void (*enable_msg_type)(camera_device_t *, int32_t);
    void (*disable_msg_type)(camera_device_t *, int32_t);
    int (*msg_type_enabled)(camera_device_t *, int32_t);
    int (*start_preview)(camera_device_t *);
    void (*stop_preview)(camera_device_t *);
    int (*preview_enabled)(camera_device_t *);
    int (*store_meta_data_in_buffers)(camera_device_t *, int);
    int (*start_recording)(camera_device_t *);
    void (*stop_recording)(camera_device_t *);
    int (*recording_enabled)(camera_device_t *);
    void (*release_recording_frame)(camera_device_t *, const void *);
    int (*auto_focus)(camera_device_t *);
    int (*cancel_auto_focus)(camera_device_t *);
    int (*take_picture)(camera_device_t *);
    int (*cancel_picture)(camera_device_t *);
    int (*set_parameters)(camera_device_t *, const char *);
    char *(*get_parameters)(camera_device_t *);
    void (*put_parameters)(camera_device_t *, char *);
    int (*send_command)(camera_device_t *, int32_t, int32_t, int32_t);
    void (*release)(camera_device_t *);
    int (*dump)(camera_device_t *, int);
} camera_device_ops_t;

struct camera_device {
    hw_device_t common;
    camera_device_ops_t *ops;
    void *priv;
};

#endif
