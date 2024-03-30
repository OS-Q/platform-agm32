#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

#ifndef __AGM_LWIPOPTS_H__
#define __AGM_LWIPOPTS_H__

#define NO_SYS        1
#define MEM_ALIGNMENT 4

#define LWIP_RAW      1
#define LWIP_NETCONN  0
#define LWIP_SOCKET   0
#define LWIP_DHCP     0
#define LWIP_ICMP     1
#define LWIP_UDP      1
#define LWIP_TCP      1
#define LWIP_IPV4     1

#define LWIP_SINGLE_NETIF          1
#define LWIP_NETIF_STATUS_CALLBACK 1
#define LWIP_CALLBACK_API          1
#define LWIP_PROVIDE_ERRNO         1
#define LWIP_TIMEVAL_PRIVATE       0

#define LWIP_CHKSUM_ALGORITHM 3 // Faster than algorithm 2
#define TCP_MSS (1500 /*mtu*/ - 20 /*ip header*/ - 20 /*tcp header*/)
#define TCP_WND (6 * TCP_MSS)
#define TCP_SND_BUF (TCP_WND)
#define MEMP_NUM_TCP_SEG  ((4 * (TCP_SND_BUF) + (TCP_MSS - 1))/(TCP_MSS)) // Same as TCP_SND_QUEUELEN

#undef LWIP_DEBUG
#define LWIP_DEBUG 1

#if LWIP_DEBUG
// #define TCP_OUTPUT_DEBUG LWIP_DBG_ON
// #define IP_DEBUG   LWIP_DBG_ON
// #define TCP_DEBUG  LWIP_DBG_ON
// #define TCP_INPUT_DEBUG LWIP_DBG_ON
// #define TCP_OUTPUT_DEBUG LWIP_DBG_ON
// #define UDP_DEBUG  LWIP_DBG_ON
// #define DHCP_DEBUG LWIP_DBG_ON
// #define AGM_DEBUG  LWIP_DBG_ON
// #define MEM_DEBUG  LWIP_DBG_ON
// #define PBUF_DEBUG LWIP_DBG_ON
#define LWIP_DBG_TYPES_ON (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE|LWIP_DBG_FRESH|LWIP_DBG_HALT)
#endif

#endif

#endif
