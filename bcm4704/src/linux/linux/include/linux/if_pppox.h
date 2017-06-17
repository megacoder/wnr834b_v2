/***************************************************************************
 * Linux PPP over X - Generic PPP transport layer sockets
 * Linux PPP over Ethernet (PPPoE) Socket Implementation (RFC 2516) 
 *
 * This file supplies definitions required by the PPP over Ethernet driver
 * (pppox.c).  All version information wrt this file is located in pppox.c
 *
 * License:
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 */

#ifndef __LINUX_IF_PPPOX_H
#define __LINUX_IF_PPPOX_H


#include <asm/types.h>
#include <asm/byteorder.h>

#ifdef  __KERNEL__
#include <linux/if_ether.h>
#include <linux/if.h>
#include <linux/netdevice.h>
#include <linux/sched.h>
#include <asm/semaphore.h>
#include <linux/ppp_channel.h>
#endif /* __KERNEL__ */

/* For user-space programs to pick up these definitions
 * which they wouldn't get otherwise without defining __KERNEL__
 */
#ifndef AF_PPPOX
#define AF_PPPOX	24
#define PF_PPPOX	AF_PPPOX
#endif /* !(AF_PPPOX) */

/************************************************************************ 
 * PPPoE addressing definition 
 */ 
typedef __u16 sid_t; 
struct pppoe_addr{ 
       sid_t           sid;                    /* Session identifier */ 
       unsigned char   remote[ETH_ALEN];       /* Remote address */ 
       char            dev[IFNAMSIZ];          /* Local device to use */ 
}; 
 
/************************************************************************
 * PPTP addressing definition
 */
struct pptp_addr{
    unsigned char   remote[ETH_ALEN];       /* Remote address */
    unsigned short  cid;                    /* PPTP call id */
    unsigned short  pcid;                   /* PPTP peer call id */
    unsigned long   seq_num;                /* Seq number of PPP packet */
    unsigned long   ack_num;                /* Ack number of PPP packet */
    unsigned long   srcaddr;                /* Source IP address */
    unsigned long   dstaddr;                /* Destination IP address */
    char            dev[IFNAMSIZ];          /* Local device to use */
};

/************************************************************************ 
 * Protocols supported by AF_PPPOX 
 */ 
#define PX_PROTO_OE    0 /* Currently just PPPoE */
#if 0   
#define PX_MAX_PROTO   1	
#else
#define PX_PROTO_TP    1 /* Add PPTP */
#define PX_MAX_PROTO   2
#endif  
 
struct sockaddr_pppox { 
       sa_family_t     sa_family;            /* address family, AF_PPPOX */ 
       unsigned int    sa_protocol;          /* protocol identifier */ 
       union{ 
               struct pppoe_addr       pppoe; 
       }sa_addr; 
}__attribute__ ((packed)); 

struct sockaddr_pptpox {
    sa_family_t     sa_family;            /* address family, AF_PPPOX */
    unsigned int    sa_protocol;          /* protocol identifier */
    union{
        struct pptp_addr    pptp;
    }sa_addr;
}__attribute__ ((packed));

/*********************************************************************
 *
 * ioctl interface for defining forwarding of connections
 *
 ********************************************************************/

#define PPPOEIOCSFWD	_IOW(0xB1 ,0, sizeof(struct sockaddr_pppox))
#define PPPOEIOCDFWD	_IO(0xB1 ,1)
/*#define PPPOEIOCGFWD	_IOWR(0xB1,2, sizeof(struct sockaddr_pppox))*/

#define PPTPIOCSFWD     _IOW(0xB1 ,0, sizeof(struct sockaddr_pptpox))
#define PPTPIOCDFWD     _IO(0xB1 ,1)
/*#define PPPOEIOCGFWD	_IOWR(0xB1,2, sizeof(struct sockaddr_pptpox))*/

/* Codes to identify message types */
#define PADI_CODE	0x09
#define PADO_CODE	0x07
#define PADR_CODE	0x19
#define PADS_CODE	0x65
#define PADT_CODE	0xa7
struct pppoe_tag {
	__u16 tag_type;
	__u16 tag_len;
	char tag_data[0];
} __attribute ((packed));

/* Tag identifiers */
#define PTT_EOL		__constant_htons(0x0000)
#define PTT_SRV_NAME	__constant_htons(0x0101)
#define PTT_AC_NAME	__constant_htons(0x0102)
#define PTT_HOST_UNIQ	__constant_htons(0x0103)
#define PTT_AC_COOKIE	__constant_htons(0x0104)
#define PTT_VENDOR 	__constant_htons(0x0105)
#define PTT_RELAY_SID	__constant_htons(0x0110)
#define PTT_SRV_ERR     __constant_htons(0x0201)
#define PTT_SYS_ERR  	__constant_htons(0x0202)
#define PTT_GEN_ERR  	__constant_htons(0x0203)

