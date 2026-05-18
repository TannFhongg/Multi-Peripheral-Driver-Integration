// ring_buffer.h - Generic Ring Buffer for UART RX

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>


typedef struct {
    uint8_t *buffer;         // Pointer to storage array
    uint16_t size;           // Total buffer size
    volatile uint16_t head;  // Write position (updated by ISR only)
    volatile uint16_t tail;  // Read position (updated by application only)
} ring_buffer_t;

// Public API
void ring_buffer_init(ring_buffer_t *rb, uint8_t *buffer, uint16_t size);
uint8_t ring_buffer_put(ring_buffer_t *rb, uint8_t data);
uint8_t ring_buffer_get(ring_buffer_t *rb, uint8_t *data);
uint16_t ring_buffer_available(ring_buffer_t *rb);
void ring_buffer_clear(ring_buffer_t *rb);

#endif 
