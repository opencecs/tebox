/* Local tebox host cast bridge; not for upstream QEMU.
 *
 * UI thread: GPU blit to small FBO, then either dma-buf hand-off or a single
 * glReadPixels into the sender-owned buffer (no staging memcpy). Sender uses
 * writev for header+pixels. Never block display on socket I/O.
 */
#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "tebox-cast-hook.h"

#ifdef CONFIG_OPENGL

#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <unistd.h>

#include <epoxy/gl.h>
#include <pixman.h>
#include <glib.h>

#ifdef CONFIG_GBM
#include <linux/dma-heap.h>
#include <poll.h>
#include <sys/ioctl.h>
#include "ui/egl-helpers.h"
#include "standard-headers/drm/drm_fourcc.h"
#endif

#define TEBOX_CAST_MAGIC   0x54434652u
#define TEBOX_CAST_VERSION 1u
#define TEBOX_CAST_BGRA    0u
#define TEBOX_CAST_RGBA    1u
/*
 * Format flag: the pixels are in a dma-buf passed with SCM_RIGHTS (the GPU
 * scaled the frame straight into it); tebox-cast acks with one byte once the
 * encoder is done with it. Enabled by TEBOX_CAST_DMABUF_HEAP.
 */
#define TEBOX_CAST_DMABUF  0x100u

typedef struct TeboxCastFrame {
    uint8_t *pixels;
    size_t nbytes;
    int width;
    int height;
    uint32_t format;
    uint64_t pts_us;
    int dmabuf_fd;
    int fence_fd; /* GPU done with the dma-buf; -1 if already finished */
} TeboxCastFrame;

typedef struct TeboxCastState {
    int fd;
    char path[512];
    uint8_t *readback;
    int out_w;
    int out_h;
    GLuint fbo;
    GLuint tex;
    GLuint pbo[2];
    int pbo_idx;
    bool pbo_pending; /* previous async read not yet harvested */
    int pending_w;
    int pending_h;
    uint64_t pending_pts;
    uint64_t last_us;
    bool logged;

    /* GPU -> dma-buf capture target (TEBOX_CAST_DMABUF). */
    int dma_fd;
    int dma_w;
    int dma_h;
    GLuint dma_tex;
    GLuint dma_fbo;
    void *dma_img;
    bool dma_off;

    GMutex lock;
    GCond cond;
    GThread *sender;
    bool stop;
    bool busy;
    TeboxCastFrame pending;
    bool has_pending;
    uint64_t dropped;
    uint64_t sent;
} TeboxCastState;

static TeboxCastState tebox_cast = {
    .fd = -1,
    .dma_fd = -1,
    .pending.dmabuf_fd = -1,
    .pending.fence_fd = -1,
};

bool tebox_cast_enabled(void)
{
    const char *v = getenv("TEBOX_CAST");

    return v && v[0] == '1' && v[1] == '\0';
}

uint64_t tebox_cast_refresh_interval_ms(void)
{
    const char *fps_env;
    int fps;

    if (!tebox_cast_enabled()) {
        return 0;
    }
    fps_env = getenv("TEBOX_CAST_FPS");
    fps = fps_env && fps_env[0] ? atoi(fps_env) : 60;
    if (fps < 1) {
        fps = 1;
    }
    if (fps > 60) {
        fps = 60;
    }
    /* ms, min 10 to match SDL2_REFRESH_INTERVAL_BUSY */
    return MAX(10ull, 1000ull / (uint64_t)fps);
}

static uint64_t tebox_now_us(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ull + (uint64_t)ts.tv_nsec / 1000ull;
}

static void tebox_w32be(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static void tebox_w64be(uint8_t *p, uint64_t v)
{
    tebox_w32be(p, (uint32_t)(v >> 32));
    tebox_w32be(p + 4, (uint32_t)v);
}

static void tebox_cast_close_locked(void)
{
    if (tebox_cast.fd >= 0) {
        close(tebox_cast.fd);
        tebox_cast.fd = -1;
    }
}

static bool tebox_cast_connect_locked(void)
{
    struct sockaddr_un addr;
    const char *path = getenv("TEBOX_CAST_SOCK");
    int fd;
    /* ~2 frames at 480x1080 BGRA; larger invites multi-second socket backlog. */
    int sndbuf = 4 * 1024 * 1024;

    if (!path || !path[0]) {
        path = "/tmp/tebox-cast.sock";
    }
    if (tebox_cast.fd >= 0 && !strcmp(tebox_cast.path, path)) {
        return true;
    }
    tebox_cast_close_locked();
    snprintf(tebox_cast.path, sizeof(tebox_cast.path), "%s", path);

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return false;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return false;
    }
#ifdef SO_NOSIGPIPE
    {
        int one = 1;
        setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
    }
#endif
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));
    /* Blocking send of at most one queued frame; UI drops if sender is busy. */
    tebox_cast.fd = fd;
    if (!tebox_cast.logged) {
        error_report("tebox-cast: connected (low-latency sync readback)");
        tebox_cast.logged = true;
    }
    return true;
}

