/** -*- linux-c -*- ***********************************************************
 * Linux Point-to-Point Tunneling Protocol (PPPoX/PPTP) Sockets
 *
 * PPPoX --- Generic PPP encapsulation socket family
 * PPTP  --- Point-to-Point Tunneling Protocol (RFC 2637)
 *
 *
 * Version:	0.1.0
 *
 ******************************************************************************
 * 062606 :	Create PPTP module.
 ******************************************************************************
 * Author:  Winster Chan <winster.wh.chan@foxconn.com>
 * Contributors:
 *
 * License:
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 */

#include <linux/string.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/net.h>
#include <linux/inetdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/init.h>
#include <linux/if_ether.h>
#include <linux/if_pppox.h>
#include <linux/ppp_channel.h>
#include <linux/ppp_defs.h>
#include <linux/if_ppp.h>
#include <linux/notifier.h>
#include <linux/file.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <net/sock.h>

#include <asm/uaccess.h>


#include <linux/ip.h>
#include <net/ip.h>
#include <net/route.h>
#include <net/dst.h>    /* for struct dst_entry & dst_metric() */
#include <net/ip.h>
#include <net/checksum.h>
#include <linux/ppp_comm.h>
#include "ppp_private.h"

/* define "portable" htons, etc. */
#define hton8(x)  (x)
#define ntoh8(x)  (x)
#define hton16(x) htons(x)
#define ntoh16(x) ntohs(x)
#define hton32(x) htonl(x)
#define ntoh32(x) ntohl(x)

#define NEED_SEQ        1
#define NO_SEQ          0
#define NEED_ACK        1
#define NO_ACK          0

#define INIT_SEQ        1   /* Base number of PPTP sequence */

static unsigned long call_id;       /* Call ID for local client */
static unsigned long peer_call_id;  /* Call ID for peer server */
static unsigned long seq_number = INIT_SEQ;
static unsigned long ack_number = 0;
static unsigned long src_ip_addr; /* 10.0.0.22 = 0x0A000016 */
static unsigned long dst_ip_addr; /* 10.0.0.238 = 0x0A0000EE */

static struct ppp_info ppp_ch;
static int ppp_ch_imported = 0;
static struct sock_info pptp_sock;


#define PPTP_HASH_BITS 4
#define PPTP_HASH_SIZE (1<<PPTP_HASH_BITS)

static struct ppp_channel_ops pptp_chan_ops;

static int pptp_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);
static int pptp_xmit(struct ppp_channel *chan, struct sk_buff *skb);
static int __pptp_xmit(struct sock *sk, struct sk_buff *skb);

static struct proto_ops pptp_ops;
//static DEFINE_RWLOCK(pptp_hash_lock);
static rwlock_t pptp_hash_lock = RW_LOCK_UNLOCKED;


static inline int cmp_2_addr(struct pptp_addr *a, struct pptp_addr *b)
{
	return (a->srcaddr == b->srcaddr &&
		(memcmp(a->remote, b->remote, ETH_ALEN) == 0));
}

static inline int cmp_addr(struct pptp_addr *a, unsigned long ipaddr, char *addr)
{
	return (a->srcaddr == ipaddr &&
		(memcmp(a->remote,addr,ETH_ALEN) == 0));
}

static int hash_item(unsigned long ipaddr, unsigned char *addr)
{
	char hash = 0;
	int i, j;

	for (i = 0; i < ETH_ALEN ; ++i) {
		for (j = 0; j < 8/PPTP_HASH_BITS ; ++j) {
			hash ^= addr[i] >> ( j * PPTP_HASH_BITS );
		}
	}

	for (i = 0; i < (sizeof(unsigned long)*8) / PPTP_HASH_BITS ; ++i)
		hash ^= ipaddr >> (i*PPTP_HASH_BITS);

	return hash & ( PPTP_HASH_SIZE - 1 );
}

/* zeroed because its in .bss */
static struct pppox_opt *item_hash_table[PPTP_HASH_SIZE];

/**********************************************************************
 *
 *  Set/get/delete/rehash items  (internal versions)
 *
 **********************************************************************/
static struct pppox_opt *__get_item(unsigned long ipaddr, unsigned char *addr)
{
	int hash = hash_item(ipaddr, addr);
    struct pppox_opt *ret;

	ret = item_hash_table[hash];

	while (ret && !cmp_addr(&ret->pptp_pa, ipaddr, addr))
		ret = ret->next;

	return ret;
}

static int __set_item(struct pppox_opt *po)
{
    int hash = hash_item(po->pptp_pa.srcaddr, po->pptp_pa.remote);
	struct pppox_opt *ret;

	ret = item_hash_table[hash];
	while (ret) {
		if (cmp_2_addr(&ret->pptp_pa, &po->pptp_pa))
			return -EALREADY;

		ret = ret->next;
	}

	if (!ret) {
		po->next = item_hash_table[hash];
		item_hash_table[hash] = po;
	}

	return 0;
}

static struct pppox_opt *__delete_item(unsigned long ipaddr, char *addr)
{
    int hash = hash_item(ipaddr, addr);
	struct pppox_opt *ret, **src;

	ret = item_hash_table[hash];
	src = &item_hash_table[hash];

	while (ret) {
        if (cmp_addr(&ret->pptp_pa, ipaddr, addr)) {
			*src = ret->next;
			break;
		}

		src = &ret->next;
		ret = ret->next;
	}

