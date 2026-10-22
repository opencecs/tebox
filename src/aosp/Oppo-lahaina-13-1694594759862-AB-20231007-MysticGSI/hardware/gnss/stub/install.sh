#!/usr/bin/env bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
AOSP=$(cd "$HERE/../../.." && pwd)
STUB="$AOSP/qemu/vendor"
install -m 0755 "$ROOT/out/gnss-stub/bin/android.hardware.gnss-service" "$STUB/bin/hw/"
install -m 0644 "$HERE/../init/android.hardware.gnss-service.rc" "$STUB/etc/init/"
install -m 0644 "$HERE/../vintf/android.hardware.gnss-service.xml" "$STUB/etc/vintf/manifest/"
python3 - "$STUB/manifest.xml" <<'PY'
from pathlib import Path
import re
import sys
p = Path(sys.argv[1])
text = p.read_text()
# Drop any prior gnss HAL block so this script stays idempotent.
text = re.sub(
    r'\s*<hal format="aidl">\s*<name>android\.hardware\.gnss</name>.*?</hal>\s*',
    '\n',
    text,
    flags=re.S,
)
entry = '''    <hal format="aidl">
        <name>android.hardware.gnss</name>
        <version>2</version>
        <fqname>IGnss/default</fqname>
    </hal>
'''
p.write_text(text.replace('</manifest>', entry + '</manifest>'))
PY
grep -q "android.hardware.gnss-service" "$STUB/etc/selinux/vendor_file_contexts" || \
  printf '%s\n' '/vendor/bin/hw/android\.hardware\.gnss-service    u:object_r:hal_keymint_system_exec:s0' \
    >> "$STUB/etc/selinux/vendor_file_contexts"
echo "Installed GNSS HAL into $STUB"
