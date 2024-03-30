#ifndef __AGM_LWIP_H__
#define __AGM_LWIP_H__

#include "alta.h"

#include "lwip/init.h"
#include "lwip/arch.h"
#include "lwip/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/snmp.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/lwiperf.h"

#undef AGM_DEBUG
#define AGM_DEBUG LWIP_DBG_ON

extern struct netif netif;
extern struct pbuf *rx_queue_agm[];

bool queue_try_put_agm(struct pbuf **q, struct pbuf *p);
struct pbuf *queue_try_get_agm(struct pbuf **q);
void mac_init_agm(void);
err_t netif_init_func_agm(struct netif *netif);
void netif_status_callback_agm(struct netif *netif);

sys_prot_t sys_arch_protect_agm(void);
void sys_arch_unprotect_agm(sys_prot_t pval);
u32_t sys_now_agm(void);

void MAC0_isr_agm(void);

#endif