static bool tebox_write_all_fd(int fd, const uint8_t *buf, size_t len)
{
    size_t off = 0;

    while (off < len) {
#ifdef MSG_NOSIGNAL
        ssize_t n = send(fd, buf + off, len - off, MSG_NOSIGNAL);
#else
        ssize_t n = send(fd, buf + off, len - off, 0);
#endif
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (n == 0) {
            return false;
        }
        off += (size_t)n;
    }
    return true;
}

/* Header + pixel body without a contiguous staging buffer. */
static bool tebox_writev_all_fd(int fd, const uint8_t *hdr, size_t hdr_len,
                                const uint8_t *body, size_t body_len)
{
    struct iovec iov[2] = {
        { .iov_base = (void *)hdr, .iov_len = hdr_len },
        { .iov_base = (void *)body, .iov_len = body_len },
    };
    size_t done = 0;
    size_t total = hdr_len + body_len;
#ifdef MSG_NOSIGNAL
    const int flags = MSG_NOSIGNAL;
#else
    const int flags = 0;
#endif

    while (done < total) {
        struct msghdr msg = { 0 };
        struct iovec cur[2];
        size_t skip = done;
        int vi = 0;
        ssize_t n;
        int i;

        for (i = 0; i < 2; i++) {
            if (skip >= iov[i].iov_len) {
                skip -= iov[i].iov_len;
                continue;
            }
            cur[vi].iov_base = (uint8_t *)iov[i].iov_base + skip;
            cur[vi].iov_len = iov[i].iov_len - skip;
            vi++;
            skip = 0;
        }
        if (vi == 0) {
            break;
        }
        msg.msg_iov = cur;
        msg.msg_iovlen = vi;
        n = sendmsg(fd, &msg, flags);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (n == 0) {
            return false;
        }
        done += (size_t)n;
    }
    return true;
}

static void tebox_flip_vertical(uint8_t *pixels, int width, int height)
{
    size_t stride = (size_t)width * 4;
    uint8_t *tmp = g_malloc(stride);
    int y;

    for (y = 0; y < height / 2; y++) {
        uint8_t *a = pixels + (size_t)y * stride;
        uint8_t *b = pixels + (size_t)(height - 1 - y) * stride;

        memcpy(tmp, a, stride);
        memcpy(a, b, stride);
        memcpy(b, tmp, stride);
    }
    g_free(tmp);
}

static void tebox_calc_out_size(int src_w, int src_h, int *out_w, int *out_h)
{
    const char *mw = getenv("TEBOX_CAST_MAX_W");
    const char *mh = getenv("TEBOX_CAST_MAX_H");
    int max_w = mw && mw[0] ? atoi(mw) : 360;
    int max_h = mh && mh[0] ? atoi(mh) : 800;
    int w = src_w;
    int h = src_h;

    if (max_w > 0 && w > max_w) {
        h = (int)((int64_t)h * max_w / w);
        w = max_w;
    }
    if (max_h > 0 && h > max_h) {
        w = (int)((int64_t)w * max_h / h);
        h = max_h;
    }
    w &= ~1;
    h &= ~1;
    if (w < 2) {
        w = 2;
    }
    if (h < 2) {
        h = 2;
    }
    *out_w = w;
    *out_h = h;
}

