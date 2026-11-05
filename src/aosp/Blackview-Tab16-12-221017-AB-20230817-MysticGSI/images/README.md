# Guest images

`system.img` is the Blackview Tab16 (MTK) Android 12 (SDK 31) MysticGSI
system image, extracted from
`Blackview-Tab16-12-221017-AB-20230817-MysticGSI.zip`. It is a raw ext4
image (3,009,318,912 bytes), a local input (gitignored, not tracked); verify
it with `sha256sum.txt`.

`vendor.img` and `initramfs.img` are rebuilt by `./run` from `qemu/vendor`,
busybox and the kernel. All three images stay gitignored.
