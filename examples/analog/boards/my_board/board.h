#ifndef BOARD_H
#define BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "alta.h"

#ifndef CLOCK_PERIOD
#define CLOCK_PERIOD (1e9/BOARD_PLL_FREQUENCY)
#endif
#ifndef RTC_PERIOD
#define RTC_PERIOD (1e9f/BOARD_RTC_FREQUENCY)
#endif

#ifndef FPGA_ADDR
#define FPGA_ADDR 0x80027000
#endif

#define MMIO_BASE (0x60000000)

#define LED_GPIO GPIO4
#define LED_GPIO_MASK APB_MASK_GPIO4
#define LED_GPIO_BITS (1 << 4) // LED 4

#define BUT_GPIO GPIO6
#define BUT_GPIO_MASK APB_MASK_GPIO6
#define BUT_GPIO_BITS (0x14) // Bits 2 & 4
#define BUT_GPIO_IRQ GPIO6_IRQn

#define EXT_GPIO GPIO4
#define EXT_GPIO_MASK APB_MASK_GPIO4
#define EXT_GPIO_BITS 0b1110

SYS_HSE_BypassTypeDef board_hse_source(void);
RTC_ClkSourceTypeDef  board_rtc_source(void);
uint32_t board_lse_freq(void);
uint32_t board_pll_clkin_freq(void);

void board_init(void);
void board_led_write(bool state);
uint32_t board_button_read(void);
int board_uart_read(uint8_t *buf, int len);
int board_uart_write(void const *buf, int len);
uint32_t board_millis(void);
static inline void HardFault_Handler(void) { asm("ebreak"); }

MAC_MediaInterfaceTypeDef board_mac_media(void);
uint8_t board_mac_phy_addr(void);
void board_init_mac(void);
void board_init_phy(MAC_HandleTypeDef *hmac);

#ifdef __cplusplus
}
#endif

#endif
