#!/usr/bin/env bash
# Build soft NDK Health HAL stub (IHealth AIDL v3).
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$SCRIPT_DIR/../../../../../.." && pwd)
VARIANT=$(basename "$(cd "$SCRIPT_DIR/../../.." && pwd)")
# shellcheck source=scripts/env.sh
source "$ROOT/scripts/env.sh"
HAL=$ROOT/src/aosp/$VARIANT/hardware/health
GEN=$ROOT/out/health-ndk-gen
OUT=$ROOT/out/health-stub
GSI_LIBS="${GSI_LIBS:-$ROOT/src/aosp/$VARIANT/prebuilts/gsi-lib64}"
HI=$ROOT/thirdparty/hardware-interfaces
API=34
CC=$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android${API}-clang++
NM=$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/llvm-nm

FROZEN="$HI/health/aidl/aidl_api/android.hardware.health/3"
HEALTH_HASH=$(cat "$FROZEN/.hash")

test -x "$CC" || { echo "missing $CC" >&2; exit 1; }
test -f "$GSI_LIBS/libbinder_ndk.so" || { echo "missing $GSI_LIBS/libbinder_ndk.so" >&2; exit 1; }
test -d "$FROZEN/android" || { echo "missing frozen Health v3 at $FROZEN" >&2; exit 1; }

echo "ROOT=$ROOT VARIANT=$VARIANT"
echo "Health V3 hash=$HEALTH_HASH"

echo "== generate AIDL NDK (frozen v3) =="
rm -rf "$GEN"
mkdir -p "$GEN/src" "$GEN/include" "$OUT" "$ROOT/out"

"$AIDL" --lang=ndk --structured --stability=vintf \
  --version=3 --hash="$HEALTH_HASH" \
  -o "$GEN/src" -h "$GEN/include" \
  -I "$FROZEN" \
  $(find "$FROZEN/android" -name '*.aidl' | sort)

if ! grep -q 'getInterfaceVersion' \
  "$GEN/include/aidl/android/hardware/health/IHealth.h"; then
  echo "ERROR: getInterfaceVersion missing — versioned AIDL gen failed" >&2
  exit 1
fi
# v3 must not include getHingeInfo
if grep -q 'getHingeInfo' "$GEN/include/aidl/android/hardware/health/IHealth.h"; then
  echo "ERROR: getHingeInfo present — generated as v4 by mistake" >&2
  exit 1
fi
echo "OK: IHealth v3 generated"

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
    -DLOG_TAG='"health-stub"' \
    -DBINDER_STABILITY_SUPPORT \
    -I"$GEN/include" \
    -I"$ANDROID_HEADERS" \
    -I"$GSI_LIBS/include" \
    -Wno-unused-parameter \
    -Wno-deprecated-declarations
  OBJS+=("$obj")
}

compile_one "$HAL/stub/service_stub.cpp"
while IFS= read -r f; do
  compile_one "$f"
done < <(find "$GEN/src" -name '*.cpp' | sort)

echo "== link =="
BIN="$OUT/bin/android.hardware.health-service"
"$CC" -o "$BIN" "${OBJS[@]}" \
  -L"$GSI_LIBS" \
  -Wl,-rpath,/system/lib64 \
  -static-libstdc++ \
  -lbinder_ndk -llog \
  -Wl,--allow-shlib-undefined

file "$BIN"
ls -lh "$BIN"
echo "Interface meta-methods:"
"$NM" "$BIN" | grep -E 'getInterfaceVersion|getInterfaceHash' | head -10
echo "OK $BIN"
