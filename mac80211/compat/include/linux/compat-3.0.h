#ifndef LINUX_3_0_COMPAT_H
#define LINUX_3_0_COMPAT_H

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0))

/*
 * since commit 1c5cae815d19ffe02bdfda1260949ef2b1806171
 * "net: call dev_alloc_name from register_netdevice" dev_alloc_name is
 * called automatically. This is not implemented in older kernel
 * versions so it will result in device wrong names.
 */
static inline int register_netdevice_name(struct net_device *dev)
{
	int err;

	if (strchr(dev->name, '%')) {
		err = dev_alloc_name(dev, dev->name);
		if (err < 0)
			return err;
	}

	return register_netdevice(dev);
}

#define register_netdevice(dev) register_netdevice_name(dev)

/* BCMA core, see drivers/bcma/ */
#ifndef BCMA_CORE
/* Broadcom's specific AMBA core, see drivers/bcma/ */
struct bcma_device_id {
	__u16	manuf;
	__u16	id;
	__u8	rev;
	__u8	class;
};
#define BCMA_CORE(_manuf, _id, _rev, _class)  \
	{ .manuf = _manuf, .id = _id, .rev = _rev, .class = _class, }
#define BCMA_CORETABLE_END  \
	{ 0, },

#define BCMA_ANY_MANUF		0xFFFF
#define BCMA_ANY_ID		0xFFFF
#define BCMA_ANY_REV		0xFF
#define BCMA_ANY_CLASS		0xFF
#endif /* BCMA_CORE */

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)) */

#endif /* LINUX_3_0_COMPAT_H */
