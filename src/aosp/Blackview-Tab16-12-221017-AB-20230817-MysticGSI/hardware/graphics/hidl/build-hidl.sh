#!/usr/bin/env bash
# Build the Android 12 HIDL graphics stack for this variant:
#   bin/android.hardware.graphics-service             composer 2.4 + allocator 4.0
#   lib64/hw/android.hardware.graphics.mapper@4.0-impl.so
# The GSI's libui only speaks HIDL (mapper 4.0 passthrough, composer 2.x over
# hwbinder), so these replace the AIDL stub on Android 12 images. Platform
# libraries are taken from this variant's own system.img, and the platform
# headers come from thirdparty/a13-headers.
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$SCRIPT_DIR/../../../../../.." && pwd)
VARIANT=$(basename "$(cd "$SCRIPT_DIR/../../.." && pwd)")
# shellcheck source=scripts/env.sh
source "$ROOT/scripts/env.sh"

HIDL=$SCRIPT_DIR
OUT=$ROOT/out/graphics-hidl
LIBS=$ROOT/out/gsi-hidl-libs-$VARIANT
A13=$ROOT/thirdparty/a13-headers
SYSTEM_IMG=${SYSTEM_IMG:-$IMAGES/system.img}
CC=$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/aarch64-linux-android34-clang++
READELF=$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/llvm-readelf
NM=$NDK/toolchains/llvm/prebuilt/$NDK_HOST_TAG/bin/llvm-nm
DEBUGFS=$E2/debugfs

test -x "$CC" || { echo "missing $CC" >&2; exit 1; }
test -f "$SYSTEM_IMG" || { echo "missing $SYSTEM_IMG" >&2; exit 1; }
test -f "$A13/libcxx/__config" || { echo "missing $A13/libcxx" >&2; exit 1; }

echo "ROOT=$ROOT VARIANT=$VARIANT"

