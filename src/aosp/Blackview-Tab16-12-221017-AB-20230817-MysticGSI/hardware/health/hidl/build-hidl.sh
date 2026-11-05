#!/usr/bin/env bash
# Build the Android 12 HIDL health HAL for this variant:
#   bin/android.hardware.health-service   IHealth@2.1/default over hwbinder
# Android 12 system_server (BatteryService) has no AIDL health client, so the
# A13-style AIDL stub cannot satisfy it. Platform libraries are taken from this
# variant's own system.img; headers under generated/ came from hidl-gen against
# thirdparty/hardware-interfaces/health (1.0, 2.0, 2.1).
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$SCRIPT_DIR/../../../../../.." && pwd)
VARIANT=$(basename "$(cd "$SCRIPT_DIR/../../.." && pwd)")
# shellcheck source=scripts/env.sh
source "$ROOT/scripts/env.sh"

HIDL=$SCRIPT_DIR
OUT=$ROOT/out/health-hidl
LIBS=$ROOT/out/gsi-hidl-libs-$VARIANT
A13=$ROOT/thirdparty/a13-headers
SYSTEM_IMG=${SYSTEM_IMG:-$IMAGES/system.img}
CC=$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android34-clang++
READELF=$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/llvm-readelf
DEBUGFS=$E2/debugfs

test -x "$CC" || { echo "missing $CC" >&2; exit 1; }
test -f "$SYSTEM_IMG" || { echo "missing $SYSTEM_IMG" >&2; exit 1; }
test -f "$A13/libcxx/__config" || { echo "missing $A13/libcxx" >&2; exit 1; }
test -d "$HIDL/generated/android/hardware/health/2.1" || {
  echo "missing generated health headers" >&2
  exit 1
}

echo "ROOT=$ROOT VARIANT=$VARIANT"

echo "== extract A12 link libraries =="
mkdir -p "$LIBS"
extract_lib() {
  local src=$1 dst tmp
  dst=$LIBS/$(basename "$src")
  if [[ -s "$dst" && "$dst" -nt "$SYSTEM_IMG" ]]; then return; fi
  tmp=$(mktemp "$LIBS/.extract.XXXXXX")
  if ! "$DEBUGFS" -R "dump $src $tmp" "$SYSTEM_IMG" >/dev/null 2>&1 || [[ ! -s "$tmp" ]]; then
    rm -f "$tmp"
    echo "failed to extract $src" >&2
    return 1
  fi
  mv -f "$tmp" "$dst"
}
for lib in \
    libbase.so libcutils.so liblog.so libutils.so libprocessgroup.so \
    libvndksupport.so libhidlbase.so libhidltransport.so libhwbinder.so \
    libfmq.so libsync.so libc++.so libcgrouprc.so \
    android.hardware.health@1.0.so \
    android.hardware.health@2.0.so \
    android.hardware.health@2.1.so ; do
  extract_lib "/system/lib64/$lib"
done
extract_lib /system_ext/lib64/android.hidl.base@1.0.so
echo "libraries in $LIBS"

INCLUDES=(
  -I"$HIDL/generated"
  -I"$HIDL/include"
  -I"$A13/log"
  -I"$A13/libbase"
  -I"$A13/core_libutils"
  -I"$A13/core_libcutils"
  -I"$A13/core_libsync"
  -I"$A13/libhidl_base"
  -I"$A13/libhidl_transport"
  -I"$A13/libfmq"
  -I"$A13/hwbinder"
  -I"$ANDROID_HEADERS"
)
CXXFLAGS=(
  -std=c++17 -fPIC -O2
  -fno-rtti
  -nostdinc++
  -isystem "$A13/libcxx"
  -isystem "$A13/libcxxabi"
  -D_LIBCPP_SUPPORT_XLOCALE_POSIX_L_FALLBACK_H
  -Wno-unused-parameter
  -Wno-deprecated-declarations
  -Wno-inconsistent-missing-override
)
LDFLAGS=(
  -nostdlib++
  -L"$LIBS"
  -Wl,-rpath,/vendor/lib64:/system/lib64
  -Wl,--allow-shlib-undefined
  # hidl-gen emits hidl_enum_values<> specializations with external linkage in
  # every TU; keep one copy.
  -Wl,-z,muldefs
)

rm -rf "$OUT/obj"
mkdir -p "$OUT/obj" "$OUT/bin"

echo "== compile service =="
"$CC" -c "$HIDL/service_main.cpp" -o "$OUT/obj/service_main.o" \
  "${CXXFLAGS[@]}" "${INCLUDES[@]}"

echo "== link android.hardware.health-service =="
BIN=$OUT/bin/android.hardware.health-service
"$CC" -o "$BIN" "$OUT/obj/service_main.o" "${LDFLAGS[@]}" \
  "$LIBS/android.hardware.health@2.1.so" \
  "$LIBS/android.hardware.health@2.0.so" \
  "$LIBS/android.hardware.health@1.0.so" \
  "$LIBS/android.hidl.base@1.0.so" \
  -lhidlbase -lhidltransport -lhwbinder -lutils -lbase -lcutils -llog \
  -lprocessgroup -lvndksupport -lfmq -lsync -lc++

echo "== verify =="
file "$BIN"
ls -lh "$BIN"
"$READELF" -d "$BIN" | grep NEEDED
if "$READELF" -d "$BIN" | grep -q 'libc++_shared'; then
  echo "ERROR: service must not depend on libc++_shared.so" >&2
  exit 1
fi
echo "OK $BIN"
