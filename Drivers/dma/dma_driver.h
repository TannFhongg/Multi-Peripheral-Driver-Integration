

#ifndef DMA_DRIVER_H
#define DMA_DRIVER_H

#include <stdint.h>

// DMA callback 
typedef void (*dma_callback_t)(void);

// Public API
void dma_init(uint32_t peripheral_addr, dma_callback_t callback);
void dma_start_tx(uint8_t *data, uint16_t len);
uint8_t dma_is_busy(void);
uint32_t dma_get_error_count(void);

#endif 
