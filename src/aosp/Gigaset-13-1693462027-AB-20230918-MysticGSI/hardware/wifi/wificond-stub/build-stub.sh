#!/usr/bin/env bash
# Soft wificond (userspace) — binder service wifinl80211 with fixed fake scans.
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
source "$ROOT/scripts/env.sh"
OUT="$ROOT/out/wificond-stub"
CC="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android34-clang++"
[[ -x "$CC" ]] || { echo 'missing NDK clang' >&2; exit 1; }

mkdir -p "$OUT/bin" "$OUT/obj"
"$CC" -c "$HERE/service.cpp" -o "$OUT/obj/service.o" -std=c++20 -O2 -fPIC \
  -DLOG_TAG='"qemu-wificond"' \
  -I"$ANDROID_HEADERS" -Wno-unused-parameter

"$CC" -o "$OUT/bin/wificond" "$OUT/obj/service.o" \
  -static-libstdc++ -L"$GSI_LIBS" -lbinder_ndk -llog -Wl,--allow-shlib-undefined
echo "Built $OUT/bin/wificond (soft wifinl80211)"
