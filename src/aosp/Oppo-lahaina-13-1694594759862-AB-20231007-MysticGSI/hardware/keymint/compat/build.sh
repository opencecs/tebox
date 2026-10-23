#!/usr/bin/env bash
# Build the libc++ symbol shim used by the KeyMint prebuilt.
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
# shellcheck source=scripts/env.sh
source "$ROOT/scripts/env.sh"
CC="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android33-clang"
NM="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/llvm-nm"
OUT="$ROOT/out/keymint-compat/lib64"
mkdir -p "$OUT"
"$CC" -shared -fPIC -O2 -nostdlib++ \
  -Wl,-soname,libkeymint_compat.so \
  -o "$OUT/libkeymint_compat.so" "$HERE/libcxx_verbose_abort.c"
"$NM" -D --defined-only "$OUT/libkeymint_compat.so" | grep '__libcpp_verbose_abort' > /dev/null
echo "built $OUT/libkeymint_compat.so"
