#!/usr/bin/env bash
# Soft virtual GNSS HAL (IGnss AIDL v2) — fixed Beijing fix @1Hz, no real RF.
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
source "$ROOT/scripts/env.sh"
FROZEN="$ROOT/thirdparty/hardware-interfaces/gnss/aidl/android.hardware.gnss/2"
OUT="$ROOT/out/gnss-stub"
GEN="$ROOT/out/gnss-ndk-gen"
CC="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android34-clang++"
HASH=$(cat "$FROZEN/.hash")
[[ -x "$CC" && -x "$AIDL" ]] || { echo 'missing NDK/aidl' >&2; exit 1; }
[[ -d "$FROZEN/android" ]] || { echo "missing $FROZEN" >&2; exit 1; }

rm -rf "$GEN" "$OUT/obj"
mkdir -p "$GEN/src" "$GEN/include" "$OUT/bin" "$OUT/obj"

sources=()
while IFS= read -r source; do sources+=("$source"); done < <(find "$FROZEN" -name '*.aidl' | sort)
"$AIDL" --lang=ndk --structured --stability=vintf --min_sdk_version=34 \
  --version=2 --hash="$HASH" \
  -o "$GEN/src" -h "$GEN/include" -I "$FROZEN" "${sources[@]}"

objects=()
while IFS= read -r src; do
  obj="$OUT/obj/$(echo "${src#"$GEN/src/"}" | tr / _).o"
  mkdir -p "$(dirname "$obj")"
  "$CC" -c "$src" -o "$obj" -std=c++20 -O2 -fPIC \
    -DBINDER_STABILITY_SUPPORT -DLOG_TAG='"qemu-gnss"' \
    -I"$GEN/include" -I"$ANDROID_HEADERS" -Wno-unused-parameter
  objects+=("$obj")
done < <(find "$GEN/src" -name '*.cpp' | sort)

"$CC" -c "$HERE/service.cpp" -o "$OUT/obj/service.o" -std=c++20 -O2 -fPIC \
  -DBINDER_STABILITY_SUPPORT -DLOG_TAG='"qemu-gnss"' \
  -I"$GEN/include" -I"$ANDROID_HEADERS" -Wno-unused-parameter

"$CC" -o "$OUT/bin/android.hardware.gnss-service" "$OUT/obj/service.o" "${objects[@]}" \
  -static-libstdc++ -L"$GSI_LIBS" -lbinder_ndk -llog -Wl,--allow-shlib-undefined
echo "Built $OUT/bin/android.hardware.gnss-service (IGnss v2 hash=$HASH)"
