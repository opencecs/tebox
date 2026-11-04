#!/usr/bin/env bash
# Assemble a products-only tebox image and deploy to R1P.
# Dockerfile at repo root; Tracked under scripts/; local copies may still live under out/.
#
# Flow: compile → out/r1p-bin (+ ffmpeg) → copy only runtime files into
# out/r1p-docker/ → docker build. Never pack the source tree, never
# docker-cp an old image as the stage base.
#
# Usage: out/r1p-deploy.sh [build|push|start|stop|status|logs|export|all]
#
# Image ships: stripped QEMU, tebox-api, tebox-cast + FFmpeg, GKI Image +
# vendor_modules/*.ko, CIX Mali debs, boot scripts. Guest system/vendor/
# initramfs stay outside (user-images).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# shellcheck source=/dev/null
source "$ROOT/scripts/env.sh"
VARIANT="${VARIANT:-aosp_arm64-BP4A.251205.006}"
AOSP_META=""
for d in "$ROOT/src/aosp/$VARIANT" "$ROOT/out/r1p-context/runtime-src/aosp/$VARIANT"; do
  [[ -f "$d/KERNEL" ]] && { AOSP_META="$d"; break; }
done
[[ -n "$AOSP_META" ]] || { echo "missing $VARIANT KERNEL metadata" >&2; exit 1; }
AOSP="$AOSP_META"
KID="$(tr -d '[:space:]' < "$AOSP_META/KERNEL")"
KDIR="$ROOT/src/kernel/$KID"
R1P_HOST="${R1P_HOST:-user@10.10.2.142}"
R1P_DIR="${R1P_DIR:-/mmc/tebox}"
R1P_NAME="${R1P_NAME:-tebox}"
R1P_ADB_PORT="${R1P_ADB_PORT:-5555}"
R1P_GRPC_PORT="${R1P_GRPC_PORT:-50051}"
R1P_CAST_PORT="${R1P_CAST_PORT:-9901}"
R1P_AUDIO_PORT="${R1P_AUDIO_PORT:-9904}"
R1P_CONTROL_PORT="${R1P_CONTROL_PORT:-9905}"
TEBOX_VERSION="${TEBOX_VERSION:-1.0.0}"
R1P_IMAGE="${R1P_IMAGE:-tebox:${TEBOX_VERSION}}"
R1P_USER_IMAGES="${R1P_USER_IMAGES:-$R1P_DIR/user-images}"
R1P_INSTANCE_ROOT="${R1P_INSTANCE_ROOT:-$R1P_DIR/instances}"
R1P_DRIVERS="${R1P_DRIVERS:-cix-8180}"
R1P_APT_MIRROR="${R1P_APT_MIRROR-http://mirrors.ustc.edu.cn}"
R1P_BUILDER="${R1P_BUILDER:-tebox-builder}"
STAGE="$ROOT/out/r1p-docker"
BIN="$ROOT/out/r1p-bin"
FFMPEG_OUT="$ROOT/out/r1p-ffmpeg"

remote() {
  ssh "$R1P_HOST" "export SUDO_ASKPASS=\${R1P_SUDO_ASKPASS:-\$HOME/.askpass}; $*"
}