	return ret;
}

/**********************************************************************
 *
 *  Set/get/delete/rehash items
 *
 **********************************************************************/
static inline struct pppox_opt *get_item(unsigned long ipaddr,
					 unsigned char *addr)
{
	struct pppox_opt *po;

	read_lock_bh(&pptp_hash_lock);
    po = __get_item(ipaddr, addr);
	if (po)
		sock_hold(po->sk);
	read_unlock_bh(&pptp_hash_lock);

	return po;
}

static inline struct pppox_opt *get_item_by_addr(struct sockaddr_pptpox *sp)
{
    return get_item(sp->sa_addr.pptp.srcaddr, sp->sa_addr.pptp.remote);
}

static inline int set_item(struct pppox_opt *po)
{
	int i;

	if (!po)
		return -EINVAL;

	write_lock_bh(&pptp_hash_lock);
	i = __set_item(po);
	write_unlock_bh(&pptp_hash_lock);

	return i;
}

static inline struct pppox_opt *delete_item(unsigned long ipaddr, char *addr)
{
	struct pppox_opt *ret;

	write_lock_bh(&pptp_hash_lock);
    ret = __delete_item(ipaddr, addr);
	write_unlock_bh(&pptp_hash_lock);

	return ret;
}



/***************************************************************************
 *
 *  Handler for device events.
 *  Certain device events require that sockets be unconnected.
 *
 **************************************************************************/

static void pptp_flush_dev(struct net_device *dev)
{
	int hash;

	BUG_ON(dev == NULL);

	read_lock_bh(&pptp_hash_lock);
	for (hash = 0; hash < PPTP_HASH_SIZE; hash++) {
		struct pppox_opt *po = item_hash_table[hash];

		while (po != NULL) {
			if (po->pptp_dev == dev) {
				struct sock *sk = po->sk;

				sock_hold(sk);
				po->pptp_dev = NULL;

				/* We hold a reference to SK, now drop the
				 * hash table lock so that we may attempt
				 * to lock the socket (which can sleep).
				 */
				read_unlock_bh(&pptp_hash_lock);

				lock_sock(sk);

				if (sk->state &
				    (PPPOX_CONNECTED | PPPOX_BOUND)) {
					pppox_unbind_sock(sk);
					dev_put(dev);
					sk->state = PPPOX_ZOMBIE;
					sk->state_change(sk);
				}

				release_sock(sk);

				sock_put(sk);

				read_lock_bh(&pptp_hash_lock);

				/* Now restart from the beginning of this
				 * hash chain.  We always NULL out pptp_dev
				 * so we are guaranteed to make forward
				 * progress.
				 */
				po = item_hash_table[hash];
				continue;
			}
			po = po->next;
		}
	}
	read_unlock_bh(&pptp_hash_lock);
}

static int pptp_device_event(struct notifier_block *this,
			      unsigned long event, void *ptr)
{
	struct net_device *dev = (struct net_device *) ptr;

	/* Only look at sockets that are using this specific device. */
	switch (event) {
	case NETDEV_CHANGEMTU:
		/* A change in mtu is a bad thing, requiring
		 * LCP re-negotiation.
		 */

	case NETDEV_GOING_DOWN:
	case NETDEV_DOWN:
		/* Find every socket on this device and kill it. */
		pptp_flush_dev(dev);
		break;

	default:
		break;
	};

	return NOTIFY_DONE;
}


static struct notifier_block pptp_notifier = {
	notifier_call: pptp_device_event,
};


/************************************************************************
 *
 * Get the length of IP header to be stripped off.
 * receive function for internal use.
 *
 ***********************************************************************/
static int pptp_get_ip_header(char *pktdata)
{
    struct pptp_ip_hdr *piphdr = (struct pptp_ip_hdr *)pktdata;

    if (piphdr->protocol == IP_PROTOCOL_GRE)
        return (piphdr->ihl * 4);
    else
        return 0;
}

/************************************************************************
 *
 * Get the length of GRE header to be stripped off.
 * receive function for internal use.
 *
 ***********************************************************************/
static int pptp_get_gre_header(char *pktdata)
{
    struct pptp_gre_hdr *pgrehdr = (struct pptp_gre_hdr *)pktdata;
    int grehdrlen = 8;

    if (pgrehdr->protocol != GRE_PROTOCOL_PPTP)
        return 0;

    /* Check if sequence number presents */
    if (GRE_IS_S(pgrehdr->flags))
        grehdrlen += 4;

    /* Check if acknowledgement number presents */
    if (GRE_IS_A(pgrehdr->version))
        grehdrlen += 4;

    return grehdrlen;
}

/************************************************************************
 *
 * Padding the IP header before transmitting
 * xmit function for internal use.
 *
 ***********************************************************************/
