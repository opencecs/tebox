"""Generate explicit UNSUPPORTED implementations for optional frozen AIDL methods."""
import re
import sys
from pathlib import Path

root = Path(sys.argv[1])
classes = []
for package, name in [('core', 'Module'), ('core', 'StreamCommon'),
                      ('core', 'StreamIn'), ('core', 'StreamOut')]:
    base = f'aidl/android/hardware/audio/{package}'
    interface = (root / base / f'I{name}.h').read_text()
    methods = re.findall(r'virtual ::ndk::ScopedAStatus\s+([^;]+?)\s*=\s*0;', interface)
    if not methods:
        raise SystemExit(f'No methods found in I{name}')
    classes.append(f'#include <{base}/Bn{name}.h>\n'
                   f'class Unsupported{name} : public aidl::android::hardware::audio::{package}::Bn{name} {{\npublic:\n')
    for signature in methods:
        if signature.startswith(('getInterfaceVersion(', 'getInterfaceHash(')):
            continue  # Bn* supplies the frozen version/hash as final methods.
        classes.append('  ::ndk::ScopedAStatus ' + signature +
                       ' override { return ::ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION); }\n')
    classes.append('};\n')
(root / 'audio_defaults.h').write_text(''.join(classes))
