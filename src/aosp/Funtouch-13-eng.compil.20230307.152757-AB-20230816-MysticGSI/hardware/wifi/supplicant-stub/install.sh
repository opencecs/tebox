#!/usr/bin/env bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
AOSP=$(cd "$HERE/../../.." && pwd)
STUB="$AOSP/qemu/vendor"
mkdir -p "$STUB/bin/hw" "$STUB/etc/init" "$STUB/etc/vintf/manifest"
install -m 0755 "$ROOT/out/supplicant-stub/bin/android.hardware.wifi.supplicant-service" \
  "$STUB/bin/hw/"
install -m 0644 "$HERE/../init/android.hardware.wifi.supplicant-service.rc" "$STUB/etc/init/"
install -m 0644 "$HERE/../vintf/android.hardware.wifi.supplicant-service.xml" \
  "$STUB/etc/vintf/manifest/"
# SELinux file_contexts (bring-up reuses hal_keymint_system_exec).
FC="$STUB/etc/selinux/vendor_file_contexts"
if [[ -f "$FC" ]] && ! grep -q 'android\.hardware\.wifi\.supplicant-service' "$FC"; then
  printf '%s\n' \
    '/vendor/bin/hw/android\.hardware\.wifi\.supplicant-service    u:object_r:hal_keymint_system_exec:s0' \
    >> "$FC"
fi
python3 - "$STUB/manifest.xml" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
text = p.read_text()
if '<name>android.hardware.wifi.supplicant</name>' not in text:
    entry = '''    <hal format="aidl">
        <name>android.hardware.wifi.supplicant</name>
        <version>3</version>
        <fqname>ISupplicant/default</fqname>
    </hal>
'''
    p.write_text(text.replace('</manifest>', entry + '</manifest>'))
PY
