#!/usr/bin/env bash
# Pack vendor (+ optional Mesa prebuilts) into images/vendor.img.
set -euo pipefail
# Guest libraries must be readable independently of the launcher's host umask.
umask 022
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# Prefer explicit arg, then env (./run / lunch).
VARIANT="${1:-${VARIANT:-${TARGET_PRODUCT:-aosp_arm64-BP4A.251205.006}}}"
AOSP="$ROOT/src/aosp/$VARIANT"
[[ -d "$AOSP" ]] || { echo "missing variant tree: $AOSP (VARIANT=$VARIANT)" >&2; exit 1; }
STUB="$AOSP/qemu/vendor"
IMG="$AOSP/images/vendor.img"
STAGE="$ROOT/out/vendor-img-stage-$VARIANT"
python3 "$ROOT/scripts/check-runtime-inputs.py" "$AOSP/images/system.img"
source "$ROOT/scripts/env.sh"
# Prefer tracked guest Mesa under prebuilts/; fall back to legacy thirdparty path.
MESA_PRE_CANDIDATES=(
  "${MESA_PRE:-$ANDROID_PRE/mesa}"
  "$ROOT/thirdparty/mesa/prebuilt/arm64"
)
MESA_PRE=
for c in "${MESA_PRE_CANDIDATES[@]}"; do
  if [[ -d "$c" ]] && find "$c" -type f \( -name '*.so' -o -name '*.so.*' \) 2>/dev/null | grep -q .; then
    MESA_PRE="$c"
    break
  fi
done
SIZE_MB="${VENDOR_IMG_MB:-128}"
# src/aosp/<variant>/GPU, as read by boot-qemu.sh. mali renders with the stock
# CIX libGLES_mali + Arm gralloc from the template, so the Mesa and gfxstream
# render stacks stay out of the image.
GPU_BACKEND="$(tr -d '[:space:]' < "$AOSP/GPU" 2>/dev/null || true)"

[[ -d "$STUB" ]] || { echo "missing $STUB" >&2; exit 1; }
[[ -x "$E2/mke2fs" && -x "$E2/debugfs" ]] || { echo "need e2fsprogs" >&2; exit 1; }

# Incremental HAL compile into qemu/vendor before packing.
if [[ -f "$AOSP/device.mk" ]]; then
  echo "HALs: soong --variant $VARIANT"
  python3 "$ROOT/build/soong/soong.py" --variant "$VARIANT"
elif [[ -f "$AOSP/hardware/Makefile" ]]; then
  echo "HALs: make -C $AOSP/hardware VARIANT=$VARIANT"
  make -C "$AOSP/hardware" VARIANT="$VARIANT"
fi

rm -rf "$STAGE"
mkdir -p "$STAGE"
cp -a "$STUB"/. "$STAGE"/

# tebox display-rotation reporter: a common component (src/common), packed here
# at build time rather than committed under the variant's qemu/vendor. It runs
# in the permissive hal_keymint_system domain (seclabel in the .rc) and publishes
# the rotation on the tebox control bus (/dev/tebox_ctl). Harmless where the bus
# is absent (the device write just fails). The generic ea_set loop below labels
# bin/* vendor_file and etc/* vendor_configs_file; /vendor/etc/init/*.rc is
# imported by init.
ROT="$ROOT/src/common/hardware/interfaces/graphics/aidl/default/rotation"
if [[ -f "$ROT/tebox-rotation.sh" ]]; then
  install -D -m 0755 "$ROT/tebox-rotation.sh" "$STAGE/bin/tebox-rotation.sh"
  install -D -m 0644 "$ROT/tebox-rotation.rc" "$STAGE/etc/init/tebox-rotation.rc"
fi

# OriginOS publishes init as init.qcom.rc (androidboot.hardware=qcom) with the
# real script inside the image, so the initramfs does not bind-mount it.
if [[ -f "$AOSP/qemu/init/bake-overlays" ]]; then
  install -m 0644 "$AOSP/qemu/init/init.ranchu.rc" "$STAGE/etc/init/hw/init.qcom.rc"
  rm -f "$STAGE/etc/init/hw/init.ranchu.rc"
fi

if [[ "$GPU_BACKEND" == mali ]]; then
  # The composer and Arm gralloc still link libdrm, and the composer links
  # libgbm_mesa for its stub-allocator paths.
  [[ -n "$MESA_PRE" ]] || { echo "mali vendor needs libdrm/libgbm_mesa from Mesa prebuilts" >&2; exit 1; }
  echo "gpu: mali (no Mesa/gfxstream render stack)"
  for lib in libdrm.so libgbm_mesa.so; do
    cp -fL "$MESA_PRE/$lib" "$STAGE/lib64/$lib"
  done
