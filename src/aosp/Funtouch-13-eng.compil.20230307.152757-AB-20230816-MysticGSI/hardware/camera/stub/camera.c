#include <hardware/camera.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define WIDTH 640
#define HEIGHT 480
#define FRAME_BYTES (WIDTH * HEIGHT * 3 / 2)

typedef struct {
    camera_device_t device;
    camera_device_ops_t ops;
    pthread_mutex_t lock;
    pthread_t thread;
    bool thread_started, stop, preview, recording;
    int32_t messages;
    camera_notify_callback notify;
    camera_data_callback data;
    camera_data_timestamp_callback timestamp;
    camera_request_memory request_memory;
    void *user;
    char parameters[256];
} tebox_camera_t;

static int64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static void fill_frame(unsigned char *frame, unsigned int tick) {
    unsigned char *y = frame, *uv = frame + WIDTH * HEIGHT;
    for (int j = 0; j < HEIGHT; ++j) {
        for (int i = 0; i < WIDTH; ++i) {
            y[j * WIDTH + i] = (unsigned char)((i * 255 / WIDTH + j * 40 / HEIGHT + tick) & 255);
        }
    }
    for (int j = 0; j < HEIGHT / 2; ++j) {
        for (int i = 0; i < WIDTH; i += 2) {
            uv[j * WIDTH + i] = 128;
            uv[j * WIDTH + i + 1] = 128;
        }
    }
}

static void *preview_loop(void *arg) {
    tebox_camera_t *c = arg;
    unsigned int tick = 0;
    while (true) {
        pthread_mutex_lock(&c->lock);
        bool stop = c->stop, active = c->preview && c->data && c->request_memory;
        camera_data_callback data = c->data;
        camera_request_memory request = c->request_memory;
        int32_t messages = c->messages;
        void *user = c->user;
        pthread_mutex_unlock(&c->lock);
        if (stop) break;
        if (active && (messages & (CAMERA_MSG_PREVIEW_FRAME | CAMERA_MSG_VIDEO_FRAME))) {
            camera_memory_t *mem = request(-1, FRAME_BYTES, 1, user);
            if (mem && mem->data && mem->size >= FRAME_BYTES) {
                fill_frame(mem->data, tick++);
                if (messages & CAMERA_MSG_PREVIEW_FRAME)
                    data(CAMERA_MSG_PREVIEW_FRAME, mem, 0, NULL, user);
                if ((messages & CAMERA_MSG_VIDEO_FRAME) && c->timestamp)
                    c->timestamp(now_ns(), CAMERA_MSG_VIDEO_FRAME, mem, 0, user);
            }
            if (mem && mem->release) mem->release(mem);
        }
        usleep(33333);
    }
    return NULL;
}

static tebox_camera_t *self(camera_device_t *d) { return (tebox_camera_t *)d->priv; }
static int set_window(camera_device_t *d, struct preview_stream_ops *w) { (void)d; (void)w; return 0; }
static void set_callbacks(camera_device_t *d, camera_notify_callback n, camera_data_callback cb,
                          camera_data_timestamp_callback ts, camera_request_memory rm, void *u) {
    tebox_camera_t *c = self(d); pthread_mutex_lock(&c->lock);
    c->notify = n; c->data = cb; c->timestamp = ts; c->request_memory = rm; c->user = u;
    pthread_mutex_unlock(&c->lock);
}
static void enable_msg(camera_device_t *d, int32_t m) { tebox_camera_t *c = self(d); c->messages |= m; }
static void disable_msg(camera_device_t *d, int32_t m) { tebox_camera_t *c = self(d); c->messages &= ~m; }
static int msg_enabled(camera_device_t *d, int32_t m) { return (self(d)->messages & m) == m; }
static int start_preview(camera_device_t *d) {
    tebox_camera_t *c = self(d); c->preview = true;
    if (!c->thread_started) { c->thread_started = true; if (pthread_create(&c->thread, NULL, preview_loop, c)) return -errno; }
    return 0;
}
static void stop_preview(camera_device_t *d) { self(d)->preview = false; }
static int preview_enabled(camera_device_t *d) { return self(d)->preview; }
static int store_meta(camera_device_t *d, int enable) { return enable ? -EINVAL : 0; }
static int start_recording(camera_device_t *d) { self(d)->recording = true; return start_preview(d); }
static void stop_recording(camera_device_t *d) { self(d)->recording = false; }
static int recording_enabled(camera_device_t *d) { return self(d)->recording; }
static void release_recording(camera_device_t *d, const void *o) { (void)d; (void)o; }
static int autofocus(camera_device_t *d) { tebox_camera_t *c = self(d); if (c->notify) c->notify(CAMERA_MSG_FOCUS, 1, 0, c->user); return 0; }
static int cancel_autofocus(camera_device_t *d) { (void)d; return 0; }
static int take_picture(camera_device_t *d) {
    tebox_camera_t *c = self(d); if (!c->data || !c->request_memory) return -ENODEV;
    camera_memory_t *mem = c->request_memory(-1, FRAME_BYTES, 1, c->user);
    if (!mem || !mem->data || mem->size < FRAME_BYTES) return -ENOMEM;
    fill_frame(mem->data, 255); c->data(CAMERA_MSG_RAW_IMAGE, mem, 0, NULL, c->user);
    if (mem->release) mem->release(mem); return 0;
}
static int cancel_picture(camera_device_t *d) { (void)d; return 0; }
static int set_parameters(camera_device_t *d, const char *p) { if (!p) return -EINVAL; strncpy(self(d)->parameters, p, sizeof(self(d)->parameters)-1); return 0; }
static char *get_parameters(camera_device_t *d) { return strdup(self(d)->parameters); }
static void put_parameters(camera_device_t *d, char *p) { (void)d; free(p); }
static int send_command(camera_device_t *d, int32_t cmd, int32_t a, int32_t b) { (void)d; (void)cmd; (void)a; (void)b; return 0; }
static void release_device(camera_device_t *d) { tebox_camera_t *c = self(d); c->stop = true; if (c->thread_started) pthread_join(c->thread, NULL); }
static int dump_device(camera_device_t *d, int fd) { dprintf(fd, "tebox camera %dx%d NV21\n", WIDTH, HEIGHT); return 0; }

