

#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H

#include <stdint.h>

// callback type
typedef void (*spi_callback_t)(void);

void spi_init(spi_callback_t callback);
void spi_transmit_dma(uint8_t *data, uint16_t len);
uint8_t spi_is_busy(void);

#endif