echo "== extract A12 link libraries =="
mkdir -p "$LIBS"
extract_lib() {
  local src=$1 dst tmp
  dst=$LIBS/$(basename "$src")
  # Re-extract when the image is newer than the copy (image was replaced).
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
    android.hardware.graphics.common@1.0.so \
    android.hardware.graphics.common@1.1.so \
    android.hardware.graphics.common@1.2.so \
    android.hardware.graphics.composer@2.1.so \
    android.hardware.graphics.composer@2.2.so \
    android.hardware.graphics.composer@2.3.so \
    android.hardware.graphics.composer@2.4.so \
    android.hardware.graphics.allocator@4.0.so \
    android.hardware.graphics.mapper@2.0.so \
    android.hardware.graphics.mapper@2.1.so \
    android.hardware.graphics.mapper@3.0.so \
    android.hardware.graphics.mapper@4.0.so ; do
  extract_lib "/system/lib64/$lib"
done
# android.hidl.base@1.0 lives in system_ext on this GSI, not system.
extract_lib /system_ext/lib64/android.hidl.base@1.0.so
echo "libraries in $LIBS"

INCLUDES=(
  -I"$HIDL/generated"
  -I"$A13/log"
  -I"$HIDL/include"
  -I"$HIDL/composer"
  -I"$HIDL/aosp/include"
  -I"$A13/libbase"
  -I"$A13/core_libutils"
  -I"$A13/core_libcutils"
  -I"$A13/core_libsync"
  -I"$A13/libhidl_base"
  -I"$A13/libhidl_transport"
  -I"$A13/libfmq"
  -I"$A13/hwbinder"
  -I"$ANDROID_HEADERS"
  -I"$MESA_PREFIX/include"
  -I"$DRM_PREFIX/include"
  -I"$DRM_PREFIX/include/libdrm"
)
CXXFLAGS=(
  -std=c++17 -fPIC -O2
  # Platform default. The generated interface libraries do not export their
  # typeinfo, and nothing here uses dynamic_cast or typeid.
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
  -L"$MESA_PREFIX/lib"
  -Wl,-rpath,/vendor/lib64:/system/lib64
  -Wl,--allow-shlib-undefined
  # hidl-gen emits the hidl_enum_values<> specializations with external
  # linkage, so every TU that includes a generated interface header defines
  # them. They are identical constants; keep one.
  -Wl,-z,muldefs
)

rm -rf "$OUT/obj"
mkdir -p "$OUT/obj" "$OUT/bin" "$OUT/lib64/hw"

echo "== compile service =="
OBJS=()
compile() {
  local src=$1 obj
  obj=$OUT/obj/$(echo "${src#"$HIDL"/}" | sed 's|/|_|g; s|\.cpp$|.o|')
  echo "  CC ${src#"$HIDL"/}"
  "$CC" -c "$src" -o "$obj" "${CXXFLAGS[@]}" "${INCLUDES[@]}"
  OBJS+=("$obj")
}
compile "$HIDL/service_main.cpp"
compile "$HIDL/compat.cpp"
compile "$HIDL/composer/composer_hal.cpp"
compile "$HIDL/allocator/allocator_impl.cpp"
compile "$HIDL/aosp/src/ComposerResources21.cpp"
# composer-resources/2.2 is header-only in this source drop: its header
# defines the three 2.2 additions inline, so there is no 22.cpp to compile.

echo "== link android.hardware.graphics-service =="
BIN=$OUT/bin/android.hardware.graphics-service
"$CC" -o "$BIN" "${OBJS[@]}" "${LDFLAGS[@]}" \
  "$LIBS/android.hardware.graphics.composer@2.4.so" \
  "$LIBS/android.hardware.graphics.composer@2.3.so" \
  "$LIBS/android.hardware.graphics.composer@2.2.so" \
  "$LIBS/android.hardware.graphics.composer@2.1.so" \
  "$LIBS/android.hardware.graphics.allocator@4.0.so" \
  "$LIBS/android.hardware.graphics.mapper@4.0.so" \
  "$LIBS/android.hardware.graphics.mapper@3.0.so" \
  "$LIBS/android.hardware.graphics.mapper@2.1.so" \
  "$LIBS/android.hardware.graphics.mapper@2.0.so" \
  "$LIBS/android.hardware.graphics.common@1.2.so" \
  "$LIBS/android.hardware.graphics.common@1.1.so" \
  "$LIBS/android.hardware.graphics.common@1.0.so" \
  "$LIBS/android.hidl.base@1.0.so" \
  -lhidlbase -lhidltransport -lhwbinder -lutils -lbase -lcutils -llog \
  -lprocessgroup -lvndksupport -lfmq -lsync -lc++ -lgbm_mesa

echo "== build mapper 4.0 passthrough =="
MAPPER=$OUT/lib64/hw/android.hardware.graphics.mapper@4.0-impl.so
"$CC" -shared -o "$MAPPER" "$HIDL/mapper/mapper_impl.cpp" \
  "${CXXFLAGS[@]}" "${INCLUDES[@]}" "${LDFLAGS[@]}" \
  -Wl,-soname,android.hardware.graphics.mapper@4.0-impl.so \
  "$LIBS/android.hardware.graphics.mapper@4.0.so" \
  -lhidlbase -lutils -lbase -lcutils -llog -lc++ -lgbm_mesa

echo "== verify =="
file "$BIN" "$MAPPER"
ls -lh "$BIN" "$MAPPER"
echo "-- service NEEDED"
"$READELF" -d "$BIN" | grep NEEDED
echo "-- mapper NEEDED"
"$READELF" -d "$MAPPER" | grep NEEDED
"$NM" -D --defined-only "$MAPPER" | grep -q ' HIDL_FETCH_IMapper$' || {
  echo "ERROR: mapper does not export HIDL_FETCH_IMapper" >&2
  exit 1
}
if "$READELF" -d "$MAPPER" | grep -q 'libc++_shared'; then
  echo "ERROR: mapper must not depend on libc++_shared.so" >&2
  exit 1
fi
echo "OK $BIN"
echo "OK $MAPPER"
