#!/system/bin/sh
# Inject a fixed-for-this-boot fake WiFi SSID list into the framework.
# Uses `cmd wifi add-fake-scan` (no kernel/nl80211). Connection is not expected.
#
# List is generated once when this script starts (boot_completed) and reused
# for every re-inject / scan refresh so Settings does not reshuffle SSIDs.

export PATH=/system/bin:/vendor/bin:${PATH:-}

TAG=qemu-wifi-fake-scan
LIST=/data/local/tmp/qemu-fake-ssids.txt
COUNT=16
CAPS='[WPA2-PSK-CCMP][RSN-PSK-CCMP][ESS]'

log() {
  /system/bin/log -t "$TAG" "$*" 2>/dev/null || echo "$TAG: $*"
}

rand_hex() {
  # 4 hex chars from /dev/urandom (toybox od).
  od -An -N2 -tx1 /dev/urandom 2>/dev/null | tr -d ' \n' | tr 'a-f' 'A-F'
}

rand_byte() {
  # Decimal 0-255.
  od -An -N1 -tu1 /dev/urandom 2>/dev/null | tr -d ' \n'
}

wait_for_wifi_cmd() {
  i=0
  while [ "$i" -lt 90 ]; do
    if cmd wifi help >/dev/null 2>&1; then
      return 0
    fi
    sleep 2
    i=$((i + 1))
  done
  return 1
}

# Build a stable table for this boot. Script runs once per boot via init rc.
generate_list() {
  : > "$LIST" || return 1
  prefixes="QEMU Cafe Office Guest Home Mesh Lab Studio Shop Park Metro Hotel Campus Lobby Loft Hub"
  freqs="2412 2417 2422 2437 2462 5180 5200 5220 5745 5765"
  n=0
  for p in $prefixes; do
    [ "$n" -ge "$COUNT" ] && break
    hx=$(rand_hex)
    [ -z "$hx" ] && hx=$(printf '%04X' "$n")
    b1=$(rand_byte); b2=$(rand_byte); b3=$(rand_byte)
    [ -z "$b1" ] && b1=$n
    [ -z "$b2" ] && b2=$((n * 3))
    [ -z "$b3" ] && b3=$((n * 7))
    bssid=$(printf '02:00:00:%02X:%02X:%02X' $((b1 % 256)) $((b2 % 256)) $((b3 % 256)))
    # Pick frequency / RSSI deterministically from index so re-reads match.
    fi=$((n % 10 + 1))
    freq=$(echo "$freqs" | cut -d' ' -f"$fi")
    level=$((-45 - (n % 40)))
    ssid="${p}_${hx}"
    # ssid bssid freq level
    printf '%s %s %s %s\n' "$ssid" "$bssid" "$freq" "$level" >> "$LIST"
    n=$((n + 1))
  done
  log "generated $n fixed SSIDs -> $LIST"
}

inject() {
  cmd wifi reset-fake-scans >/dev/null 2>&1 || true
  while read -r ssid bssid freq level; do
    [ -n "$ssid" ] || continue
    cmd wifi add-fake-scan "$ssid" "$bssid" "$CAPS" "$freq" "$level" >/dev/null 2>&1 || \
      log "add-fake-scan failed: $ssid"
  done < "$LIST"
  cmd wifi start-faking-scans >/dev/null 2>&1 || log "start-faking-scans failed"
  cmd wifi start-scan >/dev/null 2>&1 || true
}

log "waiting for WifiService..."
if ! wait_for_wifi_cmd; then
  log "cmd wifi never became ready; exit"
  exit 0
fi

cmd wifi set-wifi-enabled enabled >/dev/null 2>&1 || true
svc wifi enable >/dev/null 2>&1 || true
sleep 2

generate_list || { log "cannot write $LIST"; exit 0; }
inject
log "initial inject done"

# Keep the same list visible: re-inject only if scan results vanish (e.g. wifi toggle).
while true; do
  sleep 20
  # Re-enable faking / scan using the same on-disk table.
  if ! cmd wifi list-scan-results 2>/dev/null | grep -Eq 'QEMU_|Cafe_|Office_|Guest_|Home_'; then
    log "scan list empty; re-injecting fixed table"
    cmd wifi set-wifi-enabled enabled >/dev/null 2>&1 || true
    inject
  else
    cmd wifi start-faking-scans >/dev/null 2>&1 || true
    cmd wifi start-scan >/dev/null 2>&1 || true
  fi
done
