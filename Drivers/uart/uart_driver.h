/* uart_driver.h - UART Driver with Interrupt-Driven RX */

#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>

/**
 * UART Driver Overview
 * 
 * RX: Interrupt-driven with ring buffer
 * - Hardware receives byte → interrupt fires → byte stored in buffer
 * - Application reads from buffer when ready (non-blocking)
 * - Prevents data loss during burst reception (e.g., AT responses)
 * 
 * TX: Polling mode
 * - Simple blocking send
 * - Suitable for sending AT commands
 * 
 * Typical use case: GSM/GPS/BLE modules with AT commands
 */

/* Public API */
void uart_init(uint32_t baudrate);
void uart_send(const uint8_t *data, uint16_t len);
uint8_t uart_read_byte(uint8_t *byte);
uint16_t uart_available(void);
void uart_clear_rx(void);

#endif /* UART_DRIVER_H */
