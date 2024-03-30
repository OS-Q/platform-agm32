#include "board.h"
#include "agm_lwip.h"

#include "apps/tcpecho_raw/tcpecho_raw.h"
#include "examples/lwiperf/lwiperf_example.h"

sys_prot_t sys_arch_protect()
{
  return sys_arch_protect_agm();
}

void sys_arch_unprotect(sys_prot_t pval)
{
  sys_arch_unprotect_agm(pval);
}

u32_t sys_now()
{
  return sys_now_agm();
}

void MAC0_isr()
{
  MAC0_isr_agm();
}

int main(void)
{
  board_init();
  lwip_init();
  mac_init_agm();

  const ip_addr_t ip = IPADDR4_INIT_BYTES(192, 168, 5, 1);
  netif_add(&netif, &ip, IP4_ADDR_ANY, IP4_ADDR_ANY, NULL, netif_init_func_agm, netif_input);
  netif.name[0] = 'e';
  netif.name[1] = 'h';
  netif_set_status_callback(&netif, netif_status_callback_agm);
  netif_set_default(&netif);
  netif_set_up(&netif);
  
  netif_set_link_up(&netif);
  tcpecho_raw_init();
  httpd_init();
  lwiperf_example_init();

  while(1) {
    sys_arch_protect();
    struct pbuf* p = queue_try_get_agm(rx_queue_agm);
    sys_arch_unprotect(PLIC_MAX_PRIORITY);

    if(p != NULL) {
      if(netif.input(p, &netif) != ERR_OK) {
        pbuf_free(p);
      }
    }
     
    sys_check_timeouts();
  }

  return 0;
}