static bool tebox_ensure_scale_fbo(int w, int h)
{
    size_t nbytes = (size_t)w * (size_t)h * 4;

    if (tebox_cast.fbo && tebox_cast.out_w == w && tebox_cast.out_h == h) {
        return true;
    }
    if (tebox_cast.fbo) {
        glDeleteFramebuffers(1, &tebox_cast.fbo);
        tebox_cast.fbo = 0;
    }
    if (tebox_cast.tex) {
        glDeleteTextures(1, &tebox_cast.tex);
        tebox_cast.tex = 0;
    }
    if (tebox_cast.pbo[0]) {
        glDeleteBuffers(2, tebox_cast.pbo);
        tebox_cast.pbo[0] = tebox_cast.pbo[1] = 0;
    }
    tebox_cast.pbo_pending = false;

    glGenTextures(1, &tebox_cast.tex);
    glBindTexture(GL_TEXTURE_2D, tebox_cast.tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    /* GL_RGBA/GL_RGBA is valid on both desktop GL and GLES. */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, NULL);

    glGenFramebuffers(1, &tebox_cast.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, tebox_cast.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           tebox_cast.tex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        error_report("tebox-cast: scale FBO incomplete");
        return false;
    }

    glGenBuffers(2, tebox_cast.pbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, tebox_cast.pbo[0]);
    glBufferData(GL_PIXEL_PACK_BUFFER, (GLsizeiptr)nbytes, NULL, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, tebox_cast.pbo[1]);
    glBufferData(GL_PIXEL_PACK_BUFFER, (GLsizeiptr)nbytes, NULL, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    tebox_cast.out_w = w;
    tebox_cast.out_h = h;
    (void)nbytes;
    return true;
}

static void tebox_close_fence(int fence_fd)
{
    if (fence_fd >= 0) {
        close(fence_fd);
    }
}

static void tebox_queue_frame(uint8_t *pixels, size_t nbytes, int dmabuf_fd,
                              int fence_fd, int w, int h, uint32_t format,
                              uint64_t pts)
{
    g_mutex_lock(&tebox_cast.lock);
    if (tebox_cast.busy || tebox_cast.has_pending) {
        /* Keep newest frame only. */
        if (tebox_cast.has_pending) {
            g_free(tebox_cast.pending.pixels);
            if (tebox_cast.pending.dmabuf_fd >= 0) {
                close(tebox_cast.pending.dmabuf_fd);
            }
            tebox_close_fence(tebox_cast.pending.fence_fd);
        } else {
            tebox_cast.dropped++;
            g_mutex_unlock(&tebox_cast.lock);
            g_free(pixels);
            if (dmabuf_fd >= 0) {
                close(dmabuf_fd);
            }
            tebox_close_fence(fence_fd);
            return;
        }
        tebox_cast.dropped++;
    }
    tebox_cast.pending.pixels = pixels;
    tebox_cast.pending.dmabuf_fd = dmabuf_fd;
    tebox_cast.pending.fence_fd = fence_fd;
    tebox_cast.pending.nbytes = nbytes;
    tebox_cast.pending.width = w;
    tebox_cast.pending.height = h;
    tebox_cast.pending.format = format;
    tebox_cast.pending.pts_us = pts;
    tebox_cast.has_pending = true;
    g_cond_signal(&tebox_cast.cond);
    g_mutex_unlock(&tebox_cast.lock);
}

#ifdef CONFIG_GBM
/* Sender thread: the blit is already queued. Wait here, not on the UI thread. */
static void tebox_wait_fence(int fence_fd)
{
    struct pollfd pfd = { .fd = fence_fd, .events = POLLIN };

    if (fence_fd < 0) {
        return;
    }
    while (poll(&pfd, 1, 1000) < 0 && errno == EINTR) {
    }
    close(fence_fd);
}

/* Header + 4-byte payload with the dma-buf attached; waits for the ack. */
static bool tebox_send_dmabuf(int fd, const TeboxCastFrame *fr)
{
    uint8_t pkt[40] = { 0 };
    char ctrl[CMSG_SPACE(sizeof(int))];
    struct iovec iov = { pkt, sizeof(pkt) };
    struct msghdr msg = { 0 };
    struct cmsghdr *cmsg;
    struct pollfd pfd = { .fd = fd, .events = POLLIN };
    uint8_t ack;
    ssize_t n;

    tebox_w32be(pkt + 0, TEBOX_CAST_MAGIC);
    tebox_w32be(pkt + 4, TEBOX_CAST_VERSION);
    tebox_w32be(pkt + 8, (uint32_t)fr->width);
    tebox_w32be(pkt + 12, (uint32_t)fr->height);
    tebox_w32be(pkt + 16, (uint32_t)fr->width * 4);
    tebox_w32be(pkt + 20, fr->format);
    tebox_w64be(pkt + 24, fr->pts_us);
    tebox_w32be(pkt + 32, 4);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = ctrl;
    msg.msg_controllen = sizeof(ctrl);
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg), &fr->dmabuf_fd, sizeof(int));
    do {
        n = sendmsg(fd, &msg, MSG_NOSIGNAL);
    } while (n < 0 && errno == EINTR);
    if (n != sizeof(pkt)) {
        return false;
    }
    /* One frame in flight: the buffer is reused only after the ack. */
    if (poll(&pfd, 1, 1000) <= 0) {
        return false;
    }
    do {
        n = recv(fd, &ack, 1, 0);
    } while (n < 0 && errno == EINTR);
    return n == 1;
}

static void tebox_dmabuf_target_free(void)
{
    if (tebox_cast.dma_fbo) {
        glDeleteFramebuffers(1, &tebox_cast.dma_fbo);
        tebox_cast.dma_fbo = 0;
    }
    if (tebox_cast.dma_tex) {
        glDeleteTextures(1, &tebox_cast.dma_tex);
        tebox_cast.dma_tex = 0;
    }
    if (tebox_cast.dma_img) {
        eglDestroyImageKHR(qemu_egl_display, tebox_cast.dma_img);
        tebox_cast.dma_img = NULL;
    }
    if (tebox_cast.dma_fd >= 0) {
        close(tebox_cast.dma_fd);
        tebox_cast.dma_fd = -1;
    }
}

