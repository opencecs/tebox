#!/usr/bin/env bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
AOSP=$(cd "$HERE/../../.." && pwd)
STUB="$AOSP/qemu/vendor"
mkdir -p "$STUB/bin/hw" "$STUB/etc/init" "$STUB/etc/vintf/manifest" "$STUB/etc/permissions"
install -m 0755 "$ROOT/out/fingerprint-stub/bin/android.hardware.biometrics.fingerprint-service" \
  "$STUB/bin/hw/"
install -m 0644 "$HERE/../init/android.hardware.biometrics.fingerprint-service.rc" "$STUB/etc/init/"
install -m 0644 "$HERE/../vintf/android.hardware.biometrics.fingerprint-service.xml" \
  "$STUB/etc/vintf/manifest/"
FC="$STUB/etc/selinux/vendor_file_contexts"
if [[ -f "$FC" ]] && ! grep -q 'android\.hardware\.biometrics\.fingerprint-service' "$FC"; then
  echo '/vendor/bin/hw/android\.hardware\.biometrics\.fingerprint-service    u:object_r:hal_keymint_system_exec:s0' >> "$FC"
fi
python3 - "$STUB/manifest.xml" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
text = p.read_text()
if '<name>android.hardware.biometrics.fingerprint</name>' not in text:
    entry = '''    <hal format="aidl">
        <name>android.hardware.biometrics.fingerprint</name>
        <version>4</version>
        <fqname>IFingerprint/default</fqname>
    </hal>
'''
    p.write_text(text.replace('</manifest>', entry + '</manifest>'))
PY
