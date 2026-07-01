#ifndef APPLICATION_APP_H
#define APPLICATION_APP_H

#include "app_config.h"

#include <stdint.h>

typedef enum {
    APP_STATE_BOOT = 0,
    APP_STATE_READY,
    APP_STATE_SPI_BUSY,
    APP_STATE_ERROR
} app_state_t;

typedef struct {
    app_state_t state;
    uint32_t loop_count;
    uint32_t spi_transfer_count;
    uint32_t spi_error_count;
    uint32_t dma_error_count;
    uint32_t i2c_success_count;
    uint32_t i2c_error_count;
    uint8_t last_i2c_sample[APP_I2C_SAMPLE_LENGTH];
} app_status_t;

void app_init(void);
void app_run(void);
void app_process(void);
const app_status_t *app_get_status(void);

#endif
