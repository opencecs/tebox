#!/system/bin/sh
# ColorOS's QPerformance.jar (com.qualcomm.qti.Performance) declares 16 native
# methods and never calls System.loadLibrary, so ART can only bind them with
# dlsym(RTLD_DEFAULT, <mangled name>).  Zygote preloads our no-op
# libqti_performance.so via LD_PRELOAD, but libart.so lives in the
# "com_android_art" linker namespace and that namespace's link to the default
# namespace is a soname whitelist
# (namespace.com_android_art.link.default.shared_libs).  Unless the stub's
# soname is on that list, ART cannot see the preloaded library and
# Performance.native_* throws UnsatisfiedLinkError, killing system_server when
# the wallpaper picker launches an app.
#
# Run from init.zygote64_32.rc on post-fs-data: init.rc's post-fs-data action
# calls enter_default_mount_ns (init switches into the default mount namespace
# and binds /linkerconfig/default over /linkerconfig) and then
# perform_apex_config, which regenerates ld.config.txt with the mounted APEX
# modules.  This rc file is parsed after init.rc, so this action runs after
# init.rc's post-fs-data action -- i.e. after that regeneration -- and before
# the zygote-start action, which is queued behind the post-fs-data batch.
#
# The exec service may run in the bootstrap mount namespace (a service started
# before the default namespace is ready is permanently marked pre-APEX), where
# /linkerconfig is the bootstrap config without com_android_art.  Address the
# file through /proc/1/root instead: path resolution then follows init's mount
# namespace (the default one), which is what zygote also binds at exec time.
patched=0
for f in \
    /proc/1/root/linkerconfig/ld.config.txt \
    /proc/1/root/linkerconfig/default/ld.config.txt \
    /linkerconfig/ld.config.txt \
    /linkerconfig/default/ld.config.txt; do
    [ -f "$f" ] || continue
    /system/bin/grep '^namespace\.com_android_art\.link\.default\.shared_libs' "$f" >/dev/null 2>&1 || continue
    if /system/bin/grep 'libqti_performance\.so' "$f" >/dev/null 2>&1; then
        patched=1
        echo "qti_perf_stub: already whitelisted in $f" > /dev/kmsg
        continue
    fi
    # Insert after the first whitelist line; the whitelist is a set, so order
    # inside it does not matter.  com_android_art only exists in [system].
    /system/bin/awk '
        /^namespace\.com_android_art\.link\.default\.shared_libs/ && !done {
            print
            print "namespace.com_android_art.link.default.shared_libs += libqti_performance.so"
            done = 1
            next
        }
        { print }
    ' "$f" > "$f.new" || continue
    /system/bin/cat "$f.new" > "$f" || { /system/bin/rm -f "$f.new"; continue; }
    /system/bin/rm -f "$f.new"
    patched=1
    echo "qti_perf_stub: whitelisted in $f (matches: $(/system/bin/grep -c 'libqti_performance\.so' "$f" 2>/dev/null))" > /dev/kmsg
done
if [ "$patched" != 1 ]; then
    echo "qti_perf_stub: WARNING: no linker config patched" > /dev/kmsg
    for f in \
        /proc/1/root/linkerconfig/ld.config.txt \
        /proc/1/root/linkerconfig/default/ld.config.txt \
        /linkerconfig/ld.config.txt \
        /linkerconfig/default/ld.config.txt; do
        if [ -f "$f" ]; then
            echo "qti_perf_stub: candidate $f art=$(/system/bin/grep -c 'com_android_art' "$f" 2>/dev/null)" > /dev/kmsg
        else
            echo "qti_perf_stub: candidate $f missing" > /dev/kmsg
        fi
    done
fi
exit 0
