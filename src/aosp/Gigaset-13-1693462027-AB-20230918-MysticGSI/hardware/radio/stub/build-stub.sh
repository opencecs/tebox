#!/usr/bin/env bash
# Soft NDK Radio/GSM HAL (AIDL v3 config + slot1 modem/network/sim/data/messaging/voice).
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
source "$ROOT/scripts/env.sh"
HI="$ROOT/thirdparty/hardware-interfaces"
OUT="$ROOT/out/radio-stub"
GEN="$ROOT/out/radio-ndk-gen"
CC="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android34-clang++"
[[ -x "$CC" && -x "$AIDL" ]] || { echo 'missing NDK/aidl' >&2; exit 1; }

PACKAGES=(
  "$HI/radio/aidl/aidl_api/android.hardware.radio/3"
  "$HI/radio/aidl/aidl_api/android.hardware.radio.config/3"
  "$HI/radio/aidl/aidl_api/android.hardware.radio.modem/3"
  "$HI/radio/aidl/aidl_api/android.hardware.radio.network/3"
  "$HI/radio/aidl/aidl_api/android.hardware.radio.sim/3"
  "$HI/radio/aidl/aidl_api/android.hardware.radio.data/3"
  "$HI/radio/aidl/aidl_api/android.hardware.radio.messaging/3"
  "$HI/radio/aidl/aidl_api/android.hardware.radio.voice/3"
)

rm -rf "$GEN" "$OUT/obj"
mkdir -p "$GEN/src" "$GEN/include" "$OUT/bin" "$OUT/obj"

INCLUDES=()
for package in "${PACKAGES[@]}"; do
  [[ -s "$package/.hash" ]] || { echo "missing $package" >&2; exit 1; }
  INCLUDES+=(-I "$package")
done

for package in "${PACKAGES[@]}"; do
  sources=()
  while IFS= read -r source; do sources+=("$source"); done < <(find "$package" -name '*.aidl' | sort)
  "$AIDL" --lang=ndk --structured --stability=vintf --min_sdk_version=34 \
    --version=3 --hash="$(cat "$package/.hash")" \
    -o "$GEN/src" -h "$GEN/include" "${INCLUDES[@]}" "${sources[@]}"
done

python3 "$HERE/generate-stubs.py" "$GEN/include" "$GEN/include/radio_stubs.h"

objects=()
while IFS= read -r src; do
  obj="$OUT/obj/$(echo "${src#"$GEN/src/"}" | tr / _).o"
  mkdir -p "$(dirname "$obj")"
  "$CC" -c "$src" -o "$obj" -std=c++20 -O2 -fPIC \
    -DBINDER_STABILITY_SUPPORT -DLOG_TAG='"qemu-radio"' \
    -I"$GEN/include" -I"$ANDROID_HEADERS" -Wno-unused-parameter
  objects+=("$obj")
done < <(find "$GEN/src" -name '*.cpp' | sort)

"$CC" -c "$HERE/service.cpp" -o "$OUT/obj/service.o" -std=c++20 -O2 -fPIC \
  -DBINDER_STABILITY_SUPPORT -DLOG_TAG='"qemu-radio"' \
  -I"$GEN/include" -I"$HERE" -I"$ANDROID_HEADERS" -Wno-unused-parameter

"$CC" -o "$OUT/bin/android.hardware.radio-service" "$OUT/obj/service.o" "${objects[@]}" \
  -static-libstdc++ -L"$GSI_LIBS" -lbinder_ndk -llog -Wl,--allow-shlib-undefined
echo "Built $OUT/bin/android.hardware.radio-service"
