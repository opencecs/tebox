#!/usr/bin/env bash
# Open a remote Tebox guest in a local scrcpy window via SSH tunnels.
set -euo pipefail

if [[ $# -lt 2 ]]; then
  echo "Usage: $0 USER@HOST ADB_SERIAL [--view-only] [scrcpy options...]" >&2
  exit 2
fi
target="$1"
serial="$2"
shift 2
[[ "$target" == *@* && "$target" != -* ]] || {
  echo "Expected USER@HOST as the first argument." >&2
  exit 2
}

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
for offset in $(seq 0 99); do
  adb_port=$((15038 + offset))
  video_port=$((27184 + offset))
  if ! lsof -nP -iTCP:"$adb_port" -sTCP:LISTEN >/dev/null 2>&1 &&
     ! lsof -nP -iTCP:"$video_port" -sTCP:LISTEN >/dev/null 2>&1; then
    break
  fi
done
if [[ "$offset" -eq 99 ]] && {
  lsof -nP -iTCP:"$adb_port" -sTCP:LISTEN >/dev/null 2>&1 ||
  lsof -nP -iTCP:"$video_port" -sTCP:LISTEN >/dev/null 2>&1
}; then
  echo "No free local ADB/video port pair." >&2
  exit 1
fi

tmpdir="$(mktemp -d /tmp/sc.XXXXXX)"
socket="$tmpdir/ssh.sock"
cleanup() {
  ssh -S "$socket" -O exit "$target" >/dev/null 2>&1 || true
  rmdir "$tmpdir" >/dev/null 2>&1 || true
}
trap cleanup EXIT

ssh -o ExitOnForwardFailure=yes -M -S "$socket" -fN \
  -L "127.0.0.1:$adb_port:127.0.0.1:5037" \
  -L "127.0.0.1:$video_port:127.0.0.1:$video_port" \
  "$target"

export ADB_SERVER_SOCKET="tcp:127.0.0.1:$adb_port"
"$root/scripts/mirror-device.sh" "$serial" \
  --force-adb-forward --port="$video_port" "$@"
