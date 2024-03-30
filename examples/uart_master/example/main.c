#include "board.h"
#include "uart_master.h"

__attribute__((section(".flash")))
static uint8_t config[] = {
#include "config.inc"
};

__attribute__((section(".flash")))
static uint8_t config1[] = {
#include "config1.inc"
};

__attribute__((section(".flash")))
static uint8_t program_buf[] = {
#include "example.inc"
};

__attribute__((section(".flash")))
static uint8_t logic_buf[] = {
#include "logic.inc"
};

void Button_isr(void)
{
  uint32_t tick = UTIL_GetTick();
  if (!uart_init(1000000)) { // Can support up to 1M baud rate
    printf("uart_init failed!\n");
  }
  if (GPIO_IsMaskedIntActive(BUT_GPIO, GPIO_BIT2)) {
    // Must be called when writing to flash for the first time.
    if (!uart_init_flash()) {
      printf("uart_init_flash failed!\n");
    }

    // This is called to write logic configuration to flash.
    if (!uart_update_logic(sizeof(config), config)) {
      printf("uart_update_logic failed!\n");
    }

    if (!uart_update_program(sizeof(program_buf), program_buf)) {
      printf("uart_update_program failed\n");
    }
    printf("Uart update logic and program in %dms!\n", UTIL_GetTick() - tick);
  } else {
    if (!uart_update_logic(sizeof(config1), config1)) {
      printf("uart_update_logic 1 failed!\n");
    }
    printf("Uart update logic in %dms!\n", UTIL_GetTick() - tick);
  }
  // Reset target with BOOT0 low and enter normal operation.
  uart_final(true);
  GPIO_ClearInt(BUT_GPIO, BUT_GPIO_BITS);
}

int main(void)
{
  // Platform and board dependent initialization
  board_init();
  plic_isr[BUT_GPIO_IRQ] = Button_isr;
  INT_EnableIRQ(BUT_GPIO_IRQ, PLIC_MAX_PRIORITY);
  uint32_t tick = UTIL_GetTick();

  // Reset target with BOOT0 high and enter UART bootloader
  if (!uart_init(1000000)) { // Can support up to 1M baud rate
    printf("uart_init failed!\n");
  }

  // Write logic configuration to SRAM
  if (!uart_sram(sizeof(logic_buf), logic_buf)) {
    printf("uart_sram failed!\n");
  }

  // Do not reset target when writing sram.
  uart_final(false);

  printf("Uart done in %dms!\n", UTIL_GetTick() - tick);
  while (1);
}
