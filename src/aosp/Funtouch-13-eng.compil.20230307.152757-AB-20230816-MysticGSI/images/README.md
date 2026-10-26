# Guest images

`system.img` is the Funtouch OS 13 (vivo/iQOO I2212, Android 13) GSI used by
this variant, extracted from
`Funtouch-13-eng.compil.20230307.152757-AB-20230816-MysticGSI.zip`. It is a
local input (gitignored, not tracked); verify it with `sha256sum.txt`.

`vendor.img` and `initramfs.img` are rebuilt by `./run` from `qemu/vendor`,
busybox and the kernel. They stay gitignored.
