# Virtual camera HAL

This is a legacy Camera HAL1 module for the Android 14 GSI. It exposes one
back-facing camera (`camera.default.so`) and produces deterministic 640x480
NV21 frames through the framework-owned camera memory callback. The frame is a
real YUV buffer, not a fabricated size marker or metadata-only result.

The module is intentionally hardware-independent: QEMU has no camera device,
so the provider generates a moving test pattern suitable for preview, video,
and raw-image smoke tests. Build/install it with `scripts/build-hals.sh`.