/* A linear RGBA dma-buf from the heap, rendered to through an EGLImage. */
static bool tebox_ensure_dmabuf_target(int w, int h)
{
    const char *heap = getenv("TEBOX_CAST_DMABUF_HEAP");
    struct dma_heap_allocation_data alloc = { 0 };
    uint32_t stride = (uint32_t)w * 4;
    GLint tex_binding, fb_binding;
    EGLint attrs[13];
    bool ok = false;
    int heap_fd;

    if (tebox_cast.dma_off || !heap || !heap[0]) {
        return false;
    }
    if (tebox_cast.dma_fbo && tebox_cast.dma_w == w && tebox_cast.dma_h == h) {
        return true;
    }
    tebox_dmabuf_target_free();

    heap_fd = open(heap, O_RDWR | O_CLOEXEC);
    alloc.len = QEMU_ALIGN_UP((uint64_t)stride * h, 4096);
    alloc.fd_flags = O_RDWR | O_CLOEXEC;
    if (heap_fd < 0 || ioctl(heap_fd, DMA_HEAP_IOCTL_ALLOC, &alloc) < 0) {
        error_report("tebox-cast: dma-heap %s: %s", heap, strerror(errno));
        goto out;
    }
    tebox_cast.dma_fd = (int)alloc.fd;
    attrs[0] = EGL_WIDTH;                     attrs[1] = w;
    attrs[2] = EGL_HEIGHT;                    attrs[3] = h;
    attrs[4] = EGL_LINUX_DRM_FOURCC_EXT;      attrs[5] = DRM_FORMAT_ABGR8888;
    attrs[6] = EGL_DMA_BUF_PLANE0_FD_EXT;     attrs[7] = tebox_cast.dma_fd;
    attrs[8] = EGL_DMA_BUF_PLANE0_OFFSET_EXT; attrs[9] = 0;
    attrs[10] = EGL_DMA_BUF_PLANE0_PITCH_EXT; attrs[11] = (EGLint)stride;
    attrs[12] = EGL_NONE;
    tebox_cast.dma_img = eglCreateImageKHR(qemu_egl_display, EGL_NO_CONTEXT,
                                           EGL_LINUX_DMA_BUF_EXT, NULL, attrs);
    if (tebox_cast.dma_img == EGL_NO_IMAGE_KHR) {
        tebox_cast.dma_img = NULL;
        error_report("tebox-cast: EGL dma-buf import failed (0x%x)", eglGetError());
        goto out;
    }
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex_binding);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fb_binding);
    glGenTextures(1, &tebox_cast.dma_tex);
    glBindTexture(GL_TEXTURE_2D, tebox_cast.dma_tex);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)tebox_cast.dma_img);
    glGenFramebuffers(1, &tebox_cast.dma_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, tebox_cast.dma_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           tebox_cast.dma_tex, 0);
    ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, fb_binding);
    glBindTexture(GL_TEXTURE_2D, tex_binding);
    if (!ok) {
        error_report("tebox-cast: dma-buf FBO incomplete");
        goto out;
    }
    tebox_cast.dma_w = w;
    tebox_cast.dma_h = h;
    error_report("tebox-cast: GPU -> dma-buf capture %dx%d (no CPU readback)", w, h);
out:
    if (heap_fd >= 0) {
        close(heap_fd);
    }
    if (!ok) {
        tebox_dmabuf_target_free();
        tebox_cast.dma_off = true;
    }
    return ok;
}

/* ---- Rotated GPU blit: glBlitFramebuffer can scale/flip but not rotate 90,
 * so landscape goes through a small shader to keep the dma-buf zero-copy. ---- */
static struct {
    GLuint prog, vao, vbo, tex, fbo;
    int tw, th;
    GLint u_tex;
    bool failed;
} tebox_rot;

