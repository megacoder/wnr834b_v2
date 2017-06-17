/*******************************************************************************
 * Copyright 2007  Hon Hai Precision Ind. Co. Ltd.
 * All Rights Reserved.
 * No portions of this material shall be reproduced in any form without the
 * written permission of Hon Hai Precision Ind. Co. Ltd.
 *
 * All information contained in this document is Hon Hai Precision Ind.
 * Co. Ltd. company private, proprietary, and trade secret property and
 * are protected by international intellectual property laws and treaties.
 *
 ******************************************************************************/

/******************************************************************************
***    Filename: et_qos.c
***    Description:
***        1. purpose
***        2. Side effect (optional)
***
***    History:
***
*** Modify Reason       Author      Date          Search Flag(Option)
*** ----------------------------------------------------------------------------
*** File Creation       pling       01/31/2007 
********************************************************************************/

#ifdef INCLUDE_QOS

#undef _DEBUG_QOS_INPUT
#undef _DEBUG_QOS_OUTPUT
#undef DEBUG

#include <net/sock.h>
#include "bcmnvram.h"

#define ETHER_TYPE_BCM_TAG      0x8874
#define BCM_TAG_SIZE            6
#define CRC_SIZE                4
#define MIN_PACKET_SIZE         64

#define QOS_PRIORITY_HIGHEST    3
#define QOS_PRIORITY_HIGH       2
#define QOS_PRIORITY_NORMAL     1
#define QOS_PRIORITY_LOW        0
#define QOS_PRIORITY_UNSPECIFIED -1

#define C_MAX_QOS_RULES         32

static int qos_enable = 0;
static char qos_port_priority[4];
static int qos_do_add_strip_tag = 0;

extern int dma_qos_enable;      /* flag to tell DMA when QoS is enabled */

typedef struct T_qos_mac_priority
{
    unsigned char mac[6];
    char priority;
} s_qos_mac_priority;

static s_qos_mac_priority qos_mac_priority[C_MAX_QOS_RULES];
static int qos_mac_count = 0;

static int et_qos_init(void)
{
    char qos_port[512];
    char *temp;
    int port_num, port_priority;

    char qos_mac[1024];
    unsigned int mac[6], mac_priority;
    int i;

    static int first_time = 1;

    /* Init only once */
    if (!first_time)
        return 0;

    first_time = 0;

    /* Enable / disable QoS */
    if (nvram_match("qos_enable", "1") && 
        !nvram_match("wla_repeater", "1"))
    {
        qos_enable = 1;
    }
    else
    {
        qos_enable = 0;
    }

    /* Set port priority */
    for (i=0; i<sizeof(qos_port_priority); i++)
        qos_port_priority[i] = QOS_PRIORITY_UNSPECIFIED;

    strcpy(qos_port, nvram_safe_get("qos_port"));

    if (strlen(qos_port) == 0)
        qos_do_add_strip_tag = 0;
    else
        qos_do_add_strip_tag = 1;

    /* Foxconn added start pling 01/30/2008 */
    if (qos_enable && qos_do_add_strip_tag)
        dma_qos_enable = 1;     /* tell DMA engine to shift bytes */
    else
        dma_qos_enable = 0;     /* tell DMA engine not to shift bytes */
    /* Foxconn added end pling 01/30/2008 */
    
    temp = strtok(qos_port, " ");
    while (temp)
    {
        sscanf(temp, "%d:%d", &port_num, &port_priority);
#ifdef DEBUG
        printk("port %d, priority %d\n", port_num, port_priority);
#endif

        /* Do Sanity check */
        if (port_num >= 0 && port_num <= 3 &&
            port_priority >= QOS_PRIORITY_LOW && port_priority <= QOS_PRIORITY_HIGHEST)
            qos_port_priority[port_num] = port_priority;

        temp = strtok(NULL, " ");
    }

    /* Set MAC address priority */
    memset(qos_mac_priority, 0, sizeof(qos_mac_priority));
    qos_mac_count = 0;

    strcpy(qos_mac, nvram_safe_get("qos_mac"));
    temp = strtok(qos_mac, " ");
    while (temp)
    {
        sscanf(temp, "%d-%d-%d-%d-%d-%d:%d", 
                &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5], &mac_priority);

        /* Do Sanity check */
        if (mac_priority >= QOS_PRIORITY_LOW && mac_priority <= QOS_PRIORITY_HIGHEST)
        {
            for (i=0; i<6; i++)
                qos_mac_priority[qos_mac_count].mac[i] = (unsigned char)(mac[i] & 0xFF);
            qos_mac_priority[qos_mac_count++].priority = (char)mac_priority;

#ifdef DEBUG
            printk("mac %02X:%02X:%02X:%02X:%02X:%02X, priority %d\n",
                    qos_mac_priority[qos_mac_count-1].mac[0],
                    qos_mac_priority[qos_mac_count-1].mac[1],
                    qos_mac_priority[qos_mac_count-1].mac[2],
                    qos_mac_priority[qos_mac_count-1].mac[3],
                    qos_mac_priority[qos_mac_count-1].mac[4],
                    qos_mac_priority[qos_mac_count-1].mac[5],
                    mac_priority);
#endif
        }

        temp = strtok(NULL, " ");
    }

    return 0;
}

