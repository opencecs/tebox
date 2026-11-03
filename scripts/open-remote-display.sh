#!/usr/bin/env bash
# Show the remote Mac desktop that contains the running Tebox QEMU windows.
set -euo pipefail

if [[ "$(uname -s)" != Darwin ]]; then
  echo "macOS Screen Sharing is required on the viewing computer." >&2
  exit 1
fi

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "Usage: $0 USER@HOST [--check-only]" >&2
  exit 2
fi

target="$1"
mode="${2:-}"
[[ -z "$mode" || "$mode" == --check-only ]] || {
  echo "Unknown option: $mode" >&2
  exit 2
}
host="${target##*@}"
[[ -n "$host" && "$host" != *[!A-Za-z0-9.:-]* ]] || {
  echo "Expected a DNS name or IP address." >&2
  exit 2
}

if ! nc -z -G 3 "$host" 5900 >/dev/null 2>&1; then
  echo "Screen Sharing is not reachable on $host:5900." >&2
  exit 1
fi
echo "Screen Sharing is reachable on $host:5900."
[[ "$mode" == --check-only ]] && exit 0

# The OS asks for remote credentials. No password is stored in this project.
open "vnc://$target"
