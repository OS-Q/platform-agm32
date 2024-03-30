#include "bsp/board.h"
#include "tusb.h"
#include "device/dcd.h"

#include "alta.h"
#include "board.h"

#include "rndis_protocol.h"

#define INFO printf

const uint8_t tud_network_mac_address[6] = {0x90,0x02,0x84,0x6A,0x96,0x00};

MAC_HandleTypeDef hmac;

// Use 2 descriptors to improve performance with circular operation. More descriptors will not further improve 
// performance since it's limited by USB FS.
#define RX_NUM_DESC 2
#define TX_NUM_DESC 2
MAC_DescriptorTypeDef RxDesc[RX_NUM_DESC] TU_ATTR_ALIGNED(MAC_GET_DESC_ALIGN(RX_NUM_DESC));
MAC_DescriptorTypeDef TxDesc[TX_NUM_DESC] TU_ATTR_ALIGNED(MAC_GET_DESC_ALIGN(TX_NUM_DESC));

uint32_t rx_buf[RX_NUM_DESC][(CFG_TUD_NET_MTU + 3) / 4] TU_ATTR_SECTION(".noinit") TU_ATTR_ALIGNED(4);
uint32_t tx_buf[TX_NUM_DESC][(CFG_TUD_NET_MTU + 3) / 4] TU_ATTR_SECTION(".noinit") TU_ATTR_ALIGNED(4);

void mac_rx_prepare(void)
{
  for (int i = 0; i < RX_NUM_DESC; ++i) {
    MAC_Receive(&hmac, rx_buf[i]);
  }
}

void mac_check_phy(void)
{
  uint16_t mdio = MAC_MdioRead(&hmac, MAC_PHY_REG_INT);
  if (mdio & MAC_PHY_INT_LINK_CHG) {
    // Read twice for current link status
    mdio = MAC_MdioRead(&hmac, MAC_PHY_REG_STAT);
    mdio = MAC_MdioRead(&hmac, MAC_PHY_REG_STAT);
    mdio &= MAC_PHY_STAT_LINK_STATUS;
    tud_network_set_connected(mdio);
    if (mdio) {
      MAC_UpdateLink(&hmac);
    }
    INFO("netif %s\n", mdio ? "up" : "down");
  }
}

void MAC0_isr(void)
{
  volatile uint32_t stat = MAC_GetStatus(hmac.Instance);
  if (stat & MAC_STAT_PHY_STAT) {
    mac_check_phy();
  }
  MAC_ClearStatus(hmac.Instance, stat);
}

void mac_init(void)
{
  if (board_mac_media() == MAC_MEDIA_INTERFACE_MII) {
    PERIPHERAL_MAC_ENABLE_MII;
  } else {
    PERIPHERAL_MAC_ENABLE_RMII;
  }

  MAC_HandleInit(&hmac, TxDesc, TX_NUM_DESC, RxDesc, RX_NUM_DESC);
  memcpy(hmac.Init.MacAddress, tud_network_mac_address, 6);
  hmac.Init.PhyAddress      = board_mac_phy_addr();
  hmac.Init.MediaInterface  = board_mac_media();

  while (MAC_Init(&hmac) != RET_OK) {
    INFO("Mac init failed\n");
  }
  board_init_phy(&hmac);
  uint16_t mac_phy_stat = MAC_MdioRead(&hmac, MAC_PHY_REG_STAT);
  tud_network_set_connected(mac_phy_stat & MAC_PHY_STAT_LINK_STATUS);

  // Change LED mode
  MAC_MdioWrite(&hmac, MAC_PHY_REG_PAGE, 7);
  uint32_t reg = MAC_MdioRead(&hmac, MAC_PHY_REG_IWL);
  MAC_MdioWrite(&hmac, MAC_PHY_REG_IWL, reg & ~((3 << 4) | (1 << 10)));
  MAC_MdioWrite(&hmac, MAC_PHY_REG_PAGE, 0);

  INT_EnableIRQ(MAC0_IRQn, PLIC_MAX_PRIORITY);
  MAC_EnableIntRx(hmac.Instance);
  MAC_EnableIntPhy(hmac.Instance);
  mac_rx_prepare();

  INFO("Mac init done\n");
}

bool tud_network_recv_cb(const uint8_t *src, uint16_t size)
{
  if (MAC_IsDescEnabled(&TxDesc[hmac.TxPointer])) {
    return false;
  } else if (size) {
    memcpy(tx_buf[hmac.TxPointer], src, size);
    MAC_Transmit(&hmac, tx_buf[hmac.TxPointer], size);
  }
  tud_network_recv_renew();
  return true;
}

uint16_t tud_network_xmit_cb(uint8_t *dst, void *ref, uint16_t arg)
{
  memcpy(dst, ref, arg);
  return arg;
}

void tud_network_init_cb(void)
{
  INFO("tud_network_init_cb\n");
}

static void service_traffic(void)
{
  if (board_mac_media() == MAC_MEDIA_INTERFACE_MII) {
    mac_check_phy();
  }

  if (!MAC_IsDescEnabled(&RxDesc[hmac.RxPointer])) {
    if (RxDesc[hmac.RxPointer].Ctrl & (MAC_RX_DESC_ALIGN_ERR | MAC_RX_DESC_TOO_LONG | MAC_RX_DESC_CRC_ERR | MAC_RX_DESC_OVERRUN_ERR | MAC_RX_DESC_LENGTH_ERR)) {
      INFO("RX error: 0x%x\n", RxDesc[hmac.RxPointer].Ctrl);
      MAC_Receive(&hmac, rx_buf[hmac.RxPointer]);
    } else {
      uint32_t usb_tx_size = MAC_GetDescLength(&RxDesc[hmac.RxPointer]);
      if (tud_network_can_xmit(usb_tx_size)) {
        tud_network_xmit(rx_buf[hmac.RxPointer], usb_tx_size);
        MAC_Receive(&hmac, rx_buf[hmac.RxPointer]);
      }
    }
  }
}

enum  {
  BLINK_NOT_MOUNTED = 1000,
  BLINK_MOUNTED = 250,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

void led_blinking_task(void)
{
  static uint32_t board_ms = 0;
  static bool led_state = false;

  if (board_millis() > board_ms) {
    board_led_write(led_state);
    led_state = !led_state;
    board_ms += blink_interval_ms;
  }
}

int main(void)
{
  board_init();
  tusb_init();
  mac_init();

  while (1) {
    tud_task();
    service_traffic();
    led_blinking_task();
  }

  return 0;
}