static void pptp_fill_ip_header(struct sock *sk,
                struct sk_buff *skb,
                struct pptp_ip_hdr *iphdr,
                int total_len)
{
    struct rtable *rt = (struct rtable *)skb->dst;

    iphdr->version = 4; /* Version */
    iphdr->ihl = 5; /* Header length (5*4)=20 */
    iphdr->tos = sk->protinfo.af_inet.tos; /* Differentiated services field */
    //iphdr->tot_len = hton16(skb->len + grehdrlen + 20 + 2); /* Total length */
    iphdr->tot_len = hton16(total_len); /* Total length */
    /* Fragment flags & offset */
    //iphdr->frag_off = hton16(IP_DF); /* Don't fragment */
    iphdr->frag_off = 0;
    /* Select identification */
    ip_select_ident((struct iphdr *)iphdr, &rt->u.dst, sk);
    iphdr->ttl = 255; /* Time to live (64), maximum is 255 */
    iphdr->protocol = IP_PROTOCOL_GRE; /* Protocol: GRE(0x2f) */
    iphdr->saddr = src_ip_addr; /* Source IP address */
    iphdr->daddr = dst_ip_addr; /* Destination IP address */
    /* Header checksum */
    iphdr->check = 0;
    iphdr->check = ip_fast_csum((unsigned char *) iphdr, iphdr->ihl);
}

/************************************************************************
 *
 * Padding the GRE header before transmitting
 * xmit function for internal use.
 *
 ***********************************************************************/
static int pptp_fill_gre_header(unsigned long callid,
                struct pptp_gre_hdr *grehdr,
                int payloadlen, int need_seq, int need_ack)
{
    int hdrlen = 8; /* GRE header length */

    grehdr->flags = 0x00; /* bitfield */
    grehdr->flags |= GRE_FLAG_K; /* Key present */
    if (need_seq)
        grehdr->flags |= GRE_FLAG_S; /* Sequence number present */

    grehdr->version = GRE_VERSION_PPTP;
    if (need_ack)
        grehdr->version |= GRE_FLAG_A; /* Acknowledgement number present */

    grehdr->protocol = GRE_PROTOCOL_PPTP;
    /*
     * size of ppp payload, not inc. gre header;
     * besides the 0xFF03 was appened in front of the ppp packet.
     * So the total payload length should += 2.
     */
    //grehdr->payload_len = hton16(skb->len + 2);
    grehdr->payload_len = hton16(payloadlen);
    grehdr->call_id = hton16(callid); /* peer's call_id for this session */
    if (GRE_IS_S(grehdr->flags)) {
        grehdr->seq = hton32(seq_number); /* sequence number.  Present if S==1 */
        seq_number++;
        hdrlen += 4;
    }
    if (GRE_IS_A(grehdr->version)) {
        ack_number += 1;
        grehdr->ack = hton32(ack_number); /* seq number of highest packet recieved by */
        hdrlen += 4;
    }
    return hdrlen;
}

/************************************************************************
 *
 * Do the real work of receiving a PPPoE Session frame.
 *
 ***********************************************************************/
static int pptp_rcv_core(struct sock *sk, struct sk_buff *skb)
{
	struct pppox_opt *po = sk->protinfo.pppox;
	struct pppox_opt *relay_po = NULL;
	int hdrlen, iphdrlen, grehdrlen;

    iphdrlen = pptp_get_ip_header((char *)skb->nh.raw);
    grehdrlen = pptp_get_gre_header((char *)skb->nh.raw + iphdrlen);
    hdrlen = iphdrlen + grehdrlen + 2; /* 2 is length for 0xFF03 */

	if (sk->state & PPPOX_BOUND) {
#if 0
        struct pptp_ip_hdr *piphdr = (struct pptp_ip_hdr *) skb->nh.raw;
        struct pptp_gre_hdr *pgrehdr =
            (struct pptp_gre_hdr *) ((char *)skb->nh.raw + iphdrlen);
        int len = ntoh16(pgrehdr->payload_len - 2);
#endif

        skb_pull(skb, hdrlen);
#if 0
		skb_postpull_rcsum(skb, piphdr, hdrlen);

		if (pskb_trim_rcsum(skb, len))
			goto abort_kfree;
#endif

        if (!ppp_ch_imported) {
            ppp_ch = ppp_export_info();
    		if (ppp_ch.pchan != 0)
                ppp_ch_imported = 1;
        }

		if (ppp_ch.pchan != 0)
		    ppp_input(ppp_ch.pchan, skb);
	    else
	        goto abort_kfree;
	} else if (sk->state & PPPOX_RELAY) {
		relay_po = get_item_by_addr(&po->pptp_relay);

		if (relay_po == NULL)
			goto abort_kfree;

		if ((relay_po->sk->state & PPPOX_CONNECTED) == 0)
			goto abort_put;

		skb_pull(skb, hdrlen);
		if (!__pptp_xmit( relay_po->sk, skb))
			goto abort_put;
	} else {
		if (sock_queue_rcv_skb(sk, skb))
			goto abort_kfree;
	}

	return NET_RX_SUCCESS;

abort_put:
	sock_put(relay_po->sk);

abort_kfree:
	kfree_skb(skb);
	return NET_RX_DROP;
}

/************************************************************************
 *
 * Receive wrapper called in BH context.
 *
 ***********************************************************************/
static int pptp_rcv(struct sk_buff *skb,
		     struct net_device *dev,
		     struct packet_type *pt)

{
	struct pppox_opt *po;
	struct sock *sk;
	int ret;
	int iphdrlen, grehdrlen, hdrlen;
	struct pptp_ip_hdr *piphdr;
	struct pptp_gre_hdr *pgrehdr;
	//unsigned short *ppp_proto;

    iphdrlen = pptp_get_ip_header((char *)skb->nh.raw);
    grehdrlen = pptp_get_gre_header((char *)skb->nh.raw + iphdrlen);
    hdrlen = iphdrlen + grehdrlen + 2; /* 2 is length for 0xFF03 */

#if 0
	if (!pskb_may_pull(skb, hdrlen))
		goto drop;

