#!/usr/bin/env bash
# Rebuild only AIMAPPER5, preserving the installed allocator/composer service.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../../../../../.." && pwd)"
VARIANT="$(basename "$(cd "$SCRIPT_DIR/../../.." && pwd)")"
source "$ROOT/scripts/env.sh"
CC="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android34-clang++"
READELF="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/llvm-readelf"
OUT="$ROOT/out/graphics-stub/lib64/hw/mapper.stub.so"
mkdir -p "$(dirname "$OUT")"
"$CC" -shared -o "$OUT" "$SCRIPT_DIR/mapper_stub.cpp" \
  -std=c++20 -fPIC -O2 -fno-exceptions -fno-rtti -nostdlib++ \
  -I"$ROOT/thirdparty/hardware-interfaces/graphics/mapper/stable-c/include" \
  -I"$ANDROID_HEADERS" -I"$GSI_LIBS/include" \
  -I"$MESA_PREFIX/include" -I"$DRM_PREFIX/include" -I"$DRM_PREFIX/include/libdrm" \
  -L"$GSI_LIBS" -L"$MESA_PREFIX/lib" -llog -lcutils -lgbm_mesa \
  -Wl,--allow-shlib-undefined -Wl,-soname,mapper.stub.so -Wno-unused-parameter
"$READELF" -d "$OUT" | grep NEEDED
if "$READELF" -d "$OUT" | grep -q 'libc++_shared'; then
  echo 'ERROR: mapper must not depend on libc++_shared.so' >&2
  exit 1
fi
echo "built $OUT (install into vendor only after stopping QEMU)"
