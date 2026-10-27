#!/usr/bin/env bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
AOSP=$(cd "$HERE/../../.." && pwd)
STUB="$AOSP/qemu/vendor"
BIN="$ROOT/out/codec2-stub/bin/android.hardware.media.c2-service"

test -x "$BIN" || { echo "missing $BIN — run build.sh first" >&2; exit 1; }

install -m 0755 "$BIN" "$STUB/bin/hw/android.hardware.media.c2-service"
install -m 0644 "$HERE/../init/android.hardware.media.c2-service.rc" "$STUB/etc/init/"
install -m 0644 "$HERE/../vintf/android.hardware.media.c2-service.xml" "$STUB/etc/vintf/manifest/"
# Keep XML out of MediaCodecList unless the host renderer was built with its
# real video backend. VIRGL_VIDEO=1 is the Linux VA-API opt-in; the explicit
# override is useful for packaging a matching image in CI.
rm -f "$STUB/etc/media_codecs_c2.xml" "$STUB/etc/media_codecs.xml"
if [[ "${GKI_MESA_VIDEO_ENABLE:-0}" == 1 || "${VIRGL_VIDEO:-0}" == 1 ]]; then
  install -m 0644 "$HERE/../etc/media_codecs_c2.xml" "$STUB/etc/media_codecs_c2.xml"
  install -m 0644 "$HERE/../etc/media_codecs_c2.xml" "$STUB/etc/media_codecs.xml"
fi
# Hosts with a VirGL video encoder opt in at boot with
# androidboot.tebox.video_encode=1 (ro.media.xml_variant.codecs=_tebox_video).
install -m 0644 "$HERE/../etc/media_codecs_c2.xml" "$STUB/etc/media_codecs_tebox_video.xml"
install -m 0644 "$HERE/../etc/seccomp_policy/android.hardware.media.c2-service.policy" \
  "$STUB/etc/seccomp_policy/android.hardware.media.c2-service.policy"

python3 - "$STUB/manifest.xml" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
text = p.read_text()
if 'android.hardware.media.c2' not in text:
    entry = '''    <hal format="aidl">
        <name>android.hardware.media.c2</name>
        <version>1</version>
        <fqname>IComponentStore/default</fqname>
    </hal>
'''
    p.write_text(text.replace('</manifest>', entry + '</manifest>'))
PY

python3 - "$STUB/etc/selinux/vendor_file_contexts" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
line = '/vendor/bin/hw/android\\.hardware\\.media\\.c2-service    u:object_r:hal_keymint_system_exec:s0\n'
text = p.read_text()
if 'media\\.c2-service' not in text:
    p.write_text(text.rstrip() + '\n' + line)
PY

echo "codec2 HAL installed into $STUB"
