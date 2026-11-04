/* SPDX-License-Identifier: GPL-2.0 */
/*
 * tebox-ctl wire ABI, shared by the guest driver (tebox_ctl.ko) and the QEMU
 * device (qemu/hw/display/tebox-ctl-proto.h, keep the transport ids identical).
 *
 * Transport: a virtio device (id 62) with two queues -- RX host->guest, TX
 * guest->host -- carrying an ordered byte stream. Framing rides on top.
 *
 * Frame (little-endian), a 12-byte header then payload; identical to the Go
 * host side in src/service/api/internal/teboxctl:
 *   off size field
 *   0   2    magic   = 'T','B'
 *   2   1    version = 1
 *   3   1    flags   OPEN|DATA|CLOSE|ACK|LOCAL
 *   4   2    channel logical subsystem id
 *   6   1    type    per-channel message type
 *   7   1    reserved
 *   8   4    length  payload byte count
 *   12  ...  payload
 *
 * The driver routes each frame by channel: to other /dev/tebox_ctl openers
 * subscribed to that channel (guest-internal), and -- unless FLAG_LOCAL is
 * set -- to the host over TX. Host frames (RX) go to subscribed openers only.
 */
#ifndef TEBOX_CTL_ABI_H
#define TEBOX_CTL_ABI_H

#include <linux/ioctl.h>
#include <linux/types.h>

/* Transport (must match qemu/hw/display/tebox-ctl-proto.h). */
#define TEBOX_CTL_VIRTIO_ID 62
enum {
	TEBOX_CTL_VQ_RX = 0,	/* host -> guest */
	TEBOX_CTL_VQ_TX = 1,	/* guest -> host */
	TEBOX_CTL_VQ_MAX = 2,
};

/* Frame layout. */
#define TEBOX_CTL_HEADER_SIZE 12
#define TEBOX_CTL_MAGIC0 'T'
#define TEBOX_CTL_MAGIC1 'B'
#define TEBOX_CTL_VERSION 1

/* Frame flags. */
#define TEBOX_CTL_FLAG_OPEN  0x01
#define TEBOX_CTL_FLAG_DATA  0x02
#define TEBOX_CTL_FLAG_CLOSE 0x04
#define TEBOX_CTL_FLAG_ACK   0x08
#define TEBOX_CTL_FLAG_LOCAL 0x10	/* guest-internal only, never sent to host */

/* Limits. Control messages are small; these bound kernel allocations. */
#define TEBOX_CTL_MAX_PAYLOAD  65536
#define TEBOX_CTL_MAX_CHANNELS 1024

/* ioctls on /dev/tebox_ctl: subscribe/unsubscribe this fd to a channel so it
 * receives frames (guest-internal or host-originated) on that channel. An fd
 * may write (publish) any channel without subscribing. */
#define TEBOX_CTL_IOC_MAGIC 'T'
#define TEBOX_CTL_IOC_SUBSCRIBE   _IOW(TEBOX_CTL_IOC_MAGIC, 1, __u32)
#define TEBOX_CTL_IOC_UNSUBSCRIBE _IOW(TEBOX_CTL_IOC_MAGIC, 2, __u32)

/* Channel ids (mirror teboxctl). 0 reserved for channel-level control. */
#define TEBOX_CTL_CHANNEL_CONTROL  0
#define TEBOX_CTL_CHANNEL_ROTATION 1

#endif /* TEBOX_CTL_ABI_H */
