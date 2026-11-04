/*
 * tebox-ctl (local, not for upstream): the guest<->host leg of the tebox
 * control bus. A transparent, ordered byte pipe between the guest's two
 * virtqueues (TEBOX_CTL_VQ_RX host->guest, TEBOX_CTL_VQ_TX guest->host) and a
 * host chardev (ctl.sock, which tebox-api reads). The device does not parse
 * the teboxctl message frames; the guest kernel module (tebox_ctl.ko) and
 * tebox-api handle framing and channel routing. See tebox-ctl-proto.h.
 *
 * Local experiment under qemu/AGENTS.md ("local experiments not intended to be
 * upstreamed"); not prepared as an upstream QEMU submission.
 */
#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "qemu/iov.h"
#include "qemu/module.h"
#include "hw/virtio/virtio.h"
#include "hw/virtio/virtio-pci.h"
#include "hw/pci/pci.h"
#include "hw/core/qdev-properties.h"
#include "hw/core/qdev-properties-system.h"
#include "chardev/char-fe.h"
#include "qapi/error.h"
#include "qom/object.h"

#include "tebox-ctl-proto.h"

#define TYPE_TEBOX_CTL "tebox-ctl"
OBJECT_DECLARE_SIMPLE_TYPE(TeboxCtl, TEBOX_CTL)

/* Flow-control cap on host->guest bytes held while the guest drains them. */
#define TEBOX_CTL_RX_MAX (64 * 1024)

struct TeboxCtl {
    VirtIODevice parent_obj;
    VirtQueue *rx_vq;             /* host -> guest */
    VirtQueue *tx_vq;             /* guest -> host */
    CharFrontend chr;
    uint8_t *rx_buf;              /* host->guest bytes not yet in guest buffers */
    uint32_t rx_len;
    uint32_t rx_cap;
};

/* Move pending host->guest bytes into the guest's posted RX buffers. */
static void tebox_ctl_rx_flush(TeboxCtl *c)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(c);
    while (c->rx_len > 0) {
        VirtQueueElement *elem = virtqueue_pop(c->rx_vq, sizeof(VirtQueueElement));
        if (!elem) {
            break;
        }
        size_t cap = iov_size(elem->in_sg, elem->in_num);
        size_t n = cap < c->rx_len ? cap : c->rx_len;
        if (n) {
            iov_from_buf(elem->in_sg, elem->in_num, 0, c->rx_buf, n);
        }
        virtqueue_push(c->rx_vq, elem, n);
        virtio_notify(vdev, c->rx_vq);
        g_free(elem);
        memmove(c->rx_buf, c->rx_buf + n, c->rx_len - n);
        c->rx_len -= n;
    }
}

/* chardev: how many host->guest bytes we can take right now. */
static int tebox_ctl_chr_can_read(void *opaque)
{
    TeboxCtl *c = opaque;
    tebox_ctl_rx_flush(c);
    return c->rx_len < TEBOX_CTL_RX_MAX ? TEBOX_CTL_RX_MAX - c->rx_len : 0;
}

/* chardev: the host sent bytes destined for the guest. */
static void tebox_ctl_chr_read(void *opaque, const uint8_t *buf, int size)
{
    TeboxCtl *c = opaque;
    if (size <= 0) {
        return;
    }
    if (c->rx_len + (uint32_t)size > c->rx_cap) {
        c->rx_cap = c->rx_len + (uint32_t)size;
        c->rx_buf = g_realloc(c->rx_buf, c->rx_cap);
    }
    memcpy(c->rx_buf + c->rx_len, buf, size);
    c->rx_len += (uint32_t)size;
    tebox_ctl_rx_flush(c);
}

/* guest -> host: drain each posted TX buffer to the chardev. */
static void tebox_ctl_handle_tx(VirtIODevice *vdev, VirtQueue *vq)
{
    TeboxCtl *c = TEBOX_CTL(vdev);
    for (;;) {
        VirtQueueElement *elem = virtqueue_pop(vq, sizeof(VirtQueueElement));
        if (!elem) {
            break;
        }
        size_t len = iov_size(elem->out_sg, elem->out_num);
        if (len) {
            g_autofree uint8_t *data = g_malloc(len);
            iov_to_buf(elem->out_sg, elem->out_num, 0, data, len);
            /* Drop if the host side is not attached, so the guest never
             * blocks on a missing reader. */
            if (qemu_chr_fe_backend_connected(&c->chr)) {
                qemu_chr_fe_write_all(&c->chr, data, len);
            }
        }
        virtqueue_push(vq, elem, 0);
        virtio_notify(vdev, vq);
        g_free(elem);
    }
}

/* Guest posted fresh RX buffers: fill them and re-open chardev flow. */
static void tebox_ctl_handle_rx(VirtIODevice *vdev, VirtQueue *vq)
{
    TeboxCtl *c = TEBOX_CTL(vdev);
    (void)vq;
    tebox_ctl_rx_flush(c);
    qemu_chr_fe_accept_input(&c->chr);
}

