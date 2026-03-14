// ring_buffer.c - Generic Ring Buffer Implementation

#include "ring_buffer.h"

void ring_buffer_init(ring_buffer_t *rb, uint8_t *buffer, uint16_t size)
{
    rb->buffer = buffer;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

uint8_t ring_buffer_put(ring_buffer_t *rb, uint8_t data)
{
    // Check if buffer is full
    if (rb->count >= rb->size) {
        return 0;  // Buffer full - data lost!
    }
    
    // Store byte at head position
    rb->buffer[rb->head] = data;
    
    // Move head forward with wrap-around
    rb->head = (rb->head + 1) % rb->size;
    
    // Increment count
    rb->count++;
    
    return 1;
}

uint8_t ring_buffer_get(ring_buffer_t *rb, uint8_t *data)
{
    // Check if buffer is empty
    if (rb->count == 0) {
        return 0;  // No data available
    }
    
    // Read byte from tail position
    *data = rb->buffer[rb->tail];
    
    // Move tail forward with wrap-around
    rb->tail = (rb->tail + 1) % rb->size;
    
    // Decrement count
    rb->count--;
    
    return 1;
}

uint16_t ring_buffer_available(ring_buffer_t *rb)
{
    return rb->count;
}

void ring_buffer_clear(ring_buffer_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}
