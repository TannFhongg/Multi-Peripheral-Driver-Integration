/* i2c_driver.h - Simple I2C Master Driver (Polling Mode) */

#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include <stdint.h>

/* Return status */
#define I2C_OK      0
#define I2C_ERROR   1

/* Public API */
void i2c_init(void);
uint8_t i2c_write(uint8_t slave_addr, uint8_t *data, uint8_t len);
uint8_t i2c_read(uint8_t slave_addr, uint8_t *buf, uint8_t len);

#endif /* I2C_DRIVER_H */
