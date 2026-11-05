#!/usr/bin/env bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
AOSP=$(cd "$HERE/../../.." && pwd)
STUB="$AOSP/qemu/vendor"
mkdir -p "$STUB/bin/hw" "$STUB/etc/init" "$STUB/etc/vintf/manifest" "$STUB/etc/permissions"
install -m 0755 "$ROOT/out/bluetooth-stub/bin/android.hardware.bluetooth-service" "$STUB/bin/hw/"
install -m 0644 "$HERE/../init/android.hardware.bluetooth-service.rc" "$STUB/etc/init/"
install -m 0644 "$HERE/../vintf/android.hardware.bluetooth-service.xml" "$STUB/etc/vintf/manifest/"
# Ensure feature flags exist (also patched into qemu-framework.xml).
if [[ ! -f "$STUB/etc/permissions/android.hardware.bluetooth.xml" ]]; then
  cat > "$STUB/etc/permissions/android.hardware.bluetooth.xml" <<'EOF'
<?xml version="1.0" encoding="utf-8"?>
<permissions>
    <feature name="android.hardware.bluetooth"/>
    <feature name="android.hardware.bluetooth_le"/>
</permissions>
EOF
fi
# SELinux file_contexts entry for the HAL binary.
FC="$STUB/etc/selinux/vendor_file_contexts"
if [[ -f "$FC" ]] && ! grep -q 'android\.hardware\.bluetooth-service' "$FC"; then
  echo '/vendor/bin/hw/android\.hardware\.bluetooth-service    u:object_r:hal_keymint_system_exec:s0' >> "$FC"
fi
python3 - "$STUB/manifest.xml" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
text = p.read_text()
if '<name>android.hardware.bluetooth</name>' not in text:
    entry = '''    <hal format="aidl">
        <name>android.hardware.bluetooth</name>
        <version>1</version>
        <fqname>IBluetoothHci/default</fqname>
    </hal>
'''
    p.write_text(text.replace('</manifest>', entry + '</manifest>'))
PY