struct pppoe_hdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	__u8 ver : 4;
	__u8 type : 4;
#elif defined(__BIG_ENDIAN_BITFIELD)
	__u8 type : 4;
	__u8 ver : 4;
#else
#error	"Please fix <asm/byteorder.h>"
#endif
	__u8 code;
	__u16 sid;
	__u16 length;
	struct pppoe_tag tag[0];
} __attribute__ ((packed));

/* IP PROTOCOL HEADER */

/* GRE Protocol field */
#define IP_PROTOCOL_ICMP    0x01
#define IP_PROTOCOL_GRE     0x2f

struct pptp_ip_hdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
    __u8    ihl:4,      /* Header length */
        version:4;      /* Version */
#elif defined (__BIG_ENDIAN_BITFIELD)
    __u8    version:4,  /* Version */
        ihl:4;          /* Header length */
#else
#error	"Please fix <asm/byteorder.h>"
#endif
    __u8	tos;        /* Differentiated services field */
    __u16	tot_len;    /* Total length */
    __u16	id;         /* Identification */
    __u16	frag_off;   /* Fragment flags & offset */
    __u8	ttl;        /* Time to live */
    __u8	protocol;   /* Protocol: GRE(0x2f), ICMP(0x01) */
    __u16	check;      /* Header checksum */
    __u32	saddr;      /* Source IP address */
    __u32	daddr;      /* Destination IP address */
    /*The options start here. */
} __attribute__ ((packed));

/* GRE PROTOCOL HEADER */

/* GRE Version field */
//#define GRE_VERSION_1701	0x0
#define GRE_VERSION_PPTP	0x1

/* GRE Protocol field */
#define GRE_PROTOCOL_PPTP	__constant_htons(0x880B)

/* GRE Flags */
#define GRE_FLAG_C		0x80
#define GRE_FLAG_R		0x40
#define GRE_FLAG_K		0x20
#define GRE_FLAG_S		0x10
#define GRE_FLAG_A		0x80

#define GRE_IS_C(f)	((f)&GRE_FLAG_C)
#define GRE_IS_R(f)	((f)&GRE_FLAG_R)
#define GRE_IS_K(f)	((f)&GRE_FLAG_K)
#define GRE_IS_S(f)	((f)&GRE_FLAG_S)
#define GRE_IS_A(f)	((f)&GRE_FLAG_A)

/* GRE is a mess: Four different standards */
/* modified GRE header for PPTP */
struct pptp_gre_hdr {
    __u8  flags;        /* bitfield */
    __u8  version;      /* should be GRE_VERSION_PPTP */
    __u16 protocol;     /* should be GRE_PROTOCOL_PPTP */
    __u16 payload_len;  /* size of ppp payload, not inc. gre header */
    __u16 call_id;      /* peer's call_id for this session */
    __u32 seq;          /* sequence number.  Present if S==1 */
    __u32 ack;          /* seq number of highest packet recieved by */
                        /*  sender in this session */
} __attribute__ ((packed));

/* PPTP packet (IP & GRE) header */
struct pptp_hdr {
    struct pptp_ip_hdr      iphdr;      /* IP header */
    struct pptp_gre_hdr     grehdr;     /* GRE header */
} __attribute__ ((packed));

#ifdef __KERNEL__

struct pppox_proto {
	int (*create)(struct socket *sock);
	int (*ioctl)(struct socket *sock, unsigned int cmd,
		     unsigned long arg);
};

extern int register_pppox_proto(int proto_num, struct pppox_proto *pp);
extern void unregister_pppox_proto(int proto_num);
extern void pppox_unbind_sock(struct sock *sk);/* delete ppp-channel binding */
extern int pppox_channel_ioctl(struct ppp_channel *pc, unsigned int cmd,
			       unsigned long arg);

/* PPPoX socket states */
enum {
    PPPOX_NONE		= 0,  /* initial state */
    PPPOX_CONNECTED	= 1,  /* connection established ==TCP_ESTABLISHED */
    PPPOX_BOUND		= 2,  /* bound to ppp device */
    PPPOX_RELAY		= 4,  /* forwarding is enabled */
    PPPOX_ZOMBIE	= 8,  /* dead, but still bound to ppp device */
    PPPOX_DEAD		= 16  /* dead, useless, please clean me up!*/
};

extern struct ppp_channel_ops pppoe_chan_ops;

extern int pppox_proto_init(struct net_proto *np);

#endif /* __KERNEL__ */

#endif /* !(__LINUX_IF_PPPOX_H) */
