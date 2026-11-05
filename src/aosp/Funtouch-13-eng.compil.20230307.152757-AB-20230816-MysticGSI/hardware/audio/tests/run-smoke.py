#!/usr/bin/env python3
"""Build and run AudioTrack playback through a QEMU_DEBUG=1 guest."""
import base64
import os
from pathlib import Path
import subprocess
import sys

here = Path(__file__).resolve().parent
root = here.parents[5]
sdk = Path(os.environ.get('ANDROID_SDK_ROOT', str(root / 'toolchains/android-sdk')))
android_jar = sdk / 'platforms/android-36/android.jar'
out = root / 'out/audio-smoke'
(out / 'classes').mkdir(parents=True, exist_ok=True)
(out / 'dex').mkdir(exist_ok=True)
subprocess.run(['javac', '-source', '8', '-target', '8', '-Xlint:-options',
                '-classpath', str(android_jar), '-d', str(out / 'classes'),
                str(here / 'AudioSmoke.java')], check=True)
subprocess.run([str(sdk / 'build-tools/36.1.0/d8'), '--min-api', '34',
                '--lib', str(android_jar), '--output', str(out / 'dex'),
                str(out / 'classes/AudioSmoke.class')], check=True)

def guest(command, log, timeout=20):
    result = subprocess.run([sys.executable, str(root / 'scripts/qemu-console.py'),
                             command, '--timeout', str(timeout), '--output', str(out / log)],
                            cwd=root, stdout=subprocess.DEVNULL, check=True)
    return (out / log).read_text(errors='replace')

data = base64.b64encode((out / 'dex/classes.dex').read_bytes()).decode()
guest('mkdir -p /data/local/tmp; : > /data/local/tmp/audio-smoke.b64', 'prepare.log')
# Short lines fit the serial console's canonical input buffer.
for offset in range(0, len(data), 2400):
    batch = data[offset:offset+2400]
    command = '\n'.join("printf '%s' '" + batch[i:i+400] + "' >> /data/local/tmp/audio-smoke.b64"
                        for i in range(0, len(batch), 400))
    guest(command, 'transfer.log')
guest('base64 -d /data/local/tmp/audio-smoke.b64 > /data/local/tmp/audio-smoke.dex; '
      'chmod 444 /data/local/tmp/audio-smoke.dex', 'decode.log')
result = guest('CLASSPATH=/data/local/tmp/audio-smoke.dex app_process / AudioSmoke', 'result.log', 30)
for line in result.splitlines():
    if line.startswith('AUDIO_SMOKE_PASS '):
        print(line)
        break
else:
    print(result)
    raise SystemExit('AudioTrack smoke test failed; see ' + str(out / 'result.log'))
