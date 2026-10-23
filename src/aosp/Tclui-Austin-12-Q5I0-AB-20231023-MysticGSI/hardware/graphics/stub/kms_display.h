#pragma once

// Synchronous client-composition scanout. The client target is copied only
// after its acquire fence signals, so a returned -1 fence means work is done.
class KmsDisplay {
    struct GpuFrame {
        int drm_fd;
        uint32_t fb = 0;
        ~GpuFrame() { if (fb) ioctl(drm_fd, DRM_IOCTL_MODE_RMFB, &fb); }
    };
    struct Frame {
        uint32_t handle = 0, fb = 0, pitch = 0;
        size_t size = 0;
        void* map = MAP_FAILED;
    } frames_[2];
    struct Target {
        ScopedFileDescriptor fd;
        std::shared_ptr<GpuFrame> gpu_frame;
        int width = 0, height = 0, stride = 0, format = 0;
    };
    int fd_ = -1, next_ = 0, slot_ = -1;
    uint32_t connector_ = 0, crtc_ = 0;
    drm_mode_modeinfo mode_ = {};
    std::unordered_map<int, Target> targets_;
    ScopedFileDescriptor acquire_;
    bool virgl_ = false;
    std::shared_ptr<GpuFrame> current_gpu_frame_;

  public:
    ~KmsDisplay() {
        if (fd_ < 0) return;
        targets_.clear();
        current_gpu_frame_.reset();
        for (auto& frame : frames_) {
            if (frame.map != MAP_FAILED) munmap(frame.map, frame.size);
            if (frame.fb) ioctl(fd_, DRM_IOCTL_MODE_RMFB, &frame.fb);
            if (frame.handle) {
                drm_mode_destroy_dumb destroy = {};
                destroy.handle = frame.handle;
                ioctl(fd_, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
            }
        }
        close(fd_);
    }

    bool initialize() {
        fd_ = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
        if (fd_ < 0) return fail("open card0");
        if (ioctl(fd_, DRM_IOCTL_SET_MASTER, 0)) return fail("DRM master");
        drm_mode_card_res res = {};
        if (ioctl(fd_, DRM_IOCTL_MODE_GETRESOURCES, &res)) return fail("get resources");
        std::vector<uint32_t> connectors(res.count_connectors), crtcs(res.count_crtcs);
        std::vector<uint32_t> fbs(res.count_fbs), encs(res.count_encoders);
        res.fb_id_ptr = reinterpret_cast<uint64_t>(fbs.data());
        res.encoder_id_ptr = reinterpret_cast<uint64_t>(encs.data());
        res.connector_id_ptr = reinterpret_cast<uint64_t>(connectors.data());
        res.crtc_id_ptr = reinterpret_cast<uint64_t>(crtcs.data());
        if (ioctl(fd_, DRM_IOCTL_MODE_GETRESOURCES, &res)) return fail("get resource IDs");
        for (uint32_t id : connectors) {
            drm_mode_get_connector conn = {};
            conn.connector_id = id;
            if (ioctl(fd_, DRM_IOCTL_MODE_GETCONNECTOR, &conn) ||
                conn.connection != 1 || !conn.count_modes) continue;
            std::vector<drm_mode_modeinfo> modes(conn.count_modes);
            std::vector<uint32_t> encoders(conn.count_encoders);
            std::vector<uint32_t> props(conn.count_props);
            std::vector<uint64_t> values(conn.count_props);
            conn.props_ptr = reinterpret_cast<uint64_t>(props.data());
            conn.prop_values_ptr = reinterpret_cast<uint64_t>(values.data());
            conn.modes_ptr = reinterpret_cast<uint64_t>(modes.data());
            conn.encoders_ptr = reinterpret_cast<uint64_t>(encoders.data());
            if (ioctl(fd_, DRM_IOCTL_MODE_GETCONNECTOR, &conn)) continue;
            for (const auto& mode : modes) {
                if (mode.hdisplay != kWidth || mode.vdisplay != kHeight) continue;
                for (uint32_t encoder : encoders) {
                    drm_mode_get_encoder enc = {};
                    enc.encoder_id = encoder;
                    if (ioctl(fd_, DRM_IOCTL_MODE_GETENCODER, &enc)) continue;
                    for (size_t i = 0; i < crtcs.size(); ++i) {
                        if (!(enc.possible_crtcs & (1U << i))) continue;
                        crtc_ = crtcs[i]; connector_ = id; mode_ = mode;
                        break;
                    }
                    if (crtc_) break;
                }
                if (crtc_) break;
            }
            if (crtc_) break;
        }
        if (!crtc_) return fail("no connector/CRTC matching the configured display size");
        virgl_ = qemuGpuHas3D(fd_);
        if (virgl_) {
            ALOGI("KMS ready: connector=%u crtc=%u %dx%d VirGL direct scanout",
                  connector_, crtc_, kWidth, kHeight);
            return true;
        }
        for (auto& frame : frames_) {
            drm_mode_create_dumb create = {};
            create.width = kWidth; create.height = kHeight; create.bpp = 32;
            if (ioctl(fd_, DRM_IOCTL_MODE_CREATE_DUMB, &create)) return fail("create scanout");
            frame.handle = create.handle; frame.pitch = create.pitch; frame.size = create.size;
            drm_mode_fb_cmd2 fb = {};
            fb.width = kWidth; fb.height = kHeight;
            fb.pixel_format = fourcc('X','R','2','4');
            fb.handles[0] = frame.handle; fb.pitches[0] = frame.pitch;
            if (ioctl(fd_, DRM_IOCTL_MODE_ADDFB2, &fb)) return fail("add framebuffer");
            frame.fb = fb.fb_id;
            drm_mode_map_dumb map = {};
            map.handle = frame.handle;
            if (ioctl(fd_, DRM_IOCTL_MODE_MAP_DUMB, &map)) return fail("map scanout offset");
            frame.map = mmap(nullptr, frame.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, map.offset);
            if (frame.map == MAP_FAILED) return fail("map scanout");
        }
        ALOGI("KMS ready: connector=%u crtc=%u %dx%d", connector_, crtc_, kWidth, kHeight);
        return true;
    }

    void clearTargets() { targets_.clear(); slot_ = -1; acquire_.set(-1); }

    bool setTarget(const Buffer& buffer) {
        if (buffer.slot < 0) return false;
        if (buffer.handle) {
            const auto& h = *buffer.handle;
            TeboxBufferInfo info;
            if (h.fds.size() != 1 ||
                !teboxBufferInfoFromInts(h.ints.data(), h.ints.size(), &info)) return false;
            const int bytes = bufferFormat(info.format).bytes;
            if (!bytes || info.width != kWidth || info.height != kHeight ||
                info.stride < kWidth || info.stride > 32768) return false;
            struct stat st;
            if (fstat(h.fds[0].get(), &st) ||
                uint64_t(st.st_size) < uint64_t(info.stride) * kHeight * bytes) return false;
            Target t;
            t.fd.set(fcntl(h.fds[0].get(), F_DUPFD_CLOEXEC, 0));
            if (t.fd.get() < 0) return false;
            t.width = info.width; t.height = info.height; t.stride = info.stride; t.format = info.format;
            if (virgl_) {
                drm_prime_handle prime = {};
                prime.fd = t.fd.get();
                if (ioctl(fd_, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime)) return fail("import GPU target");
                drm_mode_fb_cmd2 fb = {};
                fb.width = t.width; fb.height = t.height;
                fb.pixel_format = bufferFormat(t.format).fourcc;
                fb.handles[0] = prime.handle;
                fb.pitches[0] = t.stride * bytes;
                int ret = ioctl(fd_, DRM_IOCTL_MODE_ADDFB2, &fb);
                // The framebuffer keeps its own BO reference.
                drm_gem_close close_handle = {};
                close_handle.handle = prime.handle;
                ioctl(fd_, DRM_IOCTL_GEM_CLOSE, &close_handle);
                if (ret) return fail("add GPU framebuffer");
                t.gpu_frame = std::make_shared<GpuFrame>();
                t.gpu_frame->drm_fd = fd_;
                t.gpu_frame->fb = fb.fb_id;
            }
            targets_[buffer.slot] = std::move(t);
        }
        if (!targets_.count(buffer.slot)) return false;
        slot_ = buffer.slot;
        acquire_.set(buffer.fence.get() >= 0 ?
                     fcntl(buffer.fence.get(), F_DUPFD_CLOEXEC, 0) : -1);
        return !(buffer.fence.get() >= 0 && acquire_.get() < 0);
    }

    static int64_t nowUs() {
        struct timespec t;
        clock_gettime(CLOCK_MONOTONIC, &t);
        return int64_t(t.tv_sec) * 1000000 + t.tv_nsec / 1000;
    }
    // Present timing, logged every ~2 s: time blocked on the acquire fence and
    // in the scanout ioctl, both inside SurfaceFlinger's present call.
    struct PresentStats {
        int64_t windowStart = 0, fenceUs = 0, flipUs = 0, maxFenceUs = 0, maxFlipUs = 0;
        int frames = 0;
    } stats_;
    void notePresent(int64_t fenceUs, int64_t flipUs) {
        const int64_t t = nowUs();
        if (!stats_.windowStart) stats_.windowStart = t;
        stats_.frames++;
        stats_.fenceUs += fenceUs;
        stats_.flipUs += flipUs;
        if (fenceUs > stats_.maxFenceUs) stats_.maxFenceUs = fenceUs;
        if (flipUs > stats_.maxFlipUs) stats_.maxFlipUs = flipUs;
        if (t - stats_.windowStart >= 2000000) {
            ALOGI("present: %.1f fps, fence wait avg %lld us (max %lld), flip avg %lld us (max %lld)",
                  stats_.frames * 1e6 / double(t - stats_.windowStart),
                  (long long)(stats_.fenceUs / stats_.frames), (long long)stats_.maxFenceUs,
                  (long long)(stats_.flipUs / stats_.frames), (long long)stats_.maxFlipUs);
            stats_ = PresentStats{};
            stats_.windowStart = t;
        }
    }

    bool present() {
        if ((!virgl_ && !frames_[next_].fb) || !targets_.count(slot_)) return false;
        const int64_t t0 = nowUs();
        if (acquire_.get() >= 0) {
            struct pollfd fence = {acquire_.get(), POLLIN, 0};
            int ret;
            do { ret = poll(&fence, 1, 3000); } while (ret < 0 && errno == EINTR);
            if (ret <= 0 || (fence.revents & (POLLERR | POLLNVAL))) return fail("acquire fence");
            acquire_.set(-1);
        }
        const int64_t t1 = nowUs();
        const auto& target = targets_.at(slot_);
        if (virgl_) {
            if (!target.gpu_frame) return false;
            drm_mode_crtc crtc = {};
            crtc.crtc_id = crtc_; crtc.fb_id = target.gpu_frame->fb;
            crtc.set_connectors_ptr = reinterpret_cast<uint64_t>(&connector_);
            crtc.count_connectors = 1; crtc.mode_valid = 1; crtc.mode = mode_;
            // GPU content is already in the host texture. DIRTYFB or a raw
            // CPU mmap would upload stale guest memory over the rendered frame.
            if (ioctl(fd_, DRM_IOCTL_MODE_SETCRTC, &crtc)) return fail("GPU scanout");
            current_gpu_frame_ = target.gpu_frame;
            notePresent(t1 - t0, nowUs() - t1);
            return true;
        }
        const auto fmt = bufferFormat(target.format);
        size_t size = size_t(target.stride) * target.height * fmt.bytes;
        void* map = mmap(nullptr, size, PROT_READ, MAP_SHARED, target.fd.get(), 0);
        if (map == MAP_FAILED) return fail("map client target");
        dma_buf_sync sync = {DMA_BUF_SYNC_START | DMA_BUF_SYNC_READ};
        ioctl(target.fd.get(), DMA_BUF_IOCTL_SYNC, &sync);
        auto& frame = frames_[next_];
        for (int y = 0; y < kHeight; ++y) {
            const auto* src = static_cast<const uint8_t*>(map) + size_t(y) * target.stride * fmt.bytes;
            auto* dst = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(frame.map) + y * frame.pitch);
            // The normal SDR client targets need only R/B exchange, not
            // three variable-width divisions and memcpy per pixel. This
            // fixed-width loop vectorizes on AArch64, including padded rows.
            if (target.format == 1 || target.format == 2) {
                const auto* rgba = reinterpret_cast<const uint32_t*>(src);
                for (int x = 0; x < kWidth; ++x) {
                    uint32_t p = rgba[x];
                    dst[x] = ((p & 0xff) << 16) | (p & 0xff00) |
                             ((p >> 16) & 0xff);
                }
                continue;
            }
            if (target.format == 5) {
                memcpy(dst, src, size_t(kWidth) * sizeof(uint32_t));
                continue;
            }
            for (int x = 0; x < kWidth; ++x) {
                uint64_t pixel = 0;
                memcpy(&pixel, src + x * fmt.bytes, fmt.bytes);
                uint32_t channels[3];
                for (int c = 0; c < 3; ++c) {
                    const uint32_t mask = (1U << fmt.bits[c]) - 1;
                    uint32_t value = (pixel >> fmt.offsets[c]) & mask;
                    if (target.format == 0x16) {
                        __fp16 half;
                        uint16_t bits = value;
                        memcpy(&half, &bits, 2);
                        float v = float(half);
                        channels[c] = !(v > 0) ? 0 : v >= 1 ? 255 : uint32_t(v * 255 + 0.5f);
                    } else {
                        channels[c] = (value * 255 + mask / 2) / mask;
                    }
                }
                dst[x] = (channels[0] << 16) | (channels[1] << 8) | channels[2];
            }
        }
        sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_READ;
        ioctl(target.fd.get(), DMA_BUF_IOCTL_SYNC, &sync);
        munmap(map, size);
        drm_mode_fb_dirty_cmd dirty = {};
        dirty.fb_id = frame.fb;
        if (ioctl(fd_, DRM_IOCTL_MODE_DIRTYFB, &dirty)) return fail("flush scanout");
        drm_mode_crtc crtc = {};
        crtc.crtc_id = crtc_; crtc.fb_id = frame.fb;
        crtc.set_connectors_ptr = reinterpret_cast<uint64_t>(&connector_);
        crtc.count_connectors = 1; crtc.mode_valid = 1; crtc.mode = mode_;
        if (ioctl(fd_, DRM_IOCTL_MODE_SETCRTC, &crtc)) return fail("set scanout");
        next_ ^= 1;
        return true;
    }

  private:
    bool fail(const char* operation) {
        ALOGE("KMS %s failed: %s", operation, strerror(errno));
        return false;
    }
};
