/*
 * tebox-ctl (local, not for upstream): virtio transport ABI shared by the QEMU
 * device (hw/display/tebox-ctl.c) and the guest kernel driver (tebox_ctl.ko).
 *
 * tebox-ctl is the tebox control bus -- "our own binder": a guest char device
 * (/dev/tebox_ctl) the kernel module routes by channel, so guest processes can
 * talk to each other (guest-internal, never leaving the VM) and/or to the host
 * (tebox-api), without Android properties or binder. This QEMU device is only
 * the guest<->host leg: a transparent, ordered byte pipe between the guest's
 * two virtqueues and a host chardev (ctl.sock). It does NOT parse the message
 * frames -- framing (magic|ver|flags|channel|type|len|payload, see
 * src/service/api/internal/teboxctl) is done by the guest module and tebox-api.
 * Both directions are stream sockets, so frame boundaries need not align with
 * virtqueue buffers.
 *
 * Little-endian wire (guest and host are both aarch64/x86 LE).
 */
#ifndef TEBOX_CTL_PROTO_H
#define TEBOX_CTL_PROTO_H

/* Local, non-upstream virtio device id (62; 60=tebox-v4l2, 61=tebox-mali).
 * virtio_init() takes a uint16_t; the modern virtio-pci device id is
 * 0x1040 + this. Must match the guest driver. */
#define TEBOX_CTL_VIRTIO_ID 62

/* Virtqueues. RX carries host->guest bytes (guest posts empty buffers, the
 * host fills them); TX carries guest->host bytes (guest posts full buffers). */
enum {
    TEBOX_CTL_VQ_RX = 0,
    TEBOX_CTL_VQ_TX = 1,
    TEBOX_CTL_VQ_MAX = 2,
};

/* Virtqueue depth. Frames are small and infrequent; a shallow ring is plenty. */
#define TEBOX_CTL_VQ_SIZE 128

#endif /* TEBOX_CTL_PROTO_H */
