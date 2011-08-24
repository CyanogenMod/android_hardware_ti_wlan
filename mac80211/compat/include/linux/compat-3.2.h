#ifndef LINUX_3_2_COMPAT_H
#define LINUX_3_2_COMPAT_H

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0))

/*
 * This is not part of The 2.6.37 kernel yet but we
 * we use it to optimize the backport code we
 * need to implement. Instead of using ifdefs
 * to check what version of the check we use
 * we just replace all checks on current code
 * with this. I'll submit this upstream too, that
 * way all we'd have to do is to implement this
 * for older kernels, then we would not have to
 * edit the upstrema code for backport efforts.
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
#define br_port_exists(dev)	(dev->priv_flags & IFF_BRIDGE_PORT)
#else
#define br_port_exists(dev)	(dev->br_port)
#endif

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0)) */

#endif /* LINUX_3_2_COMPAT_H */