	if (!(skb = skb_share_check(skb, GFP_ATOMIC)))
		goto out;
#endif

    piphdr = (struct pptp_ip_hdr *) skb->nh.raw;
    pgrehdr = (struct pptp_gre_hdr *)((char *)piphdr + iphdrlen);

    po = get_item(piphdr->daddr, skb->mac.ethernet->h_source);
	if (!po)
		goto drop;

	sk = po->sk;
	bh_lock_sock(sk);

	/* Socket state is unknown, must put skb into backlog. */
	if (sk->lock.users != 0) {
		sk_add_backlog(sk, skb);
		ret = NET_RX_SUCCESS;
	} else {
		ret = pptp_rcv_core(sk, skb);
	}

	bh_unlock_sock(sk);
	sock_put(sk);

	return ret;
drop:
	kfree_skb(skb);
#if 0
out:
#endif
	return NET_RX_DROP;
}


//Winster ETH_P_IP = 0x0800, include/linux/if_ether.h
//Winster ETH_P_PPTP_GRE = 0x082F, include/linux/if_ether.h
static struct packet_type pptp_gre_ptype = {
    type:   __constant_htons(ETH_P_PPTP_GRE),
    func:   pptp_rcv,
};

#if 0
static struct proto pptp_sk_proto = {
    .name       = "PPTP",
    .owner      = THIS_MODULE,
    .obj_size   = sizeof(struct pppox_sock),
};
#endif

/***********************************************************************
 *
 * Really kill the socket. (Called from sock_put if refcnt == 0.)
 *
 **********************************************************************/
void pptp_sock_destruct(struct sock *sk)
{
	if (sk->protinfo.destruct_hook)
		kfree(sk->protinfo.destruct_hook);
	MOD_DEC_USE_COUNT;
}

/***********************************************************************
 *
 * Initialize a new struct sock.
 *
 **********************************************************************/
static int pptp_create(struct socket *sock)
{
	int error = 0;
	struct sock *sk;

	MOD_INC_USE_COUNT;

	sk = sk_alloc(PF_PPPOX, GFP_KERNEL, 1);
	if (!sk)
		return -ENOMEM;

	sock_init_data(sock, sk);

    pptp_sock.sk = sk;
    pptp_sock.protocol_type = PPP_PROTOCOL_PPTP;
    ppp_import_sock_info(&pptp_sock);

	sock->state         = SS_UNCONNECTED;
	sock->ops           = &pptp_ops;

	sk->backlog_rcv     = pptp_rcv_core;
	sk->state           = PPPOX_NONE;
	sk->type            = SOCK_STREAM;
	sk->family          = PF_PPPOX;
	sk->protocol        = PX_PROTO_TP;
	sk->next            = NULL;
	sk->pprev           = NULL;
	sk->destruct        = pptp_sock_destruct;

	sk->protinfo.pppox = kmalloc(sizeof(struct pppox_opt), GFP_KERNEL);
	if (!sk->protinfo.pppox) {
		error = -ENOMEM;
		goto free_sk;
	}

	memset((void *) sk->protinfo.pppox, 0, sizeof(struct pppox_opt));
	sk->protinfo.pppox->sk = sk;

	/* Delete the protinfo when it is time to do so. */
	sk->protinfo.destruct_hook = sk->protinfo.pppox;
	sock->sk = sk;

	return 0;

free_sk:
	sk_free(sk);
	return error;
}

static int pptp_release(struct socket *sock)
{
	struct sock *sk = sock->sk;
	struct pppox_opt *po;
	int error = 0;

	if (!sk)
		return 0;

	if (sk->dead != 0)
		return -EBADF;

	pppox_unbind_sock(sk);

	/* Signal the death of the socket. */
	sk->state = PPPOX_DEAD;

	po = sk->protinfo.pppox;
    if (po->pptp_pa.srcaddr) {
		delete_item(po->pptp_pa.srcaddr, po->pptp_pa.remote);
	}

	if (po->pptp_dev)
		dev_put(po->pptp_dev);

	po->pptp_dev = NULL;

	sock_orphan(sk);
	sock->sk = NULL;

	skb_queue_purge(&sk->receive_queue);
	sock_put(sk);

    /* Need to re-import ppp_ch.pchan */
    ppp_ch_imported = 0;

	return error;
}


