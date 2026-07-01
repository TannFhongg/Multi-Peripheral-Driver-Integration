

#include "system_init.h"
#include "../BSP/board.h"


void system_clock_init(void)
{
    bsp_clock_init();
}


void system_gpio_init(void)
{
    bsp_gpio_init();
}


void system_nvic_init(void)
{
    bsp_nvic_init();
}

void system_init_all(void)
{
    bsp_board_init();
}
