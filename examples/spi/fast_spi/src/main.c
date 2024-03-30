#include "board.h"

static SPI_TypeDef *spi = SPIx(0);
static DMAC_ChannelNumTypeDef spi_dmac_channel = DMAC_CHANNEL6;

void run(int sclk_div)
{
  printf("\nRunning test with sclk_div: %d, SPI_RX_PIPELINE: %d\n", sclk_div, SPI_RX_PIPELINE);

  SPI_FLASH_Init(spi, sclk_div << SPI_CTRL_SCLK_DIV_OFFSET);
  printf("Status reg: 0x%04x\n", SPI_FLASH_ReadStatus(spi));

  SPI_FLASH_PowerDown(spi);
  UTIL_IdleUs(100);

  printf("Electronic ID: 0x%x\n", SPI_FLASH_ReleaseDP(spi));
  printf("RDID: 0x%06x\n", SPI_FLASH_ReadID(spi));
  printf("MFID: 0x%04x\n", SPI_FLASH_ReadManufacturerID(spi));
  printf("Capacity: %d\n", SPI_FLASH_GetCapacity(spi)); // May not be supported by some flash models
  uint32_t uid[4] = {0, 0, 0, 0};
  SPI_FLASH_ReadUniqueID(spi, uid, spi_dmac_channel);
  printf("Unique ID: 0x%08x%08x%08x%08x\n", uid[3], uid[2], uid[1], uid[0]);

  bool success = true;

  const uint32_t FLASH_ADDR = 0x000000;
  const uint32_t FLASH_SIZE = 0x10000;
  uint32_t buf[FLASH_SIZE / 4];
  printf("Testing flash from addr: 0x%06x, size: 0x%x\n\n", FLASH_ADDR, FLASH_SIZE);
  for (int i = 0; i < 2; ++i) {
    const SPI_PhaseModeTypeDef FLASH_READ_MODE = i == 0 ? SPI_PHASE_MODE_SINGLE : SPI_PHASE_MODE_QUAD;
    SPI_PhaseModeTypeDef FLASH_WRITE_MODE = FLASH_READ_MODE;

#define FLASH_MODE_STR(__mode) (__mode == SPI_PHASE_MODE_SINGLE ? "single" : __mode == SPI_PHASE_MODE_DUAL ? "dual" : "quad")
    printf("  flash read mode: %s, write mode: %s\n", FLASH_MODE_STR(FLASH_READ_MODE), FLASH_MODE_STR(FLASH_WRITE_MODE));

    SPI_FLASH_Erase(spi, FLASH_ADDR, FLASH_SIZE);
    SPI_FLASH_Read(spi, buf, FLASH_ADDR, FLASH_SIZE, FLASH_READ_MODE, spi_dmac_channel);
    for (int i = 0; i < FLASH_SIZE / 4; ++i) {
      if (buf[i] != 0xffffffff) {
        printf("Erase check failed at addr: 0x%06x, data: 0x%08x\n", FLASH_ADDR + i * 4, buf[i]);
        success = false;
        break;
      }
      buf[i] = random();
    }

    uint32_t *buf1 = buf + FLASH_SIZE / 8; // Pointer at half way
    uint32_t mcycle = UTIL_GetMcycle();
    SPI_FLASH_Write(spi, buf, FLASH_ADDR, FLASH_SIZE/2, FLASH_WRITE_MODE, spi_dmac_channel);
    mcycle = UTIL_GetMcycle() - mcycle;
    printf("  Write time: %.3f ms\n", UTIL_McycleToUs(mcycle) / 1000.0);
    mcycle = UTIL_GetMcycle();
    SPI_FLASH_Read(spi, buf1, FLASH_ADDR, FLASH_SIZE/2, FLASH_READ_MODE, spi_dmac_channel);
    mcycle = UTIL_GetMcycle() - mcycle;
    printf("  Read  time: %.3f ms\n", UTIL_McycleToUs(mcycle) / 1000.0);
    for (int i = 0; i < FLASH_SIZE / 8; ++i) {
      if (buf[i] != buf1[i]) {
        printf("Write/read check failed at addr: 0x%06x, data: 0x%08x vs 0x%08x\n", FLASH_ADDR + i * 4, buf[i], buf1[i]);
        success = false;
        break;
      }
    }
    SPI_FLASH_WritePage(spi, buf, FLASH_ADDR, 0x100, FLASH_WRITE_MODE, spi_dmac_channel, SPI_INTERRUPT_OFF);
    DMAC_WaitChannel(spi_dmac_channel);
    printf("\n");
  }

  SPI_FLASH_EraseSector(spi, FLASH_ADDR, SPI_INTERRUPT_OFF);
  DMAC_WaitChannel(spi_dmac_channel);

  printf("SPI %d test %s!\n\n", 0, success ? "passed" : "failed");
}