static int et_rx_strip_bcm_tag(et_info_t *et, struct sk_buff *skb)
{
    unsigned long *pMacHdr;
    unsigned long hdr[3];
    struct ethhdr *eth_hdr;
    int port, port_priority = QOS_PRIORITY_UNSPECIFIED;
    char src_mac[6];
    int mac_priority = QOS_PRIORITY_UNSPECIFIED;
    int i;

    eth_hdr = (struct ethhdr *)&(skb->data[0]);

    /* 1. Check Src MAC and assign priority */
    memcpy(src_mac, (char*)(eth_hdr->h_source), 6);

    for (i=0; i<qos_mac_count; i++)
    {
        if (memcmp(src_mac, qos_mac_priority[i].mac, 6) == 0)
        {
            mac_priority = qos_mac_priority[i].priority;
#ifdef DEBUG
            printk("QoS: rx mac %02x:%02x:%02x:%02x:%02x:%02x pkt, p%d\n", 
                    (unsigned char)src_mac[0],
                    (unsigned char)src_mac[1],
                    (unsigned char)src_mac[2],
                    (unsigned char)src_mac[3],
                    (unsigned char)src_mac[4],
                    (unsigned char)src_mac[5],
                    mac_priority);
#endif
            break;
        }
    }

    /* 2. Check whether BCM tag is added */
    if (ntohs(eth_hdr->h_proto) == ETHER_TYPE_BCM_TAG)
    {
#ifdef _DEBUG_QOS_INPUT
        printk("Before strip:  ");
        for (i=0; i<skb->len; i++)
            printk("%02X ", skb->data[i]);
        printk("\n");
#endif

        /* Store the src and dst MAC address first */
        pMacHdr = (unsigned long *)&(skb->data[0]);
        hdr[0] = pMacHdr[0];
        hdr[1] = pMacHdr[1];
        hdr[2] = pMacHdr[2];

        /* Check port number and assign priority */
        port = skb->data[17] & 0x0F;
        if (port >=0 && port <= 3)
        {
            port_priority = qos_port_priority[port];
#ifdef DEBUG
            printk("QoS: rx port#%d, p%d\n", port, port_priority);
#endif
        }

        /* Remove Broadcom tag */
        skb_pull(skb, BCM_TAG_SIZE);

        pMacHdr = (unsigned long *)&(skb->data[0]);
        pMacHdr[0] = hdr[0];
        pMacHdr[1] = hdr[1];
        pMacHdr[2] = hdr[2];

        /* Remove trailing CRC */
        skb_trim(skb, skb->len - CRC_SIZE);

#ifdef _DEBUG_QOS_INPUT
        printk("AFTER strip: ");
        for (i=0; i<skb->len; i++)
            printk("%02X ", skb->data[i]);
        printk("\n");
#endif        
    }
        
    /* Set priority to skb */
    skb->cb[sizeof(skb->cb)-3] = port_priority > mac_priority ? port_priority : mac_priority;

    /* Mark a bit in skb to indicate this is rx from LAN ethernet port */
    skb->cb[sizeof(skb->cb)-4] = 0xaa;
        
#ifdef DEBUG
    printk("QoS: pkt pri = %d\n", skb->cb[sizeof(skb->cb)-3]);
#endif

    return 0;
}

static struct sk_buff * et_tx_add_bcm_tag(et_info_t *et, struct sk_buff **pskb)
{
    struct sk_buff *skb = *pskb;
    struct sk_buff *new_skb;
    int i;
    uint32 *old_hdr, *new_hdr;
    uint16 *bcm_tag;
    uint32 *bcm_tag_value;
    int new_tailroom = 0;
    char *crc;

#ifdef _DEBUG_QOS_OUTPUT
    printk("--- Before Insert : ");
    for (i=0; i<skb->len; i++)
        printk("%02X ", skb->data[i]);
    printk("\n");
#endif

    if (skb->len < MIN_PACKET_SIZE)
        new_tailroom = MIN_PACKET_SIZE - skb->len;

    if (new_tailroom < CRC_SIZE)
        new_tailroom = CRC_SIZE;

    /* Reallocate skbuff if we are short in headroom or tailroom */
    if (skb_headroom(skb) < BCM_TAG_SIZE || skb_tailroom(skb) < new_tailroom)
    {
        new_skb = skb_copy_expand(skb, BCM_TAG_SIZE, new_tailroom, GFP_ATOMIC);
        if (!new_skb)
        {
            printk("No room for BRCM tag\n");
            return skb;
        }
        kfree_skb(skb);
        skb = new_skb;
    }

    old_hdr = (uint32 *)&(skb->data[0]);
    new_hdr = (uint32 *)skb_push(skb, BCM_TAG_SIZE);
            
    /* Move the dest and src mac to new location */
    for (i=0; i<3; i++)
        new_hdr[i] = old_hdr[i];

    /* Add Broadcom tag */
    bcm_tag  = (uint16 *)&new_hdr[3];
    *bcm_tag = htons(ETHER_TYPE_BCM_TAG);
    bcm_tag_value  = (uint32 *)&bcm_tag[1];
    *bcm_tag_value = htonl(5L);

    crc = (char *)skb_put(skb, new_tailroom);

#ifdef _DEBUG_QOS_OUTPUT
    printk("--- AFTER Insert (%d): ", skb->len);
    for (i=0; i<skb->len; i++)
        printk("%02X ", skb->data[i]);
    printk("\n");
#endif
        
    return skb;
}

#endif      /* INCLUDE_QOS */
