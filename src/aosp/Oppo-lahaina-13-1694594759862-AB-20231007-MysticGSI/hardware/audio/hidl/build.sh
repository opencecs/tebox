#!/usr/bin/env bash
# Build the HIDL audio HAL stub used by the Android 13 GSI.
#
# The Android 13 GSI audioserver only speaks the HIDL audio HAL
# (android.hardware.audio@7.1 and android.hardware.audio.effect@7.0); the AIDL
# stub under ../stub is for the Android 16 variant and is invisible to it.
#
# Generated HIDL C++ headers/sources are produced with hidl-gen from the
# Android 13 hardware/interfaces and libhidl sources and kept under out/.
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
# shellcheck source=scripts/env.sh
source "$ROOT/scripts/env.sh"

# Same A13 source snapshot the HIDL mapper build uses.
A13SRC="${A13SRC:-/mnt/c/Users/llsai/AppData/Local/Temp/aosp-src}"
HWI="$A13SRC/hwi_a13"
LIBHIDL="$A13SRC/libhidl_a13/transport"
GEN="${HIDL_AUDIO_GEN:-$ROOT/out/hidl-audio-gen}"
SRC="${HIDL_AUDIO_SRC:-$ROOT/out/hidl-audio-src}"
A13LIBS="${A13LIBS:-$ROOT/out/mystic-a13-libs}"
OUT="$ROOT/out/audio-hidl"

PACKAGES=(
  android.hardware.audio@7.1
  android.hardware.audio@7.0
  android.hardware.audio.common@7.0
  android.hardware.audio.common@7.1
  android.hardware.audio.effect@7.0
)

if [[ ! -f "$GEN/android/hardware/audio/7.1/IDevicesFactory.h" ]]; then
  command -v hidl-gen >/dev/null || { echo 'hidl-gen is not installed' >&2; exit 1; }
  [[ -d "$HWI" && -d "$LIBHIDL" ]] || {
    echo "missing Android 13 hardware/interfaces or libhidl sources under $A13SRC" >&2
    exit 1
  }
  echo "generating HIDL headers/sources in $GEN and $SRC"
  rm -rf "$GEN" "$SRC"
  mkdir -p "$GEN" "$SRC"
  # Headers are also needed for android.hidl.base@1.0 and
  # android.hidl.manager@1.0, which the generated code includes; their
  # implementations live in libhidlbase, so only generate headers for them and
  # compile the audio packages' sources.
  hidl-gen -o "$GEN" -L c++-headers \
    -r android.hardware:"$HWI" -r android.hidl:"$LIBHIDL" \
    "${PACKAGES[@]}" android.hidl.base@1.0 android.hidl.manager@1.0 \
    android.hidl.safe_union@1.0
  hidl-gen -o "$SRC" -L c++-sources \
    -r android.hardware:"$HWI" -r android.hidl:"$LIBHIDL" "${PACKAGES[@]}"
  # The distro hidl-gen (10.0.0) predates C++17 inline variable templates: it
  # emits "template<> constexpr std::array<...> hidl_enum_values<...>", which
  # gives every translation unit a strong definition. Android 13's hidl-gen
  # emits "template<> inline constexpr ..." instead. Add the inline so the
  # generated code is ODR-clean and links.
  grep -rl 'template<> constexpr std::array<' "$GEN" | while IFS= read -r header; do
    sed -i 's/template<> constexpr std::array</template<> inline constexpr std::array</' "$header"
  done
fi

A13_LIBCXX="$A13SRC/libcxx_a13/include"
A13_LIBCXXABI="$A13SRC/libcxxabi_a13/include"
for d in "$A13_LIBCXX" "$A13_LIBCXXABI" "$A13SRC/libhidl_a13/base/include" \
         "$A13SRC/libhidl_a13/transport/include" "$A13SRC/hwbinder_a13" \
         "$A13SRC/core_a13/libutils/include" \
         "$A13SRC/core_a13/libcutils/include" "$A13SRC/log_a13/log" \
         "$A13SRC/libsystem_a13/system" "$A13SRC/libbase_a13"; do
  [[ -d "$d" ]] || { echo "missing A13 header dir: $d" >&2; exit 1; }
done
for lib in libhidlbase.so libutils.so libbase.so libcutils.so libc++.so; do
  [[ -f "$A13LIBS/$lib" ]] || { echo "missing $A13LIBS/$lib" >&2; exit 1; }
done

CC="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android33-clang++"
[[ -x "$CC" ]] || { echo "missing $CC" >&2; exit 1; }

mkdir -p "$OUT/bin" "$OUT/obj"
INCS=(
  -I"$GEN"
  -I"$HERE"
  -I"$A13SRC/log_a13"
  -I"$A13SRC/libhidl_a13/base/include"
  -I"$A13SRC/libhidl_a13/transport/include"
  -I"$A13SRC/hwbinder_a13"
  -I"$A13SRC/core_a13/libutils/include"
  -I"$A13SRC/core_a13/libcutils/include"
  -I"$A13SRC/libsystem_a13"
  -I"$A13SRC/libbase_a13"
  -I"$ANDROID_HEADERS"
)
[[ -d "$A13SRC/libfmq_a13/base" ]] && INCS+=(-I"$A13SRC/libfmq_a13/base")

# The platform C++ ABI namespace is std::__1; the NDK's libc++ uses
# std::__ndk1, which does not resolve against the A13 vndk libraries.
# Compile with the A13 libc++ headers instead of the NDK ones.
COMMON_FLAGS=(
  -std=gnu++17 -fPIC -O2 -fno-exceptions -fno-rtti
  -nostdlib++ -nostdinc++
  -isystem "$A13_LIBCXX" -isystem "$A13_LIBCXXABI"
  -D_LIBCPP_SUPPORT_XLOCALE_POSIX_L_FALLBACK_H
  # hidl-gen output uses gServicePrioMap/gServiceSidMap, which the AOSP build
  # pulls in through hidl/InternalStatic.h; libhidlbase exports both.
  -include "$LIBHIDL/InternalStatic.h"
  -Wall -Wextra -Wno-unused-parameter
  "${INCS[@]}"
)

objects=()
while IFS= read -r source; do
  relative="${source#"$SRC/"}"
  object="$OUT/obj/${relative//\//_}.o"
  "$CC" -c "$source" -o "$object" "${COMMON_FLAGS[@]}"
  objects+=("$object")
done < <(find "$SRC" -name '*.cpp' | sort)
"$CC" -c "$HERE/service.cpp" -o "$OUT/obj/service.o" "${COMMON_FLAGS[@]}"

"$CC" -o "$OUT/bin/android.hardware.audio.hidl-service" \
  "$OUT/obj/service.o" "${objects[@]}" \
  -L"$A13LIBS" -lhidlbase -lutils -lbase -lcutils -llog -lc++ \
  -Wl,-rpath,/vendor/lib64

if readelf -d "$OUT/bin/android.hardware.audio.hidl-service" | grep -q 'libc++_shared'; then
  echo 'ERROR: audio HAL stub must not depend on libc++_shared.so' >&2
  exit 1
fi
echo "built $OUT/bin/android.hardware.audio.hidl-service"