void run_psram(int sclk_div, bool qpi_mode)
{
  printf("\nRunning test with sclk_div: %d, SPI_RX_PIPELINE: %d, QPI Mode: %d\n", sclk_div, SPI_RX_PIPELINE, qpi_mode);

  SPI_Init(spi, sclk_div << SPI_CTRL_SCLK_DIV_OFFSET);

  if (qpi_mode) {
    // Enter QPI mode
    SPI_PRAM_EnableQPI(spi);
  } else {
    SPI_FLASH_SetCmdPhase(spi, SPI_FLASH_CMD_RDID);
    SPI_SetPhaseCtrl(spi, SPI_PHASE_1, SPI_PHASE_ACTION_DUMMY_TX, SPI_PHASE_MODE_QUAD, SPI_RX_PIPELINE ? 13 : 12);
    SPI_SetPhaseCtrl(spi, SPI_PHASE_2, SPI_PHASE_ACTION_RX, SPI_PHASE_MODE_SINGLE, 4);
    SPI_Start(spi, SPI_CTRL_PHASE_CNT3, SPI_CTRL_DMA_OFF, SPI_INTERRUPT_OFF);
    SPI_WaitForDone(spi);
    uint32_t rdid = spi->PHASE_DATA[SPI_PHASE_2];
    printf("RDID: 0x%08x\n", rdid);
  }

  bool success = true;
  const uint32_t PSRAM_ADDR = 0x010088;
  const uint32_t PSRAM_SIZE = 0x10000;
  uint32_t buf[PSRAM_SIZE / 4];
  printf("Testing psram from addr: 0x%06x, size: 0x%x\n\n", PSRAM_ADDR, PSRAM_SIZE);
  SPI_PhaseModeTypeDef PSRAM_READ_MODE;
  SPI_PhaseModeTypeDef PSRAM_WRITE_MODE;
  for (int i = qpi_mode ? 1 : 0; i < 2; ++i) {
    SPI_PhaseModeTypeDef PSRAM_READ_MODE = i == 0 ? SPI_PHASE_MODE_SINGLE : SPI_PHASE_MODE_QUAD;
    SPI_PhaseModeTypeDef PSRAM_WRITE_MODE = PSRAM_READ_MODE;

#define PSRAM_MODE_STR(__mode) (__mode == SPI_PHASE_MODE_SINGLE ? "single" : __mode == SPI_PHASE_MODE_DUAL ? "dual" : "quad")
    printf("  flash read mode: %s, write mode: %s\n", PSRAM_MODE_STR(PSRAM_READ_MODE), PSRAM_MODE_STR(PSRAM_WRITE_MODE));
    for (int i = 0; i < PSRAM_SIZE / 4; ++i) {
      buf[i] = random();
    }
    uint32_t *buf1 = buf + PSRAM_SIZE / 8; // Pointer at half way

    uint32_t mcycle = UTIL_GetMcycle();
    SPI_PSRAM_Write(spi, buf, PSRAM_ADDR, PSRAM_SIZE/2, PSRAM_WRITE_MODE, spi_dmac_channel, qpi_mode);
    mcycle = UTIL_GetMcycle() - mcycle;
    printf("  Write time: %.3f ms\n", UTIL_McycleToUs(mcycle) / 1000.0);

    mcycle = UTIL_GetMcycle();
    SPI_PSRAM_Read(spi, buf1, PSRAM_ADDR, PSRAM_SIZE/2, PSRAM_READ_MODE, spi_dmac_channel, qpi_mode);
    mcycle = UTIL_GetMcycle() - mcycle;
    printf("  Read  time: %.3f ms\n", UTIL_McycleToUs(mcycle) / 1000.0);
    for (int i = 0; i < PSRAM_SIZE / 8; ++i) {
      if (buf[i] != buf1[i]) {
        printf("Write/read check failed at addr: 0x%06x, data: 0x%08x vs 0x%08x\n", PSRAM_ADDR + i * 4, buf[i], buf1[i]);
        success = false;
        break;
      }
    }
    DMAC_WaitChannel(spi_dmac_channel);
    printf("\n");
  }

  if (qpi_mode) {
    // Exit QPI mode
    SPI_PRAM_DisableQPI(spi);
  }

  printf("SPI %d test %s!\n\n", 0, success ? "passed" : "failed");
}

int main()
{
  board_init();
  SYS_EnableAHBClock(AHB_MASK_DMAC0);
  DMAC_Init();
  PERIPHERAL_ENABLE_ALL(SPI, 0);

  // SPI can run up to 100MHz with SPI_RX_PIPELINE, and 40MHz without it.
  int sclk_div = SPI_RX_PIPELINE ? 2 : 6;

#ifdef TEST_PSRAM
  run_psram(sclk_div, false);
  run_psram(sclk_div, true);
#else
  // Make sure the flash is quad enabled and not write protected.
  SPI_FLASH_Init(spi, SPI_CTRL_SCLK_DIV8);
  SPI_FLASH_WriteEnable(spi);
  SPI_FLASH_WriteStatus(spi, SPI_FLASH_QE_FLAG);
  run(sclk_div);
#endif

  while (1);
}
