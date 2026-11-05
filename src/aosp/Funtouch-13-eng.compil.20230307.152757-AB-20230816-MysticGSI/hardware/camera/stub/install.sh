#!/usr/bin/env bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
VARIANT=${VARIANT:-aosp_arm64-BP4A.251205.006}
DEST="$ROOT/src/aosp/$VARIANT/qemu/vendor/lib64/hw"
mkdir -p "$DEST"
install -m 0755 "$ROOT/out/camera-stub/lib64/camera.default.so" "$DEST/camera.default.so"
