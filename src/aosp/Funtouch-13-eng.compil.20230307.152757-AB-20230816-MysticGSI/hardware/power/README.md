# QEMU Power HAL

Minimal frozen AIDL v1 `IPower/default` service. All boost/mode capability queries
return false; requests are no-ops because the host schedules QEMU's virtual CPUs.
Performance sessions and headroom telemetry are not advertised. Android 16's
HintManagerService requires a Power instance to construct its fallback support
information, even when none of those optional capabilities exists.

```sh
bash src/aosp/aosp_arm64-BP4A.251205.006/hardware/power/stub/build-stub.sh
bash src/aosp/aosp_arm64-BP4A.251205.006/hardware/power/stub/install.sh
bash scripts/build-vendor-img.sh
bash scripts/build-initramfs.sh
```

Frozen interface source: LineageOS/android_hardware_interfaces, lineage-23.0,
`power/aidl/aidl_api/android.hardware.power/1` (AOSP interface snapshot).