static uint64_t tebox_ctl_get_features(VirtIODevice *vdev, uint64_t features,
                                       Error **errp)
{
    (void)vdev; (void)errp;
    return features;
}

static void tebox_ctl_realize(DeviceState *dev, Error **errp)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    TeboxCtl *c = TEBOX_CTL(dev);
    (void)errp;
    virtio_init(vdev, TEBOX_CTL_VIRTIO_ID, 0);
    c->rx_vq = virtio_add_queue(vdev, TEBOX_CTL_VQ_SIZE, tebox_ctl_handle_rx);
    c->tx_vq = virtio_add_queue(vdev, TEBOX_CTL_VQ_SIZE, tebox_ctl_handle_tx);
    c->rx_buf = NULL;
    c->rx_len = 0;
    c->rx_cap = 0;
    qemu_chr_fe_set_handlers(&c->chr, tebox_ctl_chr_can_read, tebox_ctl_chr_read,
                             NULL, NULL, c, NULL, true);
    info_report("tebox-ctl: virtio bus ready (id=%d)", TEBOX_CTL_VIRTIO_ID);
}

static void tebox_ctl_unrealize(DeviceState *dev)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    TeboxCtl *c = TEBOX_CTL(dev);
    qemu_chr_fe_set_handlers(&c->chr, NULL, NULL, NULL, NULL, NULL, NULL, false);
    virtio_delete_queue(c->rx_vq);
    virtio_delete_queue(c->tx_vq);
    virtio_cleanup(vdev);
    g_free(c->rx_buf);
}

static const VMStateDescription tebox_ctl_vmstate = {
    .name = "tebox-ctl",
    .unmigratable = 1,
};

static const Property tebox_ctl_properties[] = {
    DEFINE_PROP_CHR("chardev", TeboxCtl, chr),
};

static void tebox_ctl_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);
    (void)data;
    dc->vmsd = &tebox_ctl_vmstate;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
    device_class_set_props(dc, tebox_ctl_properties);
    vdc->realize = tebox_ctl_realize;
    vdc->unrealize = tebox_ctl_unrealize;
    vdc->get_features = tebox_ctl_get_features;
}

static const TypeInfo tebox_ctl_info = {
    .name = TYPE_TEBOX_CTL,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(TeboxCtl),
    .class_init = tebox_ctl_class_init,
};

static void tebox_ctl_register_types(void)
{
    type_register_static(&tebox_ctl_info);
}

type_init(tebox_ctl_register_types)

/* ---- virtio-pci transport wrapper: -device tebox-ctl-pci,chardev=... ----- */

#define TYPE_TEBOX_CTL_PCI "tebox-ctl-pci-base"
typedef struct TeboxCtlPCI TeboxCtlPCI;
DECLARE_INSTANCE_CHECKER(TeboxCtlPCI, TEBOX_CTL_PCI, TYPE_TEBOX_CTL_PCI)

struct TeboxCtlPCI {
    VirtIOPCIProxy parent_obj;
    TeboxCtl vdev;
};

static void tebox_ctl_pci_realize(VirtIOPCIProxy *vpci_dev, Error **errp)
{
    TeboxCtlPCI *dev = TEBOX_CTL_PCI(vpci_dev);
    DeviceState *vdev = DEVICE(&dev->vdev);
    qdev_realize(vdev, BUS(&vpci_dev->bus), errp);
}

static void tebox_ctl_pci_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioPCIClass *k = VIRTIO_PCI_CLASS(klass);
    PCIDeviceClass *pcidev_k = PCI_DEVICE_CLASS(klass);
    (void)data;
    k->realize = tebox_ctl_pci_realize;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
    pcidev_k->vendor_id = PCI_VENDOR_ID_REDHAT_QUMRANET;
    pcidev_k->device_id = 0x1040 + TEBOX_CTL_VIRTIO_ID;  /* modern virtio-pci */
    pcidev_k->revision = VIRTIO_PCI_ABI_VERSION;
    pcidev_k->class_id = PCI_CLASS_OTHERS;
}

static void tebox_ctl_pci_initfn(Object *obj)
{
    TeboxCtlPCI *dev = TEBOX_CTL_PCI(obj);
    virtio_instance_init_common(obj, &dev->vdev, sizeof(dev->vdev),
                                TYPE_TEBOX_CTL);
}

static const VirtioPCIDeviceTypeInfo tebox_ctl_pci_info = {
    .base_name = TYPE_TEBOX_CTL_PCI,
    .generic_name = "tebox-ctl-pci",
    .instance_size = sizeof(TeboxCtlPCI),
    .instance_init = tebox_ctl_pci_initfn,
    .class_init = tebox_ctl_pci_class_init,
};

static void tebox_ctl_pci_register(void)
{
    virtio_pci_types_register(&tebox_ctl_pci_info);
}

type_init(tebox_ctl_pci_register)
