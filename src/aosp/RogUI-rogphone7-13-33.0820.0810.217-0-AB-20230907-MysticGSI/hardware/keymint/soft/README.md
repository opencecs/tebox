# QEMU software KeyMint

This service registers the AOSP software KeyMint, SecureClock and SharedSecret
implementations already present in this variant's `system.img`. It replaces the
bring-up stub whose `importKey`/`begin` methods returned `UNIMPLEMENTED`, causing
LockSettingsService to crash while creating a synthetic password.

The default instance emulates the TEE slot expected by keystore2. All operations
run in software; this provides no hardware key isolation or hardware attestation.
The image's own software implementation controls key blob storage and crypto.

Run from the repository root:

```sh
bash src/aosp/aosp_arm64-BP4A.251205.006/hardware/keymint/soft/build.sh
bash src/aosp/aosp_arm64-BP4A.251205.006/hardware/keymint/soft/install.sh
bash scripts/build-vendor-img.sh
bash scripts/build-initramfs.sh
```

Requires the local NDK, AIDL compiler and Binder NDK headers in
`thirdparty/android-headers/include`. The build extracts matching libraries from `system.img`
without modifying it. The small launcher uses the platform libc++ ABI (`__1`)
and the frozen KeyMint v4 / SecureClock v1 / SharedSecret v1 interfaces.
It is specific to this GSI's `libkeymint_fake_latest.so` and the matching
`thirdparty/system-keymaster/ng/include` headers.

`include/android/binder_shell.h` is the platform header from
LineageOS/android_frameworks_native (lineage-23.0). It must be visible when
compiling: omitting it removes `ICInterface::handleShellCommand` from the vtable
and makes a platform `createBinder()` call dispatch to the wrong method.
The QEMU init configuration also supplies `ro.vendor.build.security_patch`
before the service starts, using the patch level of the GSI providing the code.
