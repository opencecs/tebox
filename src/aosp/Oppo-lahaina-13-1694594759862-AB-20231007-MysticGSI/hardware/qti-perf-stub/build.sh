#!/usr/bin/env bash
# Build the no-op libqti_performance.so that replaces the GSI's system_ext
# library (see qti_perf_stub.c for the crash it prevents and how it registers
# the native methods itself).  The output lands in the variant's initramfs
# system-overlay, which bind-mounts it over
# /system/system_ext/lib64/libqti_performance.so before Android init starts.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../../../../.." && pwd)"
VARIANT="$(basename "$(cd "$SCRIPT_DIR/../.." && pwd)")"
export VARIANT
# shellcheck source=scripts/env.sh
source "$ROOT/scripts/env.sh"

CC="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android33-clang"
READELF="$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/llvm-readelf"
[[ -x "$CC" ]] || { echo "missing $CC" >&2; exit 1; }

OUT="$AOSP/qemu/init/system-overlay/system/system_ext/lib64/libqti_performance.so"
mkdir -p "$(dirname "$OUT")"
# -fno-optimize-sibling-calls keeps dlsym() from tail-calling __loader_dlsym:
# the caller address is captured from x30 on entry and must stay valid.
# -ldl puts libdl.so in the stub's DT_NEEDED so __loader_dlsym resolves
# through its dependency ld-android.so.
"$CC" -shared -fPIC -O2 -fno-optimize-sibling-calls -o "$OUT" "$SCRIPT_DIR/qti_perf_stub.c" \
  -ldl -Wl,-soname,libqti_performance.so -Wl,--no-undefined
# No grep -q here: it exits early and SIGPIPEs llvm-readelf, which pipefail
# turns into a false negative.  ART's FindNative looks up the JNI-mangled
# symbol names, and the interposed dlsym must be exported and be the one the
# loader binds libart's dlsym@LIBC reference to.
if ! "$READELF" -s "$OUT" | grep 'Java_com_qualcomm_qti_Performance_native_1perf_1get_1feedback_1extn' > /dev/null; then
  echo 'ERROR: native_perf_get_feedback_extn is not exported under its JNI name' >&2
  exit 1
fi
if ! "$READELF" -s "$OUT" | grep ' FUNC    GLOBAL DEFAULT    [0-9]* dlsym$' > /dev/null; then
  echo 'ERROR: dlsym interposer is not exported' >&2
  exit 1
fi
if ! "$READELF" -d "$OUT" | grep 'libqti_performance.so' > /dev/null; then
  echo 'ERROR: soname is not libqti_performance.so' >&2
  exit 1
fi
if ! "$READELF" -d "$OUT" | grep '\[libdl.so\]' > /dev/null; then
  echo 'ERROR: libdl.so is not in DT_NEEDED' >&2
  exit 1
fi
echo "built $OUT"
