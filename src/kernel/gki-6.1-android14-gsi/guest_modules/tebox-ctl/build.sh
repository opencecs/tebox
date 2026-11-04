#!/usr/bin/env bash
# Build tebox_ctl.ko for the guest GKI (android14-6.1) so it loads on the
# mirror's prebuilt Image. Runs the kernel build in a linux/amd64 container
# (OrbStack/Rosetta on Apple Silicon) = AOSP clang's native host; cross-builds
# the aarch64 module. Shared cached kernel build in the tebox-kbuild volume,
# so after the first module this is seconds.
#
# CRITICAL: the kernel is configured from guest.config (the running guest's
# exact /proc/config.gz), NOT `gki_defconfig`. They differ (e.g. the guest has
# CONFIG_DEBUG_INFO_BTF_MODULES=y) in ways that change sizeof(struct module):
# gki_defconfig yields 0x400, the guest yields 0x440. A module built with the
# wrong struct module layout loads far enough (vermagic is forced to match) to
# PANIC the guest in mod_sysfs_setup when the loader reads struct module fields
# at the wrong offsets. Using guest.config makes the ABI match so the .ko loads.
#
# Prereq: a repo-synced workspace at $WS (common-android14-6.1):
#   mkdir -p out/gki-workspace && cd out/gki-workspace
#   repo init -u https://android.googlesource.com/kernel/manifest -b common-android14-6.1 --depth=1
#   repo sync -c -j8
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../.." && pwd)
KROOT=$(cd "$HERE/../.." && pwd)          # src/kernel/gki-6.1-android14-gsi
GUEST_CONFIG="$KROOT/guest.config"
WS="${WS:-$ROOT/out/gki-workspace}"
# Match the guest vermagic exactly (see tebox-v4l2/build.sh for the rationale).
KCOMMIT="${KCOMMIT:-108b53924ea33cadfc5b446f4e1d841465f278f1}"  # Merge android14-6.1.172_r00
SCMVERSION="${SCMVERSION:--11-g87c0a9ca734b-ab16116086}"
LOCALVERSION="${LOCALVERSION:--android14}"
IMAGE="${IMAGE:-debian:trixie}"

[[ -d "$WS/common" ]] || { echo "missing $WS/common (run repo sync first)" >&2; exit 1; }
[[ -f "$GUEST_CONFIG" ]] || { echo "missing $GUEST_CONFIG (adb shell zcat /proc/config.gz)" >&2; exit 1; }

cat > "$WS/.build-tebox-ctl.sh" <<INNER
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq --no-install-recommends \
    make bc bison flex libssl-dev libelf-dev kmod rsync python3 \
    git ca-certificates cpio zstd xz-utils gcc g++ libc6-dev >/dev/null

git config --global --add safe.directory '*'
CLANG=\$(ls -d /ws/prebuilts/clang/host/linux-x86/clang-* 2>/dev/null | grep -v stable | sort | tail -1)
[[ -n "\$CLANG" ]] || { echo "no prebuilt clang under /ws/prebuilts/clang/host/linux-x86" >&2; exit 1; }
export PATH="\$CLANG/bin:/ws/prebuilts/kernel-build-tools/linux-x86/bin:\$PATH"
MK="make LLVM=1 ARCH=arm64 O=/build/out -j\$(nproc)"

# Reuse the cache only if it was configured from guest.config (the marker
# CONFIG_DEBUG_INFO_BTF_MODULES=y is absent in gki_defconfig). A stale
# gki_defconfig cache gives the wrong struct module ABI and must be rebuilt.
if [ -f /build/out/Module.symvers ] && [ -d /build/common ] && \
   grep -q '^CONFIG_DEBUG_INFO_BTF_MODULES=y' /build/out/.config 2>/dev/null; then
  echo "reusing cached kernel build in /build/out (guest.config)"
  cd /build/common
else
  rm -rf /build/common /build/out
  git clone --quiet --shared /ws/common /build/common
  cd /build/common
  git checkout -f -q "$KCOMMIT" || { echo "commit $KCOMMIT missing" >&2; exit 1; }
  printf '%s' "$SCMVERSION" > .scmversion
  mkdir -p /build/out
  # Exact guest config (struct module ABI match), not gki_defconfig.
  cp /guest.config /build/out/.config
  ./scripts/config --file /build/out/.config --set-str LOCALVERSION "$LOCALVERSION"
  \$MK olddefconfig
  \$MK
  echo "built release: \$(cat /build/out/include/config/kernel.release)"
fi
\$MK M=/mod modules
echo "=== built module ==="
ls -l /mod/*.ko
RE=\$(ls /ws/prebuilts/clang/host/linux-x86/*/bin/llvm-readelf 2>/dev/null | head -1)
"\$RE" -p .modinfo /mod/tebox_ctl.ko 2>/dev/null | grep -iE "vermagic|name=" || true
# struct module size must be 0x440 (guest ABI); 0x400 means the wrong config.
echo "this_module=0x\$("\$RE" -SW /mod/tebox_ctl.ko 2>/dev/null | grep this_module | awk '{print \$6}' | head -1)"
INNER
chmod +x "$WS/.build-tebox-ctl.sh"

echo "building in $IMAGE (linux/amd64)..."
docker run --rm --platform linux/amd64 \
    -v "$WS:/ws" -v "$HERE:/mod" -v "$GUEST_CONFIG:/guest.config:ro" \
    -v tebox-kbuild:/build -w /ws \
    "$IMAGE" bash /ws/.build-tebox-ctl.sh

echo "done: $HERE/tebox_ctl.ko (copy to ../../vendor_modules/ to ship)"
