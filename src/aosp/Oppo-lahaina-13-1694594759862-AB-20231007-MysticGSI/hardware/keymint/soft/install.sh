#!/usr/bin/env bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
AOSP=$(cd "$HERE/../../.." && pwd)
install -m 0755 "$ROOT/out/keymint-soft/bin/android.hardware.security.keymint-service" \
    "$AOSP/qemu/vendor/bin/hw/android.hardware.security.keymint-service"
# The AIDL security libraries must match the GSI's libbinder_ndk. The vendor
# tree was cloned from the Android 16 variant and carries its
# sharedsecret/secureclock libs, which reference
# AIBinder_Class_setTransactionCodeToFunctionNameMap@LIBBINDER_NDK36; the
# Android 13 GSI's libbinder_ndk stops at LIBBINDER_NDK31. Use the copies
# extracted from this variant's system.img instead.
for lib in android.hardware.security.sharedsecret-V1-ndk.so \
           android.hardware.security.secureclock-V1-ndk.so; do
    install -m 0644 "$AOSP/prebuilts/gsi-lib64/$lib" "$AOSP/qemu/vendor/lib64/$lib"
done
