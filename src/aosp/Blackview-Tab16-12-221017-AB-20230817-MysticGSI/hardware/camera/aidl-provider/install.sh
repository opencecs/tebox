#!/usr/bin/env bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
DEST="$ROOT/src/aosp/aosp_arm64-BP4A.251205.006/qemu/vendor"
mkdir -p "$DEST/bin/hw" "$DEST/etc/init" "$DEST/etc/vintf/manifest"
install -m 0755 "$ROOT/out/camera-aidl-provider/bin/android.hardware.camera.provider-service" \
  "$DEST/bin/hw/android.hardware.camera.provider-service"
install -m 0644 "$HERE/android.hardware.camera.provider-service.rc" \
  "$DEST/etc/init/android.hardware.camera.provider-service.rc"
install -m 0644 "$HERE/android.hardware.camera.provider-service.xml" \
  "$DEST/etc/vintf/manifest/android.hardware.camera.provider-service.xml"
printf '/vendor/bin/hw/android\\.hardware\\.camera\\.provider-service    u:object_r:hal_keymint_system_exec:s0\n' \
  >> "$DEST/etc/selinux/vendor_file_contexts"
