#!/usr/bin/env bash
# Soft NDK WiFi HAL (IWifi AIDL v2) — full frozen API + fixed fake scan catalog.
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
source "$ROOT/scripts/env.sh"
HI="$ROOT/thirdparty/hardware-interfaces"
FROZEN="$HI/wifi/aidl/aidl_api/android.hardware.wifi/2"
COMMON_WIFI="$HI/wifi/common/aidl/aidl_api/android.hardware.wifi.common/1"
COMMON="$HI/common/aidl/aidl_api/android.hardware.common/2"
OUT="$ROOT/out/wifi-stub"
GEN="$ROOT/out/wifi-ndk-gen"
CC="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android34-clang++"
HASH=$(cat "$FROZEN/.hash")
[[ -x "$CC" && -x "$AIDL" ]] || { echo 'missing NDK/aidl' >&2; exit 1; }
[[ -d "$FROZEN/android" ]] || { echo "missing $FROZEN" >&2; exit 1; }

rm -rf "$GEN" "$OUT/obj" "$OUT/aidl-trim"
mkdir -p "$GEN/src" "$GEN/include" "$OUT/bin" "$OUT/obj" "$OUT/aidl-trim"

# Stage AIDL trees and inject opaque android.os.PersistableBundle for OuiKeyedData.
TRIM="$OUT/aidl-trim"
cp -a "$COMMON" "$TRIM/common"
cp -a "$COMMON_WIFI" "$TRIM/wifi-common"
cp -a "$FROZEN" "$TRIM/wifi"
mkdir -p "$TRIM/wifi-common/android/os"
cat > "$TRIM/wifi-common/android/os/PersistableBundle.aidl" <<'EOF'
package android.os;
@VintfStability
parcelable PersistableBundle {}
EOF

PACKAGES=("$TRIM/common" "$TRIM/wifi-common" "$TRIM/wifi")
INCLUDES=()
for package in "${PACKAGES[@]}"; do
  [[ -s "$package/.hash" ]] || { echo "missing $package" >&2; exit 1; }
  INCLUDES+=(-I "$package")
done

for package in "${PACKAGES[@]}"; do
  sources=()
  while IFS= read -r source; do sources+=("$source"); done < <(find "$package" -name '*.aidl' | sort)
  case "$package" in
    */wifi-common) ver=$(basename "$COMMON_WIFI"); hash=$(cat "$COMMON_WIFI/.hash") ;;
    */wifi) ver=$(basename "$FROZEN"); hash=$(cat "$FROZEN/.hash") ;;
    */common) ver=$(basename "$COMMON"); hash=$(cat "$COMMON/.hash") ;;
    *) echo "unknown package $package" >&2; exit 1 ;;
  esac
  "$AIDL" --lang=ndk --structured --stability=vintf --min_sdk_version=34 \
    --version="$ver" --hash="$hash" \
    -o "$GEN/src" -h "$GEN/include" "${INCLUDES[@]}" "${sources[@]}"
done

python3 "$HERE/generate-stubs.py" "$GEN/include" "$GEN/include/wifi_stubs.h"

objects=()
while IFS= read -r src; do
  obj="$OUT/obj/$(echo "${src#"$GEN/src/"}" | tr / _).o"
  mkdir -p "$(dirname "$obj")"
  "$CC" -c "$src" -o "$obj" -std=c++20 -O2 -fPIC \
    -DBINDER_STABILITY_SUPPORT -DLOG_TAG='"qemu-wifi"' \
    -I"$GEN/include" -I"$ANDROID_HEADERS" -Wno-unused-parameter
  objects+=("$obj")
done < <(find "$GEN/src" -name '*.cpp' | sort)

"$CC" -c "$HERE/service.cpp" -o "$OUT/obj/service.o" -std=c++20 -O2 -fPIC \
  -DBINDER_STABILITY_SUPPORT -DLOG_TAG='"qemu-wifi"' \
  -I"$GEN/include" -I"$ANDROID_HEADERS" -Wno-unused-parameter

"$CC" -o "$OUT/bin/android.hardware.wifi-service" "$OUT/obj/service.o" "${objects[@]}" \
  -static-libstdc++ -L"$GSI_LIBS" -lbinder_ndk -llog -Wl,--allow-shlib-undefined
echo "Built $OUT/bin/android.hardware.wifi-service (IWifi hash=$HASH)"
