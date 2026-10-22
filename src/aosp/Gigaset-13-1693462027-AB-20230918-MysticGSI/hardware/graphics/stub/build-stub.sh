#!/usr/bin/env bash
# Build soft NDK graphics HAL stub: IComposer (composer3 v3) + IAllocator (v2).
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$SCRIPT_DIR/../../../../../.." && pwd)
VARIANT=$(basename "$(cd "$SCRIPT_DIR/../../.." && pwd)")
# shellcheck source=scripts/env.sh
source "$ROOT/scripts/env.sh"
HAL=$ROOT/src/aosp/$VARIANT/hardware/graphics
GEN=$ROOT/out/graphics-ndk-gen
OUT=$ROOT/out/graphics-stub
GSI_LIBS="${GSI_LIBS:-$ROOT/src/aosp/$VARIANT/prebuilts/gsi-lib64}"
HI=$ROOT/thirdparty/hardware-interfaces
API=34
CC=$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android${API}-clang++
NM=$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/llvm-nm

COMPOSER_HASH=$(cat "$HI/graphics/composer/aidl/aidl_api/android.hardware.graphics.composer3/3/.hash")
ALLOCATOR_HASH=$(cat "$HI/graphics/allocator/aidl/aidl_api/android.hardware.graphics.allocator/2/.hash")
COMMON_HASH=$(cat "$HI/common/aidl/aidl_api/android.hardware.common/2/.hash")
GCOMMON_HASH=$(cat "$HI/graphics/common/aidl/aidl_api/android.hardware.graphics.common/5/.hash")
DRM_HASH=$(cat "$HI/drm/common/aidl/aidl_api/android.hardware.drm.common/1/.hash")

test -x "$CC" || { echo "missing $CC" >&2; exit 1; }
test -f "$GSI_LIBS/libbinder_ndk.so" || { echo "missing $GSI_LIBS/libbinder_ndk.so" >&2; exit 1; }

echo "ROOT=$ROOT VARIANT=$VARIANT"
echo "composer3 V3 hash=$COMPOSER_HASH"
echo "allocator V2 hash=$ALLOCATOR_HASH"

echo "== generate AIDL NDK =="
rm -rf "$GEN"
mkdir -p "$GEN/src" "$GEN/include" "$OUT" "$ROOT/out"

# Supporting packages first (parcelables/enums referenced by composer/allocator).
"$AIDL" --lang=ndk --structured --stability=vintf \
  --version=2 --hash="$COMMON_HASH" \
  -o "$GEN/src" -h "$GEN/include" \
  -I "$HI/common/aidl" \
  $(find "$HI/common/aidl/android" -name '*.aidl' | sort)

"$AIDL" --lang=ndk --structured --stability=vintf \
  --version=5 --hash="$GCOMMON_HASH" \
  -o "$GEN/src" -h "$GEN/include" \
  -I "$HI/graphics/common/aidl" \
  -I "$HI/common/aidl" \
  $(find "$HI/graphics/common/aidl/android" -name '*.aidl' | sort)

"$AIDL" --lang=ndk --structured --stability=vintf \
  --version=1 --hash="$DRM_HASH" \
  -o "$GEN/src" -h "$GEN/include" \
  -I "$HI/drm/common/aidl" \
  $(find "$HI/drm/common/aidl/android" -name '*.aidl' | sort)

"$AIDL" --lang=ndk --structured --stability=vintf \
  --version=3 --hash="$COMPOSER_HASH" \
  -o "$GEN/src" -h "$GEN/include" \
  -I "$HI/graphics/composer/aidl" \
  -I "$HI/graphics/common/aidl" \
  -I "$HI/common/aidl" \
  -I "$HI/drm/common/aidl" \
  $(find "$HI/graphics/composer/aidl/android" -name '*.aidl' | sort)

"$AIDL" --lang=ndk --structured --stability=vintf \
  --version=2 --hash="$ALLOCATOR_HASH" \
  -o "$GEN/src" -h "$GEN/include" \
  -I "$HI/graphics/allocator/aidl" \
  -I "$HI/graphics/common/aidl" \
  -I "$HI/common/aidl" \
  $(find "$HI/graphics/allocator/aidl/android" -name '*.aidl' | sort)

if ! grep -q 'static inline const int32_t version = 3' \
  "$GEN/include/aidl/android/hardware/graphics/composer3/IComposer.h"; then
  echo "ERROR: IComposer version != 3" >&2
  exit 1
fi
test -f "$GEN/include/aidl/android/hardware/common/NativeHandle.h" || {
  echo "ERROR: NativeHandle.h missing" >&2; exit 1;
}
echo "OK: IComposer v3 + NativeHandle present"

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
    -DLOG_TAG='"graphics-stub"' \
    -DBINDER_STABILITY_SUPPORT \
    -I"$GEN/include" \
    -I"$HAL/stub" \
    -I"$ANDROID_HEADERS" \
    -I"$MESA_PREFIX/include" \
    -I"$DRM_PREFIX/include" -I"$DRM_PREFIX/include/libdrm" \
    -I"$GSI_LIBS/include" \
    -Wno-unused-parameter \
    -Wno-deprecated-declarations
  OBJS+=("$obj")
}

compile_one "$HAL/stub/service_stub.cpp"
while IFS= read -r f; do
  compile_one "$f"
done < <(find "$GEN/src" -name '*.cpp' | sort)

echo "== link graphics-service =="
BIN="$OUT/bin/android.hardware.graphics-service"
"$CC" -o "$BIN" "${OBJS[@]}" \
  -L"$GSI_LIBS" \
  -L"$MESA_PREFIX/lib" \
  -Wl,-rpath,/system/lib64 \
  -static-libstdc++ \
  -lbinder_ndk -llog -lgbm_mesa \
  -Wl,--allow-shlib-undefined

file "$BIN"
ls -lh "$BIN"
echo "Interface meta-methods:"
"$NM" "$BIN" | grep -E 'getInterfaceVersion|getInterfaceHash' | head -15
echo "OK $BIN"

echo "== build mapper.stub.so (AIMAPPER5) =="
bash "$SCRIPT_DIR/build-mapper.sh"
