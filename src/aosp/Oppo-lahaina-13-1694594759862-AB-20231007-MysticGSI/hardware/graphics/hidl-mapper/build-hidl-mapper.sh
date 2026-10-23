#!/usr/bin/env bash
# Build the HIDL IMapper@4.0 passthrough library for the Mystic GSI bring-up.
#
# Android 13 libui requires a passthrough android.hardware.graphics.mapper@4.0
# implementation; Gralloc4Mapper::preload() looks for a file named
# android.hardware.graphics.mapper@4.0-impl*.so under the sphal HAL dirs and
# dlopen()s HIDL_FETCH_IMapper from it.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../../../../../.." && pwd)"
VARIANT="$(basename "$(cd "$SCRIPT_DIR/../../.." && pwd)")"
# shellcheck source=scripts/env.sh
source "$ROOT/scripts/env.sh"

# Generated HIDL C++ headers (hidl-gen -L c++-headers). Kept in out/ so the
# build does not depend on /tmp; regenerate from the A13 source trees with the
# command in the header of mapper_impl.cpp if the interface changes.
GEN="${HIDL_GEN_OUT:-$ROOT/out/hidl-a13-gen}"
if [[ ! -d "$GEN/android/hardware/graphics/mapper/4.0" ]]; then
  if [[ -d /tmp/hidl-out/android/hardware/graphics/mapper/4.0 ]]; then
    mkdir -p "$GEN"
    cp -a /tmp/hidl-out/. "$GEN/"
  else
    echo "missing generated HIDL headers in $GEN (and /tmp/hidl-out)" >&2
    exit 1
  fi
fi

# A13 platform headers used to compile against the extracted vndk libraries.
A13SRC="${A13SRC:-/mnt/c/Users/llsai/AppData/Local/Temp/aosp-src}"
A13_INCLUDES=(
  "$A13SRC/libhidl_a13/base/include"
  "$A13SRC/libhidl_a13/transport/include"
  "$A13SRC/core_a13/libutils/include"
  "$A13SRC/core_a13/libcutils/include"
  "$A13SRC/libfmq_a13/base"
)
for d in "${A13_INCLUDES[@]}"; do
  [[ -d "$d" ]] || { echo "missing A13 header dir: $d" >&2; exit 1; }
done
# The platform C++ ABI namespace is std::__1; the NDK's libc++ uses
# std::__ndk1, which does not resolve against the A13 vndk libraries. Compile
# with the A13 libc++ headers instead of the NDK ones (-nostdinc++ below).
A13_LIBCXX="$A13SRC/libcxx_a13/include"
A13_LIBCXXABI="$A13SRC/libcxxabi_a13/include"
for d in "$A13_LIBCXX" "$A13_LIBCXXABI"; do
  [[ -d "$d" ]] || { echo "missing A13 libc++ header dir: $d" >&2; exit 1; }
done
# Not part of the IMapper include chain, but harmless when present.
for d in "$A13SRC/core_a13/libbase/include" "$A13SRC/core_a13/liblog/include"; do
  [[ -d "$d" ]] && A13_INCLUDES+=("$d")
done

A13LIBS="${A13LIBS:-$ROOT/out/mystic-a13-libs}"
for lib in libhidlbase.so android.hardware.graphics.mapper@4.0.so libutils.so \
           libcutils.so libbase.so liblog.so libc++.so; do
  [[ -f "$A13LIBS/$lib" ]] || { echo "missing $A13LIBS/$lib" >&2; exit 1; }
done

CC="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android33-clang++"
READELF="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/llvm-readelf"
test -x "$CC" || { echo "missing $CC" >&2; exit 1; }

OUT="$ROOT/out/graphics-stub/lib64/hw/android.hardware.graphics.mapper@4.0-impl.so"
mkdir -p "$(dirname "$OUT")"

INCS=(-I"$GEN" -I"$SCRIPT_DIR" -I"$SCRIPT_DIR/../stub")
# A13 platform headers must win over the newer thirdparty set: the vndk
# libhidl/libutils ABI matches these, not the A16 copies.
for d in "${A13_INCLUDES[@]}"; do INCS+=(-I"$d"); done
INCS+=(-I"$ANDROID_HEADERS"
       -I"$MESA_PREFIX/include" -I"$DRM_PREFIX/include" -I"$DRM_PREFIX/include/libdrm")

"$CC" -shared -o "$OUT" "$SCRIPT_DIR/mapper_impl.cpp" \
  -std=gnu++17 -fPIC -O2 -fno-exceptions -fno-rtti -nostdlib++ -nostdinc++ \
  -isystem "$A13_LIBCXX" -isystem "$A13_LIBCXXABI" \
  -D_LIBCPP_SUPPORT_XLOCALE_POSIX_L_FALLBACK_H \
  "${INCS[@]}" \
  -L"$A13LIBS" -L"$MESA_PREFIX/lib" \
  -Wl,-l:android.hardware.graphics.mapper@4.0.so \
  -Wl,-l:android.hardware.graphics.common@1.0.so \
  -Wl,-l:android.hardware.graphics.common@1.1.so \
  -Wl,-l:android.hardware.graphics.common@1.2.so \
  -lhidlbase -lutils -lcutils -lbase -llog -lc++ \
  -lgbm_mesa \
  -Wl,--allow-shlib-undefined \
  -Wl,-soname,android.hardware.graphics.mapper@4.0-impl.so \
  -Wno-unused-parameter

"$READELF" -d "$OUT" | grep NEEDED
if "$READELF" -d "$OUT" | grep -q 'libc++_shared'; then
  echo 'ERROR: mapper impl must not depend on libc++_shared.so' >&2
  exit 1
fi
# No grep -q here: it exits early and SIGPIPEs llvm-readelf, which pipefail
# turns into a false negative.
if ! "$READELF" -s "$OUT" | grep 'HIDL_FETCH_IMapper' > /dev/null; then
  echo 'ERROR: HIDL_FETCH_IMapper is not exported' >&2
  exit 1
fi
echo "built $OUT"