static GLuint tebox_rot_shader(GLenum type, const char *src)
{
    GLuint s = glCreateShader(type);
    GLint ok = 0;

    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static bool tebox_rot_program(void)
{
    const char *vs, *fs;
    GLuint v, f, p;
    GLint ok = 0;

    if (tebox_rot.prog) {
        return true;
    }
    if (tebox_rot.failed) {
        return false;
    }
    if (epoxy_is_desktop_gl()) {
        vs = "#version 330 core\n"
             "layout(location=0) in vec2 p;layout(location=1) in vec2 t;"
             "out vec2 tc;void main(){tc=t;gl_Position=vec4(p,0.0,1.0);}";
        fs = "#version 330 core\n"
             "in vec2 tc;uniform sampler2D s;out vec4 c;"
             "void main(){c=texture(s,tc);}";
    } else {
        vs = "#version 300 es\n"
             "layout(location=0) in vec2 p;layout(location=1) in vec2 t;"
             "out vec2 tc;void main(){tc=t;gl_Position=vec4(p,0.0,1.0);}";
        fs = "#version 300 es\nprecision mediump float;"
             "in vec2 tc;uniform sampler2D s;out vec4 c;"
             "void main(){c=texture(s,tc);}";
    }
    v = tebox_rot_shader(GL_VERTEX_SHADER, vs);
    f = tebox_rot_shader(GL_FRAGMENT_SHADER, fs);
    if (!v || !f) {
        tebox_rot.failed = true;
        return false;
    }
    p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    glDeleteShader(v);
    glDeleteShader(f);
    if (!ok) {
        glDeleteProgram(p);
        tebox_rot.failed = true;
        return false;
    }
    tebox_rot.prog = p;
    tebox_rot.u_tex = glGetUniformLocation(p, "s");
    glGenVertexArrays(1, &tebox_rot.vao);
    glGenBuffers(1, &tebox_rot.vbo);
    return true;
}

/* guest fbo -> upright portrait intermediate (int_w x int_h) -> dst_fbo
 * rotated clockwise by cw (dst is landscape for 90/270). */
static bool tebox_rot_blit(unsigned int guest_fb, int sw, int sh, bool y0_top,
                           GLuint dst_fbo, int dst_w, int dst_h,
                           int int_w, int int_h, int cw)
{
    GLint draw_fb, read_fb, prog0, vao0, arr0, tex0, act0, vp0[4];
    const float u[4] = { 0, 1, 0, 1 };
    const float vv[4] = { 0, 0, 1, 1 };
    const float px[4] = { -1, 1, -1, 1 };
    const float py[4] = { -1, -1, 1, 1 };
    float verts[16];
    int i;

    if (!tebox_rot_program()) {
        return false;
    }
    if (tebox_rot.tex == 0 || tebox_rot.tw != int_w || tebox_rot.th != int_h) {
        if (tebox_rot.fbo) {
            glDeleteFramebuffers(1, &tebox_rot.fbo);
        }
        if (tebox_rot.tex) {
            glDeleteTextures(1, &tebox_rot.tex);
        }
        glGenTextures(1, &tebox_rot.tex);
        glBindTexture(GL_TEXTURE_2D, tebox_rot.tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, int_w, int_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glGenFramebuffers(1, &tebox_rot.fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tebox_rot.fbo);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, tebox_rot.tex, 0);
        tebox_rot.tw = int_w;
        tebox_rot.th = int_h;
    }
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_fb);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_fb);
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog0);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao0);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arr0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex0);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &act0);
    glGetIntegerv(GL_VIEWPORT, vp0);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, guest_fb);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tebox_rot.fbo);
    if (y0_top) {
        glBlitFramebuffer(0, 0, sw, sh, 0, int_h, int_w, 0,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);
    } else {
        glBlitFramebuffer(0, 0, sw, sh, 0, 0, int_w, int_h,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
    for (i = 0; i < 4; i++) {
        float su, sv;

        switch (cw) {
        case 90:  su = vv[i];        sv = 1.0f - u[i];  break;
        case 180: su = 1.0f - u[i];  sv = 1.0f - vv[i]; break;
        case 270: su = 1.0f - vv[i]; sv = u[i];         break;
        default:  su = u[i];         sv = vv[i];        break;
        }
        verts[i * 4 + 0] = px[i];
        verts[i * 4 + 1] = py[i];
        verts[i * 4 + 2] = su;
        verts[i * 4 + 3] = sv;
    }
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst_fbo);
    glViewport(0, 0, dst_w, dst_h);
    glUseProgram(tebox_rot.prog);
    glBindVertexArray(tebox_rot.vao);
    glBindBuffer(GL_ARRAY_BUFFER, tebox_rot.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)(2 * sizeof(float)));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tebox_rot.tex);
    glUniform1i(tebox_rot.u_tex, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(vao0);
    glBindBuffer(GL_ARRAY_BUFFER, arr0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex0);
    glActiveTexture(act0);
    glUseProgram(prog0);
    glViewport(vp0[0], vp0[1], vp0[2], vp0[3]);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fb);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fb);
    return true;
}

/* Scales (and for landscape rotates) the guest frame into the dma-buf; false
 * if that path is off. int_w/int_h are the unrotated (portrait) size; out_w/
 * out_h are the final size (swapped for 90/270). */
