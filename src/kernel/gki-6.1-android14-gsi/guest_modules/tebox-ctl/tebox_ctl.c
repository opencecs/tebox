// SPDX-License-Identifier: GPL-2.0
/*
 * tebox_ctl: guest driver for the tebox-ctl virtio device (id 62) -- the guest
 * end of the tebox control bus ("our own binder"). It exposes /dev/tebox_ctl.
 * A frame written by one opener is routed by channel to the other openers
 * subscribed to that channel (guest-internal IPC, crossing SELinux domains
 * without binder or Android properties) and, unless FLAG_LOCAL is set, to the
 * host over virtio (ctl.sock -> tebox-api). Host-originated frames are
 * delivered to the openers subscribed to their channel.
 *
 * Framing and routing live here and in tebox-api; the QEMU device is only a
 * byte pipe. See tebox-ctl-abi.h.
 *
 * Local, non-upstream guest module (same family as tebox_v4l2 / tebox_mali).
 */
#include <linux/module.h>
#include <linux/virtio.h>
#include <linux/virtio_config.h>
#include <linux/scatterlist.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include "tebox-ctl-abi.h"

#define TEBOX_CTL_RXBUF       4096        /* bytes per host->guest buffer   */
#define TEBOX_CTL_NR_RXBUF    32          /* host->guest buffers posted     */
#define TEBOX_CTL_REASM_SIZE  (TEBOX_CTL_HEADER_SIZE + TEBOX_CTL_MAX_PAYLOAD + \
			       TEBOX_CTL_RXBUF)
#define TEBOX_CTL_WRITE_MAX   (128 * 1024) /* max bytes per write()         */
#define TEBOX_CTL_CLIENT_RXQ_MAX (256 * 1024) /* per-fd backlog cap         */

/* A frame queued for delivery to one opener. */
struct tebox_ctl_msg {
	struct list_head node;
	size_t len;
	u8 data[];
};

/* Per-open state: subscriptions + a datagram receive queue. */
struct tebox_ctl_client {
	struct list_head node;		/* dev->clients                       */
	struct tebox_ctl_dev *dev;
	unsigned long *subs;		/* bitmap[TEBOX_CTL_MAX_CHANNELS]     */
	struct list_head rxq;		/* struct tebox_ctl_msg               */
	size_t rxq_bytes;
	spinlock_t lock;		/* protects rxq, rxq_bytes            */
	wait_queue_head_t wait;
};

struct tebox_ctl_dev {
	struct virtio_device *vdev;
	struct virtqueue *rx_vq;	/* host -> guest */
	struct virtqueue *tx_vq;	/* guest -> host */
	struct miscdevice misc;

	struct mutex clients_lock;	/* protects clients list              */
	struct list_head clients;

	struct work_struct rx_work;
	u8 *reasm;			/* host->guest stream reassembly      */
	size_t reasm_len;

	spinlock_t tx_lock;		/* serialises tx_vq, IRQ-safe         */
};

static struct tebox_ctl_dev *g_dev;	/* single instance */

/* Length of the complete frame at the head of buf, 0 if more bytes are
 * needed, negative on a protocol error. */
static ssize_t tebox_ctl_frame_len(const u8 *b, size_t avail)
{
	u32 len;

	if (avail < TEBOX_CTL_HEADER_SIZE)
		return 0;
	if (b[0] != TEBOX_CTL_MAGIC0 || b[1] != TEBOX_CTL_MAGIC1)
		return -EPROTO;
	if (b[2] != TEBOX_CTL_VERSION)
		return -EPROTO;
	len = (u32)b[8] | (u32)b[9] << 8 | (u32)b[10] << 16 | (u32)b[11] << 24;
	if (len > TEBOX_CTL_MAX_PAYLOAD)
		return -EMSGSIZE;
	if (avail < TEBOX_CTL_HEADER_SIZE + len)
		return 0;
	return TEBOX_CTL_HEADER_SIZE + len;
}

