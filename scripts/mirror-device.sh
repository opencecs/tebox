#!/usr/bin/env bash
# Mirror a Tebox guest with the pinned scrcpy MJPEG fallback build.
set -euo pipefail

usage() {
  cat >&2 <<'USAGE'
Usage: scripts/mirror-device.sh SERIAL [--view-only] [scrcpy options...]

Examples:
  scripts/mirror-device.sh 127.0.0.1:5570
  scripts/mirror-device.sh 127.0.0.1:5570 --view-only

The default enables scrcpy's HID mouse and keyboard. --view-only disables
control. Other scrcpy options are passed through after the preset options.
USAGE
  exit 2
}

[[ $# -ge 1 ]] || usage
serial="$1"
shift
[[ "$serial" != -* && -n "$serial" ]] || usage

control=(--mouse=uhid --keyboard=uhid)
scrcpy_args=()
for arg in "$@"; do
  case "$arg" in
    --view-only) control=(--no-control) ;;
    --uhid-control) control=(--mouse=uhid --keyboard=uhid) ;;
    *) scrcpy_args+=("$arg") ;;
  esac
done

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
bundle="$root/prebuilts/host/darwin-arm64/scrcpy-mjpeg"
client="$bundle/scrcpy"
server="$bundle/scrcpy-server"
if [[ -x "$root/out/scrcpy-mjpeg-bundle/scrcpy" ]]; then
  client="$root/out/scrcpy-mjpeg-bundle/scrcpy"
fi

if [[ "$(uname -s)" != Darwin || "$(uname -m)" != arm64 ]]; then
  echo "This bundled scrcpy build requires macOS ARM64." >&2
  exit 1
fi
[[ -x "$client" && -f "$server" ]] || {
  echo "Missing bundled scrcpy MJPEG client or server in $bundle." >&2
  exit 1
}
if [[ -x /opt/homebrew/bin/adb ]]; then
  PATH="/opt/homebrew/bin:$PATH"
fi
command -v adb >/dev/null || {
  echo "adb is required on PATH." >&2
  exit 1
}

state="$(adb devices | awk -v serial="$serial" '$1 == serial { print $2; exit }')"
[[ "$state" == device ]] || {
  echo "ADB serial $serial is not online (state: ${state:-missing})." >&2
  exit 1
}

export SCRCPY_SERVER_PATH="$server"
exec "$client" --serial="$serial" --video-codec=mjpeg \
  --video-codec-options=quality=50 --max-fps=60 --max-size=1200 --no-audio \
  "${control[@]}" "${scrcpy_args[@]}"
