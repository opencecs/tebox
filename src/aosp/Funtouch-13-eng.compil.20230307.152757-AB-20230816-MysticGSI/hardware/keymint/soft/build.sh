#!/usr/bin/env bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
source "$ROOT/scripts/env.sh"
AOSP=$(cd "$HERE/../../.." && pwd)
OUT="$ROOT/out/keymint-soft"
GEN="$OUT/aidl"
LIBS="$GSI_LIBS"
HI="$ROOT/thirdparty/hardware-interfaces/security"
CC="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android34-clang++"
mkdir -p "$GEN/include" "$GEN/src" "$LIBS" "$OUT/bin"
bash "$ROOT/scripts/extract-gsi-libs.sh" "$(basename "$AOSP")"
KM="$HI/keymint/aidl/aidl_api/android.hardware.security.keymint/4"
SC="$HI/secureclock/aidl/aidl_api/android.hardware.security.secureclock/1"
SS="$HI/sharedsecret/aidl/aidl_api/android.hardware.security.sharedsecret/1"
for spec in "$KM:4" "$SC:1" "$SS:1"; do
    api=${spec%:*}
    version=${spec##*:}
    files=()
    while IFS= read -r file; do files+=("$file"); done < <(find "$api" -name '*.aidl' | sort)
    "$AIDL" --lang=ndk --structured --stability=vintf --version="$version" \
        --hash="$(cat "$api/.hash")" -I "$KM" -I "$SC" -I "$SS" \
        -h "$GEN/include" -o "$GEN/src" "${files[@]}"
done
# These are platform C++ interfaces: use the platform's __1 ABI and libc++,
# not the NDK's __ndk1 STL. All shared libraries come from the same system.img.
"$CC" -std=c++20 -O2 -fno-rtti -D__ndk1=__1 -DBINDER_STABILITY_SUPPORT \
    -I"$HERE/include" -I"$GEN/include" -I"$ANDROID_HEADERS" \
    -I"$ROOT/thirdparty/system-keymaster/ng/include" \
    -I"$ROOT/thirdparty/system-keymaster/include" \
    "$HERE/service.cpp" "$HERE/persistent_storage.cpp" "$HERE/root_of_trust.cpp" \
    -nostdlib++ -L"$LIBS" -Wl,-rpath,/system/lib64 -Wl,--export-dynamic \
    -lkeymint_fake_latest -lkeymaster_portable -lbinder_ndk -llog -lc++ \
    -l:android.hardware.security.keymint-V4-ndk.so \
    -l:android.hardware.security.secureclock-V1-ndk.so \
    -l:android.hardware.security.sharedsecret-V1-ndk.so \
    -Wl,--allow-shlib-undefined -o "$OUT/bin/android.hardware.security.keymint-service"
echo "Built $OUT/bin/android.hardware.security.keymint-service"
