#!/usr/bin/env bash
# Install the freshly built HAL into the staging tree. Stop QEMU before repacking.
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
RUNTIME="$HERE/../../../qemu/vendor"
BIN="$ROOT/out/audio-stub/bin/android.hardware.audio-service"
[[ -x "$BIN" ]] || { echo "build Audio HAL first" >&2; exit 1; }
mkdir -p "$RUNTIME/bin/hw" "$RUNTIME/etc/init" "$RUNTIME/etc/vintf/manifest"
cp -f "$BIN" "$RUNTIME/bin/hw/"
cp -f "$HERE/../init/android.hardware.audio-service.rc" "$RUNTIME/etc/init/"
cp -f "$HERE/../vintf/android.hardware.audio-service.xml" "$RUNTIME/etc/vintf/manifest/"
echo "installed Audio HAL in $RUNTIME"
