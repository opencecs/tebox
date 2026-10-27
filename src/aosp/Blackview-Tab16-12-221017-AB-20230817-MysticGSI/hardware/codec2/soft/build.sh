#!/usr/bin/env bash
# Build vendor Mesa Codec2 HAL (AIDL IComponentStore/default).
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$SCRIPT_DIR/../../../../../.." && pwd)
VARIANT=$(basename "$(cd "$SCRIPT_DIR/../../.." && pwd)")
# shellcheck source=scripts/env.sh
source "$ROOT/scripts/env.sh"
HAL="$ROOT/src/aosp/$VARIANT/hardware/codec2"
HI="$ROOT/thirdparty/hardware-interfaces"
GEN="$ROOT/out/codec2-ndk-gen"
OUT="$ROOT/out/codec2-stub"
GSI_LIBS="${GSI_LIBS:-$ROOT/src/aosp/$VARIANT/prebuilts/gsi-lib64}"
MESA="$ROOT/thirdparty/mesa/src"
GEN_INC="$OUT/mesa-gen-include"
API=34
CC=$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android${API}-clang++

C2_HASH=$(cat "$HI/media/c2/aidl/aidl_api/android.hardware.media.c2/1/.hash")
BP2_HASH=$(cat "$HI/media/bufferpool2/aidl/aidl_api/android.hardware.media.bufferpool2/2/.hash")
COMMON_HASH=$(cat "$HI/common/aidl/aidl_api/android.hardware.common/2/.hash")
FMQ_HASH=$(cat "$HI/common/fmq/aidl/aidl_api/android.hardware.common.fmq/1/.hash")

test -x "$CC" || { echo "missing $CC" >&2; exit 1; }
test -f "$GSI_LIBS/libbinder_ndk.so" || { echo "missing $GSI_LIBS/libbinder_ndk.so" >&2; exit 1; }

echo "ROOT=$ROOT VARIANT=$VARIANT"
echo "media.c2 hash=$C2_HASH bufferpool2 hash=$BP2_HASH"

mkdir -p "$GEN_INC/util/format" "$GEN/src" "$GEN/include" "$OUT/obj" "$OUT/bin"

if [[ ! -s "$GEN_INC/util/format/u_format_gen.h" || $(wc -l < "$GEN_INC/util/format/u_format_gen.h") -lt 100 ]]; then
  python3 "$MESA/src/util/format/u_format_table.py" \
    "$MESA/src/util/format/u_format.yaml" --enums \
    > "$GEN_INC/util/format/u_format_gen.h"
fi

echo "== generate AIDL NDK =="
rm -rf "$GEN"
mkdir -p "$GEN/src" "$GEN/include"

"$AIDL" --lang=ndk --structured --stability=vintf \
  --version=2 --hash="$COMMON_HASH" \
  -o "$GEN/src" -h "$GEN/include" \
  -I "$HI/common/aidl" \
  $(find "$HI/common/aidl/android" -name '*.aidl' | sort)

FMQ_FROZEN="$HI/common/fmq/aidl/aidl_api/android.hardware.common.fmq/1"
"$AIDL" --lang=ndk --structured --stability=vintf \
  --version=1 --hash="$FMQ_HASH" \
  -o "$GEN/src" -h "$GEN/include" \
  -I "$FMQ_FROZEN" \
  -I "$HI/common/aidl" \
  $(find "$FMQ_FROZEN/android" -name '*.aidl' | sort)

FW_AIDL="$HAL/aidl-framework"
# Only generate our structured Surface stub; HardwareBuffer/PFD use ndk_header.
"$AIDL" --lang=ndk --structured --stability=vintf \
  -o "$GEN/src" -h "$GEN/include" \
  -I "$FW_AIDL" \
  "$FW_AIDL/android/view/Surface.aidl"

"$AIDL" --lang=ndk --structured --stability=vintf \
  --version=2 --hash="$BP2_HASH" \
  -o "$GEN/src" -h "$GEN/include" \
  -I "$HI/media/bufferpool2/aidl" \
  -I "$HI/common/aidl" \
  -I "$FMQ_FROZEN" \
  -I "$FW_AIDL" \
  $(find "$HI/media/bufferpool2/aidl/android" -name '*.aidl' | sort)

"$AIDL" --lang=ndk --structured --stability=vintf \
  --version=1 --hash="$C2_HASH" \
  -o "$GEN/src" -h "$GEN/include" \
  -I "$HI/media/c2/aidl" \
  -I "$HI/media/bufferpool2/aidl" \
  -I "$HI/common/aidl" \
  -I "$FMQ_FROZEN" \
  -I "$FW_AIDL" \
  $(find "$HI/media/c2/aidl/android" -name '*.aidl' | sort)

test -f "$GEN/include/aidl/android/hardware/media/c2/IComponentStore.h" || {
  echo "ERROR: IComponentStore.h missing" >&2; exit 1;
}
echo "OK: Codec2 AIDL generated"

echo "== compile =="
rm -rf "$OUT/obj"
mkdir -p "$OUT/obj" "$OUT/bin"
OBJS=()
i=0
compile_one() {
  local src=$1
  local obj=$OUT/obj/o$i.o
  i=$((i + 1))
  echo "  CC $(basename "$src")"
  "$CC" -c "$src" -o "$obj" \
    -std=c++20 -fPIC -O2 \
    -DLOG_TAG='"mesa-c2"' \
    -DBINDER_STABILITY_SUPPORT \
    -I"$GEN/include" \
    -I"$GSI_LIBS/include" \
    -I"$ROOT/thirdparty/reference/audio-smoke-headers" \
    -I"$ROOT/thirdparty/common/minigbm/external" \
    -I"$ROOT/thirdparty/reference/hardware-interfaces/graphics/mapper/stable-c/include" \
    -I"$ANDROID_HEADERS" \
    -I"$HAL/soft/include" \
    -I"$HAL/soft" \
    -I"$MESA/src/gallium/include" \
    -I"$MESA/src/gallium/auxiliary" \
    -I"$MESA/src" \
    -I"$MESA/include" \
    -I"$MESA/src/virtio" \
    -I"$ROOT/prebuilts/android-arm64/libdrm/include" \
    -I"$GEN_INC" \
    -Wno-unused-parameter \
    -Wno-deprecated-declarations
  OBJS+=("$obj")
}

for f in \
  "$HAL/soft/mesa_pipe_video.cpp" \
  "$HAL/soft/Configurable.cpp" \
  "$HAL/soft/ComponentInterface.cpp" \
  "$HAL/soft/MesaAvcComponent.cpp" \
  "$HAL/soft/HostDecComponent.cpp" \
  "$HAL/soft/ComponentStore.cpp" \
  "$HAL/soft/service.cpp"; do
  compile_one "$f"
done
while IFS= read -r f; do
  compile_one "$f"
done < <(find "$GEN/src" -name '*.cpp' | sort)

echo "== link =="
BIN="$OUT/bin/android.hardware.media.c2-service"
NDK_LIB="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/sysroot/usr/lib/aarch64-linux-android/${API}"
"$CC" -o "$BIN" "${OBJS[@]}" \
  -L"$GSI_LIBS" \
  -L"$NDK_LIB" \
  -Wl,-rpath,/vendor/lib64 \
  -static-libstdc++ \
  -lbinder_ndk -llog -lcutils -ldl -lnativewindow -lz \
  -Wl,--allow-shlib-undefined

file "$BIN"
ls -lh "$BIN"
echo "OK $BIN"
