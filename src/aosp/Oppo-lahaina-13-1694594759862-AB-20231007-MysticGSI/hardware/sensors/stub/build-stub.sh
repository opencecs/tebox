#!/usr/bin/env bash
# Soft NDK Sensors HAL (ISensors AIDL v2).
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
source "$ROOT/scripts/env.sh"
HI="$ROOT/thirdparty/hardware-interfaces"
FROZEN="$HI/sensors/aidl/aidl_api/android.hardware.sensors/2"
COMMON="$HI/common/aidl/aidl_api/android.hardware.common/2"
FMQ="$HI/common/fmq/aidl/aidl_api/android.hardware.common.fmq/1"
OUT="$ROOT/out/sensors-stub"
GEN="$ROOT/out/sensors-ndk-gen"
CC="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android34-clang++"
HASH=$(cat "$FROZEN/.hash")
[[ -x "$CC" && -x "$AIDL" ]] || { echo 'missing NDK/aidl' >&2; exit 1; }
[[ -d "$FROZEN/android" ]] || { echo "missing $FROZEN" >&2; exit 1; }

rm -rf "$GEN" "$OUT/obj"
mkdir -p "$GEN/src" "$GEN/include" "$OUT/bin" "$OUT/obj"

INCLUDES=(-I "$COMMON" -I "$FMQ" -I "$FROZEN")
for package in "$COMMON" "$FMQ" "$FROZEN"; do
  sources=()
  while IFS= read -r source; do sources+=("$source"); done < <(find "$package" -name '*.aidl' | sort)
  ver=$(basename "$package")
  "$AIDL" --lang=ndk --structured --stability=vintf --min_sdk_version=34 \
    --version="$ver" --hash="$(cat "$package/.hash")" \
    -o "$GEN/src" -h "$GEN/include" "${INCLUDES[@]}" "${sources[@]}"
done

objects=()
while IFS= read -r src; do
  obj="$OUT/obj/$(echo "${src#"$GEN/src/"}" | tr / _).o"
  mkdir -p "$(dirname "$obj")"
  "$CC" -c "$src" -o "$obj" -std=c++20 -O2 -fPIC \
    -DBINDER_STABILITY_SUPPORT -DLOG_TAG='"qemu-sensors"' \
    -I"$GEN/include" -I"$ANDROID_HEADERS" -Wno-unused-parameter
  objects+=("$obj")
done < <(find "$GEN/src" -name '*.cpp' | sort)

"$CC" -c "$HERE/service.cpp" -o "$OUT/obj/service.o" -std=c++20 -O2 -fPIC \
  -DBINDER_STABILITY_SUPPORT -DLOG_TAG='"qemu-sensors"' \
  -I"$GEN/include" -I"$ANDROID_HEADERS" -Wno-unused-parameter

"$CC" -o "$OUT/bin/android.hardware.sensors-service" "$OUT/obj/service.o" "${objects[@]}" \
  -static-libstdc++ -L"$GSI_LIBS" -lbinder_ndk -llog -Wl,--allow-shlib-undefined
echo "Built $OUT/bin/android.hardware.sensors-service (ISensors hash=$HASH)"
