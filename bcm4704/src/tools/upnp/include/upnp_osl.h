/*
 * OS Independent Layer
 * 
 * Copyright 2007, Broadcom Corporation      
 * All Rights Reserved.      
 *       
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY      
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM      
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS      
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.      
 * $Id: upnp_osl.h,v 1.1.1.1 2010/03/05 07:31:36 reynolds Exp $
 */

#ifndef _upnp_osl_h_
#define _upnp_osl_h_

#if defined(linux)
#include <linux_osl.h>
#else
#error "Unsupported OSL requested"
#endif
#include "typedefs.h"

/* forward declarations - defined in upnp.h */
struct _if_stats;
struct iface;

typedef enum { OSL_LINK_DOWN = 0, OSL_LINK_UP = 1 } osl_link_t;

extern struct in_addr *osl_ifaddr(const char *, struct in_addr *);
extern int osl_ifstats(char *, struct _if_stats *);
extern void osl_sys_restart();
extern void osl_sys_reboot();
extern osl_link_t osl_link_status(char *devname);
extern uint osl_max_bitrates(char *devname, ulong *rx, ulong *tx);
extern void osl_sys_restart();
extern void osl_sys_reboot();
extern void osl_igd_disable(char *devname);
extern void osl_igd_enable(char *devname);
extern bool osl_wan_isup(char *devname);
extern bool osl_lan_isup(char *devname);
extern bool osl_set_macaddr(char *devname, char spoofed[]);
extern int osl_join_multicast(struct iface *, int, ulong, ushort);

#endif	/* _upnp_osl_h_ */
