/* ring_buffer.c - Generic Ring Buffer Implementation */

#include "ring_buffer.h"

/**
 * @brief Initialize ring buffer
 * 
 * @param rb: Pointer to ring buffer structure
 * @param buffer: Pointer to storage array
 * @param size: Size of storage array
 */
void ring_buffer_init(ring_buffer_t *rb, uint8_t *buffer, uint16_t size)
{
    rb->buffer = buffer;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

/**
 * @brief Put byte into ring buffer
 * 
 * Called from UART RX interrupt to store incoming byte.
 * Uses head index to track write position.
 * 
 * @param rb: Pointer to ring buffer
 * @param data: Byte to store
 * @return 1 if success, 0 if buffer full
 */
uint8_t ring_buffer_put(ring_buffer_t *rb, uint8_t data)
{
    /* Check if buffer is full */
    if (rb->count >= rb->size) {
        return 0;  /* Buffer full - data lost! */
    }
    
    /* Store byte at head position */
    rb->buffer[rb->head] = data;
    
    /* Move head forward with wrap-around */
    rb->head = (rb->head + 1) % rb->size;
    
    /* Increment count */
    rb->count++;
    
    return 1;
}

/**
 * @brief Get byte from ring buffer
 * 
 * Called from application to read received data.
 * Uses tail index to track read position.
 * 
 * @param rb: Pointer to ring buffer
 * @param data: Pointer to store retrieved byte
 * @return 1 if success, 0 if buffer empty
 */
uint8_t ring_buffer_get(ring_buffer_t *rb, uint8_t *data)
{
    /* Check if buffer is empty */
    if (rb->count == 0) {
        return 0;  /* No data available */
    }
    
    /* Read byte from tail position */
    *data = rb->buffer[rb->tail];
    
    /* Move tail forward with wrap-around */
    rb->tail = (rb->tail + 1) % rb->size;
    
    /* Decrement count */
    rb->count--;
    
    return 1;
}

/**
 * @brief Get number of bytes available in buffer
 * 
 * @param rb: Pointer to ring buffer
 * @return Number of bytes available to read
 */
uint16_t ring_buffer_available(ring_buffer_t *rb)
{
    return rb->count;
}

/**
 * @brief Clear all data in buffer
 * 
 * @param rb: Pointer to ring buffer
 */
void ring_buffer_clear(ring_buffer_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}
