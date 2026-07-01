#ifndef BSP_BOARD_H
#define BSP_BOARD_H

#if defined(BOARD_STM32F4_DISCOVERY) || !defined(BOARD_SELECTED)
#include "stm32f4_discovery.h"
#define BOARD_SELECTED 1
#else
#error "No supported BSP board selected"
#endif

#endif