static int pptp_connect(struct socket *sock, struct sockaddr *uservaddr,
		  int sockaddr_len, int flags)
{
	struct sock *sk = sock->sk;
	struct net_device *dev = NULL;
	struct sockaddr_pptpox *sp = (struct sockaddr_pptpox *) uservaddr;
	struct pppox_opt *po = sk->protinfo.pppox;
	int error;

	lock_sock(sk);

	error = -EINVAL;
	if (sp->sa_protocol != PX_PROTO_TP)
		goto end;

	/* Check for already bound sockets */
	error = -EBUSY;
    if ((sk->state & PPPOX_CONNECTED) && sp->sa_addr.pptp.srcaddr)
		goto end;

	/* Check for already disconnected sockets, on attempts to disconnect */
	error = -EALREADY;
    if ((sk->state & PPPOX_DEAD) && !sp->sa_addr.pptp.srcaddr )
		goto end;

	error = 0;
    if (po->pptp_pa.srcaddr) {
		pppox_unbind_sock(sk);

		/* Delete the old binding */
        delete_item(po->pptp_pa.srcaddr,po->pptp_pa.remote);

		if(po->pptp_dev)
			dev_put(po->pptp_dev);

		memset(po, 0, sizeof(struct pppox_opt));
		po->sk = sk;

		sk->state = PPPOX_NONE;
	}

	/* Don't re-bind if sid==0 */
    if (sp->sa_addr.pptp.srcaddr != 0) {
        call_id = sp->sa_addr.pptp.cid;
        peer_call_id = sp->sa_addr.pptp.pcid;
		dev = dev_get_by_name(sp->sa_addr.pptp.dev);

		error = -ENODEV;
		if (!dev)
			goto end;

		po->pptp_dev = dev;

		if (!(dev->flags & IFF_UP))
			goto err_put;

		memcpy(&po->pptp_pa,
		       &sp->sa_addr.pptp,
		       sizeof(struct pptp_addr));

        src_ip_addr = sp->sa_addr.pptp.srcaddr;
        dst_ip_addr = sp->sa_addr.pptp.dstaddr;

		error = set_item(po);
		if (error < 0)
			goto err_put;

        /* struct (pptp_hdr) = struct (pptp_ip_hdr) + struct (pptp_gre_hdr) */
		po->chan.hdrlen = (sizeof(struct pptp_hdr) + 2 +
				   dev->hard_header_len); /* 0xFF03 len = 2 */

		po->chan.private = sk;
		po->chan.ops = &pptp_chan_ops;

		error = ppp_register_channel(&po->chan);
		if (error)
			goto err_put;

		sk->state = PPPOX_CONNECTED;
        dev_import_addr_info(&sp->sa_addr.pptp.srcaddr,
                             &sp->sa_addr.pptp.dstaddr);
	}

	sk->num = sp->sa_addr.pptp.srcaddr;

end:
	release_sock(sk);
	return error;
err_put:
	if (po->pptp_dev) {
		dev_put(po->pptp_dev);
		po->pptp_dev = NULL;
	}
	goto end;
}


static int pptp_getname(struct socket *sock, struct sockaddr *uaddr,
		  int *usockaddr_len, int peer)
{
	int len = sizeof(struct sockaddr_pptpox);
	struct sockaddr_pptpox sp;

	sp.sa_family	= AF_PPPOX;
	sp.sa_protocol	= PX_PROTO_TP;
	memcpy(&sp.sa_addr.pptp, &sock->sk->protinfo.pppox->pptp_pa,
	       sizeof(struct pptp_addr));

	memcpy(uaddr, &sp, len);

	*usockaddr_len = len;

	return 0;
}


static int pptp_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg)
{
	struct sock *sk = sock->sk;
	struct pppox_opt *po = sk->protinfo.pppox;
	int val = 0;
	int err = 0;

	switch (cmd) {
	case PPPIOCGMRU:
		err = -ENXIO;

		if (!(sk->state & PPPOX_CONNECTED))
			break;

		err = -EFAULT;
		if (put_user(po->pptp_dev->mtu -
			     (sizeof(struct pptp_hdr) + 2) -
			     PPP_HDRLEN,
			     (int *) arg))
			break;
		err = 0;
		break;

	case PPPIOCSMRU:
		err = -ENXIO;
		if (!(sk->state & PPPOX_CONNECTED))
			break;

		err = -EFAULT;
		if (get_user(val,(int *) arg))
			break;

		if (val < (po->pptp_dev->mtu
			   - (sizeof(struct pptp_hdr) + 2)
			   - PPP_HDRLEN))
			err = 0;
		else
			err = -EINVAL;
		break;

	case PPPIOCSFLAGS:
		err = -EFAULT;
		if (get_user(val, (int *) arg))
			break;
		err = 0;
		break;

	case PPTPIOCSFWD:
	{
		struct pppox_opt *relay_po;

		err = -EBUSY;
		if (sk->state & (PPPOX_BOUND | PPPOX_ZOMBIE | PPPOX_DEAD))
			break;

		err = -ENOTCONN;
		if (!(sk->state & PPPOX_CONNECTED))
			break;

		/* PPPoE address from the user specifies an outbound
		   PPPoE address to which frames are forwarded to */
		err = -EFAULT;
		if (copy_from_user(&po->pptp_relay,
				   (void *)arg,
				   sizeof(struct sockaddr_pptpox)))
			break;

		err = -EINVAL;
		if (po->pptp_relay.sa_family != AF_PPPOX ||
		    po->pptp_relay.sa_protocol!= PX_PROTO_TP)
			break;

		/* Check that the socket referenced by the address
		   actually exists. */
		relay_po = get_item_by_addr(&po->pptp_relay);

		if (!relay_po)
			break;

		sock_put(relay_po->sk);
		sk->state |= PPPOX_RELAY;
		err = 0;
		break;
	}

	case PPTPIOCDFWD:
		err = -EALREADY;
		if (!(sk->state & PPPOX_RELAY))
			break;

		sk->state &= ~PPPOX_RELAY;
		err = 0;
		break;

	case PPTPIOCGGRESEQ:
		err = -ENXIO;

		if (!(sk->state & PPPOX_CONNECTED))
			break;

		err = -EFAULT;
		if (put_user((unsigned long)seq_number,
			     (int *) arg))
			break;
		err = 0;
		seq_number++;
		break;

	default:;
	};

	return err;
}


