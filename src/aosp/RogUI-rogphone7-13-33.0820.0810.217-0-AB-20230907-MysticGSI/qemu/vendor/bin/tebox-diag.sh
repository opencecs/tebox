#!/system/bin/sh
# Bring-up diagnostics for the tebox QEMU guest. init starts this from
# "on post-fs-data" as the shell domain; the output lands on userdata where
# the host can read it with debugfs while the guest is stopped.

out=/data/local/tmp/tebox-diag.txt
: > "$out"

log() { echo "$*" >> "$out"; }
kmsg() { echo "tebox-diag: $*" > /dev/kmsg; }

kmsg start
log "=== DATE ==="
date >> "$out" 2>&1

sleep 60
kmsg dumping

log "=== RULE BEFORE ==="
ip rule >> "$out" 2>&1
log "=== ROUTES BEFORE ==="
for t in main local legacy_system legacy_network local_network 97 98 99; do
    log "-- table $t"
    ip route show table "$t" >> "$out" 2>&1
done
log "=== RT_TABLES FILES ==="
ls -l /system/etc/iproute2 /vendor/etc/iproute2 /etc/iproute2 >> "$out" 2>&1
log "=== TCP6 ==="
cat /proc/net/tcp6 >> "$out" 2>&1
log "=== NETSTAT ==="
netstat -tln >> "$out" 2>&1
log "=== PING BEFORE ==="
ping -c 1 -W 2 10.0.2.2 >> "$out" 2>&1
log "rc=$?"

kmsg fix-rule
log "=== FIX RULE pref ==="
ip rule add pref 9000 to 10.0.2.0/24 lookup main >> "$out" 2>&1
log "rc=$?"
log "=== FIX RULE priority ==="
ip rule add priority 9100 to 10.0.2.0/24 lookup main >> "$out" 2>&1
log "rc=$?"

kmsg fix-routes
log "=== FIX ROUTES ==="
ip route replace 10.0.2.0/24 dev eth0 table main >> "$out" 2>&1
log "rc main=$?"
ip route replace 10.0.2.0/24 dev eth0 table legacy_system >> "$out" 2>&1
log "rc legacy_system=$?"
ip route replace 10.0.2.0/24 dev eth0 table legacy_network >> "$out" 2>&1
log "rc legacy_network=$?"
ip route replace 10.0.2.0/24 dev eth0 table local_network >> "$out" 2>&1
log "rc local_network=$?"
ip route replace 10.0.2.0/24 dev eth0 table 98 >> "$out" 2>&1
log "rc 98=$?"

kmsg fix-done
log "=== RULE AFTER ==="
ip rule >> "$out" 2>&1
log "=== ROUTES AFTER ==="
for t in main legacy_system legacy_network local_network 97 98 99; do
    log "-- table $t"
    ip route show table "$t" >> "$out" 2>&1
done
log "=== PING AFTER 1 ==="
ping -c 1 -W 2 10.0.2.2 >> "$out" 2>&1
log "rc=$?"
log "=== PING AFTER 2 ==="
ping -c 1 -W 2 10.0.2.2 >> "$out" 2>&1
log "rc=$?"

log "=== PROPS ==="
getprop >> "$out" 2>&1
log "=== IP ADDR ==="
ip addr >> "$out" 2>&1
log "=== IPTABLES ==="
iptables -S >> "$out" 2>&1
log "=== ADBD PID ==="
pidof adbd >> "$out" 2>&1
log "=== ADBD NETNS TCP6 ==="
cat "/proc/$(pidof adbd)/net/tcp6" >> "$out" 2>&1
log "=== PS ==="
ps -A >> "$out" 2>&1
log "=== LOGCAT ==="
timeout 15 logcat -d -s adbd:V adb:V transport:V socket:V >> "$out" 2>&1

kmsg done
