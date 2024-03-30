#include "board.h"
#include "lwipopts.h"
#include "agm_lwip.h"

struct netif netif;
MAC_HandleTypeDef hmac;
const uint8_t mac_address[6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x20 };

#define TX_NUM_DESC 2
#define RX_NUM_DESC 6
MAC_DescriptorTypeDef TxDesc[TX_NUM_DESC] __attribute__((aligned(MAC_GET_DESC_ALIGN(TX_NUM_DESC))));
MAC_DescriptorTypeDef RxDesc[RX_NUM_DESC] __attribute__((aligned(MAC_GET_DESC_ALIGN(RX_NUM_DESC))));

#define MAX_PACKET_SIZE 1516 // 1500 + 14, aligned to word boundary
uint8_t tx_buf[TX_NUM_DESC][MAX_PACKET_SIZE] __attribute__((section("noinit")));
uint8_t rx_buf[RX_NUM_DESC][MAX_PACKET_SIZE] __attribute__((section("noinit")));

int rx_queue_front;
int rx_queue_back;
int rx_queue_size;
int rx_queue_max;

struct pbuf *rx_queue_agm[RX_NUM_DESC];

bool queue_try_put_agm(struct pbuf **q, struct pbuf *p)
{
  if (rx_queue_size < RX_NUM_DESC) {
    q[rx_queue_front] = p;
    ++rx_queue_size;
    if (++rx_queue_front >= RX_NUM_DESC) {
      rx_queue_front = 0;
    }
    if (rx_queue_size > rx_queue_max) {
      rx_queue_max = rx_queue_size;
      LWIP_DEBUGF(AGM_DEBUG | LWIP_DBG_TRACE, ("rx_queue max: %d\n", rx_queue_max));
    }
    return true;
  } else {
    return false;
  }
}

struct pbuf *queue_try_get_agm(struct pbuf **q)
{
  struct pbuf *p = NULL;
  if (rx_queue_size > 0) {
    --rx_queue_size;
    p = q[rx_queue_back];
    if (++rx_queue_back >= RX_NUM_DESC) {
      rx_queue_back = 0;
    }
  }
  return p;
}

static
void mac_rx_prepare(void)
{
  // Prepare all rx descriptors for receiving
  while (!MAC_IsDescEnabled(&hmac.RxDesc[hmac.RxPointer])) {
    MAC_Receive(&hmac, rx_buf[hmac.RxPointer]);
  }
}

void mac_init_agm(void)
{
  if (board_mac_media() == MAC_MEDIA_INTERFACE_MII) {
    PERIPHERAL_MAC_ENABLE_MII;
  } else {
    PERIPHERAL_MAC_ENABLE_RMII;
  }
  MAC_HandleInit(&hmac, TxDesc, TX_NUM_DESC, RxDesc, RX_NUM_DESC);
  memcpy(hmac.Init.MacAddress, mac_address, 6);
  hmac.Init.PhyAddress      = board_mac_phy_addr();
  hmac.Init.MediaInterface  = board_mac_media();

  while (MAC_Init(&hmac) != RET_OK) {
    LWIP_DEBUGF(AGM_DEBUG | LWIP_DBG_TRACE, ("Mac init failed\n"));
  }
  board_init_phy(&hmac);

  mac_rx_prepare();

  INT_SetIRQThreshold(1);
  INT_EnableIRQ(MAC0_IRQn, PLIC_MAX_PRIORITY);
  MAC_EnableIntRx(hmac.Instance);
  MAC_EnableIntPhy(hmac.Instance);

  LWIP_DEBUGF(AGM_DEBUG | LWIP_DBG_TRACE, ("Mac init done\n"));
}

static
err_t netif_output(struct netif *netif, struct pbuf *p)
{
  sys_arch_protect();
  pbuf_copy_partial(p, tx_buf[hmac.TxPointer], p->tot_len, 0);
  MAC_Transmit(&hmac, tx_buf[hmac.TxPointer], p->tot_len);
  sys_arch_unprotect(PLIC_MAX_PRIORITY);

  return ERR_OK;
}

err_t netif_init_func_agm(struct netif *netif)
{
  netif->linkoutput = netif_output;
  netif->output     = etharp_output;
  netif->mtu        = 1514;
  netif->flags      = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP;

  memcpy(netif->hwaddr, hmac.Init.MacAddress, 6);
  netif->hwaddr_len = ETH_HWADDR_LEN;

  return ERR_OK;
}

void netif_status_callback_agm(struct netif *netif)
{
  LWIP_DEBUGF(AGM_DEBUG | LWIP_DBG_TRACE, ("netif status changed %s\n", ip4addr_ntoa(netif_ip4_addr(netif))));
}

sys_prot_t sys_arch_protect_agm(void)
{
  sys_prot_t val = INT_GetIRQPriorify(MAC0_IRQn);
  INT_DisableIRQ(MAC0_IRQn);
  return val;
}

void sys_arch_unprotect_agm(sys_prot_t pval)
{
  INT_EnableIRQ(MAC0_IRQn, pval);
}

u32_t sys_now_agm(void)
{
  return UTIL_GetTick();
}

void MAC0_isr_agm(void)
{
  volatile uint32_t stat = MAC_GetStatus(hmac.Instance);
  MAC_ClearStatus(hmac.Instance, stat);
  if (stat & MAC_STAT_RX_INT) {
    int start_idx = rx_queue_front;
    for (int rx_idx = start_idx; rx_idx < start_idx + RX_NUM_DESC; ++rx_idx) {
      MAC_DescriptorTypeDef *rx_desc = &hmac.RxDesc[rx_idx % RX_NUM_DESC];
      if (MAC_IsDescEnabled(rx_desc)) {
        break; // The rest are pending descriptors. No need to check further
      } else {
        uint16_t eth_data_count = MAC_GetDescLength(rx_desc);
        struct pbuf* p = pbuf_alloc_reference((void *)(rx_desc->Addr), eth_data_count, PBUF_REF);
        if(p != NULL) {
          if(!queue_try_put_agm(rx_queue_agm, p)) {
            /* queue is full -> packet loss */
            LWIP_DEBUGF(AGM_DEBUG | LWIP_DBG_TRACE, ("packet loss\n"));
            pbuf_free(p);
          }
        }
      }
    }
  }
  if (stat & MAC_STAT_PHY_STAT) {
    uint16_t mdio = MAC_MdioRead(&hmac, MAC_PHY_REG_INT);
    if (mdio & MAC_PHY_INT_LINK_CHG) {
      // Read twice for current link status
      mdio = MAC_MdioRead(&hmac, MAC_PHY_REG_STAT);
      mdio = MAC_MdioRead(&hmac, MAC_PHY_REG_STAT);
      mdio &= MAC_PHY_STAT_LINK_STATUS;
      if (mdio & MAC_PHY_STAT_LINK_STATUS) {
        netif_set_link_up(&netif);
        LWIP_DEBUGF(AGM_DEBUG | LWIP_DBG_TRACE, ("netif up\n"));
      } else {
        netif_set_link_down(&netif);
        LWIP_DEBUGF(AGM_DEBUG | LWIP_DBG_TRACE, ("netif down\n"));
      }
    }
  }
  mac_rx_prepare();
}

