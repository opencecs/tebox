#!/usr/bin/env bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
AOSP=$(cd "$HERE/../../.." && pwd)
STUB="$AOSP/qemu/vendor"
mkdir -p "$STUB/bin/hw" "$STUB/etc/init" "$STUB/etc/vintf/manifest" "$STUB/etc/permissions"
install -m 0755 "$ROOT/out/nfc-stub/bin/android.hardware.nfc-service" "$STUB/bin/hw/"
install -m 0644 "$HERE/../init/android.hardware.nfc-service.rc" "$STUB/etc/init/"
install -m 0644 "$HERE/../vintf/android.hardware.nfc-service.xml" "$STUB/etc/vintf/manifest/"
FC="$STUB/etc/selinux/vendor_file_contexts"
if [[ -f "$FC" ]] && ! grep -q 'android\.hardware\.nfc-service' "$FC"; then
  echo '/vendor/bin/hw/android\.hardware\.nfc-service    u:object_r:hal_keymint_system_exec:s0' >> "$FC"
fi
python3 - "$STUB/manifest.xml" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
text = p.read_text()
if '<name>android.hardware.nfc</name>' not in text:
    entry = '''    <hal format="aidl">
        <name>android.hardware.nfc</name>
        <version>2</version>
        <fqname>INfc/default</fqname>
    </hal>
'''
    p.write_text(text.replace('</manifest>', entry + '</manifest>'))
PY
