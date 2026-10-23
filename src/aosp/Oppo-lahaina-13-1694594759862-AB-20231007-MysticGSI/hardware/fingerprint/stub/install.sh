#!/usr/bin/env bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
AOSP=$(cd "$HERE/../../.." && pwd)
STUB="$AOSP/qemu/vendor"
mkdir -p "$STUB/bin/hw" "$STUB/etc/init" "$STUB/etc/vintf/manifest" "$STUB/etc/permissions"
install -m 0755 "$ROOT/out/fingerprint-stub/bin/android.hardware.biometrics.fingerprint-service" \
  "$STUB/bin/hw/"
# The service rc, its VINTF fragment and the device-manifest <hal> entry are
# intentionally not installed. The Android 13 ColorOS framework invokes
# IFingerprintService.registerAuthenticators() more than once, re-registering
# sensor 0 each time; BiometricService throws "Cannot register duplicate
# authenticator" inside system_server, which kills it during boot. The binary
# stays staged so the HAL can be re-enabled once that framework path is fixed.
FC="$STUB/etc/selinux/vendor_file_contexts"
if [[ -f "$FC" ]] && ! grep -q 'android\.hardware\.biometrics\.fingerprint-service' "$FC"; then
  echo '/vendor/bin/hw/android\.hardware\.biometrics\.fingerprint-service    u:object_r:hal_keymint_system_exec:s0' >> "$FC"
fi
