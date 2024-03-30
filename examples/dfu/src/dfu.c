#include "boot.h"

static uint32_t apb_clken;
static uint32_t misc_ctrl;

#ifndef DFU_MSCRATCH
#define DFU_MSCRATCH 0xdeadbeef
#endif

void enter_boot(void)
{
  // Turn on specified LED to indicate DFU is active
#ifdef DFU_LED_GPIO
#ifdef DFU_LED_BIT
#ifndef DFU_LED_VAL
#define DFU_LED_VAL 0
#endif
#define LED_GPIO PERIPHERAL_NAME_(GPIO, DFU_LED_GPIO)
  PERIPHERAL_ENABLE_(GPIO, DFU_LED_GPIO);
  GPIO_SetOutput(LED_GPIO, DFU_LED_BIT);
  GPIO_SetValue(LED_GPIO, DFU_LED_BIT, DFU_LED_VAL);
#endif
#endif

  // Optionally disable the specified SWJ pins.
#ifdef DFU_PROTECT_SWJ
  SYS->SWJ_CNTL = DFU_PROTECT_SWJ;
#endif

  SYS->APB_CLKENABLE = apb_clken;
  SYS->MISC_CNTL = misc_ctrl;
  boot();
}

int main()
{
#ifdef DFU_APP_ADDRESS
  // Clear mscratch after power on.
  if (SYS_IsActiveResetFlag_POR()) {
    SYS_ClearResetFlags();
    write_csr(mscratch, 0x0);
  }
  uint32_t mscratch = read_csr(mscratch);
  write_csr(mscratch, 0x0);
  // Go straight to application if matched.
  if (mscratch == ~DFU_MSCRATCH) {
    UTIL_JumpToAddress(DFU_APP_ADDRESS);
  }
#endif

  apb_clken = SYS->APB_CLKENABLE;
  misc_ctrl = SYS->MISC_CNTL;
  SYS_SetHSIConfig(0xf);
  SYS_SwitchHSIClock();

  // Provide a minimal working FPGA configuration to avoid problems if the default configuration is corrupted.
#ifdef DFU_FPGA_CONFIG
  SYS->APB_CLKENABLE = APB_MASK_FCB0; // Disable all peripherial clocks execpt for FCB to avoid any noise from FPGA
  __attribute__ ((aligned(4))) uint8_t fpga_config[] = {
#include DFU_FPGA_CONFIG
  };
  FCB_AutoDecompress((uint32_t)fpga_config);
#endif

  // If the application address is provided, DFU should be able to jump there or stay.
#ifdef DFU_APP_ADDRESS
  // Hardware way to jump to application using the specified GPIO button.
#ifdef DFU_BUT_GPIO
#ifdef DFU_BUT_BIT
#ifndef DFU_BUT_VAL
#define DFU_BUT_VAL 0
#endif
#define BUT_GPIO PERIPHERAL_NAME_(GPIO, DFU_BUT_GPIO)
  PERIPHERAL_ENABLE_(GPIO, DFU_BUT_GPIO);
  GPIO_SetInput(BUT_GPIO, DFU_BUT_BIT);
  // Stay in DFU if button value matched, otherwise jump to application.
  if (GPIO_GetValue(BUT_GPIO, DFU_BUT_BIT) == DFU_BUT_VAL) {
    enter_boot();
  }
#endif
#endif

  // Software way to jump to application using mscratch
  if (mscratch == DFU_MSCRATCH) {
    enter_boot();
  }

  // Set mscratch so that next time dfu will go straight to application. Do not jump to application here since the FPGA 
  // configuration is changed. Use a reset to restore it.
  write_csr(mscratch, ~DFU_MSCRATCH);
  SYS_SoftwareReset();
#endif

  enter_boot();
}