static inline u16 tebox_ctl_channel(const u8 *frame)
{
	return (u16)frame[4] | (u16)frame[5] << 8;
}

/* Append one frame to a client's receive queue, dropping the oldest frames
 * if the backlog cap is exceeded, and wake any reader. */
static void tebox_ctl_deliver(struct tebox_ctl_client *c, const u8 *data,
			      size_t len)
{
	struct tebox_ctl_msg *msg = kmalloc(sizeof(*msg) + len, GFP_KERNEL);

	if (!msg)
		return;
	memcpy(msg->data, data, len);
	msg->len = len;

	spin_lock(&c->lock);
	while (c->rxq_bytes + len > TEBOX_CTL_CLIENT_RXQ_MAX &&
	       !list_empty(&c->rxq)) {
		struct tebox_ctl_msg *old =
			list_first_entry(&c->rxq, struct tebox_ctl_msg, node);
		list_del(&old->node);
		c->rxq_bytes -= old->len;
		kfree(old);
	}
	list_add_tail(&msg->node, &c->rxq);
	c->rxq_bytes += len;
	spin_unlock(&c->lock);

	wake_up_interruptible(&c->wait);
}

/* Send one frame to the host over TX. Reclaims completed buffers first; drops
 * the frame if the ring is full (callers tolerate loss of a transient value). */
static void tebox_ctl_tx(struct tebox_ctl_dev *dev, const u8 *data, size_t len)
{
	struct scatterlist sg;
	unsigned long flags;
	unsigned int l;
	void *old, *buf;

	spin_lock_irqsave(&dev->tx_lock, flags);
	while ((old = virtqueue_get_buf(dev->tx_vq, &l)))
		kfree(old);
	buf = kmemdup(data, len, GFP_ATOMIC);
	if (!buf) {
		spin_unlock_irqrestore(&dev->tx_lock, flags);
		return;
	}
	sg_init_one(&sg, buf, len);
	if (virtqueue_add_outbuf(dev->tx_vq, &sg, 1, buf, GFP_ATOMIC) < 0)
		kfree(buf);
	else
		virtqueue_kick(dev->tx_vq);
	spin_unlock_irqrestore(&dev->tx_lock, flags);
}

/* Route a frame published by a guest opener: to other subscribed openers and,
 * unless local-only, to the host. */
static void tebox_ctl_route_local(struct tebox_ctl_dev *dev,
				   struct tebox_ctl_client *from,
				   const u8 *data, size_t len)
{
	u16 ch = tebox_ctl_channel(data);
	struct tebox_ctl_client *c;

	if (ch < TEBOX_CTL_MAX_CHANNELS) {
		mutex_lock(&dev->clients_lock);
		list_for_each_entry(c, &dev->clients, node) {
			if (c != from && test_bit(ch, c->subs))
				tebox_ctl_deliver(c, data, len);
		}
		mutex_unlock(&dev->clients_lock);
	}
	if (!(data[3] & TEBOX_CTL_FLAG_LOCAL))
		tebox_ctl_tx(dev, data, len);
}

/* Route a frame received from the host: to subscribed openers only. */
static void tebox_ctl_route_host(struct tebox_ctl_dev *dev, const u8 *data,
				 size_t len)
{
	u16 ch = tebox_ctl_channel(data);
	struct tebox_ctl_client *c;

	if (ch >= TEBOX_CTL_MAX_CHANNELS)
		return;
	mutex_lock(&dev->clients_lock);
	list_for_each_entry(c, &dev->clients, node) {
		if (test_bit(ch, c->subs))
			tebox_ctl_deliver(c, data, len);
	}
	mutex_unlock(&dev->clients_lock);
}

