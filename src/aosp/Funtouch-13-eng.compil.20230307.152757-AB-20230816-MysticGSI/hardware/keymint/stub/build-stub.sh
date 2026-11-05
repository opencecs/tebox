#!/usr/bin/env bash
# Build NDK KeyMint stub for boot bring-up.
# Must emit frozen V4 (KeyMint) / V1 (SecureClock, SharedSecret) so keystore2's
# getInterfaceVersion/getInterfaceHash transactions succeed.
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$SCRIPT_DIR/../../../../../.." && pwd)
VARIANT=$(basename "$(cd "$SCRIPT_DIR/../../.." && pwd)")
# shellcheck source=scripts/env.sh
source "$ROOT/scripts/env.sh"
HAL=$ROOT/src/aosp/$VARIANT/hardware/keymint
GEN=$ROOT/out/keymint-ndk-gen
OUT=$ROOT/out/keymint-stub
GSI_LIBS="${GSI_LIBS:-$ROOT/src/aosp/$VARIANT/prebuilts/gsi-lib64}"
HI=$ROOT/thirdparty/hardware-interfaces
API=34
CC=$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android${API}-clang++
NM=$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/llvm-nm

KEYMINT_HASH=$(cat "$HI/security/keymint/aidl/aidl_api/android.hardware.security.keymint/4/.hash")
SECURECLOCK_HASH=$(cat "$HI/security/secureclock/aidl/aidl_api/android.hardware.security.secureclock/1/.hash")
SHAREDSECRET_HASH=$(cat "$HI/security/sharedsecret/aidl/aidl_api/android.hardware.security.sharedsecret/1/.hash")

test -x "$CC" || { echo "missing $CC" >&2; exit 1; }
test -f "$GSI_LIBS/libbinder_ndk.so" || { echo "missing $GSI_LIBS/libbinder_ndk.so" >&2; exit 1; }

echo "ROOT=$ROOT VARIANT=$VARIANT"
echo "KeyMint V4 hash=$KEYMINT_HASH"
echo "SecureClock V1 hash=$SECURECLOCK_HASH"
echo "SharedSecret V1 hash=$SHAREDSECRET_HASH"

echo "== generate AIDL NDK (frozen versions) =="
rm -rf "$GEN"
mkdir -p "$GEN/src" "$GEN/include" "$OUT" "$ROOT/out"

# Each aidl_interface must be compiled with its own --version/--hash.
# Import paths (-I) resolve types; only listed .aidl files are emitted.
"$AIDL" --lang=ndk --structured --stability=vintf \
  --version=4 --hash="$KEYMINT_HASH" \
  -o "$GEN/src" -h "$GEN/include" \
  -I "$HI/security/keymint/aidl" \
  -I "$HI/security/secureclock/aidl" \
  $(find "$HI/security/keymint/aidl/android" -name '*.aidl' | sort)

"$AIDL" --lang=ndk --structured --stability=vintf \
  --version=1 --hash="$SECURECLOCK_HASH" \
  -o "$GEN/src" -h "$GEN/include" \
  -I "$HI/security/secureclock/aidl" \
  $(find "$HI/security/secureclock/aidl/android" -name '*.aidl' | sort)

"$AIDL" --lang=ndk --structured --stability=vintf \
  --version=1 --hash="$SHAREDSECRET_HASH" \
  -o "$GEN/src" -h "$GEN/include" \
  -I "$HI/security/sharedsecret/aidl" \
  $(find "$HI/security/sharedsecret/aidl/android" -name '*.aidl' | sort)

if ! grep -q 'getInterfaceVersion' \
  "$GEN/include/aidl/android/hardware/security/keymint/IKeyMintDevice.h"; then
  echo "ERROR: getInterfaceVersion missing — versioned AIDL gen failed" >&2
  exit 1
fi
echo "OK: IKeyMintDevice has getInterfaceVersion"

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
    -DLOG_TAG='"keymint-service"' \
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
"$CC" -o "$OUT/bin/android.hardware.security.keymint-service" "${OBJS[@]}" \
  -L"$GSI_LIBS" \
  -Wl,-rpath,/system/lib64 \
  -static-libstdc++ \
  -lbinder_ndk -llog \
  -Wl,--allow-shlib-undefined

file "$OUT/bin/android.hardware.security.keymint-service"
ls -lh "$OUT/bin/android.hardware.security.keymint-service"
echo "Interface meta-methods:"
"$NM" "$OUT/bin/android.hardware.security.keymint-service" | \
  grep -E 'getInterfaceVersion|getInterfaceHash' | head -10
echo "OK $OUT/bin/android.hardware.security.keymint-service"