# Minimal shared FFmpeg for tebox-cast (V4L2 is primary encode on R1P).
build_min_ffmpeg_cast() {
  if [[ -x "$FFMPEG_OUT/lib/libavcodec.so" && -x "$BIN/service/tebox-cast" ]]; then
    echo "reuse $FFMPEG_OUT + tebox-cast"
    return 0
  fi
  docker image inspect "$R1P_BUILDER" >/dev/null 2>&1 || {
    echo "missing $R1P_BUILDER (needed to build minimal FFmpeg/cast)" >&2
    exit 1
  }
  mkdir -p "$FFMPEG_OUT" "$BIN/service" "$ROOT/out/r1p-ffmpeg-build"
  docker run --rm --platform linux/arm64 \
    -v "$ROOT/thirdparty/ffmpeg:/src/thirdparty/ffmpeg:ro" \
    -v "$ROOT/src/service/cast:/src/src/service/cast:ro" \
    -v "$FFMPEG_OUT:/out/ffmpeg" \
    -v "$BIN/service:/out/service" \
    -v "$ROOT/out/r1p-ffmpeg-build:/build" \
    -w /build "$R1P_BUILDER" bash -c '
      set -euo pipefail
      if [[ ! -f /out/ffmpeg/lib/libavcodec.so ]]; then
        /src/thirdparty/ffmpeg/configure \
          --prefix=/out/ffmpeg \
          --disable-static --enable-shared \
          --disable-doc --disable-debug --disable-network \
          --disable-programs --disable-autodetect \
          --disable-avdevice --disable-avformat --disable-avfilter \
          --disable-everything \
          --enable-avcodec --enable-avutil --enable-swscale --enable-swresample \
          --enable-parser=h264,hevc,aac,mjpeg \
          --enable-decoder=h264,hevc,aac,mjpeg \
          --enable-encoder=aac,mjpeg \
          --enable-filter=scale,aresample
        make -j"$(nproc)"
        make install
      fi
      rm -rf /tmp/cast-out && mkdir -p /tmp/cast-out /out/service
      make -C /src/src/service/cast \
        ROOT=/src OUT=/tmp/cast-out \
        CAST_SYSTEM_FFMPEG=1 \
        PKG_CONFIG_PATH=/out/ffmpeg/lib/pkgconfig \
        CFLAGS="-O2 -Wall -Iinclude -Isrc" \
        LDFLAGS="-Wl,-rpath,/opt/tebox/lib"
      strip --strip-unneeded /tmp/cast-out/tebox-cast
      cp -L /tmp/cast-out/tebox-cast /out/service/tebox-cast
    '
}

build_tebox_api() {
  mkdir -p "$BIN"
  if [[ -x "$BIN/tebox-api" ]]; then
    # Rebuild when sources are newer than the product.
    local newest
    newest="$(find "$ROOT/src/service/api" -type f \( -name '*.go' -o -name 'go.mod' \) -newer "$BIN/tebox-api" | head -1 || true)"
    [[ -z "$newest" ]] && { echo "reuse $BIN/tebox-api"; return 0; }
  fi
  (cd "$ROOT/src/service/api" && GOOS=linux GOARCH=arm64 CGO_ENABLED=0 \
    go build -ldflags='-s -w' -o "$BIN/tebox-api" ./cmd/server)
}

require_products() {
  local qemu=
  for qemu in "$BIN/qemu-system-aarch64" \
      "$BIN/host/qemu/bin/qemu-system-aarch64"; do
    [[ -x "$qemu" ]] && break
    qemu=
  done
  [[ -n "$qemu" ]] || { echo "missing qemu product under $BIN (build host QEMU first)" >&2; exit 1; }
  [[ -x "$BIN/tebox-api" ]] || { echo "missing $BIN/tebox-api" >&2; exit 1; }
  [[ -x "$BIN/service/tebox-cast" ]] || { echo "missing $BIN/service/tebox-cast" >&2; exit 1; }
  [[ -f "$KDIR/gki/Image" ]] || { echo "missing $KDIR/gki/Image" >&2; exit 1; }
  [[ -d "$KDIR/vendor_modules" ]] || { echo "missing $KDIR/vendor_modules" >&2; exit 1; }
  local n
  n="$(find "$KDIR/vendor_modules" -maxdepth 1 -name '*.ko' | wc -l | tr -d ' ')"
  [[ "$n" -gt 0 ]] || { echo "no *.ko in $KDIR/vendor_modules" >&2; exit 1; }
  echo "$qemu"
}

# Copy one lib family (real file + soname symlinks) into dest.
copy_lib_family() {
  local dest="$1" name="$2" search="$3"
  local real
  real="$(find $search -type f \( -name "${name}.so" -o -name "${name}.so.*" \) \
    ! -name '*.a' 2>/dev/null | awk '{print length,$0}' | sort -n | tail -1 | cut -d' ' -f2- || true)"
  [[ -n "$real" ]] || return 0
  local base
  base="$(basename "$real")"
  cp -a "$real" "$dest/$base"
  # Prefer existing soname links next to the real file.
  local link
  for link in "$(dirname "$real")"/"${name}".so*; do
    [[ -L "$link" ]] || continue
    ln -sfn "$base" "$dest/$(basename "$link")"
  done
  [[ -e "$dest/${name}.so" ]] || ln -sfn "$base" "$dest/${name}.so"
}