//static int pptp_sendmsg(struct kiocb *iocb, struct socket *sock,
//		  struct msghdr *m, size_t total_len)
static int pptp_sendmsg(struct socket *sock, struct msghdr *m,
		  int total_len, struct scm_cookie *scm)
{
	struct sk_buff *skb = NULL;
	struct sock *sk = sock->sk;
	//struct pppox_sock *po = pppox_sk(sk);
	int error = 0;
	struct net_device *dev;
	char *start;
    int grehdrlen = 0;
    int hdrlen = sizeof(struct pptp_ip_hdr) + sizeof(struct pptp_gre_hdr) + 2;
    unsigned char tphdr[hdrlen]; /* Tunnel protocol header */
    unsigned char *ptphdr; /* Pointer for tunnel protocol header */

	if (sk->dead || !(sk->state & PPPOX_CONNECTED)) {
		error = -ENOTCONN;
		goto end;
	}

    /*
     * In this case, we set the IP header size to fixed size 20 bytes,
     * (without any optional field).
     */
    /* Fill in the GRE header data */
    grehdrlen = pptp_fill_gre_header(peer_call_id,
                    (struct pptp_gre_hdr *)&tphdr[20],
                    (total_len + 2), NEED_SEQ, NO_ACK);
    /*
     * IP header length = 20, 0xFF03 length = 2,
     * total header length = IP header + GRE header + 2
     */
    hdrlen = 20 + grehdrlen;
    tphdr[hdrlen++] = 0xFF; /* Address field of PPP header */
    tphdr[hdrlen++] = 0x03; /* Control field of PPP header */
    /* Fill in the IP header data */
    pptp_fill_ip_header(sk, skb, (struct pptp_ip_hdr *)&tphdr[0],
                    (total_len + hdrlen));

	lock_sock(sk);

	dev = sk->protinfo.pppox->pptp_dev;

	error = -EMSGSIZE;
 	if (total_len > (dev->mtu + dev->hard_header_len))
		goto end;


    skb = sock_wmalloc(sk, total_len + dev->hard_header_len + 32 + hdrlen,
			   0, GFP_KERNEL);
	if (!skb) {
		error = -ENOMEM;
		goto end;
	}

	/* Reserve space for headers. */
	skb_reserve(skb, dev->hard_header_len);
	skb->nh.raw = skb->data;

	skb->dev = dev;

	skb->priority = sk->priority;
	skb->protocol = __constant_htons(ETH_P_IP);

    /*
     * Push IP header, GRE header, and PPP address(0xFF) + Control(0x03)
     * in one shot.
     */
    //ptphdr = (unsigned char *) skb_push(skb, (hdrlen + total_len));
	ptphdr = (unsigned char *) skb_put(skb, total_len + hdrlen);

	start = (char *) ptphdr + hdrlen;

	error = memcpy_fromiovec(start, m->msg_iov, total_len);

	if (error < 0) {
		kfree_skb(skb);
		goto end;
	}

	error = total_len;
    dev->hard_header(skb, dev, ETH_P_IP,
			 sk->protinfo.pppox->pptp_pa.remote, NULL, total_len);

    memcpy(ptphdr, tphdr, hdrlen);

	dev_queue_xmit(skb);

end:
	release_sock(sk);
	return error;
}


/************************************************************************
 *
 * xmit function for internal use.
 *
 ***********************************************************************/
static int __pptp_xmit(struct sock *sk, struct sk_buff *skb)
{
	//struct pppox_sock *po = pppox_sk(sk);
	struct net_device *dev = sk->protinfo.pppox->pptp_dev;
	int headroom = skb_headroom(skb);
	struct sk_buff *skb2;
    int grehdrlen = 0;
    int hdrlen = sizeof(struct pptp_ip_hdr) + sizeof(struct pptp_gre_hdr) + 2;
    unsigned char tphdr[hdrlen]; /* Tunnel protocol header */
    unsigned char *ptphdr; /* Pointer for tunnel protocol header */

	if (sk->dead || !(sk->state & PPPOX_CONNECTED))
		goto abort;

    /*
     * In this case, we set the IP header size to fixed size 20 bytes,
     * (without any optional field).
     */
    /* Fill in the GRE header data */
    grehdrlen = pptp_fill_gre_header(peer_call_id,
                    (struct pptp_gre_hdr *)&tphdr[20],
                    (skb->len + 2), NEED_SEQ, NO_ACK);
    /*
     * IP header length = 20, 0xFF03 length = 2,
     * total header length = IP header + GRE header + 2
     */
    hdrlen = 20 + grehdrlen;
    tphdr[hdrlen++] = 0xFF; /* Address field of PPP header */
    tphdr[hdrlen++] = 0x03; /* Control field of PPP header */
    /* Fill in the IP header data */
    pptp_fill_ip_header(sk, skb, (struct pptp_ip_hdr *)&tphdr[0],
                    (skb->len + hdrlen));

	if (!dev)
		goto abort;

	/* Copy the skb if there is no space for the header. */
	/* Length + 2 is for PPP address 0xFF and Control 0x03 */
	if (headroom < (hdrlen + dev->hard_header_len)) {
		//skb2 = dev_alloc_skb(32+skb->len +
		skb2 = dev_alloc_skb(32 + skb->len + hdrlen +
				     dev->hard_header_len);

		if (skb2 == NULL)
			goto abort;

		skb_reserve(skb2, dev->hard_header_len + hdrlen);
		memcpy(skb_put(skb2, skb->len), skb->data, skb->len);
	} else {
		/* Make a clone so as to not disturb the original skb,
		 * give dev_queue_xmit something it can free.
		 */
		skb2 = skb_clone(skb, GFP_ATOMIC);
	}

    /*
     * Push IP header, GRE header, and PPP address(0xFF) + Control(0x03)
     * in one shot.
     */
    ptphdr = (unsigned char *) skb_push(skb2, hdrlen);
    memcpy(ptphdr, tphdr, hdrlen);
    skb2->protocol = __constant_htons(ETH_P_IP);

	skb2->nh.raw = skb2->data;

	skb2->dev = dev;

	dev->hard_header(skb2, dev, ETH_P_IP,
			 sk->protinfo.pppox->pptp_pa.remote, NULL, skb->len);

	/* We're transmitting skb2, and assuming that dev_queue_xmit
	 * will free it.  The generic ppp layer however, is expecting
	 * that we give back 'skb' (not 'skb2') in case of failure,
	 * but free it in case of success.
	 */

	if (dev_queue_xmit(skb2) < 0)
		goto abort;

	kfree_skb(skb);
	return 1;

abort:
	return 0;
}


