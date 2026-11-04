#!/usr/bin/env bash
# Boot one AOSP variant on QEMU. Kernel comes from src/aosp/<variant>/KERNEL.
# Usage: boot-qemu.sh <variant>
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VARIANT="${1:?usage: boot-qemu.sh <variant>}"
AOSP="$ROOT/src/aosp/$VARIANT"
[[ -d "$AOSP" ]] || { echo "missing $AOSP" >&2; exit 1; }

# Kernel Image: KERNEL_IMG env (tebox-api CreateInstance, an absolute path to a
# caller-supplied Image) wins; else the variant's KERNEL file selects the
# built-in src/kernel/<id>/gki/Image.
if [[ -n "${KERNEL_IMG:-}" ]]; then
  KERNEL="$KERNEL_IMG"
  KID="custom"
else
  [[ -f "$AOSP/KERNEL" ]] || { echo "missing $AOSP/KERNEL (or set KERNEL_IMG)" >&2; exit 1; }
  KID="$(tr -d '[:space:]' < "$AOSP/KERNEL")"
  KERNEL="$ROOT/src/kernel/$KID/gki/Image"
fi
# SYSTEM_IMG / VENDOR_IMG / INITRD_IMG point an instance at its own images
# (tebox-api CreateInstance); the variant's images are the default.
SYSTEM="${SYSTEM_IMG:-$AOSP/images/system.img}"
VENDOR="${VENDOR_IMG:-$AOSP/images/vendor.img}"
INITRD="${INITRD_IMG:-$AOSP/images/initramfs.img}"
WORKDIR="$ROOT/out/test-$VARIANT"
# The service client gives every Android instance its own userdata, QMP and
# debug sockets.  Keep the legacy per-variant directory when no instance was
# requested from the command line.
if [[ -n "${TEBOX_INSTANCE_DIR:-}" ]]; then
  WORKDIR="${TEBOX_INSTANCE_DIR%/}"
fi
python3 "$ROOT/scripts/check-runtime-inputs.py" "$KERNEL" "$SYSTEM" "$VENDOR" "$INITRD"
mkdir -p "$WORKDIR"

source "$ROOT/scripts/env.sh"

resolve_qemu() {
  if [[ -n "${QEMU:-}" ]]; then
    if [[ -x "$QEMU" ]]; then
      echo "$QEMU"
    elif command -v "$QEMU" >/dev/null 2>&1; then
      command -v "$QEMU"
    else
      echo "QEMU is not executable: $QEMU" >&2
      return 1
    fi
    return 0
  fi
  # Prefer the tree-built QEMU so local UI patches (ui/sdl2.c, cast hook)
  # take effect. Fall back to prebuilts when absent.
  local candidates=(
    "$ROOT/out/qemu-build/qemu-system-aarch64"
    "$HOST_PRE/qemu/bin/qemu-system-aarch64"
    "$HOST_PRE/qemu/qemu"
    "$ROOT/prebuilts/host/$HOST_ID/qemu/bin/qemu-system-aarch64"
  )
  local c
  for c in "${candidates[@]}"; do
    [[ -x "$c" ]] && { echo "$c"; return; }
  done
  if command -v qemu-system-aarch64 >/dev/null 2>&1; then
    command -v qemu-system-aarch64
    return
  fi
  echo "missing qemu-system-aarch64 (build host QEMU or set QEMU=...)" >&2
  exit 1
}

QEMU_BIN="$(resolve_qemu)"

# Mesa's libGL must not shadow Apple's private libGL on macOS.
if [[ "$HOST_OS" == darwin ]]; then
  export DYLD_LIBRARY_PATH="$EPOXY_PREFIX/lib:$VIRGL_PREFIX/lib:$RUTABAGA_PREFIX/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
else
  export LD_LIBRARY_PATH="$EPOXY_PREFIX/lib:$VIRGL_PREFIX/lib:$RUTABAGA_PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

[[ -f "$KERNEL" ]] || { echo "missing $KERNEL (fetch kernel for $KID)" >&2; exit 1; }
[[ -f "$SYSTEM" ]] || { echo "missing $SYSTEM" >&2; exit 1; }
[[ -f "$VENDOR" ]] || { echo "missing $VENDOR" >&2; exit 1; }
[[ -f "$INITRD" ]] || { echo "missing $INITRD" >&2; exit 1; }