stage_products() {
  bash "$ROOT/scripts/fetch-host-drivers.sh" "$R1P_DRIVERS"
  build_min_ffmpeg_cast
  build_tebox_api
  local qemu_src
  qemu_src="$(require_products)"

  rm -rf "$STAGE"
  local t="$STAGE/tebox"
  # No src/aosp in the image: the variant tree (KERNEL/GPU/cmdline/images) is
  # provided at runtime — bind-mounted per instance, or supplied over the API
  # at CreateInstance. Only the host runtime (qemu/api/cast) + the kernel
  # Image/*.ko ship baked.
  mkdir -p "$STAGE/host-drivers" \
    "$t/bin" "$t/lib" "$t/out/service" "$t/scripts" "$t/build/make/core" \
    "$t/src/kernel/$KID/gki" "$t/src/kernel/$KID/vendor_modules" \
    "$t/prebuilts/host/linux-arm64/qemu/bin" \
    "$t/prebuilts/host/linux-arm64/qemu/share/qemu"

  # --- host binaries (from out/r1p-bin) ---
  cp -L "$BIN/tebox-api" "$t/bin/tebox-api"
  cp -L "$BIN/service/tebox-cast" "$t/out/service/tebox-cast"
  cp -L "$qemu_src" "$t/prebuilts/host/linux-arm64/qemu/bin/qemu-system-aarch64"
  # Visible alias so `ls /opt/tebox` shows qemu without digging prebuilts/.
  ln -sfn ../prebuilts/host/linux-arm64/qemu/bin/qemu-system-aarch64 \
    "$t/bin/qemu-system-aarch64"

  local rom=
  for rom in "$BIN/host/qemu/share/qemu/efi-virtio.rom" \
      "$ROOT/out/r1p-context/qemu/pc-bios/efi-virtio.rom"; do
    [[ -f "$rom" ]] && break
    rom=
  done
  [[ -n "$rom" ]] || { echo "missing efi-virtio.rom" >&2; exit 1; }
  cp -L "$rom" "$t/prebuilts/host/linux-arm64/qemu/share/qemu/efi-virtio.rom"

  docker run --rm --platform linux/arm64 \
    -v "$t/prebuilts/host/linux-arm64/qemu/bin/qemu-system-aarch64:/q" \
    debian:trixie-slim \
    bash -c 'apt-get update -qq && apt-get install -y -qq binutils >/dev/null && strip --strip-unneeded /q'

  # --- private .so (built products only; system libs come from apt) ---
  copy_lib_family "$t/lib" libepoxy "$BIN/host/libepoxy/lib"
  copy_lib_family "$t/lib" libvirglrenderer "$BIN/host/virglrenderer/lib"
  copy_lib_family "$t/lib" librutabaga_gfx_ffi "$BIN/host/rutabaga/lib"
  copy_lib_family "$t/lib" libgfxstream_backend "$BIN/host/rutabaga/lib"
  cp -a "$FFMPEG_OUT"/lib/libavcodec.so* "$FFMPEG_OUT"/lib/libavutil.so* \
    "$FFMPEG_OUT"/lib/libswscale.so* "$FFMPEG_OUT"/lib/libswresample.so* "$t/lib/" 2>/dev/null || true

  # --- runtime scripts (not source tree) ---
  cp "$ROOT/scripts/"{env.sh,boot-qemu.sh,check-runtime-inputs.py,cix-mali-env.sh,cix-r1p-start.sh} \
    "$t/scripts/"
  cp "$ROOT/build/make/core/config.sh" "$t/build/make/core/config.sh"
  printf '#!/bin/sh\nexec /opt/tebox/scripts/boot-qemu.sh "$@"\n' > "$t/run"
  chmod +x "$t/run" "$t/scripts/"*.sh "$t/bin/"* "$t/out/service/tebox-cast" \
    "$t/prebuilts/host/linux-arm64/qemu/bin/qemu-system-aarch64"

  # --- kernel products: Image + *.ko (guest imgs stay outside) ---
  cp "$KDIR/gki/Image" "$t/src/kernel/$KID/gki/Image"
  cp -a "$KDIR/vendor_modules/"*.ko "$t/src/kernel/$KID/vendor_modules/"
  [[ ! -f "$KDIR/vendor_modules/modules.load" ]] || \
    cp "$KDIR/vendor_modules/modules.load" "$t/src/kernel/$KID/vendor_modules/"
  # The variant tree ($AOSP: KERNEL/GPU/cmdline/images) is NOT baked — it is
  # bind-mounted per instance or supplied over the API at CreateInstance. $AOSP
  # is still read above only to resolve $KID (which kernel to ship).

  cp "$ROOT/thirdparty/host-drivers/$R1P_DRIVERS"/*.deb "$STAGE/host-drivers/"

  # Guard: stage must not contain guest imgs or source leftovers.
  if find "$t" -name '*.img' | grep -q .; then
    echo "stage leaked *.img:" >&2
    find "$t" -name '*.img' >&2
    exit 1
  fi
  for bad in hardware images qemu/init qemu/overlay guest_modules; do
    if find "$t" -path "*/$bad" -o -path "*/$bad/*" | grep -q .; then
      echo "stage leaked $bad" >&2
      exit 1
    fi
  done

  echo "staged $(du -sh "$STAGE" | awk '{print $1}') → $STAGE"
  echo "layout:"
  (cd "$t" && find . -maxdepth 2 | sort)
  echo "kernel modules: $(find "$t/src/kernel/$KID/vendor_modules" -name '*.ko' | wc -l | tr -d ' ') .ko"
  du -sh "$t"/* 2>/dev/null | sort -hr
}

build() {
  stage_products
  # Always tag tebox:<ver> + tebox:latest (ignore stale R1P_IMAGE=tebox-r1p:*).
  local img="tebox:${TEBOX_VERSION}"
  docker build --platform linux/arm64 -f "$ROOT/Dockerfile" \
    --build-arg APT_MIRROR="$R1P_APT_MIRROR" \
    -t "$img" -t tebox:latest "$STAGE"
  R1P_IMAGE="$img"
  docker images tebox
  docker run --rm --entrypoint sh "$img" -c '
    set -e
    ls -la /opt/tebox
    ls -lh /opt/tebox/bin/qemu-system-aarch64 /opt/tebox/bin/tebox-api
    test -n "$(ls /opt/tebox/src/kernel/*/gki/Image 2>/dev/null)"
    test -n "$(ls /opt/tebox/src/kernel/*/vendor_modules/*.ko 2>/dev/null)"
    test -z "$(printenv VARIANT)"
    test -z "$(printenv KERNEL_ID)"
    ! find /opt/tebox -name "*.img" | grep -q .
    du -sh /opt/tebox
    echo "ko=$(ls /opt/tebox/src/kernel/*/vendor_modules/*.ko | wc -l)"
  '
}