/************************************************************************
 *
 * xmit function called by generic PPP driver
 * sends PPP frame over PPPoE socket
 *
 ***********************************************************************/
static int pptp_xmit(struct ppp_channel *chan, struct sk_buff *skb)
{
	struct sock *sk = (struct sock *) chan->private;
	return __pptp_xmit(sk, skb);
}

int pptp_xmit_wrap(struct sock *sk, struct sk_buff *skb)
{
	return __pptp_xmit(sk, skb);
}


#if 0
static struct ppp_channel_ops pptp_chan_ops = {
	.start_xmit = pptp_xmit,
};
#else
static struct ppp_channel_ops pptp_chan_ops = { pptp_xmit , NULL };
#endif

//static int pptp_recvmsg(struct kiocb *iocb, struct socket *sock,
//		  struct msghdr *m, size_t total_len, int flags)
static int pptp_recvmsg(struct socket *sock, struct msghdr *m,
        int total_len, int flags, struct scm_cookie *scm)
{
	struct sock *sk = sock->sk;
	struct sk_buff *skb = NULL;
	int error = 0;
	int len;
    struct pptp_ip_hdr *piphdr;
    struct pptp_gre_hdr *pgrehdr;
    int iphdrlen, grehdrlen;

	if (sk->state & PPPOX_BOUND) {
		error = -EIO;
		goto end;
	}

	skb = skb_recv_datagram(sk, flags & ~MSG_DONTWAIT,
				flags & MSG_DONTWAIT, &error);

	if (error < 0) {
		goto end;
	}

	m->msg_namelen = 0;

	if (skb) {
		error = 0;
        iphdrlen = pptp_get_ip_header((char *)skb->nh.raw);
        grehdrlen = pptp_get_gre_header((char *)skb->nh.raw + iphdrlen);
        piphdr = (struct pptp_ip_hdr *) skb->nh.raw;
        pgrehdr = (struct pptp_gre_hdr *) ((char *)skb->nh.raw + iphdrlen);
        len = ntoh16(pgrehdr->payload_len - 2);

		error = memcpy_toiovec(m->msg_iov,
		            ((unsigned char *) pgrehdr + grehdrlen + 2), len);
		if (error < 0)
			goto do_skb_free;
		error = len;
	}

do_skb_free:
	if (skb)
		kfree_skb(skb);
end:
	return error;
}

#if 0

#ifdef CONFIG_PROC_FS
static int pptp_seq_show(struct seq_file *seq, void *v)
{
	struct pppox_sock *po;
	char *dev_name;

	if (v == SEQ_START_TOKEN) {
		seq_puts(seq, "Id       Address              Device\n");
		goto out;
	}

	po = v;
	dev_name = po->pptp_pa.dev;

	seq_printf(seq, "%08X %02X:%02X:%02X:%02X:%02X:%02X %s\n",
           po->pptp_pa.srcaddr,
		   po->pptp_pa.remote[0], po->pptp_pa.remote[1],
		   po->pptp_pa.remote[2], po->pptp_pa.remote[3],
		   po->pptp_pa.remote[4], po->pptp_pa.remote[5], dev_name);
out:
	return 0;
}

static __inline__ struct pppox_sock *pptp_get_idx(loff_t pos)
{
	struct pppox_sock *po = NULL;
	int i = 0;

	for (; i < PPTP_HASH_SIZE; i++) {
		po = item_hash_table[i];
		while (po) {
			if (!pos--)
				goto out;
			po = po->next;
		}
	}
out:
	return po;
}

static void *pptp_seq_start(struct seq_file *seq, loff_t *pos)
{
	loff_t l = *pos;

	read_lock_bh(&pptp_hash_lock);
	return l ? pptp_get_idx(--l) : SEQ_START_TOKEN;
}

static void *pptp_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	struct pppox_sock *po;

	++*pos;
	if (v == SEQ_START_TOKEN) {
		po = pptp_get_idx(0);
		goto out;
	}
	po = v;
	if (po->next)
		po = po->next;
	else {
		int hash = hash_item(po->pptp_pa.srcaddr, po->pptp_pa.remote);

		while (++hash < PPTP_HASH_SIZE) {
			po = item_hash_table[hash];
			if (po)
				break;
		}
	}