/* Feed host bytes into the reassembly buffer and route complete frames. */
static void tebox_ctl_rx_feed(struct tebox_ctl_dev *dev, const u8 *buf,
			      unsigned int len)
{
	size_t pos;

	if (len > TEBOX_CTL_RXBUF)
		len = TEBOX_CTL_RXBUF;
	if (dev->reasm_len + len > TEBOX_CTL_REASM_SIZE) {
		/* Desynchronised stream; drop what we have and resync. */
		dev->reasm_len = 0;
		if (len > TEBOX_CTL_REASM_SIZE)
			return;
	}
	memcpy(dev->reasm + dev->reasm_len, buf, len);
	dev->reasm_len += len;

	pos = 0;
	while (pos < dev->reasm_len) {
		ssize_t fl = tebox_ctl_frame_len(dev->reasm + pos,
						 dev->reasm_len - pos);
		if (fl == 0)
			break;			/* need more bytes */
		if (fl < 0) {			/* resync: drop one byte */
			pos++;
			continue;
		}
		tebox_ctl_route_host(dev, dev->reasm + pos, fl);
		pos += fl;
	}
	if (pos) {
		memmove(dev->reasm, dev->reasm + pos, dev->reasm_len - pos);
		dev->reasm_len -= pos;
	}
}

static void tebox_ctl_rx_work(struct work_struct *work)
{
	struct tebox_ctl_dev *dev =
		container_of(work, struct tebox_ctl_dev, rx_work);
	struct scatterlist sg;
	unsigned int len;
	void *buf;

	do {
		virtqueue_disable_cb(dev->rx_vq);
		while ((buf = virtqueue_get_buf(dev->rx_vq, &len))) {
			tebox_ctl_rx_feed(dev, buf, len);
			sg_init_one(&sg, buf, TEBOX_CTL_RXBUF);
			if (virtqueue_add_inbuf(dev->rx_vq, &sg, 1, buf,
						GFP_KERNEL) < 0)
				kfree(buf);
		}
		virtqueue_kick(dev->rx_vq);
	} while (!virtqueue_enable_cb(dev->rx_vq));
}

static void tebox_ctl_rx_cb(struct virtqueue *vq)
{
	struct tebox_ctl_dev *dev = vq->vdev->priv;

	schedule_work(&dev->rx_work);
}

static void tebox_ctl_tx_cb(struct virtqueue *vq)
{
	struct tebox_ctl_dev *dev = vq->vdev->priv;
	unsigned long flags;
	unsigned int len;
	void *buf;

	spin_lock_irqsave(&dev->tx_lock, flags);
	while ((buf = virtqueue_get_buf(vq, &len)))
		kfree(buf);
	spin_unlock_irqrestore(&dev->tx_lock, flags);
}

/* ---- char device -------------------------------------------------------- */

static int tebox_ctl_open(struct inode *ino, struct file *filp)
{
	struct tebox_ctl_dev *dev = g_dev;
	struct tebox_ctl_client *c;

	if (!dev)
		return -ENODEV;
	c = kzalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;
	c->subs = bitmap_zalloc(TEBOX_CTL_MAX_CHANNELS, GFP_KERNEL);
	if (!c->subs) {
		kfree(c);
		return -ENOMEM;
	}
	c->dev = dev;
	INIT_LIST_HEAD(&c->rxq);
	spin_lock_init(&c->lock);
	init_waitqueue_head(&c->wait);

	mutex_lock(&dev->clients_lock);
	list_add_tail(&c->node, &dev->clients);
	mutex_unlock(&dev->clients_lock);

	filp->private_data = c;
	return 0;
}

static int tebox_ctl_release(struct inode *ino, struct file *filp)
{
	struct tebox_ctl_client *c = filp->private_data;
	struct tebox_ctl_msg *msg, *tmp;

	mutex_lock(&c->dev->clients_lock);
	list_del(&c->node);
	mutex_unlock(&c->dev->clients_lock);

	list_for_each_entry_safe(msg, tmp, &c->rxq, node)
		kfree(msg);
	bitmap_free(c->subs);
	kfree(c);
	return 0;
}

