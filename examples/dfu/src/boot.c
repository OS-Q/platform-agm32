#include "board.h"
#include "boot.h"

#ifdef DFU_PROTECT_OPTIONS
#define FLASH_OptionByteUnlock() FLASH_Unlock()
#endif

#define WRITE_MEM_DMA DMAC_CHANNEL1
#define CALCU_CRC_DMA DMAC_CHANNEL1

UART_TypeDef *MSG_UART __attribute__((aligned (4)));
static BootAddr jump_to_addr;
static uint8_t hsi_config;

static void sendACK();

static UART_TypeDef *enableUart0()
{
  GPIO_AF_ENABLE(UART0_UARTRXD);
  SYS_EnableAPBClock(APB_MASK_UART0);
  return UARTx(0);
}
static void disableUart0()
{
  PERIPHERAL_UART_DISABLE(UART0);
}
static void checkFlashBusy()
{
  BootWord fast_pg = FLASH->CR&FLASH_CR_FASTPG;
  FLASH->CR &= ~(FLASH_CR_FASTPG);
  while (FLASH_IsBusy());
  FLASH->CR |= fast_pg;
}

#if BOOT_INTERFACE == 1
UART_TypeDef *BOOT_UART_ __attribute__((aligned (4)));

static bool send(unsigned char *p, unsigned int num)
{
  return UART_Send(BOOT_UART_, p, num) == RET_OK;
}
static unsigned int receive(unsigned char *p, unsigned int num, bool blocking)
{
  unsigned int cnt = 0;
  while (cnt < num) {
    while (UART_IsRxFifoEmpty(BOOT_UART_));
    *p++ = BOOT_UART_->DR;
    ++cnt;
  }
  return cnt;
}

// Align to cache block size
static void MeasureBaudRate(GPIO_TypeDef *uart_rxd_port, uint64_t uart_times[]) __attribute__((aligned(64)));
static void MeasureBaudRate(GPIO_TypeDef *uart_rxd_port, uint64_t uart_times[])
{
  // This function should be as small as possible, so that it is loaded to cache once running.
  volatile uint32_t uart_rx = UART0_UARTRXD_AF_GPIO_MASK;
  for (uint32_t idx = 0; idx < 4; ++idx) {
    while (uart_rx == uart_rxd_port->GpioDATA[UART0_UARTRXD_AF_GPIO_MASK]); // Wait for rx to toggle
    uart_times[idx] = INT_GetMtime();
    uart_rx = ~uart_rx & UART0_UARTRXD_AF_GPIO_MASK;
  }
}

static uint32_t waitForInit()
{
  dbg_printf("waitForInit\n");
  int uart_pulse = 0;

  // Disable UART0 and use the shared GPIO to capture the handshake data on RX
  disableUart0();
  PERIPHERAL_ENABLE_(GPIO, UART0_UARTRXD_AF_GPIO);
  // Have to enable TXD before host open the UART stream
  GPIO_AF_ENABLE(UART0_UARTTXD);
  GPIO_TypeDef *uart_rxd_port = GPIOx(UART0_UARTRXD_AF_GPIO);
  uint64_t uart_times[4];
  uint32_t lo_pulse, hi_pulse;
  while (1) {
    while (!uart_rxd_port->GpioDATA[UART0_UARTRXD_AF_GPIO_MASK]); // Wait for rx to be 1
    MeasureBaudRate(uart_rxd_port, uart_times);
    lo_pulse = ((uint32_t)(uart_times[1] - uart_times[0]) + (uint32_t)(uart_times[3] - uart_times[2]) + 1) >> 1;
    hi_pulse  = uart_times[2] - uart_times[1];
    int pulse = hi_pulse + lo_pulse;
    if (10 * hi_pulse < 73 * lo_pulse && 10 * hi_pulse > 67 * lo_pulse) {
      uart_pulse = pulse;
      break;
    } else if (abs(pulse - uart_pulse) * 25 < pulse) { // Less than 4% error
      uart_pulse = (pulse + uart_pulse + 1 ) >> 1;
      break;
    }
    uart_pulse = pulse;
  }
  dbg_printf("UART pulse %d+%d=%d\n", hi_pulse, lo_pulse, uart_pulse);

  return uart_pulse;
}

static void setup()
{
  hsi_config = SYS->MISC_CNTL;
  SYS_SwitchHSIClock();
  SYS_SetHSIConfig(hsi_config | 0xf);
}

static void setupBoot()
{
  dbg_printf("setupBoot\n");

  uint32_t uart_pulse = waitForInit();
  volatile BootWord ibrd = uart_pulse >> 7; // integer baud rate: pulse / 8 / 16
  volatile BootWord fbrd = (uart_pulse - (ibrd << 7) + 1) >> 1; // frational baud rate: (pulse % 128) / 8 / 16 * 64 + 0.5

  // Enable BOOT_UART
  BOOT_UART_ = enableUart0();
  UART_InitBaud(BOOT_UART_, ibrd, fbrd,
                UART_LCR_DATABITS_8, UART_LCR_STOPBITS_1,
                UART_LCR_PARITY_EVEN, UART_LCR_FIFO_16);
  sendACK();
}

static void resetBoot()
{
  DMAC_WaitChannel(WRITE_MEM_DMA);
  checkFlashBusy();
  while (UART_IsTxBusy(BOOT_UART_));
  SYS_APBResetAndDisable(APB_MASK_UART0);
  SYS_APBResetAndDisable(APB_MASK_GPIOx(UART0_UARTRXD_AF_GPIO));
}

static void processBoot()
{
}

#elif BOOT_INTERFACE == 2

