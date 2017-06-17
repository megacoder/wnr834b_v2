/* --------------------------------------------------------------------------------------------------------------
 * FILE NAME       l7_filter_main.h (for Linux Platform)
 * DATE            10/25/2007
 * AUTHOR/S        Max Ding
 * Description     Layer 7 filter
 * --------------------------------------------------------------------------------------------------------------
 */
 
#ifndef __LAYER7_FILTER_MAIN_H
#define __LAYER7_FILTER_MAIN_H

//#define L7_DEBUG_ON

/* enum of these protocol*/
#define L7_ENUM_INIT            0xff
#define L7_ENUM_BITTORRENT      1
#define L7_ENUM_FASTTRACK       2
#define L7_ENUM_EDONKEY         3
#define L7_ENUM_GNUTELLA        4
#define L7_ENUM_SKYPETOSKYPE    5
#define L7_ENUM_SKYPEOUT        6
#define L7_ENUM_TOTAL           L7_ENUM_SKYPEOUT    /* make this equal the to the last app */

#define APP_DATA_BUF_MAX_LEN    8192

#define L7_SUCCESS 0
#define L7_ERROR 1

/* for connection direction */
#define AG_INBOUND   0
#define AG_OUTBOUND  1

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef INOUT
#define INOUT
#endif

#ifndef BYTE
#define BYTE        unsigned char
#endif

#ifndef WORD
#define WORD        unsigned short  /* 2-byte */
#endif

#ifndef DWORD
#define DWORD       unsigned long    /* 4-byte */
#endif

#ifndef BOOL
#define BOOL        int
#endif

#ifndef TRUE
#define TRUE        1
#endif

#ifndef FALSE
#define FALSE       0
#endif

#ifndef UINT16
#define UINT16      unsigned short  /* 2-byte */
#endif

#ifndef UINT32
#define UINT32      unsigned int    /* 4-byte */
#endif

#ifndef RT_HANDLE
#define RT_HANDLE void*
#endif

typedef struct _proto_regexp
{
    char * const proto_name;
    //unsigned char proto_pri;
    unsigned char proto_enum;
    int proto_packet_count;
    char *proto_regexp;
} s_proto_regexp;

typedef struct _pattern_cache {
    char * regex_string;
    regexp * pattern;
    struct pattern_cache * next;
    //unsigned char proto_pri;
    unsigned char proto_enum;
    char *proto_name;
} s_pattern_cache;


/*Attention: The following code should sync with agconntbl.h and agUserApi.h*/
typedef struct S_ConnHashEntry
{
    unsigned short iInNextConnId;
    unsigned short iOutNextConnId;
    BYTE           bEntryReady;    
} T_ConnHashEntry;

typedef struct S_ConnEntry
{
    T_ConnHashEntry tHash;
    short iConnId;
    short iAlg;
#define wServerEspSpiHalf2 iAlgSession    //Ambit add, Peter Chen, 10/28/2004
    short iAlgSession;
    UINT32 dwSourceIp;
    UINT32 dwDestIp;
    UINT32 dwModifiedIp;
    UINT32 dwTimestamp;
#define wClientEspSpiHalf1 wSourcePort  //Ambit add, Peter Chen, 10/28/2004
#define wIcmpIdentity wSourcePort
    UINT16 wSourcePort; /*use to save id field if packet type is icmp, network order*/
#define wClientEspSpiHalf2 wDestPort    //Ambit add, Peter Chen, 10/28/2004
#define wIcmpSequence wDestPort
    UINT16 wDestPort;  /*use to save seq field if packet type is icmp, network order*/
#define wServerEspSpiHalf1 wModifiedPort  //Ambit add, Peter Chen, 10/28/2004
#define wIcmpModifiedSequence wModifiedPort
    UINT16 wModifiedPort; /*use to save modified seq field if packet type is icmp, network order*/
    BYTE bProtocol; /* same with ip->proto */
    BYTE bConnState:4,
         bNatType:4; /*Basic NAT, NAPT, Port Forwarding */

	BYTE bSelf;
	UINT32 dwTimeOut;

#ifdef __VXWORKS__
    struct rtentry  *pOut_rtentry;
    struct rtentry  *pIn_rtentry;
#elif __LINUX__
    RT_HANDLE pOut_rtentry;
    RT_HANDLE pIn_rtentry;
#endif


   
#ifdef INCLUDE_TMSS
    #define URL_DENY    0
    #define URL_ALLOW   1
    BYTE bIsDeny;
    char tmssURLHostString[256];
    char tmssURLPathString[512];
#endif
    
#ifdef INCLUDE_L7_FILTER
    BYTE packet_count;
    BYTE proto_enum;
#endif
    

} T_ConnEntry;

#endif /*__LAYER7_FILTER_MAIN_H*/
