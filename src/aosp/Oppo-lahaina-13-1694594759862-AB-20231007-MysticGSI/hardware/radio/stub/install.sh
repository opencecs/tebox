#!/usr/bin/env bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../../../.." && pwd)
AOSP=$(cd "$HERE/../../.." && pwd)
STUB="$AOSP/qemu/vendor"
install -m 0755 "$ROOT/out/radio-stub/bin/android.hardware.radio-service" "$STUB/bin/hw/"
install -m 0644 "$HERE/../init/android.hardware.radio-service.rc" "$STUB/etc/init/"
install -m 0644 "$HERE/../vintf/android.hardware.radio-service.xml" "$STUB/etc/vintf/manifest/"
python3 - "$STUB/manifest.xml" <<'PY'
from pathlib import Path
import re
import sys
p = Path(sys.argv[1])
text = p.read_text()
# Drop any prior radio HAL blocks so dual-SIM fqnames stay authoritative.
text = re.sub(
    r'\s*<hal format="aidl">\s*<name>android\.hardware\.radio(?:\.[a-z.]+)?</name>.*?</hal>\s*',
    '\n',
    text,
    flags=re.S,
)
entry = '''    <hal format="aidl">
        <name>android.hardware.radio.config</name>
        <version>3</version>
        <fqname>IRadioConfig/default</fqname>
    </hal>
    <hal format="aidl">
        <name>android.hardware.radio.modem</name>
        <version>3</version>
        <fqname>IRadioModem/slot1</fqname>
        <fqname>IRadioModem/slot2</fqname>
    </hal>
    <hal format="aidl">
        <name>android.hardware.radio.network</name>
        <version>3</version>
        <fqname>IRadioNetwork/slot1</fqname>
        <fqname>IRadioNetwork/slot2</fqname>
    </hal>
    <hal format="aidl">
        <name>android.hardware.radio.sim</name>
        <version>3</version>
        <fqname>IRadioSim/slot1</fqname>
        <fqname>IRadioSim/slot2</fqname>
    </hal>
    <hal format="aidl">
        <name>android.hardware.radio.data</name>
        <version>3</version>
        <fqname>IRadioData/slot1</fqname>
        <fqname>IRadioData/slot2</fqname>
    </hal>
    <hal format="aidl">
        <name>android.hardware.radio.messaging</name>
        <version>3</version>
        <fqname>IRadioMessaging/slot1</fqname>
        <fqname>IRadioMessaging/slot2</fqname>
    </hal>
    <hal format="aidl">
        <name>android.hardware.radio.voice</name>
        <version>3</version>
        <fqname>IRadioVoice/slot1</fqname>
        <fqname>IRadioVoice/slot2</fqname>
    </hal>
'''
p.write_text(text.replace('</manifest>', entry + '</manifest>'))
PY
