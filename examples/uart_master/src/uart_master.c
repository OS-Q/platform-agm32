#include "uart_master.h"

#define SRAM_BASE          (0x20000000)
#define FLASH_BASE         (0x80000000)
#define FLASH_SECTOR_SIZE  (0x1000)  // 4K
#define FLASH_BLOCK_SIZE   (0x10000) // 64K
#define FLASH_OPTION_BASE  (0x81000000)
#define FLASH_OPTION_SIZE  (128)
#define FLASH_RDP_WORD     ((uint16_t)0x5AA5)

#define FLASH_SR        (0x4000100C)
#define FLASH_CR        (0x40001010)
#define FLASH_AR        (0x40001014)
#define FLASH_READ_CTRL (0x4000102C)
#define FLASH_READ_DATA (0x40001030)
#define FLASH_CR_STRT   (1 << 6)
#define FLASH_CR_READ   (1 << 14)
#define FLASH_SR_BSY    (1 << 0)

static uint32_t flash_size;
static bool logic_compress;
#define COMPRESSED_SIZE   (0xc000) // 48KB
#define FLASH_CONFIG_SIZE (logic_compress ? COMPRESSED_SIZE : 0x19000)
#ifndef FLASH_CONFIG_ADDR
#define FLASH_CONFIG_ADDR (FLASH_BASE + flash_size - FLASH_CONFIG_SIZE)
#endif
#define FLASH_LOGIC       (FLASH_CONFIG_ADDR + 0xb00)
#define FLASH_DECOMP      (FLASH_CONFIG_ADDR)

#define SYS_MISC 0x03000018
#define UART_BRD 0x40025024

#ifndef MASTER_BAUD
#define MASTER_BAUD 1000000
#endif

extern int printf(const char* fmt, ...);
#ifndef PRINT
#define PRINT(...)
#endif

#define ENSURE(...) \
  if (!(__VA_ARGS__)) { \
    return false; \
  }

#define INIT          10
#define TIMEOUT       20
#define WRITE_TIMEOUT 10
#define ERASE_TIMEOUT 20
#define READ_TIMEOUT  10

#define JUMP_WAIT 10 // In ms

#define DFU_MAX_FRAME   256
#define DFU_BLOCK_ERASE 0x8000 // Special flag in page number

#define DFU_ACK  0x79
#define DFU_NACK 0x1F
#define DFU_BUSY 0x76

#define DFU_CMD_GET   0x00
#define DFU_CMD_RM    0x11 // read memory
#define DFU_CMD_RE    0x13 // extended read memory
#define DFU_CMD_GO    0x21
#define DFU_CMD_WM    0x31 // write memory
#define DFU_CMD_WE    0x33 // extended write memory
#define DFU_CMD_EE    0x44 // extended erase
#define DFU_CMD_UR    0x92 // read unprotect
#define DFU_CMD_CRC   0xA1
#define DFU_CMD_RESET 0xA2
#define DFU_CMD_OO    0xA3 // Option erase

#define DFU_MMODE_MEM 0x00

#define DFU_MIN_VERSION 0x20
// These are used when target version is less than DFU_MIN_VERSION
#ifndef DFU_SLOW_BAUD
#define DFU_SLOW_BAUD 200000
#endif
static uint8_t uart_version;
static const uint8_t dfu_uart_buf[] = {
#include "dfu_uart.inc"
};
static const uint32_t dfu_uart_addr = 0x20008000;
static const uint32_t sram_decomp_addr = 0x2000f000;
static const uint32_t sram_config_addr = 0x20010000;

static const uint8_t sram_decomp_buf[] = {
#include "sram_decomp.inc"
};

static uint8_t fpga_decomp_buf[] __attribute__((aligned(4))) = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
#include "agrv32_fpga_decomp.inc"
};

