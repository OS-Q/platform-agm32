#include "board.h"

static SPI_TypeDef *spi = SPIx(0);
static DMAC_ChannelNumTypeDef spi_dmac_channel = DMAC_CHANNEL6;

// Registers defined in logic based full duplex SPI
#define SPI_CTRL (MMIO_BASE + 0) // Supoort SPI_CTRL_DMA_EN and SPI_CTRL_ENDIAN
#define SPI_DATA (MMIO_BASE + 4) // Read only RX data

// Values supported in the above SPI_CTRL register
#define SPI_CTRL_CPOL  (1 << 24) // Set to make CPOL = 1. Default: 0.
#define SPI_CTRL_CPHA  (1 << 25) // Set to make CPHA = 1. Default: 0.
// SPI_CTRL_DMA_EN: Defined in SDK. Set to enable DMA. Default: disable.
// SPI_CTRL_ENDIAN: Defined in SDK. Set to use little endian. Default: big endian.

uint32_t full_duplex_rdid()
{
  // The usual way to read ID is to use SPI_PHASE_ACTION_RX in the second phase. Here, to demonstrate full duplex 
  // transfer, SPI_Send is used with command and dummy data. Then the received data is read back from logic.
  SPI_Send(spi, 4, SPI_FLASH_CMD_RDID);
  // Ingore the lower 8 bits, which are received in command phase.
  return RD_REG(SPI_DATA) >> 8;
}

uint32_t full_duplex_mfid()
{
  // When more than 4 bytes are transferred, only the last 4 bytes received will be available for read without DMA.
  SPI_Send(spi, 6, SPI_FLASH_CMD_REMS);
  return RD_REG(SPI_DATA) & 0xffff;
}

void full_duplex_ruid(uint32_t *unique_id)
{
  MOD_REG_BIT(SPI_CTRL, SPI_CTRL_DMA_EN, SPI_CTRL_DMA_ON);
  uint32_t rx_data[6];
  DMAC_Config(spi_dmac_channel, SPI_DATA, (uint32_t)rx_data,
              DMAC_ADDR_INCR_OFF, DMAC_ADDR_INCR_ON, DMAC_WIDTH_32_BIT, DMAC_WIDTH_32_BIT,
              DMAC_BURST_1, DMAC_BURST_1, 0, DMAC_PERIPHERAL_TO_MEM_PERIPHERAL_CTRL,
              EXT_DMA0_REQ, 0);
  // Total transfer bytes: 1 command + 4 dummy + 16 ID = 21
  SPI_Send(spi, 21, SPI_FLASH_CMD_RDUID);
  // Ignore the first 5 bytes in rx_data. Those are the bits received in command and dummy bytes.
  unique_id[0] = RD_UNALIGNED_U32((uint8_t *)rx_data + 5);
  unique_id[1] = RD_UNALIGNED_U32((uint8_t *)rx_data + 9);
  unique_id[2] = RD_UNALIGNED_U32((uint8_t *)rx_data + 13);
  unique_id[3] = RD_UNALIGNED_U32((uint8_t *)rx_data + 17);
  MOD_REG_BIT(SPI_CTRL, SPI_CTRL_DMA_EN, SPI_CTRL_DMA_OFF);
}

void run()
{
  // Initial SPI.
  SPI_FLASH_Init(spi, SPI_CTRL_SCLK_DIV4);
  uint32_t unique_id[4];

  // Test SPI with logic based full duplex functions.
  printf("\nTesting flash with CPOL = 0, CPHA = 0:\n");
  WR_REG(SPI_CTRL, SPI_CTRL_LITTLE_ENDIAN); // CPOL = 0, CPHA = 0
  printf("RDID from full duplex: 0x%06x\n", full_duplex_rdid());
  printf("MFID from full duplex: 0x%04x\n", full_duplex_mfid());
  full_duplex_ruid(unique_id);
  printf("RUID from full duplex: 0x%04x%04x%04x%04x\n", unique_id[3], unique_id[2], unique_id[1], unique_id[0]);

  printf("\nTesting flash with CPOL = 1, CPHA = 1:\n");
  WR_REG(SPI_CTRL, SPI_CTRL_LITTLE_ENDIAN | SPI_CTRL_CPOL | SPI_CTRL_CPHA); // CPOL = 1, CPHA = 1
  printf("RDID from full duplex: 0x%06x\n", full_duplex_rdid());
  printf("MFID from full duplex: 0x%04x\n", full_duplex_mfid());
  full_duplex_ruid(unique_id);
  printf("RUID from full duplex: 0x%04x%04x%04x%04x\n", unique_id[3], unique_id[2], unique_id[1], unique_id[0]);
}

int main()
{
  board_init();
  SYS_EnableAHBClock(AHB_MASK_DMAC0);
  DMAC_Init();
  PERIPHERAL_ENABLE_ALL(SPI, 0);

  run();

  while (1);
}
