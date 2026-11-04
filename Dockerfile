# tebox runtime base — products only (no sources, no AOSP *.img).
# Image: tebox:<version>  (e.g. tebox:1.0.0)
# Context: out/r1p-docker/  (assembled by scripts/r1p-deploy.sh)
#   host-drivers/*.deb
#   tebox/   → /opt/tebox
#
# No baked VARIANT / KERNEL_ID and no src/aosp: CreateInstance supplies guest
# imgs + cmdline/gpu/kernel; the variant tree is bind-mounted per instance.
# Only the host runtime + the kernel Image + *.ko ship in the image.
FROM debian:trixie-slim
ARG APT_MIRROR=
RUN echo 'Acquire::Retries "8";' > /etc/apt/apt.conf.d/80-retries \
 && if [ -n "$APT_MIRROR" ]; then \
      sed -i "s|http://deb.debian.org|$APT_MIRROR|g" /etc/apt/sources.list.d/debian.sources; fi
RUN apt-get update && apt-get install -y --no-install-recommends \
      python3-minimal e2fsprogs procps iproute2 adb \
      libdrm2 libwayland-client0 libwayland-server0 libvulkan1 \
      libglib2.0-0 libgomp1 libpixman-1-0 libpng16-16t64 zlib1g \
      libslirp0 libfdt1 libzstd1 libjpeg62-turbo \
      libwayland-egl1 libwayland-cursor0 libxkbcommon0 libdecor-0-0 libdbus-1-3 \
      libsndfile1 libx11-xcb1 libasyncns0 \
      libsdl2-2.0-0 libgbm1 libasound2t64 libpulse0 libsndio7.0 \
    && rm -rf /var/lib/apt/lists/*

COPY host-drivers/ /tmp/drivers/
RUN for d in /tmp/drivers/*.deb; do dpkg -x "$d" /tmp/drivers/root; done \
 && cp -a /tmp/drivers/root/opt/. /opt/ \
 && if [ -d /tmp/drivers/root/etc/vulkan/icd.d ]; then \
      mkdir -p /etc/vulkan/icd.d && cp -a /tmp/drivers/root/etc/vulkan/icd.d/. /etc/vulkan/icd.d/; fi \
 && rm -rf /tmp/drivers

ENV TEBOX=/opt/tebox
COPY tebox/ /opt/tebox/
RUN chmod +x /opt/tebox/bin/tebox-api /opt/tebox/out/service/tebox-cast \
      /opt/tebox/scripts/*.sh /opt/tebox/run \
      /opt/tebox/prebuilts/host/linux-arm64/qemu/bin/qemu-system-aarch64 \
 && test -n "$(ls /opt/tebox/src/kernel/*/gki/Image 2>/dev/null)" \
 && test -n "$(ls /opt/tebox/src/kernel/*/vendor_modules/*.ko 2>/dev/null)" \
 && ! find /opt/tebox -name '*.img' | grep -q .

ENTRYPOINT ["/opt/tebox/scripts/cix-r1p-start.sh"]