static uint8_t program_buf[] = {
#ifndef DEFAULT_PROGRAM
// Default program from assembly:
// li t0, 0x03000000
// li t1, 0x1C
// sw t1, 0x14(t0)
// _wfi:
// wfi
// j _wfi
  0xb7,0x02,0x00,0x03,0x71,0x43,0x23,0xaa,0x62,0x00,0x73,0x00,0x50,0x10,0xf5,0xbf,
#else
#include DEFAULT_PROGRAM
#endif
};

static int  uart_timeout;
static int  uart_extended_rd;
static int  uart_extended_wr;
static bool uart_has_crc_cmd;
static bool uart_is_init;
static bool uart_silent;

#define DFU_MAX_RLEN (DFU_MAX_FRAME * uart_extended_rd)
#define DFU_MAX_WLEN (DFU_MAX_FRAME * uart_extended_wr)
#define UART_ERROR(__msg, __error) \
  if (!uart_silent) { \
    PRINT("Error: %s, 0x%x\n", __msg, __error); \
  } \
  return false;

#define REVERSE_BYTES(__data) ((uint16_t)(((__data) << 8) | ((__data) >> 8)))

bool uart_tx_ch(unsigned char ch)
{
  return uart_tx(&ch, 1);
}

bool uart_rx_ch(unsigned char *ch, int timeout)
{
  return uart_rx(ch, 1, timeout);
}

bool uart_get_ack(int timeout)
{
  unsigned char ch;
  if (uart_rx_ch(&ch, timeout) == 1) {
    if (ch == DFU_ACK) {
      PRINT("ACK\n");
      return true;
    }
  }
  UART_ERROR("NACK", ch);
}

bool uart_init_seq(uint32_t baud)
{
  uart_disable();
  uart_enable(baud);

  unsigned char ack;
  int timeout = 10000000 / baud;
  for (int init = 0; init < INIT; ++init) {
    uart_tx_ch(0x7f);
    if (uart_rx_ch(&ack, timeout) == 1 && ack == DFU_ACK) {
      uart_is_init = true;
      return true;
    }
  }
  UART_ERROR("init failed", ack);
}

bool uart_send_cmd(unsigned char cmd)
{
  PRINT("Send cmd: 0x%0x, ", cmd);
  unsigned char buf[2] = { cmd, ~cmd };
  uart_tx(buf, 2);
  if (!uart_get_ack(uart_timeout)) {
    UART_ERROR("send cmd", cmd);
  }
  return true;
}

bool uart_send_addr_timeout(uint32_t address, int timeout)
{
  PRINT("Send addr: 0x%0x, ", address);
  uint8_t buffer[5];
  buffer[0] = (address >> 24) & 0xff;
  buffer[1] = (address >> 16) & 0xff;
  buffer[2] = (address >>  8) & 0xff;
  buffer[3] = (address >>  0) & 0xff;
  buffer[4] = buffer[0] ^ buffer[1] ^ buffer[2] ^ buffer[3];
  uart_tx(buffer, 5);
  if (!uart_get_ack(timeout)) {
    UART_ERROR("send address", address);
  }
  return true;
}

bool uart_send_addr(uint32_t address)
{
  return uart_send_addr_timeout(address, uart_timeout);
}

bool uart_send_data16(uint32_t length, const uint16_t *data)
{
  if (!length) {
    return true;
  }
  PRINT("Send %d hwords, ", length);
  uint16_t buffer[length + 2];
  uint16_t checksum = length - 1;
  buffer[0] = REVERSE_BYTES(length - 1);
  for (int i = 0; i < length; ++i) {
    buffer[i + 1] = REVERSE_BYTES(data[i]);
    checksum ^= data[i];
  }
  buffer[length + 1] = checksum ^ (checksum >> 8);
  return uart_tx((const unsigned char *)buffer, (length + 1) * 2 + 1);
}

bool uart_send_bytes(uint32_t length, const uint8_t *data)
{
  if (!length) {
    return true;
  }
  PRINT("Send %d bytes, ", length);
  uint8_t checksum = length - 1;
  if (length <= DFU_MAX_FRAME) {
    uart_tx_ch(length - 1);
    if (!data) {
      checksum = ~checksum;
    }
  } else {
    uart_tx_ch((length - 1) >> 8);
    uart_tx_ch(length - 1);
    checksum ^= (length - 1) >> 8;
  }
  if (data) {
    uart_tx(data, length);
    for (int i = 0; i < length; ++i) {
      checksum ^= data[i];
    }
  }
  return uart_tx_ch(checksum);
}