#include "tusb.h"
#include "device/usbd_pvt.h"

uint8_t BOOT_USB_ = 0;

static bool send(unsigned char *p, unsigned int num)
{
  tud_cdc_n_write(BOOT_USB_, p, num);
  tud_cdc_n_write_flush(BOOT_USB_);
  tud_task();
  return true;
}
static int receive(unsigned char *p, unsigned int num, bool blocking)
{
  if (blocking) {
    int bytes = num;
    while (bytes > 0) {
      int cnt = tud_cdc_n_read(BOOT_USB_, p, bytes);
      tud_task();
      p += cnt;
      bytes -= cnt;
    }
    return num;
  } else {
    return tud_cdc_n_read(BOOT_USB_, p, num);
  }
}

static BootErrType waitForInit()
{
  dbg_printf("waitForInit\n");

  while (1) {
    BootByte cmd;
    receive(&cmd, 1, true);
    if (cmd==0x7F) {
      return BOOT_ERR_OK;
    }
  }
  return BOOT_ERR_NACK;
}

#ifdef BOARD_HSE_NONE
static FCB_IO_TypeDef io_cfg;
static FCB_OSC_TypeDef osc_cfg;
static uint32_t sof_min, sof_max;
#define SOF_ERROR 0.01
#define SOF_COUNT 10

#include "mcu/agm/agrv2k.h"
void osc_sof_cb(uint32_t status)
{
  static int count;
  static int cycles;
  cycles += GPTIMER_GetCounter(GPTIMER0);
  GPTIMER_SetCounter(GPTIMER0, 0);
  if (++count < SOF_COUNT) {
    return;
  }
  int step = cycles < sof_min ? 1 : cycles > sof_max ? -1 : 0;
  if (step) {
    osc_cfg.CFG_RCOSCCAL += step;
    FCB_SetOscConfig(&io_cfg, &osc_cfg);
    FCB_WriteIOConfig(&io_cfg);
  }
  count = 0;
  cycles = 0;
}

void osc_init(void)
{
  // OSC should be enabled in logic. Also PLL input should come from OSC rather than HSE.
  PERIPHERAL_ENABLE(FCB, 0);
  FCB_ReadIOConfig(&io_cfg);
  FCB_GetOscConfig(&io_cfg, &osc_cfg);

  // The min/max number of cycles expected for each SOF
  sof_min = SYS_GetPLLFreq() * SOF_COUNT * (1 - SOF_ERROR) / 1000;
  sof_max = SYS_GetPLLFreq() * SOF_COUNT * (1 + SOF_ERROR) / 1000;

  PERIPHERAL_ENABLE(GPTIMER, 0);
  GPTIMER_EnableCounter(GPTIMER0);
  dcd_sof_cb = osc_sof_cb;
}
#endif

static void setup()
{
#ifdef BOARD_HSE_NONE
  osc_init();
#endif
  SYS_SetPLLFreq(BOARD_PLL_FREQUENCY);
  SYS_SwitchPLLClock(board_hse_source());
  INT_EnablePLIC();
}

static void setupBoot()
{
  dbg_printf("setupBoot\n");

  PERIPHERAL_ENABLE(USB, 0);
  tusb_init();
  BOOT_USB_ = 0;

  waitForInit();
  sendACK();
}

static void resetBoot()
{
  DMAC_WaitChannel(WRITE_MEM_DMA);
  checkFlashBusy();
  while (usbd_edpt_busy(BOOT_USB_, CDC_EPIN)) {
    tud_task();
  }
  BOOT_USB_ = 0;
  SYS_AHBResetAndDisable(AHB_MASK_USB0);
}

static void processBoot()
{
  tud_task();
}

#elif BOOT_INTERFACE == 3

#define I2C_S0 ((I2C_TypeDef *) 0x60000000)
#define I2C_S0_OAR ((uint32_t *) 0x60000014)

#define I2C_SR_TXE  (1 << 2) // TX empty
#define I2C_SR_RXNE (1 << 3) // RX not empty
#define I2C_SR_ADDR (1 << 4) // Address matched

static bool send(unsigned char *p, unsigned int num)
{
  uint32_t i2c_sr;
  while ((i2c_sr = I2C_GetStatus(I2C_S0)) & I2C_SR_TIP) {}
  for (int i = 0; i < num; ++i) {
    while (!((i2c_sr = I2C_GetStatus(I2C_S0)) & I2C_SR_TXE));
    I2C_S0->TXR = *p++;
  }
  return true;
}

static int receive(unsigned char *p, unsigned int num, bool blocking)
{
  uint32_t i2c_sr;
  int cnt = 0;
  while (cnt < num) {
    while (!((i2c_sr = I2C_GetStatus(I2C_S0)) & I2C_SR_RXNE));
    *p++ = I2C_S0->RXR;
    ++cnt;
  }
  return cnt;
}

static void setup()
{
  SYS_SetPLLFreq(BOARD_PLL_FREQUENCY);
  SYS_SwitchPLLClock(board_hse_source());
}

static void setupBoot()
{
  *I2C_S0_OAR = 0x4a;
  I2C_Enable(I2C_S0);
  // This I2C slave can operate under any baud rate. But the SCL frequency decides the filter cycles.
  I2C_Init(I2C_S0, 400000);
}

static void resetBoot()
{
  I2C_WaitForBusy(I2C_S0);
  I2C_Disable(I2C_S0);
}

static void processBoot()
{
}

#else
# error("Unknown BOOT_INTERFACE, must be either 1 (UART) or 2 (USB), or 3 (I2C)")
#endif //BOOT_INTERFACE