# Inject Mesa guest stack if prebuilts exist.
elif [[ -n "$MESA_PRE" ]]; then
  echo "injecting Mesa from $MESA_PRE"
  mkdir -p "$STAGE/lib64/egl" "$STAGE/lib64/dri" "$STAGE/lib64/hw" "$STAGE/lib64/gbm"
  copy_tree() {
    local src=$1 dst=$2
    [[ -d "$src" ]] || return 0
    mkdir -p "$dst"
    find "$src" \( -type f -o -type l \) | while read -r f; do
      base=$(basename "$f")
      cp -fL "$f" "$dst/$base"
    done
  }
  copy_tree "$MESA_PRE/egl" "$STAGE/lib64/egl"
  copy_tree "$MESA_PRE/dri" "$STAGE/lib64/dri"
  copy_tree "$MESA_PRE/hw" "$STAGE/lib64/hw"
  copy_tree "$MESA_PRE/gbm" "$STAGE/lib64/gbm"
  shopt -s nullglob
  for f in "$MESA_PRE"/*.so "$MESA_PRE"/*.so.*; do
    base="$(basename "$f")"
    case "$base" in
      *EGL*|*GLES*) cp -fL "$f" "$STAGE/lib64/egl/" ;;
      *_dri.so|libgallium*)
        cp -fL "$f" "$STAGE/lib64/dri/"
        cp -fL "$f" "$STAGE/lib64/"
        ;;
      gralloc.*|hwcomposer.*|mapper.*) cp -fL "$f" "$STAGE/lib64/hw/" ;;
      *) cp -fL "$f" "$STAGE/lib64/" ;;
    esac
  done
  shopt -u nullglob
  if [[ -f "$STAGE/lib64/dri/libgallium_dri.so" ]]; then
    cp -f "$STAGE/lib64/dri/libgallium_dri.so" "$STAGE/lib64/dri/virtio_gpu_dri.so"
  fi
  # Extra GBM backends from install prefix / local out (optional).
  for GBM_SRC in \
      "${MESA_PREFIX:-}/lib/gbm" \
      "$ROOT/out/mesa-android/install/lib/gbm"; do
    [[ -d "$GBM_SRC" ]] && copy_tree "$GBM_SRC" "$STAGE/lib64/gbm"
  done
  [[ -f "$STAGE/lib64/libgbm_mesa.so" ]] || {
    echo "missing libgbm_mesa.so after Mesa inject from $MESA_PRE" >&2
    exit 1
  }
  ls -la "$STAGE/lib64" "$STAGE/lib64/egl" "$STAGE/lib64/dri" "$STAGE/lib64/gbm" | head -60
else
  echo "note: no Mesa prebuilts under ${MESA_PRE_CANDIDATES[*]} (props already mesa/virtio)"
fi
if [[ "$GPU_BACKEND" == mali || -n "$MESA_PRE" ]]; then
  SYSTEM_IMG="$AOSP/images/system.img"
  [[ -f "$SYSTEM_IMG" ]] || { echo "missing $SYSTEM_IMG" >&2; exit 1; }
  # libdmabufheap is not LLNDK; vendor processes need a /vendor/lib64 copy.
  for lib in libcutils.so libhardware.so libbase.so libutils.so libc++.so libz.so libdmabufheap.so; do
    "$E2/debugfs" -R "dump /system/lib64/$lib $STAGE/lib64/$lib" "$SYSTEM_IMG" >/dev/null 2>&1
    [[ -s "$STAGE/lib64/$lib" ]] || { echo "failed to extract $lib from GSI" >&2; exit 1; }
  done
  # The stock libGLES_mali.so (ro.hardware.egl=mali) NEEDs these graphics
  # HIDL/AIDL interface libs. This GSI has VNDK deprecated (no current vndk
  # apex), so the vendor/sphal namespace sees only LLNDK + /vendor/lib64 — not
  # the versioned vndk apexes. Without them the EGL loader's dlopen of
  # libGLES_mali.so fails in SurfaceFlinger's namespace and it silently falls
  # back to the Mesa libEGL.so -> softpipe (software) rendering. Bundle them
  # (same files, so ABI stays identical to the framework's /system copies).
  for lib in android.hardware.graphics.allocator-V2-ndk.so \
             android.hardware.graphics.common-V5-ndk.so \
             android.hardware.graphics.common-V7-ndk.so \
             android.hardware.graphics.mapper@4.0.so \
             libgralloctypes.so \
             android.hardware.graphics.common@1.0.so \
             android.hardware.graphics.common@1.1.so \
             android.hardware.graphics.common@1.2.so \
             android.hardware.common-V2-ndk.so; do
    "$E2/debugfs" -R "dump /system/lib64/$lib $STAGE/lib64/$lib" "$SYSTEM_IMG" >/dev/null 2>&1
    # V7 is required by BP4A+ allocator-V2-ndk in the sphal namespace; older
    # GSIs may lack it — skip only that optional bump, fail on the rest.
    if [[ ! -s "$STAGE/lib64/$lib" ]]; then
      if [[ "$lib" == android.hardware.graphics.common-V7-ndk.so ]]; then
        echo "note: $lib absent from GSI (ok on pre-V7 images)"
        continue
      fi
      echo "failed to extract $lib from GSI" >&2
      exit 1
    fi
  done
fi
# gfxstream guest stack (scripts/fetch-gfxstream-guest.sh), selected at boot
# with androidboot.tebox.gpu=gfxstream. Never overwrite VirGL/Mesa files of
# the same name (libdrm.so, ...): the VirGL path must stay as it was.
GFX_PRE="${GFX_PRE:-$ANDROID_PRE/gfxstream}"
if [[ "$GPU_BACKEND" != mali && -d "$GFX_PRE/lib64" ]]; then
  echo "injecting gfxstream guest stack from $GFX_PRE"
  (cd "$GFX_PRE" && find . -type f | sed 's|^\./||') | while read -r f; do
    [[ -e "$STAGE/$f" ]] && continue
    mkdir -p "$STAGE/$(dirname "$f")"
    cp -p "$GFX_PRE/$f" "$STAGE/$f"
  done
fi
# gfxstream ships vulkan.ranchu.so. A variant qemu/vulkan-hal names the HAL
# the loader opens (vulkan.${name}.so) so the ranchu filename is not installed.
if [[ -f "$AOSP/qemu/vulkan-hal" && -f "$STAGE/lib64/hw/vulkan.ranchu.so" ]]; then
  VULKAN_HAL=$(tr -d '[:space:]' < "$AOSP/qemu/vulkan-hal")
  if [[ -n "$VULKAN_HAL" && "$VULKAN_HAL" != ranchu ]]; then
    install -m 0644 "$STAGE/lib64/hw/vulkan.ranchu.so" "$STAGE/lib64/hw/vulkan.${VULKAN_HAL}.so"
    rm -f "$STAGE/lib64/hw/vulkan.ranchu.so"
    echo "vulkan HAL name: $VULKAN_HAL"
  fi
fi

# Keep Android's stable mapper lookup and Mesa's explicit hw lookup identical.
# Prefer the maintained HAL template over mapper copies bundled with Mesa.
if [[ -f "$STUB/lib64/hw/mapper.stub.so" ]]; then
  mkdir -p "$STAGE/lib64/hw"
  install -m 0644 "$STUB/lib64/hw/mapper.stub.so" "$STAGE/lib64/hw/mapper.stub.so"
elif [[ -f "$STUB/lib64/mapper.stub.so" ]]; then
  mkdir -p "$STAGE/lib64/hw"
  install -m 0644 "$STUB/lib64/mapper.stub.so" "$STAGE/lib64/hw/mapper.stub.so"
fi
if [[ -f "$STAGE/lib64/hw/mapper.stub.so" ]]; then
  install -m 0644 "$STAGE/lib64/hw/mapper.stub.so" "$STAGE/lib64/mapper.stub.so"
fi

if [[ "${VENDOR_IMG_MB:-}" == "" ]]; then
  SIZE_MB=256
fi

mkdir -p "$(dirname "$IMG")"
rm -f "$IMG"
dd if=/dev/zero of="$IMG" bs=1048576 count="$SIZE_MB" status=none
"$E2/mke2fs" -t ext4 -F -L vendor -m 0 "$IMG" >/dev/null

CMD="$(mktemp)"
{
  cd "$STAGE"
  find . -type d | sed 's|^\./||' | grep -v '^$' | while read -r d; do echo "mkdir /$d"; done
  find . -type f | sed 's|^\./||' | while read -r f; do echo "write $STAGE/$f /$f"; done
  find . -mindepth 1 | sed 's|^\./||' | while read -r f; do
    case "$f" in
      etc|etc/*|manifest.xml) context=vendor_configs_file ;;
      lib64|lib64/*) context=same_process_hal_file ;;
      overlay|overlay/*) context=vendor_overlay_file ;;
      app|app/*) context=vendor_app_file ;;
      *) context=vendor_file ;;
    esac
    echo "ea_set /$f security.selinux u:object_r:$context:s0"
  done
  if [[ -f bin/hw/android.hardware.security.keymint-service ]]; then
    echo "ea_set /bin/hw/android.hardware.security.keymint-service security.selinux u:object_r:hal_keymint_system_exec:s0"
  fi
  if [[ -f bin/hw/android.hardware.graphics-service ]]; then
    echo "ea_set /bin/hw/android.hardware.graphics-service security.selinux u:object_r:hal_keymint_system_exec:s0"
  fi
  if [[ -f bin/hw/android.hardware.graphics.allocator-service.arm ]]; then
    echo "ea_set /bin/hw/android.hardware.graphics.allocator-service.arm security.selinux u:object_r:hal_keymint_system_exec:s0"
  fi
  if [[ -f bin/hw/android.hardware.health-service ]]; then
    echo "ea_set /bin/hw/android.hardware.health-service security.selinux u:object_r:hal_keymint_system_exec:s0"
  fi
  if [[ -f bin/hw/android.hardware.audio-service ]]; then
    echo "ea_set /bin/hw/android.hardware.audio-service security.selinux u:object_r:hal_keymint_system_exec:s0"
  fi
  if [[ -f bin/hw/android.hardware.power-service ]]; then
    echo "ea_set /bin/hw/android.hardware.power-service security.selinux u:object_r:hal_keymint_system_exec:s0"
  fi
  if [[ -f bin/hw/android.hardware.sensors-service ]]; then
    echo "ea_set /bin/hw/android.hardware.sensors-service security.selinux u:object_r:hal_keymint_system_exec:s0"
  fi
  if [[ -f bin/hw/android.hardware.wifi-service ]]; then
    echo "ea_set /bin/hw/android.hardware.wifi-service security.selinux u:object_r:hal_keymint_system_exec:s0"
  fi
  if [[ -f bin/hw/android.hardware.wifi.supplicant-service ]]; then
    echo "ea_set /bin/hw/android.hardware.wifi.supplicant-service security.selinux u:object_r:hal_keymint_system_exec:s0"
  fi
  if [[ -f bin/qemu-wifi-fake-scan.sh ]]; then
    echo "ea_set /bin/qemu-wifi-fake-scan.sh security.selinux u:object_r:hal_keymint_system_exec:s0"
  fi

  if [[ -f bin/hw/android.hardware.radio-service ]]; then
    echo "ea_set /bin/hw/android.hardware.radio-service security.selinux u:object_r:hal_keymint_system_exec:s0"
  fi
  if [[ -f bin/hw/android.hardware.bluetooth-service ]]; then
    echo "ea_set /bin/hw/android.hardware.bluetooth-service security.selinux u:object_r:hal_keymint_system_exec:s0"
  fi
  if [[ -f bin/hw/android.hardware.biometrics.fingerprint-service ]]; then
    echo "ea_set /bin/hw/android.hardware.biometrics.fingerprint-service security.selinux u:object_r:hal_keymint_system_exec:s0"
  fi
  if [[ -f bin/hw/android.hardware.nfc-service ]]; then
    echo "ea_set /bin/hw/android.hardware.nfc-service security.selinux u:object_r:hal_keymint_system_exec:s0"
  fi
  if [[ -f bin/hw/android.hardware.gnss-service ]]; then
    echo "ea_set /bin/hw/android.hardware.gnss-service security.selinux u:object_r:hal_keymint_system_exec:s0"
  fi
  if [[ -f bin/hw/android.hardware.camera.provider-service ]]; then
    echo "ea_set /bin/hw/android.hardware.camera.provider-service security.selinux u:object_r:hal_keymint_system_exec:s0"
  fi
  if [[ -f lib64/hw/camera.default.so ]]; then
    echo "ea_set /lib64/hw/camera.default.so security.selinux u:object_r:same_process_hal_file:s0"
  fi
  if [[ -f bin/hw/android.hardware.media.c2-service ]]; then
    echo "ea_set /bin/hw/android.hardware.media.c2-service security.selinux u:object_r:hal_keymint_system_exec:s0"
  fi
  # CIX hardware Codec2 HAL (c2.cix.*, drives the MVX VPU over tebox-v4l2
  # /dev/video0,1 directly -- no gfx).  Runs in the permissive hal_keymint_system
  # domain like the other QEMU vendor HALs.
  if [[ -f "bin/hw/android.hardware.media.c2@1.0-cix-service" ]]; then
    echo "ea_set /bin/hw/android.hardware.media.c2@1.0-cix-service security.selinux u:object_r:hal_keymint_system_exec:s0"
  fi
  echo "ea_set / security.selinux u:object_r:vendor_file:s0"
} > "$CMD"
"$E2/debugfs" -w -f "$CMD" "$IMG" >/dev/null 2>&1
rm -f "$CMD"

echo "wrote $IMG ($(du -h "$IMG" | awk '{print $1}'))"
"$E2/debugfs" -R 'ls -l /lib64' "$IMG" 2>/dev/null | head -20 || true
"$E2/debugfs" -R 'cat /build.prop' "$IMG" 2>/dev/null || true
