#!/usr/bin/env bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
AOSP=$(cd "$HERE/../../.." && pwd)
install -m 0644 "$ROOT/out/keymint-compat/lib64/libkeymint_compat.so" \
    "$AOSP/qemu/vendor/lib64/libkeymint_compat.so"
# libkeymaster_portable.so from the newer prebuilt set needs the matching
# BoringSSL; the Android 13 GSI's libcrypto does not export OPENSSL_sk_new_null.
install -m 0644 "$AOSP/prebuilts/gsi-lib64/libcrypto.so" \
    "$AOSP/qemu/vendor/lib64/libcrypto.so"