static bool sendCh(unsigned char ch)
{
  return send(&ch, 1);
}
static bool receiveCh(unsigned char *ch, bool blocking)
{
  return receive(ch, 1, blocking);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

static void sendACK()
{
  sendCh(BOOT_ACK);
}
static void sendNACK()
{
  sendCh(BOOT_NACK);
}
static void sendBUSY()
{
  sendCh(BOOT_BUSY);
}

static void waitAndResetDevice()
{
  resetBoot();
  SYS_SoftwareReset();
} 

static BootByte write_mem_buf[2][BOOT_MAX_LEN_WE];
static int write_mem_idx;
static DMAC_LLI_TypeDef dma_write_flash;
static DMAC_LLI_TypeDef dma_write_disable;
static uint32_t FLASH_CR_DISABLE = FLASH_CR_OPTWRE; // Will disable FLASH_CR_PG and FLASH_CR_FASTPG
void bootBegin()
{
  dbg_printf("bootBegin\n");

  PERIPHERAL_ENABLE(DMAC, 0);
  DMAC_Init();
  
  write_mem_idx = 0;
  // Use dma_write_disable to disable FLASH_CR_FASTPG at the end of a DMA write. FLASH_CR_PG will most likely remain 
  // effective because it cannot be modified while flash is busy (and that is ok).
  dma_write_flash.LLI       = (uint32_t)&dma_write_disable;
  dma_write_disable.SrcAddr = (uint32_t)&FLASH_CR_DISABLE;
  dma_write_disable.DstAddr = (uint32_t)&FLASH->CR;
  dma_write_disable.Control = DMAC_MakeControl(DMAC_ADDR_INCR_ON, DMAC_ADDR_INCR_ON, DMAC_WIDTH_32_BIT, DMAC_WIDTH_32_BIT,
                                           DMAC_BURST_1, DMAC_BURST_1, 1, DISABLE);
  dma_write_disable.LLI     = 0;

  setupBoot();
  FLASH_OptionByteUnlock();
}

void bootEnd()
{
  dbg_printf("bootEnd\n");
  if (jump_to_addr != (BootAddr)(-1)) {
    dbg_printf("Jump to 0x%x\n", jump_to_addr);
  }

  resetBoot();
  SYS_AHBResetAndDisable(AHB_MASK_DMAC0);
  SYS_SetHSIConfig(hsi_config);

#ifdef BOOT_DEBUG
  UTIL_StopLogger();
#endif

  if (jump_to_addr != (BootAddr)(-1)) {
    UTIL_JumpToAddress(jump_to_addr);
  }
}

void bootTask()
{
  processBoot();
}

static BootErrType waitForCmd(BootByte *cmd)
{
  BootByte xmd;
  receiveCh(cmd, true);
  receiveCh(&xmd, true);

  dbg_printf(" Cmd 0x%x 0x%x 0x%x\n", *cmd, xmd, (*cmd)^xmd);
  if (*cmd+xmd==0xFF) {
    return BOOT_ERR_OK;
  } else {
    sendNACK();
    return BOOT_ERR_NACK;
  }
}

#define CONVERT_FROM_WORD(__word, __vals) \
  { \
    (__vals)[0] = (__word>>24)&0xFF; \
    (__vals)[1] = (__word>>16)&0xFF; \
    (__vals)[2] = (__word>>8 )&0xFF; \
    (__vals)[3] = (__word    )&0xFF; \
  }
#define CONVERT_FROM_HALF(__half, __vals) \
  { \
    (__vals)[0] = (__half>>8)&0xFF; \
    (__vals)[1] = (__half   )&0xFF; \
  }
#define CONVERT_TO_WORD(__vals) \
  (BootWord)(__vals[0]<<24 | (__vals[1]<<16) | (__vals[2]<<8) | __vals[3])
#define CONVERT_TO_HALF(__vals) \
  (BootHalf)((__vals[0]<<8) | __vals[1])

static BootErrType waitForData(BootByte *vals, BootLen len)
{
  receive(vals, len, true);
  BootByte xor = 0x00;
  for (int nn = 0; nn < len; ++nn) {
    xor ^= vals[nn];
  }
  if (len == 1) {
    xor ^= 0xFF;
  }

  BootByte crc;
  receiveCh(&crc, true);
  if (xor == crc) {
    return BOOT_ERR_OK;
  } else {
    sendNACK(); return BOOT_ERR_NACK;
  }
}

static BootErrType waitForDataX(BootByte *vals, BootLen max_len, BootLen *len)
{
  BootByte len0 = 0;
  receiveCh(&len0, true); *len = len0;
  ++(*len);

  BootByte xor = 0x00;
  xor ^= len0;
  if (*len <= max_len) {
    receive(vals, *len, true);
    for (int nn=0; nn < *len; ++nn) {
      xor ^= vals[nn];
    }
  } else {
    xor ^= 0xFF;
  }

  BootByte crc;
  receiveCh(&crc, true);
  if (xor == crc) {
    return BOOT_ERR_OK;
  } else {
    sendNACK(); return BOOT_ERR_NACK;
  }
}

static BootErrType waitForDataXExtended1(BootHalf *vals, BootWord max_len, BootWord *len)
{
  BootByte lens[2];
  receive(lens, 2, true);
  *len = CONVERT_TO_HALF(lens);
  ++(*len);

  BootByte xor = 0x00;
  xor ^=lens[0]; xor ^= lens[1];
  if (*len <= max_len) {
    BootByte val0s[BOOT_MAX_LEN_EE];
    receive(val0s, (*len)*2, true);
    for (int nn=0; nn < *len; ++nn) {
      BootByte *val0  = val0s+nn*2;
      vals[nn] = CONVERT_TO_HALF(val0);
      xor ^= val0[0]; xor ^= val0[1];
    }
  }

  BootByte crc;
  receiveCh(&crc, true);
  if (xor == crc) {
    return BOOT_ERR_OK;
  } else {
    sendNACK(); return BOOT_ERR_NACK;
  }
}

static BootErrType waitForDataXExtended2(BootByte *vals, BootWord max_len, BootWord *len)
{
  BootByte lens0[2];
  receive(lens0, 2, true);
  *len = CONVERT_TO_HALF(lens0);
  ++(*len);

  BootByte xor = 0x00;
  xor ^=lens0[0]; xor ^= lens0[1];
  if (*len <= max_len) {
    receive(vals, (*len), true);
    for (int nn=0; nn < *len; ++nn) {
      xor ^= vals[nn];
    }
  }

  BootByte crc;
  receiveCh(&crc, true);
  if (xor == crc) {
    return BOOT_ERR_OK;
  } else {
    sendNACK(); return BOOT_ERR_NACK;
  }
}

static BootErrType waitForLen(BootLen *len)
{
  BootByte len0;
  BootErrType ret = waitForData(&len0, 1);
  *len = len0;
  return ret;
}

static BootErrType waitForLenExtended(BootWord *len)
{
  BootByte lens0[2];
  BootErrType ret = waitForData(lens0, 2);
  *len = CONVERT_TO_HALF(lens0);
  return ret;
}

static BootErrType waitForAddr(BootAddr *addr)
{
  BootByte addrs[4];
  if (waitForData(addrs, SIZE_OF(addrs)) != BOOT_ERR_OK) {
    return BOOT_ERR_NACK;
  }
  *addr = CONVERT_TO_WORD(addrs);
  return BOOT_ERR_OK;
}

static BootErrType sendValues(BootByte *vals, uint32_t len, bool var_len)
{
  if (var_len) { // variable length
    sendCh(len-1);
  }
  send(vals, len);
  return BOOT_ERR_OK;
}

static void flashStatusReset()
{
  FLASH->SR = FLASH_SR_EOP|FLASH_SR_PGERR|FLASH_SR_WRPRTERR;
}

static BootErrType getInfomationCmd()
{
  dbg_printf("getInfomationCmd\n");
  sendACK();
  BootByte vals[15];
  vals[0] = BOOT_VERSION+BOOT_INTERFACE-1;
  vals[1] = BOOT_CMD_GET;
  vals[2] = BOOT_CMD_GVR;
  vals[3] = BOOT_CMD_GID;
#if BOOT_RE_EXTENDED>1
  vals[4] = BOOT_CMD_RE;
#else
  vals[4] = BOOT_CMD_RM;
#endif
#if BOOT_WE_EXTENDED>1
  vals[5] = BOOT_CMD_WE;
#else
  vals[5] = BOOT_CMD_WM;
#endif
  vals[6] = BOOT_CMD_EE;
  vals[7] = BOOT_CMD_GO;
  vals[8] = BOOT_CMD_WP;
  vals[9] = BOOT_CMD_UW;
  vals[10] = BOOT_CMD_RP;
  vals[11] = BOOT_CMD_UR;
  vals[12] = BOOT_CMD_RESET;
  vals[13] = BOOT_CMD_OO;
  vals[14] = BOOT_CMD_CRC;

  sendValues(vals, SIZE_OF(vals), true);
  sendACK();
  dbg_printf("getInfomationCmd done!\n");
  return BOOT_ERR_OK;
}

static BootErrType getVersionCmd()
{
  dbg_printf("getVersionCmd\n");
  sendACK();
  BootByte vals[3];
  vals[0] = BOOT_VERSION+BOOT_INTERFACE-1;
  vals[1] = 0x0;
  vals[2] = 0x0;
  checkFlashBusy();
  CONVERT_FROM_HALF(FLASH_OB->RDP, vals+1);
  sendValues(vals, SIZE_OF(vals), false);
  sendACK();
  dbg_printf("getVersionCmd done!\n");
  return BOOT_ERR_OK;
}

static BootErrType getDeviceIDCmd()
{
  dbg_printf("getDeviceIDCmd\n");
  sendACK();
  BootWord device_id;
  device_id = SYS_GetDeviceID();
  BootByte vals[4];
  CONVERT_FROM_WORD(device_id, vals);
  sendValues(vals, SIZE_OF(vals), true);
  sendACK();
  dbg_printf("getDeviceIDCmd done!\n");
  return BOOT_ERR_OK;
}

static BootErrType resetDeviceCmd()
{
  dbg_printf("resetDeviceCmd\n");
  sendACK();
  sendACK();
  waitAndResetDevice();
  return BOOT_ERR_OK;
}

static void bootWriteDMA(BootAddr addr, BootLen len, const BootByte *vals)
{
  // Use DMA for any memory write. The extra flash operations are unnecessary for ram writes, but do not harm.
  DMAC_WaitChannel(WRITE_MEM_DMA);
  flashStatusReset();
  FLASH->CR |= (FLASH_CR_PG | FLASH_CR_OPTPG | FLASH_CR_FASTPG);

  // Align address and vals to 4 so that 32-bit DMA can be used.
  while (addr % 4) {
    *((volatile BootByte *)addr++) = *vals++;
    --len;
  }
  if (len >= 4) {
    __attribute__((aligned(4))) BootByte buf[len];
    uint32_t src_addr = (uint32_t)vals;
    if (src_addr % 4) {
      memcpy(buf, vals, len);
      src_addr = (uint32_t)buf;
    }
    dma_write_flash.SrcAddr = src_addr;
    dma_write_flash.DstAddr = addr;
    dma_write_flash.Control = DMAC_MakeControl(DMAC_ADDR_INCR_ON, DMAC_ADDR_INCR_ON, DMAC_WIDTH_32_BIT, DMAC_WIDTH_32_BIT,
                                               DMAC_BURST_256, DMAC_BURST_256, len / 4, DISABLE);
    DMAC_ConfigLLI(WRITE_MEM_DMA, DMAC_MEM_TO_MEM_DMA_CTRL, 0, 0, (uint32_t)&dma_write_flash);
  }
  if (len % 4) {
    DMAC_WaitChannel(WRITE_MEM_DMA);
    FLASH->CR |= (FLASH_CR_PG | FLASH_CR_OPTPG | FLASH_CR_FASTPG);
    BootAddr addr_end = addr + len;
    addr += (len / 4 * 4);
    const BootByte *ptr = vals + (len / 4 * 4);
    while (addr < addr_end) {
      *((volatile BootByte *)addr) = *ptr;
      ++addr;
      ++ptr;
    }
    FLASH_DisableProgram();
  }
}

static void bootReadAligned(
    BootAddr addr, BootLen len, BootByte *vals)
{
  memcpy((void *)vals, (void *)addr, len);
}

static void flashOptErase()
{
  FLASH->CR |= FLASH_CR_OPTER|FLASH_CR_STRT;
  FLASH_EnableOptionByteProgram();
  checkFlashBusy();
  FLASH_DisableProgram();
  FLASH->CR &= ~(FLASH_CR_OPTER);
  FLASH_OptionByteUnlock();
  dbg_printf("  Erase Flash Option Bytes!!!\n");
}

static BootByte *checkFlashOpt(BootAddr addr, BootLen len, const BootByte *vals)
{
    static BootByte opt_buf[BOOT_OPT_LEN];
    memcpy(opt_buf, (void *)FLASH_OPTION_BASE, BOOT_OPT_LEN);

    if ((FLASH_OPTION_BASE+BOOT_OPT_LEN) < (addr+len)) {
      return NULL; // prevent buffer overflow
    }

    BootAddr rel_addr = addr - FLASH_OPTION_BASE;
#if 0 // do not check, always update option bytes
    if (memcmp(opt_buf+rel_addr, vals, len) == 0) { // no change
        return NULL;
    }
#endif
    
    BootHalf rdp_word = RDP_WORD;
    memcpy(opt_buf+0, &rdp_word, sizeof(BootHalf)); // force unprotected after option erase
    memcpy(opt_buf+rel_addr, vals, len);
    return opt_buf;
}

static BootErrType bootWriteMemory(BootAddr addr, BootLen len, const BootByte *vals)
{
  dbg_printf(" bootWriteMemory 0x%x %d\n", addr, len);
  bootWriteDMA(addr, len, vals);
  sendACK();
  return BOOT_ERR_OK;
}

static BootErrType bootReadMemory(BootAddr addr, BootLen len, BootByte *vals)
{
  dbg_printf(" bootReadMemory 0x%x %d\n", addr, len);
  bootReadAligned(addr, len, vals);
  return BOOT_ERR_OK;
}

static bool checkReadProtect()
{
  if (FLASH_IsReadProtected()) {
    dbg_printf("  Noops, memory in read protect mode.\n");
    return false;
  }
  return true;
}

#if 0
static bool checkOptPGable(uint8_t offset, XLEN_TYPE mask, XLEN_TYPE val)
{
  return (RD_REG(FLASH_OPTION_BASE+offset) & mask & val) == val;
}

static bool checkOptPGed(uint8_t offset, XLEN_TYPE mask, XLEN_TYPE val)
{
  return (RD_REG(FLASH_OPTION_BASE+offset) & mask) == val;
}
#endif

static BootHalf getComplement(BootByte data)
{
  return data == 0xFF ? 0xFFFF : (~(BootHalf)data << 8) | data;
}

static void bootWriteProtect(BootLen len, BootByte *vals)
{
  checkFlashBusy();
  flashStatusReset();
  FLASH_EnableOptionByteProgram();
  BootWord wrpr = FLASH->WRPR;
  for (int nn=0; nn < len; ++nn) {
    BootByte val = vals[nn];
    if (val > 31) { val = 31; }
    dbg_printf(" #%d 0x%x %d %d\n", nn, val*FLASH_SECTOR_SIZE+FLASH_BASE, vals[nn], val);
    wrpr &= ~(0x01 << val);
  }
  if (FLASH->WRPR == wrpr) { // Do nothing if nothing changed
    sendACK(); return;
  }
  // Do not trigger optionbytes erase if it is in initial state (0xFFFF)
  if (FLASH->WRPR != 0xFFFFFFFF) {
    flashOptErase();
  }
  for (int ii=0; ii < 4; ++ii) {
    *((volatile BootHalf*)(FLASH_OPTION_BASE+8+(ii*2))) = getComplement((wrpr>>(ii*8))&0xFF);
  }
  checkFlashBusy();
  FLASH_DisableProgram();
  sendACK();
  return;
}

static void bootWriteUnprotect()
{
  // Use case 2, keep read protection
  checkFlashBusy();
  flashStatusReset();
  if (FLASH->WRPR == 0xFFFFFFFF) {
    sendACK(); return;
  }
  flashOptErase();
  sendACK();
#if 0 // case 1, read unprotect
  FLASH_EnableOptionByteProgram();
//*(BootHalf *)(FLASH_OPTION_BASE+0) = RDP_WORD;
  FLASH_OB->RDP = RDP_WORD;
  checkFlashBusy();
  FLASH_DisableProgram();
#endif
}

static void bootReadProtect()
{
  checkFlashBusy();
  flashStatusReset();
  // Don't erase option bytes here!
  FLASH_EnableOptionByteProgram();
  // Write 0x0000 to do protection since 0 can be always programmed regardless of current value. Also writing anything
  // else will trigger an auto erase of the entire flash.
//*(BootHalf *)(FLASH_OPTION_BASE+0) = 0x0000;
  FLASH_OB->RDP = 0x0000;
  checkFlashBusy();
  FLASH_DisableProgram();
  sendACK();
}

static void bootReadUnprotect()
{
  checkFlashBusy();
  flashStatusReset();
  if (FLASH_OB->RDP == RDP_WORD) {
    sendACK(); return;
  }
  if ((FLASH_OB->RDP & RDP_WORD) != RDP_WORD) {
    flashOptErase();
  }
  FLASH_EnableOptionByteProgram();
  FLASH_OB->RDP = RDP_WORD;
  checkFlashBusy();
  FLASH_DisableProgram();
  sendACK();
}

static void bootErsaeOptionBytes()
{
  checkFlashBusy();
  flashStatusReset();
  flashOptErase();
  checkFlashBusy();
  sendACK();
}

static void bootEraseAll()
{
  dbg_printf(" bootEraseAll\n");
  if (FLASH->WRPR != 0xFFFFFFFF) {
    return;
  }
  checkFlashBusy();
  flashStatusReset();
  FLASH->CR |= FLASH_CR_MER|FLASH_CR_STRT;
  checkFlashBusy();
  FLASH->CR &= ~(FLASH_CR_MER);
}

bool isSectorProtected(BootHalf sector)
{
  return (FLASH->WRPR & (1 << (sector < 31 ? sector : 31))) == 0;
}

bool isBlockProtected(BootHalf block)
{
  for (BootHalf sector = block * FLASH_BLOCK_SIZE / FLASH_SECTOR_SIZE; sector < (block + 1) * FLASH_BLOCK_SIZE / FLASH_SECTOR_SIZE; ++sector) {
    if (isSectorProtected(sector)) {
      return true;
    }
  }
  return false;
}

static void bootErase(BootWord len, BootHalf *vals)
{
  dbg_printf(" bootErase %d\n", len);
  checkFlashBusy();
  flashStatusReset();
  for (int nn=0; nn < len; ++nn) {
    BootHalf val = vals[nn];
    if (val & 0x8000) { // block erase
      val &= ~0x8000;
      if (isBlockProtected(val)) {
        continue;
      }
      dbg_printf(" #%d block 0x%x\n", nn, val*FLASH_BLOCK_SIZE+FLASH_BASE);
      FLASH->CR |= FLASH_CR_BER;
      FLASH->AR = val*FLASH_BLOCK_SIZE+FLASH_BASE;
      FLASH->CR |= FLASH_CR_STRT;
      checkFlashBusy();
      FLASH->CR &= ~(FLASH_CR_BER);
    } else {
      if (isSectorProtected(val)) {
        continue;
      }
      dbg_printf(" #%d sector 0x%x\n", nn, val*FLASH_SECTOR_SIZE+FLASH_BASE);
      FLASH->CR |= FLASH_CR_SER;
      FLASH->AR = val*FLASH_SECTOR_SIZE+FLASH_BASE;
      FLASH->CR |= FLASH_CR_STRT;
      checkFlashBusy();
      FLASH->CR &= ~(FLASH_CR_SER);
    }
  }
}

static void bootEraseBlock(BootWord len, BootHalf *vals)
{
  dbg_printf(" bootEraseBlock %d\n", len);
  checkFlashBusy();
  flashStatusReset();
  FLASH->CR |= FLASH_CR_BER;
  for (int nn=0; nn < len; ++nn) {
    dbg_printf(" #%d 0x%x\n", nn, vals[nn] * FLASH_BLOCK_SIZE);
    FLASH->AR = vals[nn] * FLASH_BLOCK_SIZE;
    FLASH->CR |= FLASH_CR_STRT;
    checkFlashBusy();
  }
  FLASH->CR &= ~(FLASH_CR_BER);
}

static BootErrType readMemoryCmd(bool is_extended)
{
  dbg_printf("readMemoryCmd\n");
  if (!checkReadProtect()) {
    sendNACK(); return BOOT_ERR_NACK;
  }
  DMAC_WaitChannel(WRITE_MEM_DMA);
  checkFlashBusy();
  sendACK();
  BootAddr addr;
  if (waitForAddr(&addr) != BOOT_ERR_OK) {
    return BOOT_ERR_NACK;
  }
  sendACK();

  BootWord len;
  if (is_extended) {
    if (waitForLenExtended(&len) != BOOT_ERR_OK) {
      return BOOT_ERR_NACK;
    }
  } else {
    BootLen len0;
    if (waitForLen(&len0) != BOOT_ERR_OK) {
      return BOOT_ERR_NACK;
    }
    len = len0;
  }
  ++len;
  if (len & 0x0 || len > BOOT_MAX_LEN_RE) { // 1 byte aligned
    sendNACK(); return BOOT_ERR_NACK;
  }
  sendACK();

  BootByte vals[BOOT_MAX_LEN_RE];
  bootReadMemory(addr, len, vals);
  sendValues(vals, len, false); 
  dbg_printf("readMemoryCmd done!\n");
  return BOOT_ERR_OK;
}

static BootErrType writeMemoryCmd(bool is_extended)
{
  dbg_printf("writeMemoryCmd\n");
  if (!checkReadProtect()) {
    sendNACK(); return BOOT_ERR_NACK;
  }
  sendACK();
  BootAddr addr;
  if (waitForAddr(&addr) != BOOT_ERR_OK) {
    return BOOT_ERR_NACK;
  }
  sendACK();

  BootWord len;
  if (is_extended) {
    if (waitForDataXExtended2(write_mem_buf[write_mem_idx], BOOT_MAX_LEN_WE, &len) != BOOT_ERR_OK) {
      return BOOT_ERR_NACK;
    }
  } else {
    BootLen len0;
    if (waitForDataX(write_mem_buf[write_mem_idx], BOOT_MAX_LEN_WE, &len0) != BOOT_ERR_OK) {
      return BOOT_ERR_NACK;
    }
    len = len0;
  }
  if (len & 0x0 || len > BOOT_MAX_LEN_WE) { // relax alignment requirement
    sendNACK(); return BOOT_ERR_NACK;
  }
  bootWriteMemory(addr, len, write_mem_buf[write_mem_idx]);
  write_mem_idx = 1 - write_mem_idx;
  dbg_printf("writeMemoryCmd done!\n");
  return BOOT_ERR_OK;
}

static BootErrType goCommandCmd()
{
  dbg_printf("goCommandCmd\n");
  if (!checkReadProtect()) {
    sendNACK(); return BOOT_ERR_NACK;
  }
  sendACK();
  if (waitForAddr(&jump_to_addr) != BOOT_ERR_OK) {
    return BOOT_ERR_NACK;
  }
  sendACK();
  dbg_printf("goCommandCmd done!\n");
  return BOOT_ERR_OK;
}

static bool checkSectorProtected(int sector_id)
{
  if (sector_id > 31) {
    sector_id = 31;
  }
  if ((FLASH->WRPR & (0x01 << sector_id)) == 0) { // the sector is protected
    return true;
  }
  return false;
}

static bool checkMemoryErasable(BootWord len, BootHalf *vals)
{
  static const int flash_sectors_per_block = FLASH_BLOCK_SIZE/FLASH_SECTOR_SIZE;

  if (!vals) { // Erase ALL
    if (FLASH->WRPR != 0xFFFFFFFF) {
      return false;
    }
  } else { // block or sector based
    for (int nn=0; nn < len; ++nn) {
      BootHalf val = vals[nn];
      if (val & 0x8000) { // block erase
        int block_id = val & ~0x8000;
        for (int nn = 0; nn < flash_sectors_per_block; ++nn) {
          int sector_id = block_id*flash_sectors_per_block+nn;
          if (checkSectorProtected(sector_id)) {
            return false;
          }
        }
      } else {
        int sector_id = val;
        if (checkSectorProtected(sector_id)) {
          return false;
        }
      }
    }
  }
  return true;
}

static BootErrType eraseMemoryExtendedCmd()
{
  dbg_printf("eraseMemoryExtendedCmd\n");
  if (!checkReadProtect()) {
    sendNACK(); return BOOT_ERR_NACK;
  }
  sendACK();
  BootWord len;
  BootHalf vals[BOOT_MAX_LEN_EE-1];
  if (waitForDataXExtended1(vals, BOOT_MAX_LEN_EE-1, &len) != BOOT_ERR_OK) {
    return BOOT_ERR_NACK;
  }
  if (len >= BOOT_MAX_LEN_EE) {
    if (!checkMemoryErasable(0, 0)) {
      sendNACK(); return BOOT_ERR_NACK;
    }
    bootEraseAll();
  } else {
    if (len%1 != 0) {
      sendNACK(); return BOOT_ERR_NACK;
    }
    if (!checkMemoryErasable(len, vals)) {
      sendNACK(); return BOOT_ERR_NACK;
    }
    bootErase(len, vals);
  }
  sendACK();
  dbg_printf("eraseMemoryExtendedCmd done!\n");
  return BOOT_ERR_OK;
}

static BootErrType writeProtectCmd()
{
  dbg_printf("writeProtectCmd\n");
  if (!checkReadProtect()) {
    sendNACK(); return BOOT_ERR_NACK;
  }
  sendACK();
  BootLen len;
  BootByte vals[BOOT_MAX_LEN];
  if (waitForDataX(vals, BOOT_MAX_LEN, &len) != BOOT_ERR_OK) {
    return BOOT_ERR_NACK;
  }
  if (len%1 != 0 || len > BOOT_MAX_LEN) {
    sendNACK(); return BOOT_ERR_NACK;
  }
  bootWriteProtect(len, vals);
  dbg_printf("writeProtectCmd done!\n");
  waitAndResetDevice();
  return BOOT_ERR_OK;
}

static BootErrType writeUnprotectCmd()
{
  dbg_printf("writeUnprotectCmd\n");
  if (!checkReadProtect()) {
    sendNACK(); return BOOT_ERR_NACK;
  }
  sendACK();
  bootWriteUnprotect();
  dbg_printf("writeUnprotectCmd done!\n");
  waitAndResetDevice();
  return BOOT_ERR_OK;
}

static BootErrType readProtectCmd()
{
  dbg_printf("readProtectCmd\n");
  if (!checkReadProtect()) {
    sendNACK(); return BOOT_ERR_NACK;
  }
  sendACK();
  bootReadProtect();
  dbg_printf("readProtectCmd done!\n");
  waitAndResetDevice();
  return BOOT_ERR_OK;
}

static BootErrType readUnprotectCmd()
{
  dbg_printf("readUnprotectCmd\n");
  sendACK();
  bootReadUnprotect();
  dbg_printf("readUnprotectCmd done!\n");
  waitAndResetDevice();
  return BOOT_ERR_OK;
}

static BootErrType eraseOptionBytesCmd()
{
  dbg_printf("eraseOptionBytesCmd\n");
  sendACK();
  bootErsaeOptionBytes();
  dbg_printf("eraseOptionBytesCmd done!\n");
#if 0 // unlike protect/unproctect, do not reset device automatically, let host to reset it explicitly
  waitAndResetDevice();
#endif
  return BOOT_ERR_OK;
}

static BootErrType calculateCrcCmd()
{
  dbg_printf("calculateCrcCmd\n");
  if (!checkReadProtect()) {
    sendNACK(); return BOOT_ERR_NACK;
  }
  sendACK();
  BootAddr addr;
  if (waitForAddr(&addr) != BOOT_ERR_OK) {
    return BOOT_ERR_NACK;
  }
  sendACK();

  BootAddr len;
  if (waitForAddr(&len) != BOOT_ERR_OK) {
    return BOOT_ERR_NACK;
  }

  PERIPHERAL_CRC_ENABLE(CRC0);
  CRC_Reset(CRC0);
  CRC_SetEndian(CRC0, CRC_CR_LITTLE_ENDIAN);

  uint32_t size = ((addr | len) & 0x3) == 0 ? 4 : 1;
  DMAC_WidthTypeDef width = size == 4 ? DMAC_WIDTH_32_BIT : DMAC_WIDTH_8_BIT;
  DMAC_WaitChannel(CALCU_CRC_DMA);
  DMAC_WaitedTransfer(CALCU_CRC_DMA, addr, (uint32_t)&CRC0->DR, DMAC_ADDR_INCR_ON, DMAC_ADDR_INCR_OFF,
                      width, width, DMAC_BURST_256, DMAC_BURST_256, len / size,
                      DMAC_MEM_TO_MEM_DMA_CTRL, 0, 0);
  sendACK();
  uint32_t crc = CRC_ReadData32(CRC0);
  sendValues((BootByte *)&crc, 4, false);

  dbg_printf("calculateCrcCmd done!\n");
  return BOOT_ERR_OK;
}

static BootErrType processCmd()
{
  dbg_printf("processCmd...");
  BootByte cmd;
  BootErrType err = waitForCmd(&cmd);
  if (err != BOOT_ERR_OK) {
    return err;
  }
  switch (cmd) {
  case BOOT_CMD_INIT:
    sendACK();
    err = BOOT_ERR_OK;
    break;
  case BOOT_CMD_GET:
    err = getInfomationCmd();
    break;
  case BOOT_CMD_GVR:
    err = getVersionCmd();
    break;
  case BOOT_CMD_GID:
    err = getDeviceIDCmd();
    break;

  case BOOT_CMD_RM:
  case BOOT_CMD_RE:
    err = readMemoryCmd(cmd==BOOT_CMD_RE);
    break;
  case BOOT_CMD_WM:
  case BOOT_CMD_WE:
    err = writeMemoryCmd(cmd==BOOT_CMD_WE);
    break;
  case BOOT_CMD_EE:
    err = eraseMemoryExtendedCmd();
    break;
  case BOOT_CMD_GO:
    err = goCommandCmd();
    break;
  case BOOT_CMD_WP:
    err = writeProtectCmd();
    break;
  case BOOT_CMD_UW:
    err = writeUnprotectCmd();
    break;
  case BOOT_CMD_RP:
    err = readProtectCmd();
    break;
  case BOOT_CMD_UR:
    err = readUnprotectCmd();
    break;
  case BOOT_CMD_OO:
    err = eraseOptionBytesCmd();
    break;
  case BOOT_CMD_CRC:
    err = calculateCrcCmd();
    break;

  case BOOT_CMD_ER:
  case BOOT_CMD_EE_NS:
  case BOOT_CMD_WM_NS:
  case BOOT_CMD_WP_NS:
  case BOOT_CMD_UW_NS:
  case BOOT_CMD_RP_NS:
  case BOOT_CMD_UR_NS:
    sendNACK();
    err = BOOT_ERR_NO_CMD;
    break;

  case BOOT_CMD_RESET:
    err = resetDeviceCmd();
    break;

  case BOOT_CMD_ERR:
  default:
    sendNACK();
    err = BOOT_ERR_UNKNOWN;
    break;
  }

  return err;
}

/////////////////////////////////////////////////////////////////////
void boot()
{
  jump_to_addr = (BootAddr)(-1);

  setup();

#ifdef BOOT_DEBUG
  // Enable LOGGER_UART
  UTIL_StartLogger();
  dbg_printf("\nBOOT\n.\n..\n...\n");
  dbg_printf("  Clock Frequency : %dmhz\n", SYS_GetSysClkFreq()/1000000);
  dbg_printf("  Option Byte Error %d\n", FLASH_IsOptionByteError()); 
  dbg_printf("  Read  Protection %d\n", FLASH_IsReadProtected()); 
  dbg_printf("  Write Protection 0x%x\n",  FLASH->WRPR);
#else
  MSG_UART = 0;
#endif

  dbg_printf("BOOTing...\n");
  bootBegin();

  while (1) {
    bootTask();
    processCmd();
    if (jump_to_addr != (BootAddr)(-1)) {
      break;
    }
  }

  dbg_printf("BOOTed!!!\n");
  bootEnd();
}
