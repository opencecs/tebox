#!/usr/bin/env bash
# Install the HIDL audio HAL stub into the vendor staging tree.
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
AOSP=$(cd "$HERE/../../.." && pwd)
A13LIBS="${A13LIBS:-$ROOT/out/mystic-a13-libs}"
BIN="$ROOT/out/audio-hidl/bin/android.hardware.audio.hidl-service"
[[ -x "$BIN" ]] || { echo 'build the HIDL audio HAL first' >&2; exit 1; }
mkdir -p "$AOSP/qemu/vendor/bin/hw" "$AOSP/qemu/vendor/lib64"
install -m 0755 "$BIN" "$AOSP/qemu/vendor/bin/hw/"
# The vendor linker namespace searches /vendor/lib64 before the vndk apex, so
# ship the exact libhidlbase the service was linked against.
install -m 0644 "$A13LIBS/libhidlbase.so" "$AOSP/qemu/vendor/lib64/libhidlbase.so"