bool uart_cmd_go(uint32_t address)
{
  ENSURE(uart_send_cmd(DFU_CMD_GO));
  ENSURE(uart_send_addr(address));
  uart_is_init = false;
  return true;
}

bool uart_cmd_extended_erase(int npage, const uint16_t *pages)
{
  ENSURE(uart_send_cmd(DFU_CMD_EE));
  ENSURE(uart_send_data16(npage, pages));
  return uart_get_ack(ERASE_TIMEOUT * npage);
}

bool uart_cmd_write_memory(uint32_t address, uint32_t length, const uint8_t *buffer)
{
  PRINT("DFU write mem: 0x%x, %d bytes\n", address, length);
  ENSURE(uart_send_cmd(length <= DFU_MAX_FRAME ? DFU_CMD_WM : DFU_CMD_WE));
  ENSURE(uart_send_addr(address));
  if (uart_version < DFU_MIN_VERSION) {
    ENSURE(uart_send_cmd(DFU_MMODE_MEM));
  }
  ENSURE(uart_send_bytes(length, buffer));
  if (!uart_get_ack(WRITE_TIMEOUT * length)) {
    UART_ERROR("write mem", address);
  }
  return true;
}

bool uart_cmd_read_memory(uint32_t address, uint32_t length, uint8_t *buffer)
{
  PRINT("DFU read mem: 0x%x, %d bytes\n", address, length);
  ENSURE(uart_send_cmd(length <= DFU_MAX_FRAME ? DFU_CMD_RM : DFU_CMD_RE));
  ENSURE(uart_send_addr(address));
  if (uart_version < DFU_MIN_VERSION) {
    ENSURE(uart_send_cmd(DFU_MMODE_MEM));
  }
  ENSURE(uart_send_bytes(length, NULL));
  if (!uart_get_ack(READ_TIMEOUT)) {
    UART_ERROR("read %d bytes", length);
  }
  return uart_rx(buffer, length, READ_TIMEOUT * length);
}

bool uart_cmd_reset()
{
  ENSURE(uart_send_cmd(DFU_CMD_RESET));
  ENSURE(uart_get_ack(READ_TIMEOUT));
  uart_is_init = false;
  return true;
}

bool uart_cmd_option_erase()
{
  ENSURE(uart_send_cmd(DFU_CMD_OO));
  ENSURE(uart_get_ack(ERASE_TIMEOUT));
  return true;
}

uint16_t flash_addr_to_page(uint32_t address)
{
  // Page number used in DFU actually means 4K sector
  return (address - FLASH_BASE) / FLASH_SECTOR_SIZE;
}

bool uart_erase_flash(uint32_t address, uint32_t length)
{
  if (!length) {
    return true;
  }
  uint32_t npage = 0;
  uint16_t pages[DFU_MAX_FRAME];
  uint32_t addr_begin = address & ~(FLASH_SECTOR_SIZE - 1);
  uint32_t addr_end   = (address + length + FLASH_SECTOR_SIZE - 1) & ~(FLASH_SECTOR_SIZE - 1); // Till next 4K boundary
  for (uint32_t addr = addr_begin; addr < addr_end; ) {
    if (address >= FLASH_OPTION_BASE && address < FLASH_OPTION_BASE + FLASH_OPTION_SIZE) {
      PRINT("option erase:\n");
      ENSURE(uart_cmd_option_erase());
      break;
    } else if ((addr & (FLASH_BLOCK_SIZE - 1)) == 0 && addr + FLASH_BLOCK_SIZE <= addr_end) {
      pages[npage] = flash_addr_to_page(addr) | DFU_BLOCK_ERASE;
      PRINT("block erase 0x%x, page 0x%x\n", addr, pages[npage]);
      addr += FLASH_BLOCK_SIZE;
    } else {
      pages[npage] = flash_addr_to_page(addr);
      PRINT("sector erase 0x%x, page 0x%x\n", addr, pages[npage]);
      addr += FLASH_SECTOR_SIZE;
    }

    ++npage;
    if (npage >= DFU_MAX_FRAME - 1 || addr >= addr_end) {
      PRINT("erase addr: 0x%x, length: 0x%x, from 0x%x to 0x%x, %d pages\n", address, length, addr_begin, addr_end, npage);
      ENSURE(uart_cmd_extended_erase(npage, pages));
      npage = 0;
    }
  }
  return true;
}

