#!/usr/bin/env bash
# Build a standalone NDK AIDL Audio HAL with the frozen Android 14 v1 ABI.
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$SCRIPT_DIR/../../../../../.." && pwd)
source "$ROOT/scripts/env.sh"
GEN="$ROOT/out/audio-ndk-gen"
OUT="$ROOT/out/audio-stub"
HI="$ROOT/thirdparty/hardware-interfaces"
MEDIA="$ROOT/thirdparty/audio-deps/media/aidl_api/android.media.audio.common.types/2"
CC="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android34-clang++"
PACKAGES=(
  "$HI/common/aidl/aidl_api/android.hardware.common/2"
  "$HI/common/fmq/aidl/aidl_api/android.hardware.common.fmq/1"
  "$MEDIA"
  "$HI/audio/aidl/aidl_api/android.hardware.audio.common/2"
  "$HI/audio/aidl/aidl_api/android.hardware.audio.core.sounddose/1"
  "$HI/audio/aidl/aidl_api/android.hardware.audio.effect/1"
  "$HI/audio/aidl/aidl_api/android.hardware.audio.core/1"
)
INCLUDES=()
for package in "${PACKAGES[@]}"; do
  [[ -s "$package/.hash" ]] || { echo "missing frozen AIDL: $package" >&2; exit 1; }
  INCLUDES+=(-I "$package")
done
[[ -x "$CC" && -x "$AIDL" ]] || { echo 'missing NDK compiler or aidl' >&2; exit 1; }
mkdir -p "$GEN/src" "$GEN/include" "$OUT/bin" "$OUT/obj"
for package in "${PACKAGES[@]}"; do
  sources=()
  while IFS= read -r source; do sources+=("$source"); done < <(find "$package" -name '*.aidl' | sort)
  "$AIDL" --lang=ndk --structured --stability=vintf --min_sdk_version=34 \
    --version="$(basename "$package")" --hash="$(cat "$package/.hash")" \
    -o "$GEN/src" -h "$GEN/include" "${INCLUDES[@]}" "${sources[@]}"
done
python3 "$SCRIPT_DIR/generate-defaults.py" "$GEN/include"
objects=()
while IFS= read -r source; do
  relative="${source#"$GEN/src/"}"
  object="$OUT/obj/${relative//\//_}.o"
  "$CC" -c "$source" -o "$object" -std=c++20 -O2 -fPIC \
    -DBINDER_STABILITY_SUPPORT -I"$GEN/include" -I"$SCRIPT_DIR" -I"$ANDROID_HEADERS" \
    -Wall -Wextra -Wno-unused-parameter
  objects+=("$object")
done < <(find "$GEN/src" -name '*.cpp' | sort)
"$CC" -c "$SCRIPT_DIR/service.cpp" -o "$OUT/obj/service.o" \
  -std=c++20 -O2 -fPIC -DBINDER_STABILITY_SUPPORT -I"$GEN/include" -I"$SCRIPT_DIR" -I"$ANDROID_HEADERS" \
  -Wall -Wextra -Wno-unused-parameter
"$CC" -o "$OUT/bin/android.hardware.audio-service" "$OUT/obj/service.o" "${objects[@]}" \
  -static-libstdc++ -L"$GSI_LIBS" -lbinder_ndk -llog -Wl,--allow-shlib-undefined
echo "built $OUT/bin/android.hardware.audio-service"