static int close_device(hw_device_t *hw) { tebox_camera_t *c = (tebox_camera_t *)hw; release_device(&c->device); pthread_mutex_destroy(&c->lock); free(c); return 0; }
static int open_camera(const hw_module_t *module, const char *id, hw_device_t **out) {
    if (!id || strcmp(id, "0") != 0 || !out) return -EINVAL;
    tebox_camera_t *c = calloc(1, sizeof(*c)); if (!c) return -ENOMEM;
    pthread_mutex_init(&c->lock, NULL); snprintf(c->parameters, sizeof(c->parameters), "preview-size=%dx%d;video-size=%dx%d;preview-format=yuv420sp;focus-mode=fixed", WIDTH, HEIGHT, WIDTH, HEIGHT);
    c->device.common.tag = HARDWARE_DEVICE_TAG; c->device.common.version = CAMERA_DEVICE_API_VERSION_1_0;
    c->device.common.module = (hw_module_t *)module; c->device.common.close = close_device; c->device.ops = &c->ops; c->device.priv = c;
    c->ops.set_preview_window=set_window; c->ops.set_callbacks=set_callbacks; c->ops.enable_msg_type=enable_msg; c->ops.disable_msg_type=disable_msg; c->ops.msg_type_enabled=msg_enabled;
    c->ops.start_preview=start_preview; c->ops.stop_preview=stop_preview; c->ops.preview_enabled=preview_enabled; c->ops.store_meta_data_in_buffers=store_meta;
    c->ops.start_recording=start_recording; c->ops.stop_recording=stop_recording; c->ops.recording_enabled=recording_enabled; c->ops.release_recording_frame=release_recording;
    c->ops.auto_focus=autofocus; c->ops.cancel_auto_focus=cancel_autofocus; c->ops.take_picture=take_picture; c->ops.cancel_picture=cancel_picture;
    c->ops.set_parameters=set_parameters; c->ops.get_parameters=get_parameters; c->ops.put_parameters=put_parameters; c->ops.send_command=send_command; c->ops.release=release_device; c->ops.dump=dump_device;
    *out = &c->device.common; return 0;
}

static hw_module_methods_t methods = { .open = open_camera };
static int camera_count(void) { return 1; }
static int camera_info(int id, camera_info_t *out) { if (id != 0 || !out) return -EINVAL; memset(out, 0, sizeof(*out)); out->facing = CAMERA_FACING_BACK; out->orientation = 90; out->device_version = CAMERA_DEVICE_API_VERSION_1_0; return 0; }

camera_module_t HAL_MODULE_INFO_SYM = {
    .common = { .tag=HARDWARE_MODULE_TAG, .module_api_version=CAMERA_MODULE_API_VERSION_1_0, .hal_api_version=HARDWARE_HAL_API_VERSION, .id=CAMERA_HARDWARE_MODULE_ID, .name="Tebox virtual camera", .author="tebox", .methods=&methods },
    .get_number_of_cameras=camera_count, .get_camera_info=camera_info,
};