bool uart_read_memory(uint32_t address, int length, uint8_t *buffer)
{
  PRINT("Read memory 0x%x: length: %d\n", address, length);
  while (length > 0) {
    uint32_t dfu_length = length > DFU_MAX_RLEN ? DFU_MAX_RLEN : length;
    PRINT("  rd mem 0x%x, length: %d\n", address, dfu_length);
    ENSURE(uart_cmd_read_memory(address, dfu_length, buffer));
    address += dfu_length;
    buffer += dfu_length;
    length -= dfu_length;
  }
  return true;
}

bool uart_write_memory(uint32_t address, int length, const uint8_t *buffer)
{
  uint32_t orig_address = address;
  int orig_length = length;
  const uint8_t *orig_buffer = buffer;
  PRINT("Write memory 0x%x: length: %d\n", address, length);
  while (length > 0) {
    uint32_t dfu_length = length > DFU_MAX_WLEN ? DFU_MAX_WLEN : length;
    PRINT("  wr mem 0x%x, length: %d\n", address, dfu_length);
    ENSURE(uart_cmd_write_memory(address, dfu_length, buffer));
    address += dfu_length;
    buffer += dfu_length;
    length -= dfu_length;
  }
  // A write must be followed by a verify, otherwise the write may not finish properly.
  ENSURE(uart_verify_memory(orig_address, orig_length, orig_buffer));
  return true;
}

bool uart_verify_memory(uint32_t address, int length, const uint8_t *buffer)
{
  PRINT("Verify memory 0x%x: length: %d\n", address, length);
  uint8_t rd_buf[DFU_MAX_RLEN];
  while (length > 0) {
    uint32_t dfu_length = length > DFU_MAX_RLEN ? DFU_MAX_RLEN : length;
    PRINT("  rd mem 0x%x, length: %d\n", address, dfu_length);
    ENSURE(uart_cmd_read_memory(address, dfu_length, rd_buf));
    address += dfu_length;
    length -= dfu_length;
    const uint8_t *rd_ptr = rd_buf;
    while (dfu_length-- > 0) {
      if (*rd_ptr != *buffer) {
        PRINT("  verify failed @ address 0x%08x, got 0x%02x, 0x%02x expected\n", address - dfu_length - 1, *rd_ptr, *buffer);
        return false;
      }
      ++rd_ptr;
      ++buffer;
    }
  }
  return true;
}

bool uart_get_crc(uint32_t address, int length, uint32_t *crc)
{
  if (!uart_has_crc_cmd) {
    return false;
  }
  ENSURE(uart_send_cmd(DFU_CMD_CRC));
  ENSURE(uart_send_addr(address));
  ENSURE(uart_send_addr_timeout(length, READ_TIMEOUT));
  ENSURE(uart_rx((unsigned char *)crc, 4, READ_TIMEOUT));
  return true;
}

static uint32_t option_bytes[18] = { [0 ... 17] = 0xffffffff };
static bool uart_read_options()
{
  ENSURE(uart_read_memory(FLASH_OPTION_BASE, sizeof(option_bytes), (uint8_t *)option_bytes));
  return true;
}

