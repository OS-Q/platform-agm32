#include "board.h"
#include "example.h"

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

int main(void)
{
  // This will init clock and uart on the board
  board_init();
  
  TestAnalog();
  TestMtimer(500);
}
