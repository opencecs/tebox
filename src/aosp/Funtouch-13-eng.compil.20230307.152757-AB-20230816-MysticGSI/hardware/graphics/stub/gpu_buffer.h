#pragma once

#include <gbm.h>
#include <drm/virtgpu_drm.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

static bool qemuGpuHas3D(int fd) {
    uint64_t value = 0;
    drm_virtgpu_getparam param = {VIRTGPU_PARAM_3D_FEATURES,
                                reinterpret_cast<uint64_t>(&value)};
    return ioctl(fd, DRM_IOCTL_VIRTGPU_GETPARAM, &param) == 0 && value != 0;
}

// Each process owns its GBM device. Callers serialize BO operations; the
// initialization itself must also be safe across Binder/RenderThread callers.
static pthread_once_t qemu_gbm_once = PTHREAD_ONCE_INIT;
static gbm_device* qemu_gbm_device;
static bool qemu_gbm_virgl;

static void qemuInitGbm() {
    int fd = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC);
    if (fd < 0) return;
    qemu_gbm_virgl = qemuGpuHas3D(fd);
    if (!qemu_gbm_virgl) {
        close(fd);
        return;
    }
    // Mesa's cross-build prefix is a host path, not a guest library path.
    setenv("GBM_BACKENDS_PATH", "/vendor/lib64/gbm", 1);
    qemu_gbm_device = gbm_create_device(fd);
    if (!qemu_gbm_device) close(fd);
}

static gbm_device* qemuGbmDevice() {
    pthread_once(&qemu_gbm_once, qemuInitGbm);
    return qemu_gbm_device;
}

static bool qemuGbmUsesVirgl() {
    pthread_once(&qemu_gbm_once, qemuInitGbm);
    return qemu_gbm_virgl;
}

// ---- gfxstream backend (androidboot.tebox.gpu=gfxstream) ----
// There is no VirGL context to create GBM BOs in. Buffers are virtio-gpu 3D
// resources with VirGL formats, as minigbm's virtgpu backend makes them:
// gfxstream turns each into a host ColorBuffer that ANGLE/Vulkan render to.
#include <string.h>
#include <sys/system_properties.h>

static bool qemuGpuIsGfxstream() {
    static int cached = -1;
    if (cached < 0) {
        char value[PROP_VALUE_MAX] = "";
        // ro.boot.* is readable from every domain; a ro.vendor.* copy is not.
        __system_property_get("ro.boot.tebox.gpu", value);
        cached = strcmp(value, "gfxstream") == 0;
    }
    return cached;
}

static int qemuRenderFd() {
    static int fd = -1;
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, [] { fd = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC); });
    return fd;
}

// Android PixelFormat -> VirGL format; 0 = not a GPU format here.
static uint32_t virglFormat(int format) {
    switch (format) {
        case 1: return 67;      // R8G8B8A8_UNORM
        case 2: return 134;     // R8G8B8X8_UNORM
        case 4: return 7;       // B5G6R5_UNORM
        case 5: return 1;       // B8G8R8A8_UNORM
        case 0x16: return 94;   // R16G16B16A16_FLOAT
        case 0x2b: return 8;    // R10G10B10A2_UNORM (gfxstream numbering)
        default: return 0;
    }
}

// Creates a resource and returns a writable dma-buf; *strideBytes is the row
// pitch (tightly packed, as gfxstream ColorBuffers are).
static int virtgpuCreate(uint32_t width, uint32_t height, int format, int bytes,
                         uint32_t* strideBytes, uint32_t* size) {
    int fd = qemuRenderFd();
    uint32_t vformat = virglFormat(format);
    if (fd < 0 || !vformat || !bytes) return -1;
    drm_virtgpu_resource_create create = {};
    create.target = 2;                          // PIPE_TEXTURE_2D
    create.format = vformat;
    create.bind = (1 << 1) | (1 << 3) | (1 << 18);  // RENDER_TARGET | SAMPLER_VIEW | SCANOUT
    create.width = width;
    create.height = height;
    create.depth = 1;
    create.array_size = 1;
    create.stride = width * bytes;
    create.size = create.stride * height;
    if (ioctl(fd, DRM_IOCTL_VIRTGPU_RESOURCE_CREATE, &create)) return -1;
    drm_prime_handle prime = {};
    prime.handle = create.bo_handle;
    prime.flags = DRM_CLOEXEC | DRM_RDWR;
    prime.fd = -1;
    int ret = ioctl(fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime);
    drm_gem_close close_handle = {};
    close_handle.handle = create.bo_handle;
    ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_handle);
    if (ret) return -1;
    *strideBytes = create.stride;
    *size = create.size;
    return prime.fd;
}

// Copies the host ColorBuffer into the guest backing (toHost = false) or the
// guest backing to the host (toHost = true), then waits for it. The kernel
// rejects a stride for non-blob resources (EINVAL): the host uses the
// resource's own pitch.
static bool virtgpuTransfer(int dmabuf, bool toHost, uint32_t width, uint32_t height,
                            uint32_t /*strideBytes*/) {
    int fd = qemuRenderFd();
    if (fd < 0) return false;
    drm_prime_handle prime = {};
    prime.fd = dmabuf;
    if (ioctl(fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime)) return false;
    int ret;
    if (toHost) {
        drm_virtgpu_3d_transfer_to_host xfer = {};
        xfer.bo_handle = prime.handle;
        xfer.box.w = width; xfer.box.h = height; xfer.box.d = 1;
        ret = ioctl(fd, DRM_IOCTL_VIRTGPU_TRANSFER_TO_HOST, &xfer);
    } else {
        drm_virtgpu_3d_transfer_from_host xfer = {};
        xfer.bo_handle = prime.handle;
        xfer.box.w = width; xfer.box.h = height; xfer.box.d = 1;
        ret = ioctl(fd, DRM_IOCTL_VIRTGPU_TRANSFER_FROM_HOST, &xfer);
    }
    if (!ret) {
        drm_virtgpu_3d_wait wait = {};
        wait.handle = prime.handle;
        ret = ioctl(fd, DRM_IOCTL_VIRTGPU_WAIT, &wait);
    }
    drm_gem_close close_handle = {};
    close_handle.handle = prime.handle;
    ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_handle);
    return ret == 0;
}