static bool uart_write_options()
{
  // This will erase the entire flash.
  ENSURE(uart_cmd_option_erase());
  for (int i = 0; i < sizeof(option_bytes) / sizeof(uint32_t); ++i) {
    option_bytes[i] = 0xffffffff;
  }
  *(uint16_t *)option_bytes = FLASH_RDP_WORD;
  if (!logic_compress) {
    option_bytes[12] = FLASH_CONFIG_ADDR;
    option_bytes[13] = ~FLASH_CONFIG_ADDR;
  } else {
    option_bytes[14] = FLASH_LOGIC;
    option_bytes[15] = ~FLASH_LOGIC;
    option_bytes[16] = FLASH_DECOMP;
    option_bytes[17] = ~FLASH_DECOMP;
  }
    
  // Temporarily increase timeout since this will trigger an auto full chip erase.
  uart_timeout += ERASE_TIMEOUT * 2;
  ENSURE(uart_write_memory(FLASH_OPTION_BASE, sizeof(option_bytes), (uint8_t *)option_bytes));
  uart_timeout -= ERASE_TIMEOUT * 2;
  return true;
}

bool uart_check_protected()
{
  // Read protection is on if read memory fails
  uart_silent = true;
  uint8_t buffer;
  bool protected = uart_cmd_read_memory(FLASH_BASE, 1, &buffer);
  uart_silent = false;
  return protected;
}

bool uart_read_flash_size()
{
  uint32_t ctrl = 0x8001045a;
  uint32_t addr = 0x34;
  uint32_t cr, sr;
  ENSURE(uart_cmd_write_memory(FLASH_READ_CTRL, 4, (uint8_t *)&ctrl));
  ENSURE(uart_cmd_write_memory(FLASH_AR, 4, (uint8_t *)&addr));
  ENSURE(uart_cmd_read_memory(FLASH_CR, 4, (uint8_t *)&cr));
  cr |= (FLASH_CR_STRT | FLASH_CR_READ);
  ENSURE(uart_cmd_write_memory(FLASH_CR, 4, (uint8_t *)&cr));
  for (int i = 0; i < 100; ++i) {
    ENSURE(uart_cmd_read_memory(FLASH_SR, 4, (uint8_t *)&sr));
    if ((sr & FLASH_SR_BSY) == 0) {
      break;
    }
  }
  cr &= ~(FLASH_CR_STRT | FLASH_CR_READ);
  ENSURE(uart_cmd_write_memory(FLASH_CR, 4, (uint8_t *)&cr));
  ENSURE(uart_cmd_read_memory(FLASH_READ_DATA, 4, (uint8_t *)&flash_size));
  flash_size = (flash_size >> 3) + 1;
  return true;
}

bool uart_init0(uint32_t master_baud)
{
  uint32_t baud = master_baud;
  if (uart_version < DFU_MIN_VERSION) {
    baud = baud > DFU_SLOW_BAUD ? DFU_SLOW_BAUD : baud;
  }
  uart_is_init = false;
  uart_timeout = TIMEOUT;
  ENSURE(uart_init_seq(baud));
  ENSURE(uart_send_cmd(DFU_CMD_GET));
  uint8_t bytes;
  ENSURE(uart_rx_ch(&bytes, uart_timeout));
  ++bytes;
  uint8_t buffer[bytes];
  ENSURE(uart_rx(buffer, bytes, uart_timeout));
  ENSURE(uart_get_ack(uart_timeout));
  uart_has_crc_cmd = false;
  uart_extended_wr = 1;
  uart_extended_rd = 1;
  for (int i = 1; i < bytes; ++i) {
    switch (buffer[i]) {
    case DFU_CMD_CRC:
      uart_has_crc_cmd = true;
      break;
    case DFU_CMD_RE:
      uart_extended_rd = 4;
      break;
    case DFU_CMD_WE:
      uart_extended_wr = 16;
      break;
    }
  }
  uart_version = buffer[0];
  if (!uart_check_protected()) {
    PRINT("Unlocking device\n");
    ENSURE(uart_send_cmd(DFU_CMD_UR));
    ENSURE(uart_get_ack(ERASE_TIMEOUT));
    return uart_init0(master_baud);
  }
  if (uart_version < DFU_MIN_VERSION) {
    uint32_t crc;
    ENSURE(uart_write_memory(dfu_uart_addr, sizeof(dfu_uart_buf), dfu_uart_buf));
    ENSURE(uart_cmd_go(dfu_uart_addr));
    usleep(JUMP_WAIT * 1000);
    uart_version = DFU_MIN_VERSION;
    return uart_init0(master_baud);
  } else if (baud != master_baud) {
    ENSURE(uart_cmd_reset());
    usleep(10000);
    return uart_init0(master_baud);
  }
  return true;
}