push() {
  remote "sudo -A rm -rf $R1P_DIR/bin"
  docker save "$R1P_IMAGE" | gzip -1 | remote 'gunzip | sudo -A docker load'
}

start() {
  stop >/dev/null 2>&1 || true
  local virgl= kernel_vol= autostart_env=
  remote "sudo -A test -f $R1P_DIR/bin/libvirglrenderer.so.1" && \
    virgl="-v $R1P_DIR/bin/libvirglrenderer.so.1:/opt/tebox/prebuilts/host/linux-arm64/virglrenderer/lib/libvirglrenderer.so.1:ro"
  remote "sudo -A test -f $R1P_DIR/bin/cix-r1p-start.sh" && \
    virgl+=" -v $R1P_DIR/bin/cix-r1p-start.sh:/opt/tebox/scripts/cix-r1p-start.sh:ro"
  if remote "sudo -A test -f $R1P_DIR/kernel/Image"; then
    kernel_vol="-v $R1P_DIR/kernel:/opt/tebox/src/kernel/$KID/gki:ro"
  fi
  if [[ -n "${TEBOX_AUTOSTART:-${R1P_INSTANCE:-}}" ]]; then
    autostart_env="-e TEBOX_AUTOSTART=${TEBOX_AUTOSTART:-$R1P_INSTANCE} -e ADB_PORT=$R1P_ADB_PORT -e MEM=${MEM:-4096} -e SMP=${SMP:-4} -e USERDATA_MB=${USERDATA_MB:-8192}"
  fi
  remote "sudo -A mkdir -p $R1P_USER_IMAGES $R1P_INSTANCE_ROOT $R1P_DIR/$R1P_NAME $R1P_DIR/bin"
  remote "sudo -A docker run -d --name $R1P_NAME --restart unless-stopped --network host \
    --device /dev/kvm --device /dev/mali0 --device /dev/dri \
    --device /dev/dma_heap/system --device /dev/dma_heap/system-uncached \
    --device /dev/video0 --device /dev/video1 \
    -e TEBOX_MALI=0 \
    -e TEBOX_GRPC_ADDR=0.0.0.0:$R1P_GRPC_PORT -e TEBOX_CAST_ADDR=0.0.0.0:$R1P_CAST_PORT \
    -e TEBOX_AUDIO_ADDR=0.0.0.0:$R1P_AUDIO_PORT -e TEBOX_CONTROL_ADDR=0.0.0.0:$R1P_CONTROL_PORT \
    $autostart_env \
    $(for kv in ${R1P_ENV:-}; do printf -- '-e %q ' "$kv"; done) \
    $kernel_vol \
    -v $R1P_DIR/$R1P_NAME:/data \
    -v $R1P_DIR/bin:/opt/tebox/override:ro \
    -v $R1P_USER_IMAGES:$R1P_USER_IMAGES:ro \
    -v $R1P_INSTANCE_ROOT:$R1P_INSTANCE_ROOT \
    $virgl \
    $R1P_IMAGE"
  echo "base gRPC ${R1P_HOST#*@}:$R1P_GRPC_PORT  cast ${R1P_HOST#*@}:$R1P_CAST_PORT"
  echo "CreateInstance imgs under $R1P_USER_IMAGES ; instances under $R1P_INSTANCE_ROOT"
}

