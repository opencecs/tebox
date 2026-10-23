# Guest images

`system.img` is the pinned Android GSI, stored with Git LFS. Its digest is in
`sha256sum.txt` and `.ci/guest.lock.json`. A checkout that leaves a Git LFS
pointer is replaced by `.ci/fetch-guest-inputs.py` before the Android build.

`vendor.img` and `initramfs.img` are rebuilt by `./run` from `qemu/vendor`,
busybox and the kernel. They stay gitignored.