void sys_nrst()
{
  pin_nrst_out(0);
  usleep(1000);
  pin_nrst_out(1);
  usleep(10000);
}

bool uart_init(uint32_t baud)
{
  uart_version = 0;
  pin_init();
  pin_boot0_out(1);
  sys_nrst();
  return uart_init0(baud);
}
  
bool uart_final(bool reset)
{
  pin_boot0_out(0);
  if (reset) {
    sys_nrst();
  }
  uart_disable();
  return true;
}

bool uart_init_flash()
{
  ENSURE(uart_read_flash_size());
  ENSURE(uart_write_options());
  ENSURE(uart_write_memory(FLASH_BASE, sizeof(program_buf), program_buf));
  return true;
}

bool uart_update_program(int length, const uint8_t *buffer)
{
  ENSURE(uart_erase_flash(FLASH_BASE, length));
  ENSURE(uart_write_memory(FLASH_BASE, length, buffer));
  return true;
}

bool uart_update_logic(int length, const uint8_t *buffer)
{
  logic_compress = length < COMPRESSED_SIZE;
  ENSURE(uart_read_flash_size());
  ENSURE(uart_read_options());

  bool non_compress_addr_match = 
    option_bytes[12] == FLASH_CONFIG_ADDR &&
    option_bytes[13] == ~FLASH_CONFIG_ADDR;
  bool compress_addr_match = 
    option_bytes[14] == FLASH_LOGIC &&
    option_bytes[15] == ~FLASH_LOGIC &&
    option_bytes[16] == FLASH_DECOMP &&
    option_bytes[17] == ~FLASH_DECOMP;
  bool non_compress_addr_valid = option_bytes[12] == ~option_bytes[13];

  // If address is not matched, re-write option bytes, this will also erase the full chip, including the MCU program
  if (logic_compress && (!compress_addr_match || non_compress_addr_valid) ||
      !logic_compress && !non_compress_addr_match) {
    ENSURE(uart_write_options());
    // Write the default program, no need to erase
    ENSURE(uart_write_memory(FLASH_BASE, sizeof(program_buf), program_buf));
  } else {
    ENSURE(uart_erase_flash(FLASH_CONFIG_ADDR, FLASH_CONFIG_SIZE));
  }

  if (logic_compress) {
    *(uint32_t *)(void *)(fpga_decomp_buf + 0) = SRAM_BASE | 0x2;
    *(uint32_t *)(void *)(fpga_decomp_buf + 4) = sizeof(fpga_decomp_buf) - 8;
    ENSURE(uart_write_memory(FLASH_DECOMP, sizeof(fpga_decomp_buf), fpga_decomp_buf));
    ENSURE(uart_write_memory(FLASH_LOGIC, length, buffer));
  } else {
    ENSURE(uart_write_memory(FLASH_CONFIG_ADDR, length, buffer));
  }
  return true;
}

bool uart_sram(int length, const uint8_t *buffer)
{
  uint8_t config[8];
  ENSURE(uart_read_memory(SYS_MISC, 4, config));
  ENSURE(uart_read_memory(UART_BRD, 8, config + 4));
  ENSURE(uart_write_memory(sram_config_addr - 12, 12, config));
  ENSURE(uart_write_memory(sram_config_addr, length, buffer));
  ENSURE(uart_write_memory(sram_decomp_addr, sizeof(sram_decomp_buf), sram_decomp_buf));
  ENSURE(uart_cmd_go(sram_decomp_addr));
  return uart_get_ack(500);
}
