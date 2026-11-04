#!/system/bin/sh
# tebox guest display-rotation reporter.
#
# Installed at /vendor/bin/tebox-rotation.sh and run by init as a service in the
# permissive hal_keymint_system domain (see tebox-rotation.rc). That domain lets
# it both binder to WindowManager (dumpsys, which a strict vendor domain could
# not) AND open the tebox control bus device. It reads the display rotation and
# publishes one framed teboxctl message (channel=rotation, 1-byte payload =
# quarter-turns) to /dev/tebox_ctl; tebox_ctl.ko routes it to the host where
# tebox-api rotates cast/screendump upright. No adb, no Android property, no
# scratch file.
#
# NB: the guest runs SELinux *enforcing* (androidboot.selinux=permissive is
# ignored by the GSI); only hal_keymint_system is individually permissive, which
# is why this runs there and /dev/tebox_ctl is chmod 0666 by init (ueventd
# coldplug otherwise resets the misc node to 0600 root).
#
# Frame (little-endian, see src/service/api/internal/teboxctl):
#   54 42        magic 'T''B'
#   01           version 1
#   02           flags DATA
#   01 00        channel 1 (rotation)
#   00           type 0
#   00           reserved
#   01 00 00 00  length 1
#   0q           payload: quarter-turns 0..3

DEV=/dev/tebox_ctl

while [ "$(getprop sys.boot_completed 2>/dev/null)" != "1" ]; do
  sleep 2
done

last=
while true; do
  case "$(dumpsys window displays 2>/dev/null | grep -m1 mCurrentRotation)" in
    *ROTATION_90*)  q=1 ;;
    *ROTATION_180*) q=2 ;;
    *ROTATION_270*) q=3 ;;
    *)              q=0 ;;
  esac
  if [ "$q" != "$last" ]; then
    # One write() == one whole frame (the driver does not buffer partial frames).
    printf '\x54\x42\x01\x02\x01\x00\x00\x00\x01\x00\x00\x00\x0'"$q" > "$DEV" 2>/dev/null && last=$q
  fi
  sleep 1
done
