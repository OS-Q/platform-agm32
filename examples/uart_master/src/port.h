#ifndef UART_PORT_H
#define UART_PORT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void usleep(int);

void pin_init();
void pin_nrst_out(uint32_t value);
void pin_boot0_out(uint32_t value);

void uart_enable(uint32_t baud);
void uart_disable();

bool uart_tx(const unsigned char *buf, uint32_t count);
bool uart_tx_ch(unsigned char ch);
bool uart_rx(unsigned char *buf, uint32_t count, int timeout);
bool uart_rx_ch(unsigned char *ch, int timeout);

#endif