static bool tebox_cast_capture_dmabuf(unsigned int framebuffer, int src_w,
                                      int src_h, int out_w, int out_h,
                                      int int_w, int int_h, int cw,
                                      bool y0_top, uint64_t now)
{
    GLint draw_fb, read_fb;
    int fd;

    if (!tebox_ensure_dmabuf_target(out_w, out_h)) {
        return false;
    }
    if (cw != 0) {
        if (!tebox_rot_blit(framebuffer, src_w, src_h, y0_top,
                            tebox_cast.dma_fbo, out_w, out_h,
                            int_w, int_h, cw)) {
            return false;
        }
        goto handoff;
    }
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_fb);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_fb);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tebox_cast.dma_fbo);
    /*
     * The dma-buf's first row is GL row 0, so draw the picture top at y=0
     * (the readback path gets the same result with a CPU flip).
     */
    if (y0_top) {
        glBlitFramebuffer(0, 0, src_w, src_h, 0, out_h, out_w, 0,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);
    } else {
        glBlitFramebuffer(0, 0, src_w, src_h, 0, 0, out_w, out_h,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fb);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fb);
handoff:
    /*
     * The UI thread is inside virtio-gpu's renderer_blocked. glFinish here
     * holds that lock until the blit retires and the guest spins. Queue a
     * native fence and let the sender wait; fall back only if the driver
     * cannot hand one out.
     */
    {
        EGLSyncKHR sync = egl_create_sync();
        int fence_fd = -1;

        if (sync) {
            glFlush();
            fence_fd = egl_create_fence(sync);
        }
        if (fence_fd < 0) {
            glFinish();
        }
        fd = dup(tebox_cast.dma_fd);
        if (fd < 0) {
            tebox_close_fence(fence_fd);
            return false;
        }
        tebox_queue_frame(NULL, 0, fd, fence_fd, out_w, out_h,
                          TEBOX_CAST_RGBA | TEBOX_CAST_DMABUF, now);
    }
    tebox_cast.last_us = now;
    return true;
}

#endif

static gpointer tebox_sender_thread(gpointer opaque)
{
    (void)opaque;

    g_mutex_lock(&tebox_cast.lock);
    while (!tebox_cast.stop) {
        TeboxCastFrame fr;
        int fd;
        int wr;

        while (!tebox_cast.has_pending && !tebox_cast.stop) {
            g_cond_wait(&tebox_cast.cond, &tebox_cast.lock);
        }
        if (tebox_cast.stop) {
            break;
        }

        fr = tebox_cast.pending;
        tebox_cast.pending.pixels = NULL;
        tebox_cast.pending.dmabuf_fd = -1;
        tebox_cast.pending.fence_fd = -1;
        tebox_cast.has_pending = false;
        tebox_cast.busy = true;

        if (!tebox_cast_connect_locked()) {
            g_free(fr.pixels);
            if (fr.dmabuf_fd >= 0) {
                close(fr.dmabuf_fd);
            }
            tebox_close_fence(fr.fence_fd);
            tebox_cast.busy = false;
            continue;
        }
        fd = tebox_cast.fd;
        g_mutex_unlock(&tebox_cast.lock);

#ifdef CONFIG_GBM
        if (fr.dmabuf_fd >= 0) {
            bool ok;

            tebox_wait_fence(fr.fence_fd);
            fr.fence_fd = -1;
            ok = tebox_send_dmabuf(fd, &fr);

            close(fr.dmabuf_fd);
            g_mutex_lock(&tebox_cast.lock);
            if (!ok) {
                /* Old tebox-cast or a broken encoder path: read back again. */
                tebox_cast.dma_off = true;
                tebox_cast_close_locked();
                error_report("tebox-cast: dma-buf frames not acknowledged; "
                             "falling back to readback");
            } else {
                tebox_cast.sent++;
            }
            tebox_cast.busy = false;
            continue;
        }
#endif

        {
            uint8_t hdr[36];

            tebox_w32be(hdr + 0, TEBOX_CAST_MAGIC);
            tebox_w32be(hdr + 4, TEBOX_CAST_VERSION);
            tebox_w32be(hdr + 8, (uint32_t)fr.width);
            tebox_w32be(hdr + 12, (uint32_t)fr.height);
            tebox_w32be(hdr + 16, (uint32_t)fr.width * 4);
            tebox_w32be(hdr + 20, fr.format);
            tebox_w64be(hdr + 24, fr.pts_us);
            tebox_w32be(hdr + 32, (uint32_t)fr.nbytes);
            /* writev: no second full-frame copy into a contiguous packet. */
            wr = tebox_writev_all_fd(fd, hdr, sizeof(hdr), fr.pixels, fr.nbytes)
                     ? 1 : -1;
            g_free(fr.pixels);
        }

        g_mutex_lock(&tebox_cast.lock);
        if (wr < 0) {
            tebox_cast_close_locked();
        } else {
            tebox_cast.sent++;
        }
        tebox_cast.busy = false;
    }

    if (tebox_cast.has_pending) {
        g_free(tebox_cast.pending.pixels);
        tebox_cast.pending.pixels = NULL;
        if (tebox_cast.pending.dmabuf_fd >= 0) {
            close(tebox_cast.pending.dmabuf_fd);
        }
        tebox_close_fence(tebox_cast.pending.fence_fd);
        tebox_cast.pending.fence_fd = -1;
        tebox_cast.has_pending = false;
    }
    tebox_cast_close_locked();
    g_mutex_unlock(&tebox_cast.lock);
    return NULL;
}