static ssize_t tebox_ctl_read(struct file *filp, char __user *ubuf,
			      size_t count, loff_t *ppos)
{
	struct tebox_ctl_client *c = filp->private_data;
	struct tebox_ctl_msg *msg;
	ssize_t ret;

	spin_lock(&c->lock);
	while (list_empty(&c->rxq)) {
		spin_unlock(&c->lock);
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		if (wait_event_interruptible(c->wait, !list_empty(&c->rxq)))
			return -ERESTARTSYS;
		spin_lock(&c->lock);
	}
	msg = list_first_entry(&c->rxq, struct tebox_ctl_msg, node);
	if (count < msg->len) {		/* datagram: caller must fit the frame */
		spin_unlock(&c->lock);
		return -EMSGSIZE;
	}
	list_del(&msg->node);
	c->rxq_bytes -= msg->len;
	spin_unlock(&c->lock);

	ret = copy_to_user(ubuf, msg->data, msg->len) ? -EFAULT : msg->len;
	kfree(msg);
	return ret;
}

static ssize_t tebox_ctl_write(struct file *filp, const char __user *ubuf,
			       size_t count, loff_t *ppos)
{
	struct tebox_ctl_client *c = filp->private_data;
	u8 *buf;
	size_t pos = 0;
	ssize_t ret = 0;

	if (count == 0)
		return 0;
	if (count > TEBOX_CTL_WRITE_MAX)
		return -EMSGSIZE;
	buf = memdup_user(ubuf, count);
	if (IS_ERR(buf))
		return PTR_ERR(buf);

	while (pos < count) {
		ssize_t fl = tebox_ctl_frame_len(buf + pos, count - pos);

		if (fl == 0)
			break;			/* trailing partial frame */
		if (fl < 0) {
			if (pos == 0)
				ret = fl;
			break;
		}
		tebox_ctl_route_local(c->dev, c, buf + pos, fl);
		pos += fl;
	}
	kfree(buf);
	return pos ? (ssize_t)pos : ret;
}

static __poll_t tebox_ctl_poll(struct file *filp, poll_table *wait)
{
	struct tebox_ctl_client *c = filp->private_data;
	__poll_t mask = EPOLLOUT | EPOLLWRNORM;

	poll_wait(filp, &c->wait, wait);
	spin_lock(&c->lock);
	if (!list_empty(&c->rxq))
		mask |= EPOLLIN | EPOLLRDNORM;
	spin_unlock(&c->lock);
	return mask;
}

static long tebox_ctl_ioctl(struct file *filp, unsigned int cmd,
			    unsigned long arg)
{
	struct tebox_ctl_client *c = filp->private_data;
	u32 ch;

	switch (cmd) {
	case TEBOX_CTL_IOC_SUBSCRIBE:
	case TEBOX_CTL_IOC_UNSUBSCRIBE:
		if (copy_from_user(&ch, (void __user *)arg, sizeof(ch)))
			return -EFAULT;
		if (ch >= TEBOX_CTL_MAX_CHANNELS)
			return -EINVAL;
		if (cmd == TEBOX_CTL_IOC_SUBSCRIBE)
			set_bit(ch, c->subs);
		else
			clear_bit(ch, c->subs);
		return 0;
	default:
		return -ENOTTY;
	}
}

static const struct file_operations tebox_ctl_fops = {
	.owner		= THIS_MODULE,
	.open		= tebox_ctl_open,
	.release	= tebox_ctl_release,
	.read		= tebox_ctl_read,
	.write		= tebox_ctl_write,
	.poll		= tebox_ctl_poll,
	.unlocked_ioctl	= tebox_ctl_ioctl,
	.compat_ioctl	= tebox_ctl_ioctl,
	.llseek		= no_llseek,
};

/* ---- virtio driver ------------------------------------------------------ */

