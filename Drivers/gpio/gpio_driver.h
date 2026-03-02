/* gpio_driver.h - GPIO driver header */

#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include <stdint.h>

void GPIO_Init(void);
void GPIO_WritePin(uint8_t pin, uint8_t state);
uint8_t GPIO_ReadPin(uint8_t pin);

#endif /* GPIO_DRIVER_H */
