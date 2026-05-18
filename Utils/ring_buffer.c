// ring_buffer.c - Lock-Free Ring Buffer Implementation
// Fixed: Race condition removed by eliminating shared 'count' variable
// Uses lock-free algorithm: head modified by ISR only, tail by application only

#include "ring_buffer.h"

void ring_buffer_init(ring_buffer_t *rb, uint8_t *buffer, uint16_t size)
{
    rb->buffer = buffer;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
}

uint8_t ring_buffer_put(ring_buffer_t *rb, uint8_t data)
{
    // Calculate next head position
    uint16_t next_head = (rb->head + 1) % rb->size;
    
    // Check if buffer is full (next_head would equal tail)
    if (next_head == rb->tail) {
        return 0;  // Buffer full - data lost!
    }
    
    // Store byte at head position
    rb->buffer[rb->head] = data;
    
    // Move head forward (single write, atomic on Cortex-M)
    rb->head = next_head;
    
    return 1;
}

uint8_t ring_buffer_get(ring_buffer_t *rb, uint8_t *data)
{
    // Check if buffer is empty (head equals tail)
    if (rb->head == rb->tail) {
        return 0;  // No data available
    }
    
    // Read byte from tail position
    *data = rb->buffer[rb->tail];
    
    // Move tail forward (single write, atomic on Cortex-M)
    rb->tail = (rb->tail + 1) % rb->size;
    
    return 1;
}

uint16_t ring_buffer_available(ring_buffer_t *rb)
{
    // Snapshot head and tail to avoid race condition
    uint16_t h = rb->head;
    uint16_t t = rb->tail;
    
    // Calculate available bytes
    if (h >= t) {
        return h - t;
    } else {
        return rb->size - t + h;
    }
}

void ring_buffer_clear(ring_buffer_t *rb)
{
    // Simply reset tail to head position
    // No need to clear actual buffer data
    rb->tail = rb->head;
}
