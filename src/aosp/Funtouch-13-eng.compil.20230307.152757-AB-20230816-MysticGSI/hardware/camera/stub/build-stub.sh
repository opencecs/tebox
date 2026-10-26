#!/usr/bin/env bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
source "$ROOT/scripts/env.sh"
OUT="$ROOT/out/camera-stub"
CC="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android34-clang"
[[ -x "$CC" ]] || { echo 'missing NDK compiler' >&2; exit 1; }
mkdir -p "$OUT/lib64"
"$CC" -shared -fPIC -O2 -std=c11 -D_GNU_SOURCE \
  -I"$HERE/../include" -I"$ROOT/thirdparty/mesa/src/include/android_stub" \
  "$HERE/camera.c" -o "$OUT/lib64/camera.default.so" -pthread
echo "Built $OUT/lib64/camera.default.so"
