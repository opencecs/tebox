#!/usr/bin/env bash
# Soft Fingerprint HAL (IFingerprint AIDL v4) — sensor props only; no real enroll/auth.
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
source "$ROOT/scripts/env.sh"
HI="$ROOT/thirdparty/hardware-interfaces"
FROZEN="$HI/biometrics/fingerprint/aidl/aidl_api/android.hardware.biometrics.fingerprint/4"
COMMON="$HI/biometrics/common/aidl/aidl_api/android.hardware.biometrics.common/4"
KEYMASTER="$HI/keymaster/aidl/aidl_api/android.hardware.keymaster/4"
OUT="$ROOT/out/fingerprint-stub"
GEN="$ROOT/out/fingerprint-ndk-gen"
CC="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android34-clang++"
HASH=$(cat "$FROZEN/.hash")
[[ -x "$CC" && -x "$AIDL" ]] || { echo 'missing NDK/aidl' >&2; exit 1; }
[[ -d "$FROZEN/android" ]] || { echo "missing $FROZEN" >&2; exit 1; }
[[ -s "$COMMON/.hash" ]] || { echo "missing $COMMON/.hash" >&2; exit 1; }
[[ -s "$KEYMASTER/.hash" ]] || { echo "missing $KEYMASTER/.hash" >&2; exit 1; }

rm -rf "$GEN" "$OUT/obj"
mkdir -p "$GEN/src" "$GEN/include" "$OUT/bin" "$OUT/obj"

PACKAGES=("$COMMON" "$KEYMASTER" "$FROZEN")
INCLUDES=()
for package in "${PACKAGES[@]}"; do
  INCLUDES+=(-I "$package")
done

for package in "${PACKAGES[@]}"; do
  sources=()
  while IFS= read -r source; do sources+=("$source"); done < <(find "$package" -name '*.aidl' | sort)
  ver=$(basename "$package")
  hash=$(cat "$package/.hash")
  "$AIDL" --lang=ndk --structured --stability=vintf --min_sdk_version=34 \
    --version="$ver" --hash="$hash" \
    -o "$GEN/src" -h "$GEN/include" "${INCLUDES[@]}" "${sources[@]}"
done

objects=()
while IFS= read -r src; do
  obj="$OUT/obj/$(echo "${src#"$GEN/src/"}" | tr / _).o"
  mkdir -p "$(dirname "$obj")"
  "$CC" -c "$src" -o "$obj" -std=c++20 -O2 -fPIC \
    -DBINDER_STABILITY_SUPPORT -DLOG_TAG='"qemu-fp"' \
    -I"$GEN/include" -I"$ANDROID_HEADERS" -Wno-unused-parameter
  objects+=("$obj")
done < <(find "$GEN/src" -name '*.cpp' | sort)

"$CC" -c "$HERE/service.cpp" -o "$OUT/obj/service.o" -std=c++20 -O2 -fPIC \
  -DBINDER_STABILITY_SUPPORT -DLOG_TAG='"qemu-fp"' \
  -I"$GEN/include" -I"$ANDROID_HEADERS" -Wno-unused-parameter

"$CC" -o "$OUT/bin/android.hardware.biometrics.fingerprint-service" \
  "$OUT/obj/service.o" "${objects[@]}" \
  -static-libstdc++ -L"$GSI_LIBS" -lbinder_ndk -llog -Wl,--allow-shlib-undefined
echo "Built $OUT/bin/android.hardware.biometrics.fingerprint-service (IFingerprint hash=$HASH)"