out:
	return po;
}

static void pptp_seq_stop(struct seq_file *seq, void *v)
{
	read_unlock_bh(&pptp_hash_lock);
}

static struct seq_operations pptp_seq_ops = {
    .start      = pptp_seq_start,
    .next       = pptp_seq_next,
    .stop       = pptp_seq_stop,
    .show       = pptp_seq_show,
};

static int pptp_seq_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &pptp_seq_ops);
}

static struct file_operations pptp_seq_fops = {
    .owner      = THIS_MODULE,
    .open       = pptp_seq_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = seq_release,
};

static int __init pptp_proc_init(void)
{
	struct proc_dir_entry *p;

	p = create_proc_entry("pptp", S_IRUGO, proc_net);
	if (!p)
		return -ENOMEM;

	p->proc_fops = &pptp_seq_fops;
	return 0;
}
#else /* CONFIG_PROC_FS */
static inline int pptp_proc_init(void) { return 0; }
#endif /* CONFIG_PROC_FS */

#else
int pptp_proc_info(char *buffer, char **start, off_t offset, int length)
{
	struct pppox_opt *po;
	int len = 0;
	off_t pos = 0;
	off_t begin = 0;
	int size;
	int i;

	len += sprintf(buffer,
		       "Id       Address              Device\n");
	pos = len;

	write_lock_bh(&pptp_hash_lock);

	for (i = 0; i < PPTP_HASH_SIZE; i++) {
		po = item_hash_table[i];
		while (po) {
			char *dev = po->pptp_pa.dev;

			size = sprintf(buffer + len,
				       "%08X %02X:%02X:%02X:%02X:%02X:%02X %8s\n",
				       (unsigned int)po->pptp_pa.srcaddr,
				       po->pptp_pa.remote[0],
				       po->pptp_pa.remote[1],
				       po->pptp_pa.remote[2],
				       po->pptp_pa.remote[3],
				       po->pptp_pa.remote[4],
				       po->pptp_pa.remote[5],
				       dev);
			len += size;
			pos += size;
			if (pos < offset) {
				len = 0;
				begin = pos;
			}

			if (pos > offset + length)
				break;

			po = po->next;
		}

		if (po)
			break;
  	}
	write_unlock_bh(&pptp_hash_lock);

  	*start = buffer + (offset - begin);
  	len -= (offset - begin);
  	if (len > length)
  		len = length;
	if (len < 0)
		len = 0;
  	return len;
}
#endif

/* ->ioctl are set at pppox_create */

static struct proto_ops pptp_ops = {
    family:         AF_PPPOX,
    //owner:          THIS_MODULE,
    release:        pptp_release,
    bind:		    sock_no_bind,
    connect:        pptp_connect,
    socketpair:     sock_no_socketpair,
    accept:         sock_no_accept,
    getname:        pptp_getname,
    poll:           datagram_poll,
    ioctl:          pptp_ioctl,
    listen:         sock_no_listen,
    shutdown:       sock_no_shutdown,
    setsockopt:     sock_no_setsockopt,
    getsockopt:     sock_no_getsockopt,
    sendmsg:        pptp_sendmsg,
    recvmsg:        pptp_recvmsg,
    mmap:           sock_no_mmap
};

static struct pppox_proto pptp_proto = {
    create: pptp_create,
    ioctl:  pptp_ioctl,
    //owner:  THIS_MODULE,
};


static int __init pptp_init(void)
{
#if 0
	int err = proto_register(&pptp_sk_proto, 0);

	if (err)
		goto out;

 	err = register_pppox_proto(PX_PROTO_TP, &pptp_proto);
	if (err)
		goto out_unregister_pptp_proto;

	err = pptp_proc_init();
	if (err)
		goto out_unregister_pppox_proto;

	dev_add_pack(&pptp_gre_ptype);
	register_netdevice_notifier(&pptp_notifier);

    /* Init sequence & acknowledgement values */
    seq_number = INIT_SEQ;

    /* Clear flags */
    ppp_ch_imported = 0;

out:
	return err;
out_unregister_pppox_proto:
	unregister_pppox_proto(PX_PROTO_TP);
out_unregister_pptp_proto:
	proto_unregister(&pptp_sk_proto);
	goto out;

#else

 	int err = register_pppox_proto(PX_PROTO_TP, &pptp_proto);

	if (err == 0) {
		dev_add_pack(&pptp_gre_ptype);
		register_netdevice_notifier(&pptp_notifier);
		proc_net_create("pptp", 0, pptp_proc_info);
	}
	return err;
#endif
}

static void __exit pptp_exit(void)
{
	unregister_pppox_proto(PX_PROTO_TP);
	dev_remove_pack(&pptp_gre_ptype);
	unregister_netdevice_notifier(&pptp_notifier);
#if 0
	remove_proc_entry("pptp", proc_net);
	proto_unregister(&pptp_sk_proto);
#else
	proc_net_remove("pptp");
#endif
}

module_init(pptp_init);
module_exit(pptp_exit);

//EXPORT_SYMBOL(pptp_xmit_wrap);

MODULE_AUTHOR("Winster Chan <winster.wh.chan@foxconn>");
MODULE_DESCRIPTION("PPP tunnel protocol driver");
MODULE_LICENSE("GPL");
//MODULE_ALIAS_NETPROTO(PF_PPPOX);
