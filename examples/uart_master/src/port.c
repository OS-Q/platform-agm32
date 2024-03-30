#include "port.h"
#include "alta.h"

#define GPIO_IDX   9
#define GPIO_NAME  PERIPHERAL_NAME_(GPIO, GPIO_IDX)
#define GPIO_NRST  GPIO_BIT0
#define GPIO_BOOT0 GPIO_BIT1

void usleep(int us)
{
  UTIL_IdleUs(us);
}

void pin_init()
{
  // Enable the pins to control NRST and BOOT0. Note that BOOT1 needs to be weakly pulled down.
  PERIPHERAL_ENABLE(GPIO, 9);
  GPIO_SetOutput(GPIO_NAME, GPIO_NRST | GPIO_BOOT0);
}

void pin_nrst_out(uint32_t value)
{
  GPIO_SetValue(GPIO_NAME, GPIO_NRST, value ? 0xff : 0);
}

void pin_boot0_out(uint32_t value)
{
  GPIO_SetValue(GPIO_NAME, GPIO_BOOT0, value ? 0xff : 0);
}

#define MASTER_UART_IDX 1
#define MASTER_UART PERIPHERAL_NAME_(UART, MASTER_UART_IDX)

void uart_enable(uint32_t baud)
{
  PERIPHERAL_ENABLE_(UART, MASTER_UART_IDX);
  UART_Init(MASTER_UART, baud, UART_LCR_DATABITS_8, UART_LCR_STOPBITS_1, UART_LCR_PARITY_EVEN, UART_LCR_FIFO_16);
}

void uart_disable()
{
  // Make sure the master UART is reset and any left over data is cleared.
  SYS_APBResetAndDisable(APB_MASK_UARTx(MASTER_UART_IDX));
}

bool uart_tx(const unsigned char *buf, uint32_t count)
{
  return UART_Send(MASTER_UART, buf, count) == RET_OK;
}

bool uart_rx(unsigned char *buf, uint32_t count, int timeout)
{
  return UART_Receive(MASTER_UART, buf, count, timeout) == count;
}

