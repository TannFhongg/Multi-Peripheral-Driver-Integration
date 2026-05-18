// system_init.h - System-Level Initialization (RCC, GPIO, NVIC)

#ifndef SYSTEM_INIT_H
#define SYSTEM_INIT_H

#include <stdint.h>

// Public API
void system_clock_init(void);
void system_gpio_init(void);
void system_nvic_init(void);
void system_init_all(void);

#endif
