// uart_driver.h - UART Driver with Interrupt-Driven RX

#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>

// Public API 
void uart_init(uint32_t baudrate);
void uart_send(const uint8_t *data, uint16_t len);
uint8_t uart_read_byte(uint8_t *byte);
uint16_t uart_available(void);
void uart_clear_rx(void);

#endif 
