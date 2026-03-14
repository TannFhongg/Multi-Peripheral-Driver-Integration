// spi_driver.h - SPI Master Driver with DMA Support

#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H

#include <stdint.h>

// Transfer complete callback type
typedef void (*spi_callback_t)(void);

// Public API
void spi_init(spi_callback_t callback);
void spi_transmit_dma(uint8_t *data, uint16_t len);
uint8_t spi_is_busy(void);

#endif