static void tebox_cast_ensure_sender(void)
{
    static gsize once;

    if (g_once_init_enter(&once)) {
        g_mutex_init(&tebox_cast.lock);
        g_cond_init(&tebox_cast.cond);
        tebox_cast.sender = g_thread_new("tebox-cast", tebox_sender_thread,
                                         NULL);
        g_once_init_leave(&once, 1);
    }
}

void tebox_cast_capture(unsigned int framebuffer, int src_w, int src_h,
                        bool y0_top)
{
    int out_w, out_h;
    uint32_t format;
    size_t nbytes;
    GLint draw_fb, read_fb, read_buffer, pack_buffer, alignment;
    GLint row_length, skip_rows, skip_pixels;
    uint64_t now;
    int fps;
    const char *fps_env;
    const char *flip_env;
    uint8_t *copy;
    bool cpu_flip;
    bool sender_busy;

    if (!tebox_cast_enabled()) {
        return;
    }
    if (!framebuffer || src_w <= 0 || src_h <= 0) {
        return;
    }

    fps_env = getenv("TEBOX_CAST_FPS");
    fps = fps_env && fps_env[0] ? atoi(fps_env) : 60;
    if (fps < 1) {
        fps = 1;
    }
    if (fps > 60) {
        fps = 60;
    }
    now = tebox_now_us();
    if (tebox_cast.last_us &&
        now - tebox_cast.last_us < 1000000ull / (uint64_t)fps) {
        return;
    }

    tebox_cast_ensure_sender();

    /* If previous frame still in flight, skip — never build a backlog. */
    g_mutex_lock(&tebox_cast.lock);
    sender_busy = tebox_cast.busy || tebox_cast.has_pending;
    if (sender_busy) {
        tebox_cast.dropped++;
    }
    g_mutex_unlock(&tebox_cast.lock);
    if (sender_busy) {
        return;
    }

    tebox_calc_out_size(src_w, src_h, &out_w, &out_h);
#ifdef CONFIG_GBM
    {
        /* Guest landscape: rotate upright so the dma-buf (and thus the cast
         * stream) stays correctly oriented. cw is the clockwise correction;
         * 90/270 swaps the final dimensions to landscape. */
        int cw = (360 - tebox_display_rotation()) % 360;
        int fin_w = out_w, fin_h = out_h;

        if (cw == 90 || cw == 270) {
            fin_w = out_h;
            fin_h = out_w;
        }
        if (tebox_cast_capture_dmabuf(framebuffer, src_w, src_h, fin_w, fin_h,
                                      out_w, out_h, cw, y0_top, now)) {
            return;
        }
    }
#endif
    if (!tebox_ensure_scale_fbo(out_w, out_h)) {
        return;
    }
    nbytes = (size_t)out_w * (size_t)out_h * 4;
    /* Own buffer for the sender thread — no staging readback + memcpy. */
    copy = g_try_malloc(nbytes);
    if (!copy) {
        return;
    }

    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_fb);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_fb);
    glGetIntegerv(GL_READ_BUFFER, &read_buffer);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &pack_buffer);
    glGetIntegerv(GL_PACK_ALIGNMENT, &alignment);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &row_length);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &skip_rows);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &skip_pixels);

    /* Match SDL window: flip blit when guest y0 is not top. */
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tebox_cast.fbo);
    if (!y0_top) {
        glBlitFramebuffer(0, 0, src_w, src_h,
                          0, out_h, out_w, 0,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);
    } else {
        glBlitFramebuffer(0, 0, src_w, src_h,
                          0, 0, out_w, out_h,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }

    /* Sync read of the *small* FBO only — lower latency than PBO pipeline. */
    glBindFramebuffer(GL_READ_FRAMEBUFFER, tebox_cast.fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    /* GLES (egl-headless on Mali) only guarantees GL_RGBA readback. */
    format = epoxy_is_desktop_gl() ? TEBOX_CAST_BGRA : TEBOX_CAST_RGBA;
    glReadPixels(0, 0, out_w, out_h,
                 format == TEBOX_CAST_BGRA ? GL_BGRA : GL_RGBA,
                 GL_UNSIGNED_BYTE, copy);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, pack_buffer);
    glPixelStorei(GL_PACK_ALIGNMENT, alignment);
    glPixelStorei(GL_PACK_ROW_LENGTH, row_length);
    glPixelStorei(GL_PACK_SKIP_ROWS, skip_rows);
    glPixelStorei(GL_PACK_SKIP_PIXELS, skip_pixels);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fb);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fb);
    glReadBuffer(read_buffer);

    /*
     * After y-flip blit, glReadPixels is still bottom-up in memory.
     * Default CPU flip for upright video. TEBOX_CAST_FLIP=0 to disable.
     */
    flip_env = getenv("TEBOX_CAST_FLIP");
    cpu_flip = !(flip_env && flip_env[0] == '0');
    if (cpu_flip) {
        tebox_flip_vertical(copy, out_w, out_h);
    }

    tebox_queue_frame(copy, nbytes, -1, -1, out_w, out_h, format, now);
    tebox_cast.last_us = now;
}