if [[ ! -f "$WORKDIR/userdata.img" ]]; then
  # Sparse: host space grows with guest data, not with the configured size.
  # Vendor ROMs install ~700 MB of apps on first boot; 1 GiB fills up.
  dd if=/dev/zero of="$WORKDIR/userdata.img" bs=1048576 count=0 seek="${USERDATA_MB:-8192}" status=none
  "$E2/mke2fs" -t ext4 -F -L userdata -O quota,project \
    -E quotatype=usrquota:grpquota:prjquota "$WORKDIR/userdata.img"
fi
# vold assigns project IDs to app external dirs (Android/data/<pkg>); without
# ext4 project quota getExternalCacheDir() is null and e.g. Camera cannot save.
# Upgrade images made before this in place (the VM is not running yet).
if ! "$E2/dumpe2fs" -h "$WORKDIR/userdata.img" 2>/dev/null | grep -q "features:.* project"; then
  "$E2/tune2fs" -O quota,project -Q usrquota,grpquota,prjquota "$WORKDIR/userdata.img" >/dev/null
fi

ACCEL=tcg
case "$HOST_OS" in
  darwin) ACCEL=hvf ;;
  linux)  [[ -e /dev/kvm && -r /dev/kvm && -w /dev/kvm ]] && ACCEL=kvm || ACCEL=tcg ;;
esac
CPU=host
[[ "$ACCEL" != tcg ]] || CPU=max

# Kernel cmdline: TEBOX_CMDLINE env (tebox-api, whitespace-separated tokens)
# wins; else the variant's cmdline file. Either way QEMU_EXTRA_APPEND is still
# appended below.
APPEND=()
if [[ -n "${TEBOX_CMDLINE:-}" ]]; then
  # shellcheck disable=SC2206
  APPEND+=($TEBOX_CMDLINE)
