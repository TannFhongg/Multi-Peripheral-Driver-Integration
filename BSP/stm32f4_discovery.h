#ifndef BSP_STM32F4_DISCOVERY_H
#define BSP_STM32F4_DISCOVERY_H

#include <stdint.h>

#define BSP_BOARD_NAME       "STM32F4DISCOVERY / STM32F407G-DISC1"
#define BSP_MCU_NAME         "STM32F407VGT6"

#define BSP_HSE_CLOCK_HZ     8000000UL
#define BSP_SYSCLK_HZ        168000000UL
#define BSP_AHB_CLOCK_HZ     168000000UL
#define BSP_APB1_CLOCK_HZ    42000000UL
#define BSP_APB2_CLOCK_HZ    84000000UL

typedef enum {
    BSP_LED_GREEN = 0,
    BSP_LED_ORANGE,
    BSP_LED_RED,
    BSP_LED_BLUE,
    BSP_LED_COUNT
} bsp_led_t;

typedef enum {
    BSP_BUTTON_USER = 0
} bsp_button_t;

void bsp_clock_init(void);
void bsp_gpio_init(void);
void bsp_nvic_init(void);
void bsp_board_init(void);

void bsp_led_on(bsp_led_t led);
void bsp_led_off(bsp_led_t led);
void bsp_led_toggle(bsp_led_t led);
uint8_t bsp_button_read(bsp_button_t button);

#endif
