  // Auto-generated unsupported defaults (BnComposerClient provides getInterface*).
  ndk::ScopedAStatus getDisplayedContentSample(int64_t, int64_t, int64_t,
                                               DisplayContentSample*) override {
    return ndk::ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
  }
  ndk::ScopedAStatus getDisplayedContentSamplingAttributes(
          int64_t, DisplayContentSamplingAttributes*) override {
    return ndk::ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
  }
  ndk::ScopedAStatus getReadbackBufferAttributes(int64_t,
                                                 ReadbackBufferAttributes*) override {
    return ndk::ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
  }
  ndk::ScopedAStatus getReadbackBufferFence(int64_t, ndk::ScopedFileDescriptor*) override {
    return ndk::ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
  }
  ndk::ScopedAStatus setAutoLowLatencyMode(int64_t, bool) override {
    return ndk::ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
  }
  ndk::ScopedAStatus setContentType(int64_t, ContentType) override {
    return ndk::ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
  }
  ndk::ScopedAStatus setDisplayedContentSamplingEnabled(int64_t, bool, FormatColorComponent,
                                                        int64_t) override {
    return ndk::ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
  }
  ndk::ScopedAStatus setReadbackBuffer(int64_t, const NativeHandle&,
                                       const ndk::ScopedFileDescriptor&) override {
    return ndk::ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
  }
  ndk::ScopedAStatus setIdleTimerEnabled(int64_t, int32_t) override {
    return ndk::ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
  }
  ndk::ScopedAStatus getHdrConversionCapabilities(
          std::vector<::aidl::android::hardware::graphics::common::HdrConversionCapability>*)
          override {
    return ndk::ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
  }
  ndk::ScopedAStatus setHdrConversionStrategy(
          const ::aidl::android::hardware::graphics::common::HdrConversionStrategy&,
          ::aidl::android::hardware::graphics::common::Hdr*) override {
    return ndk::ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
  }
  ndk::ScopedAStatus setRefreshRateChangedCallbackDebugEnabled(int64_t, bool) override {
    return ndk::ScopedAStatus::ok();
  }
  ndk::ScopedAStatus startHdcpNegotiation(int64_t,
                                          const ::aidl::android::hardware::drm::HdcpLevels&)
          override {
    return ndk::ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
  }
  ndk::ScopedAStatus getLuts(int64_t, const std::vector<Buffer>&,
                             std::vector<Luts>*) override {
    return ndk::ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_UNSUPPORTED);
  }