else
  CMDLINE_FILE="${CMDLINE_FILE:-$AOSP/qemu/cmdline/boot}"
  [[ -f "$CMDLINE_FILE" ]] || { echo "missing cmdline file: $CMDLINE_FILE (or set TEBOX_CMDLINE)" >&2; exit 1; }
  while IFS= read -r line || [[ -n "$line" ]]; do
    # trim
    line="${line#"${line%%[![:space:]]*}"}"
    line="${line%"${line##*[![:space:]]}"}"
    [[ -z "$line" || "$line" == \#* ]] && continue
    # allow multiple tokens on one line
    # shellcheck disable=SC2206
    APPEND+=($line)
  done < "$CMDLINE_FILE"
fi
((${#APPEND[@]} > 0)) || { echo "empty cmdline (set TEBOX_CMDLINE or $AOSP/qemu/cmdline/boot)" >&2; exit 1; }

EXTRA=()
EXTRA+=(-initrd "$INITRD")

# TCP ADB is on by default (host loopback only). Set QEMU_ADB=0 to disable.
# Initramfs forces ro.adb.secure=0 when androidboot.qemu_adb=1 (no RSA prompt).
NETDEV=user,id=net0
if [[ "${QEMU_ADB:-1}" == 1 ]]; then
  ADB_PORT="${ADB_PORT:-5555}"
  if ! [[ "$ADB_PORT" =~ ^[0-9]+$ ]] || (( ${#ADB_PORT} > 5 )); then
    echo "ADB_PORT must be an integer in 1..65535" >&2
    exit 1
  fi
  ADB_PORT=$((10#$ADB_PORT))
  (( ADB_PORT >= 1 && ADB_PORT <= 65535 )) || {
    echo "ADB_PORT must be an integer in 1..65535" >&2
    exit 1
  }
  # ADB_BIND=0.0.0.0 exposes adb to the LAN (headless remote hosts).
  ADB_BIND="${ADB_BIND:-127.0.0.1}"
  APPEND+=(androidboot.qemu_adb=1)
  NETDEV+=",hostfwd=tcp:$ADB_BIND:$ADB_PORT-:5555"
  echo "adb:     $ADB_BIND:$ADB_PORT (guest TCP 5555)"
fi
# Extra host-specific kernel/androidboot tokens, e.g. the CIX Mali launcher.
if [[ -n "${QEMU_EXTRA_APPEND:-}" ]]; then
  # shellcheck disable=SC2206
  APPEND+=($QEMU_EXTRA_APPEND)
fi

if [[ "${QEMU_DEBUG:-0}" == 1 ]]; then
  APPEND+=(androidboot.qemu_debug=1 printk.devkmsg=on)
  EXTRA+=(-device virtio-serial-pci
    -chardev socket,id=debug,path="$WORKDIR/debug.sock",server=on,wait=off
    -device virtconsole,chardev=debug)
fi
[[ "${SNAPSHOT:-0}" == 1 ]] && EXTRA+=(-snapshot)
[[ -n "${QMP_SOCKET:-}" ]] && EXTRA+=(-qmp "unix:$QMP_SOCKET,server=on,wait=off")
# A second monitor for streamed cast input, so a long-lived control client
# never holds the lifecycle QMP socket (QEMU serves one client per monitor).
[[ -n "${QMP_INPUT_SOCKET:-}" ]] && EXTRA+=(-qmp "unix:$QMP_INPUT_SOCKET,server=on,wait=off")

GPU_DEV=virtio-gpu-pci
# A normal ./run uses the verified GPU path; headless runs can use software KMS.
FORCE_VIRGL="${FORCE_VIRGL:-${GRAPHIC:-1}}"
if [[ "$FORCE_VIRGL" == 1 ]]; then
  DEVICES="$("$QEMU_BIN" -device help)"
  [[ "$DEVICES" == *virtio-gpu-gl-pci* ]] || {
    echo "QEMU has no virtio-gpu-gl-pci; build the project QEMU or use FORCE_VIRGL=0 for diagnostics" >&2
    exit 1
  }
  GPU_DEV=virtio-gpu-gl-pci
fi

# GPU backend, like KERNEL: src/aosp/<variant>/GPU holds "gfxstream" (or
# "gfx"; also the default without the file), "virgl", or "mali" (CIX Mali
# kbase passthrough + guest libGLES_mali). FORCE_RUTABAGA=1/0 overrides
# gfxstream/virgl for one boot; mali ignores FORCE_RUTABAGA.
GPU_MALI=0
if [[ -z "${FORCE_RUTABAGA:-}" ]]; then
  # TEBOX_GPU env (tebox-api) wins; else the variant's GPU file; else gfxstream.
  GPU_BACKEND="${TEBOX_GPU:-$(tr -d '[:space:]' < "$AOSP/GPU" 2>/dev/null || echo gfxstream)}"
  # R1P / TEBOX_MALI: guest drives the real Mali; never stay on gfxstream.
  if [[ "${TEBOX_MALI:-0}" == 1 && "$HOST_OS" == linux ]]; then
    case "${GPU_BACKEND:-gfxstream}" in
      gfxstream|gfx|"") GPU_BACKEND=mali ;;
    esac
  fi
  case "${GPU_BACKEND:-gfxstream}" in
    gfxstream|gfx) FORCE_RUTABAGA=1 ;;
    virgl) FORCE_RUTABAGA=0 ;;
    mali) FORCE_RUTABAGA=0; GPU_MALI=1 ;;
    *) echo "$AOSP/GPU: unknown backend '$GPU_BACKEND' (use gfxstream, virgl or mali)" >&2; exit 1 ;;
  esac
elif [[ "${TEBOX_MALI:-0}" == 1 && "$HOST_OS" == linux && "${FORCE_RUTABAGA}" == 0 ]]; then
  # Explicit FORCE_RUTABAGA=0 on a Mali host still means guest Mali EGL.
  GPU_MALI=1
fi
if [[ "$GPU_MALI" == 1 ]]; then
  # Guest: stock CIX libGLES_mali + vulkan.mali. The guest composer CPU-copies
  # the client target into a dumb buffer, so scanout needs only a 2D
  # virtio-gpu and the host never starts virglrenderer; the display backend
  # (egl-headless from cix-mali-env) still feeds cast. Encode is TEBOX_V4L2 /
  # c2.cix, not gfxstream venc.
  GPU_DEV=virtio-gpu-pci
  APPEND+=(androidboot.tebox.gpu=mali androidboot.hardwareegl=mali
    androidboot.hardware.vulkan=mali androidboot.opengles.version=196610
    androidboot.hwui.renderer="${HWUI_RENDERER:-skiagl}")
  # Every frame costs passthrough ioctls plus a scanout upload; 60 halves it.
  REFRESH_HZ="${REFRESH_HZ:-60}"
  echo "gpu:     mali (guest libGLES_mali via tebox-mali; display=$GPU_DEV)"
  # Cast still runs on the egl-headless UI thread; without a dma-heap the hook
  # falls back to sync glReadPixels (hundreds of MB/s). Same heap as gfxstream.
  if [[ "$HOST_OS" == linux && -z "${TEBOX_CAST_DMABUF_HEAP+x}" && -w /dev/dma_heap/system ]]; then
    export TEBOX_CAST_DMABUF_HEAP=/dev/dma_heap/system
  fi
elif [[ "$FORCE_RUTABAGA" == 1 ]]; then
  DEVICES="$("$QEMU_BIN" -device help)"
  [[ "$DEVICES" == *virtio-gpu-rutabaga-pci* ]] || {
    echo "QEMU has no virtio-gpu-rutabaga-pci; build the project QEMU or use FORCE_RUTABAGA=0 for diagnostics" >&2
    exit 1
  }
  # gfxstream with Vulkan only (GuestVulkanOnly): the host needs no OpenGL
  # (macOS has no EGL, and gfxstream's GLES translator needs ANGLE's shader
  # translator there), and composes with Vulkan. The guest runs GLES on
  # Vulkan through the GSI's ANGLE. Blob memory carries the
  # address-space-graphics transport.
  GPU_DEV="virtio-gpu-rutabaga,gfxstream-vulkan=on,x-gfxstream-composer=on,blob=on,hostmem=${RUTABAGA_HOSTMEM:-512M}"
  # Guest: ANGLE on gfxstream Vulkan (prebuilts/.../gfxstream). Stub
  # IAllocator creates virtio-gpu resources with cros_gralloc-layout ints
  # (gpu_buffer.h); ro.hardware.gralloc=minigbm so vulkan.ranchu parses them.
  APPEND+=(androidboot.tebox.gpu=gfxstream androidboot.hardwareegl=angle
    androidboot.hardware.vulkan=ranchu androidboot.opengles.version=196610
    androidboot.hardware.gltransport=virtio-gpu-asg
    androidboot.hardware.gralloc=minigbm)
  # HWUI renders with Vulkan (skiavk; HWUI_RENDERER=skiagl overrides): HWUI on
  # ANGLE's GLES misrenders some layers (CIX Mali: launcher scrim drawn
  # transparent; macOS: ColorOS clock dial). HWUI's Vulkan backend needs two
  # queues, so on macOS (MoltenVK has one) gfxstream adds a virtual one.
  APPEND+=(androidboot.hwui.renderer="${HWUI_RENDERER:-skiavk}")
  echo "gpu:     gfxstream (rutabaga + stub alloc / minigbm layout; from GPU file)"
  if [[ "$HOST_OS" == darwin ]]; then
    export GFXSTREAM_RENDERER_FEATURES="${GFXSTREAM_RENDERER_FEATURES:-VulkanVirtualQueue:enabled}"
  fi
  # Linux (R1P): zero-copy display. gfxstream keeps ColorBuffers linear and
  # exports them as dma-bufs that the GL display (egl-headless) imports, so
  # frames are never read back by the CPU (virtio-gpu-rutabaga.c). Drivers
  # that can import but not export image memory (CIX Mali) get ColorBuffers
  # from the dma-buf heap instead (GFXSTREAM_DMABUF_HEAP, local gfxstream patch).
  if [[ "$HOST_OS" == linux ]]; then
    if [[ -z "${GFXSTREAM_DMABUF_HEAP+x}" && -w /dev/dma_heap/system ]]; then
      export GFXSTREAM_DMABUF_HEAP=/dev/dma_heap/system
    fi
    # Cast scales on the GPU. Per-frame CPU readback stays off; QMP
    # screendump asks egl-headless for one readback of the GL scanout
    # (dma-buf scanout has no console surface, so leaving this at 0 does
    # not by itself make GetPreview work).
    if [[ -z "${TEBOX_CAST_DMABUF_HEAP+x}" && -w /dev/dma_heap/system ]]; then
      export TEBOX_CAST_DMABUF_HEAP=/dev/dma_heap/system
    fi
    export TEBOX_EGL_READBACK="${TEBOX_EGL_READBACK:-0}"
    export GFXSTREAM_LINEAR_COLOR_BUFFERS="${GFXSTREAM_LINEAR_COLOR_BUFFERS:-1}"
    export TEBOX_RUTABAGA_DMABUF="${TEBOX_RUTABAGA_DMABUF:-1}"
  fi
  # macOS: Vulkan through MoltenVK (Homebrew molten-vk) unless overridden.
  if [[ "$HOST_OS" == darwin && -z "${ANDROID_EMU_VK_LOADER_PATH:-}" && -f /opt/homebrew/lib/libMoltenVK.dylib ]]; then
    export ANDROID_EMU_VK_LOADER_PATH=/opt/homebrew/lib/libMoltenVK.dylib
  fi
else
  # VirGL: selected when src/aosp/<variant>/GPU is "virgl" (or FORCE_RUTABAGA=0).
  # video_encode selects media_codecs_tebox_video.xml so c2.tebox.* appears
  # (host needs VIRGL_VIDEO=1). GPU_DEV is virtio-gpu-gl-pci when FORCE_VIRGL=1.
  # stub allocator GBM handles match VirGL kms_display setTarget.
  APPEND+=(androidboot.tebox.gpu=virgl androidboot.hardwareegl=mesa
    androidboot.opengles.version=196610
    androidboot.hwui.renderer="${HWUI_RENDERER:-skiagl}"
    androidboot.hardware.gralloc=stub
    androidboot.tebox.video_encode=1 androidboot.tebox.video_decode=1)
  if [[ "$HOST_OS" == linux ]]; then
    # Cast scales into a dma-heap buffer the encoder reads in place; without
    # this the GL path falls back to a per-frame synchronous glReadPixels on
    # the UI thread (CIX Mali egl-headless, virtio-gpu-gl).
    if [[ -z "${TEBOX_CAST_DMABUF_HEAP+x}" && -w /dev/dma_heap/system ]]; then
      export TEBOX_CAST_DMABUF_HEAP=/dev/dma_heap/system
    fi
    export TEBOX_EGL_READBACK="${TEBOX_EGL_READBACK:-0}"
  fi
  echo "gpu:     virgl (Mesa + stub gralloc + c2.tebox; from GPU file / FORCE_RUTABAGA=0)"
fi
# Display refresh rate (graphics HAL vsync). It caps every app's frame rate,
# benchmarks included; 120 lets the GPU run past 60 fps.
APPEND+=(androidboot.tebox.refresh_hz="${REFRESH_HZ:-120}")


# Same SDL GL window for both backends. rutabaga scanouts reach it as a
# host-native (x8r8g8b8) shadow (virtio-gpu-rutabaga.c), which the GL
# console can upload.
if [[ "$FORCE_VIRGL" == 1 ]]; then
  NGFX=(-display "${QEMU_DISPLAY:-sdl,gl=core,show-cursor=on}")
elif [[ "${GRAPHIC:-1}" == 1 ]]; then
  if [[ "$HOST_OS" == darwin ]]; then
    NGFX=(-display "${QEMU_DISPLAY:-cocoa,zoom-to-fit=on,zoom-interpolation=on,show-cursor=on,full-grab=off}")
  else
    NGFX=(-display "${QEMU_DISPLAY:-sdl,show-cursor=on}")
  fi
else
  NGFX=(-display none)
fi
# PCI slot 6 matches the guest's sysfs GPU labels.
NGFX+=(-device "$GPU_DEV,addr=0x6,xres=${XRES:-1080},yres=${YRES:-2400}"
  -device virtio-keyboard-pci -device virtio-tablet-pci,touchscreen=on)

# Host cast: QEMU tebox-cast-hook → Unix socket → tebox-cast (see src/service/cast/).
if [[ "${TEBOX_CAST:-0}" == 1 ]]; then
  export TEBOX_CAST=1
  export TEBOX_CAST_SOCK="${TEBOX_CAST_SOCK:-$WORKDIR/cast.sock}"
  export TEBOX_CAST_FPS="${TEBOX_CAST_FPS:-60}"
  export TEBOX_CAST_MAX_W="${TEBOX_CAST_MAX_W:-720}" TEBOX_CAST_MAX_H="${TEBOX_CAST_MAX_H:-1600}"
  # GPU blit → dma-buf → mvxenc (no CPU glReadPixels). Backends that already
  # set this keep theirs; mali used to miss it and peg the host on readback.
  if [[ "$HOST_OS" == linux && -z "${TEBOX_CAST_DMABUF_HEAP+x}" && -w /dev/dma_heap/system ]]; then
    export TEBOX_CAST_DMABUF_HEAP=/dev/dma_heap/system
  fi
  echo "cast:    TEBOX_CAST=1 sock=$TEBOX_CAST_SOCK fps=$TEBOX_CAST_FPS max=${TEBOX_CAST_MAX_W}x$TEBOX_CAST_MAX_H${TEBOX_CAST_DMABUF_HEAP:+ dmabuf=$TEBOX_CAST_DMABUF_HEAP}"
  # Guest audio, microphone and camera reach tebox-cast through virtio-serial
  # ports; the audio and camera HALs find them by name under
  # /sys/class/virtio-ports. Sockets sit next to cast.sock.
  MEDIA_DIR="$(dirname "$TEBOX_CAST_SOCK")"
  # Fixed high slot: auto-assignment would shift virtio-net into slot 6 (GPU).
  EXTRA+=(-device virtio-serial-pci,id=media,addr=0x10)
  MEDIA_PORTS="audio-out:tebox.audio_out audio-in:tebox.audio_in camera:tebox.camera"
  if [[ "$FORCE_RUTABAGA" == 1 ]]; then
    # gfxstream has no VirGL video: the guest Codec2 encoder sends resource
    # ids over tebox.venc and tebox-cast encodes on the host, reading the
    # frames from QEMU's resource server (res.sock, virtio-gpu-rutabaga.c).
    MEDIA_PORTS+=" venc:tebox.venc aenc:tebox.aenc"
    export TEBOX_RES_SOCK="$MEDIA_DIR/res.sock"
    APPEND+=(androidboot.tebox.video_encode=1)
    if [[ "$HOST_OS" == linux ]]; then
      # Decoding the other way round: the guest sends the bitstream over
      # tebox.vdec, the host V4L2 decoder decodes and QEMU draws each picture
      # into the guest's output buffer on the GPU (blit.sock).
      MEDIA_PORTS+=" vdec:tebox.vdec"
      export TEBOX_BLIT_SOCK="$MEDIA_DIR/blit.sock"
    fi
  fi
  for port in $MEDIA_PORTS; do
    sock="${port%%:*}"
    rm -f "$MEDIA_DIR/$sock.sock"
    EXTRA+=(-chardev "socket,id=${sock/-/_},path=$MEDIA_DIR/$sock.sock,server=on,wait=off"
      -device "virtserialport,bus=media.0,chardev=${sock/-/_},name=${port#*:}")
  done
  # tebox control bus (guest <-> host leg): a dedicated virtio device, not a
  # virtio-serial port, so the guest tebox_ctl.ko can route framed teboxctl
  # messages by channel between guest processes AND the host. tebox-api reads
  # ctl.sock (rotation = channel 1 today; more channels later). QEMU is just
  # the byte pipe; the API never queries the guest over adb.
  rm -f "$MEDIA_DIR/ctl.sock"
  # disable-legacy=on: the arm virt machine is modern-only; a virtio-pci device
  # left at legacy+modern fails to realize here (as tebox-v4l2/mali also do).
  EXTRA+=(-chardev "socket,id=teboxctl,path=$MEDIA_DIR/ctl.sock,server=on,wait=off"
    -device "tebox-ctl-pci,addr=0x13,disable-legacy=on,disable-modern=off,chardev=teboxctl")
fi

# tebox (local, Linux/R1P): MVX V4L2 passthrough device so the guest's stock
# CIX c2.cix Codec2 HAL drives the host VPU (/dev/video0 mvxdec, /dev/video1
# mvxenc) directly. Fixed slot to avoid shifting the GPU/net slots.
if [[ "${TEBOX_V4L2:-0}" == 1 && "$HOST_OS" == linux ]]; then
  EXTRA+=(-device tebox-v4l2-pci,addr=0x11,disable-legacy=on,disable-modern=off)
  echo "v4l2:    TEBOX_V4L2=1 (MVX passthrough -> guest /dev/video)"
fi

# tebox-mali: CIX Mali kbase /dev/mali0 passthrough so the guest's Mali userspace
# drives the real GPU directly (no gfxstream/virgl). Needs a large host-visible
# hostmem BAR; the guest tebox_mali.ko exposes /dev/mali0. Fixed slot 0x12.
MACHINE="virt,gic-version=3"
if [[ "${TEBOX_MALI:-0}" == 1 && "$HOST_OS" == linux ]]; then
  EXTRA+=(-device tebox-mali-pci,addr=0x12,disable-legacy=on,disable-modern=off)
  # Back guest RAM with a memfd so tebox-mali can import a scattered guest
  # buffer (minigbm gives SurfaceFlinger's render targets as non-contiguous
  # guest pages) into the real GPU: it remaps each guest page segment from the
  # memfd into one contiguous host VA range and does a single kbase USER_BUFFER
  # import. Anonymous RAM (the default) has no fd to remap from. share=on is
  # required for the memfd pages to be mappable MAP_SHARED from the device.
  EXTRA+=(-object "memory-backend-memfd,id=mem0,size=${MEM:-4096}M,share=on")
  MACHINE="virt,gic-version=3,memory-backend=mem0"
  echo "mali:    TEBOX_MALI=1 (Mali kbase passthrough -> guest /dev/mali0)"
fi

# macOS host video: QEMU itself serves tebox.vdec and tebox.venc
# (hw/display/tebox-vt-{vdec,venc}.c) on VideoToolbox; gfxstream blits between
# the guest's buffers and VideoToolbox's IOSurfaces on the GPU.
# TEBOX_VDEC=0 / TEBOX_VENC=0 leave the guest on its software codecs.
mac_video_port() {  # <sock name> <port name> -> exports the socket path
  local sock="$MEDIA_DIR/$1.sock"
  # Unix socket paths are limited to 103 bytes on macOS; long variant names
  # exceed that, so use a short per-instance name under /tmp instead.
  if (( ${#sock} > 103 )); then
    sock="/tmp/tebox-$1-$(printf '%s' "$MEDIA_DIR" | cksum | cut -d' ' -f1).sock"
  fi
  rm -f "$sock"
  EXTRA+=(-chardev "socket,id=$1,path=$sock,server=on,wait=off"
    -device "virtserialport,bus=media.0,chardev=$1,name=$2")
  MAC_VIDEO_SOCK="$sock"
}
if [[ "$HOST_OS" == darwin && "$FORCE_RUTABAGA" == 1 &&
      ( "${TEBOX_VDEC:-1}" == 1 || "${TEBOX_VENC:-1}" == 1 ) ]]; then
  if [[ -z "${MEDIA_DIR:-}" ]]; then
    MEDIA_DIR="$WORKDIR"
    EXTRA+=(-device virtio-serial-pci,id=media,addr=0x10)
  fi
  if [[ "${TEBOX_VDEC:-1}" == 1 ]]; then
    mac_video_port vdec tebox.vdec
    export TEBOX_VDEC_SOCK="$MAC_VIDEO_SOCK"
    APPEND+=(androidboot.tebox.video_decode=1)
    echo "vdec:    VideoToolbox (tebox.vdec, $TEBOX_VDEC_SOCK)"
  fi
  # With TEBOX_CAST=1 tebox.venc already goes to tebox-cast.
  if [[ "${TEBOX_VENC:-1}" == 1 && "${TEBOX_CAST:-0}" != 1 ]]; then
    mac_video_port venc tebox.venc
    export TEBOX_VENC_SOCK="$MAC_VIDEO_SOCK"
    APPEND+=(androidboot.tebox.video_encode=1)
    echo "venc:    VideoToolbox (tebox.venc, $TEBOX_VENC_SOCK)"
  fi
fi

# Audio HAL always looks up tebox.audio_out / tebox.audio_in by name. Cast
# already added those ports; a local ./run still needs them or PCM is dropped.
if [[ "${TEBOX_CAST:-0}" != 1 ]]; then
  if [[ -z "${MEDIA_DIR:-}" ]]; then
    MEDIA_DIR="$WORKDIR"
    EXTRA+=(-device virtio-serial-pci,id=media,addr=0x10)
  fi
  add_audio_port() {  # <sock name> <port name>
    local sock="$MEDIA_DIR/$1.sock"
    if (( ${#sock} > 103 )); then
      sock="/tmp/tebox-$1-$(printf '%s' "$MEDIA_DIR" | cksum | cut -d' ' -f1).sock"
    fi
    rm -f "$sock"
    EXTRA+=(-chardev "socket,id=$1,path=$sock,server=on,wait=off"
      -device "virtserialport,bus=media.0,chardev=$1,name=$2")
    AUDIO_PORT_SOCK="$sock"
  }
  add_audio_port audio_out tebox.audio_out
  AUDIO_OUT_SOCK="$AUDIO_PORT_SOCK"
  add_audio_port audio_in tebox.audio_in
  echo "audio:   virtio-serial tebox.audio_out ($AUDIO_OUT_SOCK)"
  if [[ "${GRAPHIC:-1}" == 1 ]]; then
    FFPLAY=""
    for c in \
        "$ROOT/prebuilts/host/${HOST_ID:-darwin-arm64}/ffmpeg/bin/ffplay" \
        /opt/homebrew/bin/ffplay \
        "$(command -v ffplay 2>/dev/null || true)"; do
      [[ -n "$c" && -x "$c" ]] && { FFPLAY="$c"; break; }
    done
    if [[ -n "$FFPLAY" ]]; then
      (
        for _ in $(seq 1 80); do
          [[ -S "$AUDIO_OUT_SOCK" ]] && break
          sleep 0.1
        done
        while [[ -S "$AUDIO_OUT_SOCK" ]]; do
          "$FFPLAY" -nodisp -loglevel quiet -fflags nobuffer -flags low_delay \
            -f s16le -ar 48000 -ac 2 -i "unix:$AUDIO_OUT_SOCK" || true
          sleep 1
        done
      ) >/dev/null 2>&1 &
      echo "audio:   local ffplay $AUDIO_OUT_SOCK"
    fi
  fi
fi

echo "variant: $VARIANT"
echo "kernel:  $KERNEL  ($KID)"
echo "system:  $SYSTEM"
echo "vendor:  $VENDOR"
echo "initrd:  $INITRD"
echo "cmdline: $CMDLINE_FILE"
echo "qemu:    $QEMU_BIN"
echo "accel:   $ACCEL"
echo "gpu:     $GPU_DEV"
echo "==> Ctrl+C in this terminal stops QEMU"

# Keep QEMU in the terminal foreground: background stdio can trigger
# SIGTTIN/SIGTTOU and stop the VM before boot. Plain stdio preserves Ctrl+C.
exec "$QEMU_BIN" \
  -machine "${MACHINE:-virt,gic-version=3}" \
  -cpu "$CPU" \
  -accel "$ACCEL" \
  -m "${MEM:-4096}" \
  -smp "${SMP:-4}" \
  -kernel "$KERNEL" \
  "${EXTRA[@]}" \
  -append "${APPEND[*]}" \
  -drive if=none,file="$SYSTEM",format=raw,id=system,readonly=on \
  -device virtio-blk-pci,drive=system,serial=system \
  -drive if=none,file="$WORKDIR/userdata.img",format=raw,id=userdata \
  -device virtio-blk-pci,drive=userdata,serial=userdata \
  -drive if=none,file="$VENDOR",format=raw,id=vendor,readonly=on \
  -device virtio-blk-pci,drive=vendor,serial=vendor \
  -device virtio-net-pci,netdev=net0 \
  -netdev "$NETDEV" \
  "${NGFX[@]}" \
  -serial "${QEMU_SERIAL:-stdio}"
