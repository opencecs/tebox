#!/usr/bin/env bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
AOSP=$(cd "$HERE/../../.." && pwd)
install -m 0755 "$ROOT/out/keymint-soft/bin/android.hardware.security.keymint-service" \
    "$AOSP/qemu/vendor/bin/hw/android.hardware.security.keymint-service"
