#!/usr/bin/env bash
# Build AOSP-side initramfs: virtio kos + busybox init + vendor SELinux stub → switch_root Android.
set -euo pipefail
umask 022
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VARIANT="${1:-${VARIANT:-${TARGET_PRODUCT:-aosp_arm64-BP4A.251205.006}}}"
AOSP="$ROOT/src/aosp/$VARIANT"
[[ -d "$AOSP" ]] || { echo "missing variant tree: $AOSP (VARIANT=$VARIANT)" >&2; exit 1; }
[[ -d "$AOSP" ]] || { echo "missing $AOSP" >&2; exit 1; }
KID="$(tr -d '[:space:]' < "$AOSP/KERNEL")"
K="$ROOT/src/kernel/$KID"
BB="$AOSP/qemu/busybox"
STUB="$AOSP/qemu/vendor"
OUT_IMG="$AOSP/images/initramfs.img"
STAGE="$ROOT/out/initramfs-build-$VARIANT"

python3 "$ROOT/scripts/check-runtime-inputs.py" "$BB" "$K/vendor_modules"

[[ -x "$BB" ]] || { echo "missing aarch64 busybox at $BB" >&2; exit 1; }
[[ -d "$K/vendor_modules" ]] || { echo "missing $K/vendor_modules" >&2; exit 1; }
[[ -d "$STUB/etc/selinux" ]] || { echo "missing vendor stub $STUB" >&2; exit 1; }

rm -rf "$STAGE"
mkdir -p "$STAGE"/{bin,dev,proc,sys,newroot,modules}
cp -f "$BB" "$STAGE/bin/busybox"
chmod +x "$STAGE/bin/busybox"
for a in sh ls cat echo grep mount umount mkdir mknod insmod lsmod mdev ln sleep awk \
         chmod chown switch_root chroot mv rm cp setsid touch; do
  ln -sf busybox "$STAGE/bin/$a"