stop() { remote "sudo -A docker rm -f $R1P_NAME"; }
status() { remote "sudo -A docker ps -a --filter name=$R1P_NAME"; }
logs() { remote "sudo -A docker logs --tail ${TAIL:-200} $R1P_NAME"; }

export_img() {
  local stamp out link
  stamp="$(date +%Y%m%d-%H%M%S)"
  out="$ROOT/out/xiao/tebox-${TEBOX_VERSION}-${stamp}.tar.gz"
  link="$ROOT/out/xiao/tebox-${TEBOX_VERSION}.tar.gz"
  mkdir -p "$(dirname "$out")"
  echo "exporting $R1P_IMAGE → $out"
  docker save "$R1P_IMAGE" | gzip -1 > "$out"
  ln -sfn "$(basename "$out")" "$link"
  ln -sfn "$(basename "$out")" "$ROOT/out/xiao/tebox-latest.tar.gz"
  ls -lh "$out" "$link"
  docker run --rm --entrypoint sh "$R1P_IMAGE" -c "
    set -e
    test -f /opt/tebox/src/kernel/$KID/gki/Image
    test -n \"\$(ls /opt/tebox/src/kernel/$KID/vendor_modules/*.ko 2>/dev/null)\"
    test -x /opt/tebox/bin/tebox-api
    test -x /opt/tebox/bin/qemu-system-aarch64
    ! find /opt/tebox -name '*.img' | grep -q .
    du -sh /opt/tebox
  "
  echo "wrote $out"
}

case "${1:-}" in
  build|push|start|stop|status|logs) "$1" ;;
  export) export_img ;;
  all) build; push; start ;;
  *) echo "usage: $0 [build|push|start|stop|status|logs|export|all]" >&2; exit 2 ;;
esac