void tebox_cast_capture_pixels(const void *pixels, int width, int height,
                               int stride)
{
    int out_w, out_h, fps;
    const char *fps_env;
    uint64_t now;
    bool sender_busy;
    uint8_t *copy;
    pixman_image_t *src, *dst;

    if (!tebox_cast_enabled() || !pixels || width <= 0 || height <= 0) {
        return;
    }
    fps_env = getenv("TEBOX_CAST_FPS");
    fps = fps_env && fps_env[0] ? atoi(fps_env) : 60;
    fps = MAX(1, MIN(fps, 60));
    now = tebox_now_us();
    if (tebox_cast.last_us &&
        now - tebox_cast.last_us < 1000000ull / (uint64_t)fps) {
        return;
    }
    tebox_cast_ensure_sender();
    g_mutex_lock(&tebox_cast.lock);
    sender_busy = tebox_cast.busy || tebox_cast.has_pending;
    if (sender_busy) {
        tebox_cast.dropped++;
    }
    g_mutex_unlock(&tebox_cast.lock);
    if (sender_busy) {
        return;
    }
    tebox_calc_out_size(width, height, &out_w, &out_h);
    /* mali virtio-gpu-pci is 2D: keep a private CPU copy per frame. A shared
     * dma-buf fed to mvxenc caused heavy ghosting (VPU still mapped it). */
    copy = g_try_malloc((size_t)out_w * out_h * 4);
    if (!copy) {
        return;
    }
    src = pixman_image_create_bits(PIXMAN_x8r8g8b8, width, height,
                                   (uint32_t *)pixels, stride);
    dst = pixman_image_create_bits(PIXMAN_x8r8g8b8, out_w, out_h,
                                   (uint32_t *)copy, out_w * 4);
    if (out_w != width || out_h != height) {
        pixman_transform_t scale;

        pixman_transform_init_scale(&scale,
                                    pixman_double_to_fixed((double)width / out_w),
                                    pixman_double_to_fixed((double)height / out_h));
        pixman_image_set_transform(src, &scale);
        pixman_image_set_filter(src, PIXMAN_FILTER_BILINEAR, NULL, 0);
    }
    pixman_image_composite(PIXMAN_OP_SRC, src, NULL, dst, 0, 0, 0, 0, 0, 0,
                           out_w, out_h);
    pixman_image_unref(src);
    pixman_image_unref(dst);
    tebox_cast.last_us = now;
    tebox_queue_frame(copy, (size_t)out_w * out_h * 4, -1, -1, out_w, out_h,
                      0 /* CAST_FMT_BGRA8888 */, now);
}

void tebox_cast_shutdown(void)
{
    if (tebox_cast.sender) {
        g_mutex_lock(&tebox_cast.lock);
        tebox_cast.stop = true;
        g_cond_signal(&tebox_cast.cond);
        g_mutex_unlock(&tebox_cast.lock);
        g_thread_join(tebox_cast.sender);
        tebox_cast.sender = NULL;
    }
    tebox_cast_close_locked();
    if (tebox_cast.pbo[0]) {
        glDeleteBuffers(2, tebox_cast.pbo);
        tebox_cast.pbo[0] = tebox_cast.pbo[1] = 0;
    }
    if (tebox_cast.fbo) {
        glDeleteFramebuffers(1, &tebox_cast.fbo);
        tebox_cast.fbo = 0;
    }
    if (tebox_cast.tex) {
        glDeleteTextures(1, &tebox_cast.tex);
        tebox_cast.tex = 0;
    }
    g_free(tebox_cast.readback);
    tebox_cast.readback = NULL;
    tebox_cast.out_w = 0;
    tebox_cast.out_h = 0;
    tebox_cast.logged = false;
    tebox_cast.pbo_pending = false;
#ifdef CONFIG_GBM
    tebox_dmabuf_target_free();
#endif
}

#else /* !CONFIG_OPENGL */

bool tebox_cast_enabled(void)
{
    return false;
}

uint64_t tebox_cast_refresh_interval_ms(void)
{
    return 0;
}

void tebox_cast_capture(unsigned int framebuffer, int src_w, int src_h,
                        bool y0_top)
{
}

void tebox_cast_capture_pixels(const void *pixels, int width, int height,
                               int stride)
{
}

void tebox_cast_shutdown(void)
{
}

#endif
