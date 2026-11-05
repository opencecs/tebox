#!/usr/bin/env bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
source "$ROOT/scripts/env.sh"
HI="$ROOT/thirdparty/hardware-interfaces"
GEN="$ROOT/out/camera-aidl-gen"
OUT="$ROOT/out/camera-aidl-provider"
CC="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android34-clang++"
CCC="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android34-clang"
CAMMETA="$ROOT/thirdparty/reference/aosp-audio/system-media/camera"
FMQ="$ROOT/thirdparty/reference/aosp-audio/libfmq"
rm -rf "$GEN" "$OUT"
mkdir -p "$GEN/src" "$GEN/include" "$OUT/bin" "$OUT/obj"
IN=(-I "$HI/common/aidl/aidl_api/android.hardware.common/2"
    -I "$HI/common/fmq/aidl/aidl_api/android.hardware.common.fmq/1"
    -I "$HI/graphics/common/aidl/aidl_api/android.hardware.graphics.common/1"
    -I "$HI/camera/common/aidl/aidl_api/android.hardware.camera.common/1"
    -I "$HI/camera/metadata/aidl/aidl_api/android.hardware.camera.metadata/3"
    -I "$HI/camera/device/aidl/aidl_api/android.hardware.camera.device/3"
    -I "$HI/camera/provider/aidl_api/android.hardware.camera.provider/2")
for d in "$HI/common/aidl/aidl_api/android.hardware.common/2" \
         "$HI/common/fmq/aidl/aidl_api/android.hardware.common.fmq/1" \
         "$HI/graphics/common/aidl/aidl_api/android.hardware.graphics.common/1" \
         "$HI/camera/common/aidl/aidl_api/android.hardware.camera.common/1" \
         "$HI/camera/metadata/aidl/aidl_api/android.hardware.camera.metadata/3" \
         "$HI/camera/device/aidl/aidl_api/android.hardware.camera.device/3" \
         "$HI/camera/provider/aidl_api/android.hardware.camera.provider/2"; do
  "$AIDL" --lang=ndk --structured --stability=vintf --min_sdk_version=34 \
    --version="$(basename "$d")" --hash="$(cat "$d/.hash")" \
    -o "$GEN/src" -h "$GEN/include" "${IN[@]}" $(find "$d" -name '*.aidl' | sort)
done
objs=()
"$CCC" -c "$CAMMETA/src/camera_metadata.c" -o "$OUT/obj/camera_metadata.o" -std=c11 -O2 -fPIC \
  -I"$CAMMETA/include" -I"$ROOT/thirdparty/reference/aosp-audio/system-media/private/camera/include" \
  -I"$ROOT/thirdparty/mesa/src/include/android_stub" -I"$ANDROID_HEADERS" -DANDROID
objs+=("$OUT/obj/camera_metadata.o")
while IFS= read -r src; do
  obj="$OUT/obj/$(basename "$src").o"
  "$CC" -c "$src" -o "$obj" -std=c++20 -O2 -fPIC -DBINDER_STABILITY_SUPPORT \
    -I"$GEN/include" -I"$ANDROID_HEADERS" -Wno-unused-parameter
  objs+=("$obj")
done < <(find "$GEN/src" -name '*.cpp' | sort)
for src in "$FMQ/EventFlag.cpp" "$FMQ/FmqInternal.cpp"; do
  obj="$OUT/obj/$(basename "$src").o"
  "$CC" -c "$src" -o "$obj" -std=c++20 -O2 -fPIC \
    -I"$FMQ/include" -I"$FMQ/base" -I"$ANDROID_HEADERS" -I"$ROOT/thirdparty/reference/audio-smoke-headers" \
    -Wno-unused-parameter
  objs+=("$obj")
done
"$CC" -c "$HERE/fmq_compat.cpp" -o "$OUT/obj/fmq_compat.o" -std=c++20 -O2 -fPIC
objs+=("$OUT/obj/fmq_compat.o")
"$CC" -c "$HERE/service.cpp" -o "$OUT/obj/service.o" -std=c++20 -O2 -fPIC \
  -DBINDER_STABILITY_SUPPORT -I"$GEN/include" -I"$ANDROID_HEADERS" -I"$CAMMETA/include" \
  -I"$ROOT/thirdparty/mesa/src/include/android_stub" -I"$FMQ/include" -I"$FMQ/base" \
  -I"$ROOT/thirdparty/reference/audio-smoke-headers"
"$CC" -o "$OUT/bin/android.hardware.camera.provider-service" "$OUT/obj/service.o" "${objs[@]}" \
  -static-libstdc++ -L"$GSI_LIBS" -lbinder_ndk -llog -lcutils -lutils -lbase -Wl,--allow-shlib-undefined
file "$OUT/bin/android.hardware.camera.provider-service"