static int tebox_ctl_probe(struct virtio_device *vdev)
{
	struct tebox_ctl_dev *dev;
	struct virtqueue *vqs[TEBOX_CTL_VQ_MAX];
	vq_callback_t *cbs[TEBOX_CTL_VQ_MAX] = {
		[TEBOX_CTL_VQ_RX] = tebox_ctl_rx_cb,
		[TEBOX_CTL_VQ_TX] = tebox_ctl_tx_cb,
	};
	static const char * const names[TEBOX_CTL_VQ_MAX] = {
		[TEBOX_CTL_VQ_RX] = "rx",
		[TEBOX_CTL_VQ_TX] = "tx",
	};
	int err, i;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;
	dev->reasm = kmalloc(TEBOX_CTL_REASM_SIZE, GFP_KERNEL);
	if (!dev->reasm) {
		err = -ENOMEM;
		goto err_free_dev;
	}
	dev->vdev = vdev;
	vdev->priv = dev;
	mutex_init(&dev->clients_lock);
	spin_lock_init(&dev->tx_lock);
	INIT_LIST_HEAD(&dev->clients);
	INIT_WORK(&dev->rx_work, tebox_ctl_rx_work);

	err = virtio_find_vqs(vdev, TEBOX_CTL_VQ_MAX, vqs, cbs, names, NULL);
	if (err)
		goto err_free_reasm;
	dev->rx_vq = vqs[TEBOX_CTL_VQ_RX];
	dev->tx_vq = vqs[TEBOX_CTL_VQ_TX];

	/* Post the host->guest receive buffers. */
	for (i = 0; i < TEBOX_CTL_NR_RXBUF; i++) {
		struct scatterlist sg;
		void *buf = kmalloc(TEBOX_CTL_RXBUF, GFP_KERNEL);

		if (!buf)
			continue;
		sg_init_one(&sg, buf, TEBOX_CTL_RXBUF);
		if (virtqueue_add_inbuf(dev->rx_vq, &sg, 1, buf, GFP_KERNEL) < 0)
			kfree(buf);
	}
	virtqueue_kick(dev->rx_vq);

	dev->misc.minor = MISC_DYNAMIC_MINOR;
	dev->misc.name = "tebox_ctl";
	dev->misc.fops = &tebox_ctl_fops;
	dev->misc.mode = 0666;
	err = misc_register(&dev->misc);
	if (err)
		goto err_del_vqs;

	g_dev = dev;
	virtio_device_ready(vdev);
	dev_info(&vdev->dev, "tebox-ctl bus ready: /dev/tebox_ctl (id=%d)\n",
		 TEBOX_CTL_VIRTIO_ID);
	return 0;

err_del_vqs:
	vdev->config->del_vqs(vdev);
err_free_reasm:
	kfree(dev->reasm);
err_free_dev:
	kfree(dev);
	vdev->priv = NULL;
	return err;
}

static void tebox_ctl_remove(struct virtio_device *vdev)
{
	struct tebox_ctl_dev *dev = vdev->priv;
	void *buf;

	g_dev = NULL;
	misc_deregister(&dev->misc);
	virtio_reset_device(vdev);
	cancel_work_sync(&dev->rx_work);
	while ((buf = virtqueue_detach_unused_buf(dev->rx_vq)))
		kfree(buf);
	while ((buf = virtqueue_detach_unused_buf(dev->tx_vq)))
		kfree(buf);
	vdev->config->del_vqs(vdev);
	kfree(dev->reasm);
	kfree(dev);
}

static const struct virtio_device_id tebox_ctl_id_table[] = {
	{ TEBOX_CTL_VIRTIO_ID, VIRTIO_DEV_ANY_ID },
	{ 0 },
};
MODULE_DEVICE_TABLE(virtio, tebox_ctl_id_table);

static struct virtio_driver tebox_ctl_driver = {
	.driver.name	= KBUILD_MODNAME,
	.driver.owner	= THIS_MODULE,
	.id_table	= tebox_ctl_id_table,
	.probe		= tebox_ctl_probe,
	.remove		= tebox_ctl_remove,
};

module_virtio_driver(tebox_ctl_driver);

MODULE_DESCRIPTION("tebox control bus guest driver (/dev/tebox_ctl)");
MODULE_LICENSE("GPL v2");
