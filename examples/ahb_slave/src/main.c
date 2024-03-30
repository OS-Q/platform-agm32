#include "alta.h"
#include "board.h"

void MTIMER_isr(void)
{
  GPIO_Toggle(EXT_GPIO, EXT_GPIO_BITS);
  INT_SetMtime(0);
}

void TestMtimer(int ms)
{
  clint_isr[IRQ_M_TIMER] = MTIMER_isr;
  INT_SetMtime(0);
  INT_SetMtimeCmp(SYS_GetSysClkFreq() / 1000 * ms);
  INT_EnableIntTimer();
  while (1);
}

void TestAHB()
{
  const int RAM_SIZE = 4096; // Size of the AHB connected RAM in IP
  uint32_t data_buf[RAM_SIZE / 4] __attribute__((aligned(64)));
  uint32_t read_buf[RAM_SIZE / 4] __attribute__((aligned(64)));

  // Initial data_buf with ramdom number and write to AHB RAM
  for (int i = 0; i < RAM_SIZE / 4; ++i) {
    data_buf[i] = rand();
  }
  uint64_t mcycle = UTIL_GetMcycle();
  // Write to AHB RAM
  DMAC_WaitedTransfer(DMAC_CHANNEL7, (uint32_t)data_buf, MMIO_BASE, DMAC_ADDR_INCR_ON, DMAC_ADDR_INCR_ON,
                      DMAC_WIDTH_32_BIT, DMAC_WIDTH_32_BIT, DMAC_BURST_256, DMAC_BURST_256, RAM_SIZE / 4,
                      DMAC_MEM_TO_MEM_DMA_CTRL, 0, 0);
  // Read from AHB RAM
  DMAC_WaitedTransfer(DMAC_CHANNEL7, MMIO_BASE, (uint32_t)read_buf, DMAC_ADDR_INCR_ON, DMAC_ADDR_INCR_ON,
                      DMAC_WIDTH_32_BIT, DMAC_WIDTH_32_BIT, DMAC_BURST_256, DMAC_BURST_256, RAM_SIZE / 4,
                      DMAC_MEM_TO_MEM_DMA_CTRL, 0, 0);
  mcycle = UTIL_GetMcycle() - mcycle;
  // Verify AHB RAM, and fill data_buf with new random values
  for (int i = 0; i < RAM_SIZE / 4; ++i) {
    if (read_buf[i] != data_buf[i]) {
      printf("Error in master AHB RAM @ address %d: got 0x%08x, 0x%08x expected\n", i, read_buf[i], data_buf[i]);
      return;
    }
    data_buf[i] = rand();
  }
  printf("AHB master test passed in %d usec!\n", UTIL_McycleToUs(mcycle));

  // Prepare for slave transfer test. Data will be copied from data_buf to block RAM in logic with the first trigger, 
  // and then from block RAM to read_buf with the second trigger.
  WR_REG(MMIO_BASE, (uint32_t)data_buf); // Tell logic where to copy data from.
  data_buf[0] = (uint32_t)read_buf; // Tell logic where to put data into.

  // Issue a trigger to the slave AHB as defined in custom_ip to start transfer
  mcycle = UTIL_GetMcycle();
  PERIPHERAL_ENABLE(GPIO, 0);
  // Set GPIO0_0 as output trigger signal
  GPIO_SetOutput(GPIO0, GPIO_BIT0);
  GPIO_SetHigh(GPIO0, GPIO_BIT0);
  // Set GPIO0_0 as input finish signal
  GPIO_SetInput(GPIO0, GPIO_BIT0);
  // Wait for slave AHB transfer to finish
  while (GPIO_GetValue(GPIO0, GPIO_BIT0) == 0);
  printf("Slave read in %d usec.\n", UTIL_McycleToUs(UTIL_GetMcycle() - mcycle));

  // Issue the trigger again so that data will be read from block RAM in logic and written to read_buf.
  mcycle = UTIL_GetMcycle();
  // Set GPIO0_0 as output trigger signal
  GPIO_SetOutput(GPIO0, GPIO_BIT0);
  GPIO_SetHigh(GPIO0, GPIO_BIT0);
  // Set GPIO0_0 as input finish signal
  GPIO_SetInput(GPIO0, GPIO_BIT0);
  // Wait for slave AHB transfer to finish
  while (GPIO_GetValue(GPIO0, GPIO_BIT0) == 0);
  printf("Slave write in %d usec.\n", UTIL_McycleToUs(UTIL_GetMcycle() - mcycle));

  // Check read_buf vs data_buf
  for (int i = 0; i < RAM_SIZE / 4; ++i) {
    if (read_buf[i] != data_buf[i]) {
      printf("Error in slave AHB RAM @ address %d: got 0x%08x, 0x%08x expected\n", i, read_buf[i], data_buf[i]);
      return;
    }
  }

  printf("AHB slave test passed!\n");
}

int main(void)
{
  // This will init clock and uart on the board
  board_init();
  
  TestAHB();

  TestMtimer(500);
}