done
cp -f "$K/vendor_modules/"*.ko "$STAGE/modules/"
cp -a "$STUB" "$STAGE/vendor"
chmod 0755 "$STAGE/vendor/bin/enable-camera-launcher.sh" 2>/dev/null || true
INIT_DIR="$AOSP/qemu/init"
OVERLAY_DIR="$AOSP/qemu/overlay"
mkdir -p "$STAGE/overlays"
if [[ -d "$INIT_DIR" ]]; then
  # Guest path cannot be /init — that is the busybox PID 1 script.
  # init/ is only .rc (system init overlays).
  shopt -s nullglob
  for f in "$INIT_DIR"/*.rc; do
    cp -f "$f" "$STAGE/overlays/"
  done
  shopt -u nullglob
  if [[ -d "$INIT_DIR/system-overlay" ]]; then
    mkdir -p "$STAGE/system-overlay"
    cp -a "$INIT_DIR/system-overlay"/. "$STAGE/system-overlay"/
  fi
fi
# Non-rc system overlays (binaries, txt) live in qemu/overlay/<system path>.
if [[ -d "$OVERLAY_DIR" ]]; then
  mkdir -p "$STAGE/system-overlay"
  cp -a "$OVERLAY_DIR"/. "$STAGE/system-overlay"/
  if [[ -x "$OVERLAY_DIR/system/bin/wificond" ]]; then
    cp -f "$OVERLAY_DIR/system/bin/wificond" "$STAGE/overlays/wificond"
    chmod 0755 "$STAGE/overlays/wificond"
  fi
  if [[ -f "$OVERLAY_DIR/system/etc/vndkcorevariant.libraries.txt" ]]; then
    cp -f "$OVERLAY_DIR/system/etc/vndkcorevariant.libraries.txt" \
      "$STAGE/overlays/vndkcorevariant.libraries.txt"
  fi
fi
if [[ -d "$STAGE/system-overlay" ]]; then
  (cd "$STAGE/system-overlay" && find . -type f | sed 's|^\./||' | sort) > "$STAGE/system-overlay.list"
fi

cat > "$STAGE/init" <<'INIT'
#!/bin/sh
export PATH=/bin
mount -t proc none /proc
mount -t sysfs none /sys
mount -t tmpfs -o mode=0755 tmpfs /dev
rm -f /dev/null
mknod -m 666 /dev/null c 1 3
mknod -m 666 /dev/zero c 1 5
mknod -m 666 /dev/full c 1 7
mknod -m 666 /dev/random c 1 8
mknod -m 666 /dev/urandom c 1 9
mknod -m 660 /dev/kmsg c 1 11
mknod -m 666 /dev/tty c 5 0
mknod -m 600 /dev/console c 5 1
mknod -m 666 /dev/ptmx c 5 2
mknod -m 660 /dev/loop-control c 10 237 2>/dev/null || true
i=0
while [ $i -lt 64 ]; do
  mknod -m 660 /dev/loop$i b 7 $i 2>/dev/null || true
  i=$((i+1))
done
mkdir -p /dev/block /dev/pts /dev/socket /dev/__properties__ /dev/binderfs
mount -t devpts devpts /dev/pts 2>/dev/null || true
# Mount binderfs only — let Android init create /dev/binder symlinks with correct labels.
mount -t binder binder /dev/binderfs 2>/dev/null || true

load(){ f=/modules/$1; [ -f "$f" ] && insmod "$f" 2>/dev/null || true; }
load virtio_pci_modern_dev.ko
load virtio_pci_legacy_dev.ko
load virtio_pci.ko
load virtio_mmio.ko
load virtio_dma_buf.ko
load drm_dma_helper.ko
load virtio-gpu.ko
load virtio_blk.ko
load failover.ko
load net_failover.ko
load virtio_net.ko
load virtio_console.ko
load virtio_input.ko
load virtio_balloon.ko
load virtio-rng.ko
load system_heap.ko
# tebox control bus (guest half): exposes /dev/tebox_ctl, the guest<->host +
# guest-internal message bus. Needs virtio_pci (above). Harmless if the
# tebox-ctl virtio device is absent (driver just never probes).
load tebox_ctl.ko
# tebox MVX V4L2 passthrough (guest half); only present when built. Needs
# virtio_pci (above) + the builtin V4L2 core. Harmless if the QEMU device is
# absent (no tebox-v4l2 virtio device -> driver just never probes).
load tebox_v4l2.ko
# tebox Mali kbase passthrough (guest half): mali_kbase is a tiny stub that only
# exposes /sys/module/mali_kbase/parameters/large_page_conf (the stock libmali
# probes it); tebox_mali exposes /dev/mali0 + /sys/class/misc/mali0. Load the
# stub first so the sysfs is present before libmali enumerates. Harmless if the
# tebox-mali virtio device is absent (driver just never probes).
load mali_kbase.ko
load tebox_mali.ko
# Old tebox_mali.ko (count-only fire_batch, no fire_flush_us timer) deferred
# doorbells until 32 piled up and hung CSF fences. Until that .ko is rebuilt
# with the hrtimer flush, force fire_batch=1 (notify every doorbell).
if [ ! -e /sys/module/tebox_mali/parameters/fire_flush_us ]; then
  echo 1 > /sys/module/tebox_mali/parameters/fire_batch 2>/dev/null || true
fi
mdev -s 2>/dev/null || true
# Force /dev/mali0 world-rw before switch_root (same devtmpfs instance persists
# into Android).
chmod 666 /dev/mali0 2>/dev/null || true
for name in vda vdb vdc; do
  if [ ! -e /dev/$name ]; then
    maj=$(awk -v n="$name" '$4==n {print $1}' /proc/partitions)
    min=$(awk -v n="$name" '$4==n {print $2}' /proc/partitions)
    [ -n "$maj" ] && mknod /dev/$name b "$maj" "$min"
  fi
  ln -sf ../$name /dev/block/$name 2>/dev/null || true
done

mkdir -p /newroot
mount -t ext4 -o rw /dev/vda /newroot || exec /bin/sh
[ -e /dev/vdb ] && mount -t ext4 /dev/vdb /newroot/data
# Ensure keystore DB dir exists before early_hal (normally created in post-fs-data).
mkdir -p /newroot/data/misc/keystore
chmod 0700 /newroot/data/misc/keystore 2>/dev/null || true
chown 1017:1017 /newroot/data/misc/keystore 2>/dev/null || true
for d in metadata cache mnt tmp; do
  mkdir -p /newroot/$d
  mount -t tmpfs -o mode=0755 tmpfs /newroot/$d 2>/dev/null || true
done

# Prefer real vendor.img (ext4 on vdc); fall back to tmpfs stub copy.
mkdir -p /newroot/vendor
if [ -e /dev/vdc ] && mount -t ext4 -o ro /dev/vdc /newroot/vendor; then
  echo "[init] mounted vendor.img from /dev/vdc"
  ls /newroot/vendor/bin/hw 2>/dev/null || true
else
  echo "[init] WARN: no vendor.img, using tmpfs stub"
  mount -t tmpfs -o mode=0755 tmpfs /newroot/vendor
  cp -a /vendor/. /newroot/vendor/
fi

# Neutralize KeyMint stubs previously injected into shared_blocks system.img.
# skip-rootfs-binds: those files are already in the images (no extra mounts).
if [ -f /skip-rootfs-binds ]; then
  echo "[init] overlays baked into images"
elif [ -f /overlays/init.rc ]; then
  mount -o bind /overlays/init.rc /newroot/system/etc/init/hw/init.rc && \
    echo "[init] bound clean init.rc"
fi
if [ ! -f /skip-rootfs-binds ]; then
if [ -f /overlays/init.ranchu.rc ]; then
  touch /newroot/init.ranchu.rc 2>/dev/null || true
  target=/newroot/init.ranchu.rc
  if [ ! -e "$target" ]; then
    # GSIs without /init.ranchu.rc (read-only root, e.g. ColorOS): the vendor
    # carries a marked placeholder that init.rc imports as init.${ro.hardware}.rc.
    # Only the busybox applets in bin/ exist here (no head/sed).
    target=
    for f in /newroot/vendor/etc/init/hw/init.*.rc; do
      grep -q "^# Placeholder: the initramfs binds" "$f" 2>/dev/null && { target=$f; break; }
    done
  fi
  if [ -n "$target" ] && mount -o bind /overlays/init.ranchu.rc "$target"; then
    echo "[init] bound clean init.ranchu.rc on ${target#/newroot}"
  else
    echo "[init] WARNING: no place to bind init.ranchu.rc"
  fi
fi
if [ -f /overlays/android.hardware.security.keymint-service.rc ]; then
  mount -o bind /overlays/android.hardware.security.keymint-service.rc \
    /newroot/system/etc/init/android.hardware.security.keymint-service.rc && \
    echo "[init] neutralized system keymint rc"
fi
if [ -f /overlays/keystore2.rc ]; then
  mount -o bind /overlays/keystore2.rc /newroot/system/etc/init/keystore2.rc && \
    echo "[init] bound non-critical keystore2.rc"
fi
if [ -f /overlays/surfaceflinger.rc ]; then
  mount -o bind /overlays/surfaceflinger.rc /newroot/system/etc/init/surfaceflinger.rc && \
    echo "[init] bound surfaceflinger.rc (console + Mesa DRI path)"
fi
if [ -f /overlays/virtual_camera.hal.rc ]; then
  touch /newroot/system/etc/init/virtual_camera.hal.rc 2>/dev/null || true
  mount -o bind /overlays/virtual_camera.hal.rc \
    /newroot/system/etc/init/virtual_camera.hal.rc && \
    echo "[init] masked GSI virtual camera service"
fi
if [ -f /overlays/vndkcorevariant.libraries.txt ]; then
  touch /newroot/system/etc/vndkcorevariant.libraries.txt 2>/dev/null || true
  mount -o bind /overlays/vndkcorevariant.libraries.txt \
    /newroot/system/etc/vndkcorevariant.libraries.txt && \
    echo "[init] bound vndkcorevariant.libraries.txt"
fi
if [ -x /overlays/wificond ]; then
  touch /newroot/system/bin/wificond 2>/dev/null || true
  mount -o bind /overlays/wificond /newroot/system/bin/wificond && \
    echo "[init] bound soft wificond"
fi
if [ -f /system-overlay.list ]; then
  while read -r f; do
    if [ -e "/newroot/$f" ] && mount -o bind "/system-overlay/$f" "/newroot/$f"; then
      echo "[init] overlay /$f"
    else
      echo "[init] WARNING: cannot overlay /$f"
    fi
  done < /system-overlay.list
fi
fi

# GSI ships ro.adb.secure=1 (RSA dialog). For loopback TCP ADB, force insecure
# adbd before second-stage init loads /system/build.prop.
# Baked images already contain ro.adb.secure=0; binding it would show up as a
# second mount on /system/build.prop.
if [ ! -f /skip-rootfs-binds ] && \
   grep -q 'androidboot.qemu_adb=1' /proc/cmdline 2>/dev/null && \
   [ -f /newroot/system/build.prop ]; then
  awk '
    BEGIN { done=0 }
    /^ro\.adb\.secure=/ { print "ro.adb.secure=0"; done=1; next }
    { print }
    END { if (!done) print "ro.adb.secure=0" }
  ' /newroot/system/build.prop > /overlays/system.build.prop
  mount -o bind /overlays/system.build.prop /newroot/system/build.prop && \
    echo "[init] adb auth off (ro.adb.secure=0)"
fi

mkdir -p /newroot/proc /newroot/sys /newroot/dev
# hidepid=2,gid=3009 matches Android: apps cannot read other UIDs' /proc.
# readproc (3009) is on zygote, system_server and adbd.
if [ -f /skip-rootfs-binds ]; then
  mount -t proc -o nosuid,nodev,noexec,hidepid=2,gid=3009 proc /newroot/proc || \
    mount -o bind /proc /newroot/proc
else
  mount -o bind /proc /newroot/proc
fi
mount -o bind /sys /newroot/sys
mount -o bind /dev /newroot/dev
mount -o bind /dev/pts /newroot/dev/pts 2>/dev/null || true

mkdir -p /newroot/sys/fs/selinux
mount -t selinuxfs selinuxfs /newroot/sys/fs/selinux || {
  echo "[init] FATAL selinuxfs"; exec /bin/sh
}
echo "[init] exec selinux_setup"
exec switch_root /newroot /system/bin/init selinux_setup
INIT
chmod +x "$STAGE/init"
if [[ -f "$INIT_DIR/bake-overlays" ]]; then
  : > "$STAGE/skip-rootfs-binds"
fi

mkdir -p "$(dirname "$OUT_IMG")"
( cd "$STAGE" && find . | cpio -o -H newc 2>/dev/null | gzip -9 ) > "$OUT_IMG"
ls -lh "$OUT_IMG"
echo "wrote $OUT_IMG"
