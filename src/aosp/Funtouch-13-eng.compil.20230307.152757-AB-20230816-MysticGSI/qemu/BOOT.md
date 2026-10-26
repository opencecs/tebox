# QEMU boot notes (aosp_arm64-BP4A.251205.006)

## Launch

From the repo root (lunch menu if you omit the variant):

```bash
FORCE_VIRGL=1 SNAPSHOT=1 ./run
# or:
FORCE_VIRGL=1 SNAPSHOT=1 ./run aosp_arm64-BP4A.251205.006
```

`./run` reads `KERNEL`, `qemu/cmdline/boot`, ensures `images/vendor.img` /
`images/initramfs.img` are current (from `qemu/vendor`, busybox, …), then boots
via `scripts/boot-qemu.sh`.

Edit `qemu/cmdline/boot` when this variant needs different kernel /
`androidboot.*` parameters.

## Display / input invariants

- Portrait **1080 × 2400**, **420 dpi**
- Prefer `FORCE_VIRGL=1` (`virtio-gpu-gl-pci`) on macOS ARM64
- Direct virtio touchscreen / keyboard; do not regress to tablet-only paths
  without an explicit request

## Debug console

With `QEMU_DEBUG=1`, use `scripts/qemu-console.py` against the debug socket
under `out/test-<variant>/`. Confirm `__QEMU_COMMAND_DONE__` before trusting
command output.

## Soft HALs

Vendor stubs (KeyMint soft, graphics, health, …) live under `hardware/` and are
packed into `images/vendor.img` from `qemu/vendor`. Missing VINTF HALs
typically show up as `servicemanager` “Could not find … in the VINTF manifest”
loops.
