#include "board.h"

bool run_psram()
{
  bool success = true;
  const uint32_t PSRAM_ADDR = 0x010088;
  const uint32_t PSRAM_SIZE = 0x10000;
  uint32_t buf[PSRAM_SIZE / 4];
  printf("Testing psram from addr: 0x%06x, size: 0x%x\n\n", PSRAM_ADDR, PSRAM_SIZE);
  for (int r = 0; r < 4; ++r) {
    for (int i = 0; i < PSRAM_SIZE / 4; ++i) {
      buf[i] = random();
    }
    uint32_t *buf1 = buf + PSRAM_SIZE / 8; // Pointer at half way
    DMAC_WidthTypeDef dmac_width = r == 1 ? DMAC_WIDTH_32_BIT : r == 2 ? DMAC_WIDTH_16_BIT : DMAC_WIDTH_8_BIT;
    int width = r == 1 ? 4 : r == 2 ? 2 : 1;

    uint32_t *ptr = (uint32_t *)MMIO_BASE;
    uint32_t mcycle = UTIL_GetMcycle();
    if (r == 0) {
      printf("Direct access:\n");
      for (int i = 0; i < PSRAM_SIZE / 8; ++i) {
        *ptr++ = buf[i];
      }
    } else {
      printf("DMA %d bits\n", width * 8);
      DMAC_WaitedTransfer(DMAC_CHANNEL0, (uint32_t)buf, MMIO_BASE, DMAC_ADDR_INCR_ON, DMAC_ADDR_INCR_ON,
                          dmac_width, dmac_width, DMAC_BURST_128, DMAC_BURST_128,
                          PSRAM_SIZE / 2 / width, DMAC_MEM_TO_MEM_DMA_CTRL, 0, 0);
    }
    mcycle = UTIL_GetMcycle() - mcycle;
    printf("  Write time: %.3f ms\n", UTIL_McycleToUs(mcycle) / 1000.0);

    ptr = (uint32_t *)MMIO_BASE;
    mcycle = UTIL_GetMcycle();
    if (r == 0) {
      for (int i = 0; i < PSRAM_SIZE / 8; ++i) {
        buf1[i] = *ptr++;
      }
    } else {
      DMAC_WaitedTransfer(DMAC_CHANNEL0, MMIO_BASE, (uint32_t)buf1, DMAC_ADDR_INCR_ON, DMAC_ADDR_INCR_ON,
                          dmac_width, dmac_width, DMAC_BURST_128, DMAC_BURST_128,
                          PSRAM_SIZE / 2 / width, DMAC_MEM_TO_MEM_DMA_CTRL, 0, 0);
    }
    mcycle = UTIL_GetMcycle() - mcycle;
    printf("  Read  time: %.3f ms\n", UTIL_McycleToUs(mcycle) / 1000.0);
    for (int i = 0; i < PSRAM_SIZE / 8; ++i) {
      if (buf[i] != buf1[i]) {
        printf("Write/read check failed at addr: 0x%06x, data: 0x%08x vs 0x%08x\n", PSRAM_ADDR + i * 4, buf[i], buf1[i]);
        success = false;
        break;
      }
    }
    printf("\n");
  }

  printf("SPI %d test %s!\n\n", 0, success ? "passed" : "failed");
  return success;
}

int main()
{
  board_init();

  SYS_EnableAHBClock(AHB_MASK_DMAC0);
  DMAC_Init();

  bool success = run_psram();
  while (1) {
    GPIO_Toggle(LED_GPIO, LED_GPIO_BITS);
    UTIL_IdleMs(success ? 100 : 1000);
  }
}
