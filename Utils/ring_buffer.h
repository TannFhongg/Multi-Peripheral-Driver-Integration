/* ring_buffer.h - Generic Ring Buffer for UART RX */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>

/**
 * Ring Buffer Structure
 * 
 * Why ring buffer for UART RX?
 * - UART receives data asynchronously (interrupt-driven)
 * - Application may not read data immediately
 * - Buffer prevents data loss between interrupt and application read
 * - Circular structure allows continuous operation without shifting data
 */
typedef struct {
    uint8_t *buffer;    /* Pointer to storage array */
    uint16_t size;      /* Total buffer size */
    uint16_t head;      /* Write position (updated by ISR) */
    uint16_t tail;      /* Read position (updated by application) */
    uint16_t count;     /* Number of bytes in buffer */
} ring_buffer_t;

/* Public API */
void ring_buffer_init(ring_buffer_t *rb, uint8_t *buffer, uint16_t size);
uint8_t ring_buffer_put(ring_buffer_t *rb, uint8_t data);
uint8_t ring_buffer_get(ring_buffer_t *rb, uint8_t *data);
uint16_t ring_buffer_available(ring_buffer_t *rb);
void ring_buffer_clear(ring_buffer_t *rb);

#endif /* RING_BUFFER_H */
