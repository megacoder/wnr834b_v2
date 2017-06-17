
//#include <linux/pptp_test.h>

/*
 * This definition must refer to linux/if_ppp.h
 * Currently, the definition rage is from 90 downto 55
 */
#define PPTPIOCGGRESEQ  _IOR('t', 54, unsigned long)	/* get GRE sequence number */

extern int pptp_xmit_wrap(struct sock *sk, struct sk_buff *skb);
extern int pppoe_xmit_wrap(struct sock *sk, struct sk_buff *skb);
extern void ppp_import_sock_info(struct sock_info *pSockInfo);
