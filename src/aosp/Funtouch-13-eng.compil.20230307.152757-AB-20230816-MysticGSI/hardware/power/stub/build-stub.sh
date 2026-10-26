#!/usr/bin/env bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
source "$ROOT/scripts/env.sh"
FROZEN="$ROOT/thirdparty/hardware-interfaces/power/aidl/aidl_api/android.hardware.power/1"
OUT="$ROOT/out/power-stub"
CC="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android34-clang++"
mkdir -p "$OUT/src" "$OUT/include" "$OUT/bin"
"$AIDL" --lang=ndk --structured --stability=vintf --version=1 \
    --hash="$(cat "$FROZEN/.hash")" -I "$FROZEN" -o "$OUT/src" -h "$OUT/include" \
    "$FROZEN"/android/hardware/power/*.aidl
"$CC" -std=c++20 -O2 -DBINDER_STABILITY_SUPPORT \
    -I"$OUT/include" -I"$ANDROID_HEADERS" \
    "$HERE/service.cpp" "$OUT/src/android/hardware/power/IPower.cpp" \
    -L"$GSI_LIBS" -static-libstdc++ -lbinder_ndk -llog \
    -Wl,--allow-shlib-undefined -o "$OUT/bin/android.hardware.power-service"
echo "Built $OUT/bin/android.hardware.power-service"
