#ifndef UART_MASTER_H
#define UART_MASTER_H

#include "port.h"

bool uart_erase_flash(uint32_t address, uint32_t length);
bool uart_read_memory(uint32_t address, int length, uint8_t *buffer);
bool uart_write_memory(uint32_t address, int length, const uint8_t *buffer);
bool uart_verify_memory(uint32_t address, int length, const uint8_t *buffer);
bool uart_get_crc(uint32_t address, int length, uint32_t *crc);

bool uart_init(uint32_t baud);
bool uart_init_flash();
bool uart_update_program(int length, const uint8_t *buffer);
bool uart_update_logic(int length, const uint8_t *buffer);
bool uart_sram(int length, const uint8_t *buffer);
bool uart_final(bool reset);

#endif
