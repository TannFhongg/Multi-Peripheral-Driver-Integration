#include "stm32f4_discovery.h"

#define REG32(addr)          (*(volatile uint32_t *)(addr))
#define REG8(addr)           (*(volatile uint8_t *)(addr))

#define RCC_BASE             0x40023800UL
#define RCC_CR               REG32(RCC_BASE + 0x00)
#define RCC_PLLCFGR          REG32(RCC_BASE + 0x04)
#define RCC_CFGR             REG32(RCC_BASE + 0x08)
#define RCC_AHB1ENR          REG32(RCC_BASE + 0x30)
#define RCC_APB1ENR          REG32(RCC_BASE + 0x40)
#define RCC_APB2ENR          REG32(RCC_BASE + 0x44)

#define FLASH_BASE           0x40023C00UL
#define FLASH_ACR            REG32(FLASH_BASE + 0x00)

#define PWR_BASE             0x40007000UL
#define PWR_CR               REG32(PWR_BASE + 0x00)

#define GPIOA_BASE           0x40020000UL
#define GPIOB_BASE           0x40020400UL
#define GPIOD_BASE           0x40020C00UL
#define GPIOE_BASE           0x40021000UL

#define GPIO_MODER(gpio)     REG32((gpio) + 0x00)
#define GPIO_OTYPER(gpio)    REG32((gpio) + 0x04)
#define GPIO_OSPEEDR(gpio)   REG32((gpio) + 0x08)
#define GPIO_PUPDR(gpio)     REG32((gpio) + 0x0C)
#define GPIO_IDR(gpio)       REG32((gpio) + 0x10)
#define GPIO_ODR(gpio)       REG32((gpio) + 0x14)
#define GPIO_BSRR(gpio)      REG32((gpio) + 0x18)
#define GPIO_AFR(gpio, pin)  REG32((gpio) + (((pin) < 8U) ? 0x20UL : 0x24UL))

#define NVIC_ISER_BASE       0xE000E100UL
#define NVIC_IPR_BASE        0xE000E400UL
#define NVIC_ISER(irqn)      REG32(NVIC_ISER_BASE + (4UL * ((uint32_t)(irqn) / 32UL)))
#define NVIC_IPR8(irqn)      REG8(NVIC_IPR_BASE + (uint32_t)(irqn))

#define RCC_CR_HSEON         (1UL << 16)
#define RCC_CR_HSERDY        (1UL << 17)
#define RCC_CR_PLLON         (1UL << 24)
#define RCC_CR_PLLRDY        (1UL << 25)

#define RCC_PLLCFGR_PLLSRC_HSE (1UL << 22)
#define RCC_PLLCFGR_PLLM_8     (8UL << 0)
#define RCC_PLLCFGR_PLLN_336   (336UL << 6)
#define RCC_PLLCFGR_PLLP_2     (0UL << 16)
#define RCC_PLLCFGR_PLLQ_7     (7UL << 24)

#define RCC_CFGR_SW_MASK     (3UL << 0)
#define RCC_CFGR_SW_PLL      (2UL << 0)
#define RCC_CFGR_SWS_MASK    (3UL << 2)
#define RCC_CFGR_SWS_PLL     (2UL << 2)
#define RCC_CFGR_HPRE_MASK   (15UL << 4)
#define RCC_CFGR_PPRE1_MASK  (7UL << 10)
#define RCC_CFGR_PPRE2_MASK  (7UL << 13)
#define RCC_CFGR_HPRE_DIV1   (0UL << 4)
#define RCC_CFGR_PPRE1_DIV4  (5UL << 10)
#define RCC_CFGR_PPRE2_DIV2  (4UL << 13)

#define RCC_AHB1ENR_GPIOAEN  (1UL << 0)
#define RCC_AHB1ENR_GPIOBEN  (1UL << 1)
#define RCC_AHB1ENR_GPIODEN  (1UL << 3)
#define RCC_AHB1ENR_GPIOEEN  (1UL << 4)
#define RCC_AHB1ENR_DMA2EN   (1UL << 22)
#define RCC_APB1ENR_I2C1EN   (1UL << 21)
#define RCC_APB1ENR_PWREN    (1UL << 28)
#define RCC_APB2ENR_USART1EN (1UL << 4)
#define RCC_APB2ENR_SPI1EN   (1UL << 12)

#define FLASH_ACR_LATENCY_5WS (5UL << 0)
#define FLASH_ACR_PRFTEN      (1UL << 8)
#define FLASH_ACR_ICEN        (1UL << 9)
#define FLASH_ACR_DCEN        (1UL << 10)

#define PWR_CR_VOS          (1UL << 14)

#define GPIO_MODE_INPUT     0UL
#define GPIO_MODE_OUTPUT    1UL
#define GPIO_MODE_AF        2UL
#define GPIO_OTYPE_PP       0UL
#define GPIO_OTYPE_OD       1UL
#define GPIO_SPEED_HIGH     3UL
#define GPIO_PUPD_NONE      0UL
#define GPIO_PUPD_UP        1UL

#define GPIO_AF_I2C1        4UL
#define GPIO_AF_SPI1        5UL
#define GPIO_AF_USART1      7UL

#define USART1_IRQn         37U
#define DMA2_STREAM3_IRQn   59U
#define USART1_PRIORITY     2U
#define DMA2_STREAM3_PRIORITY 3U

#define BSP_CLOCK_TIMEOUT   1000000UL
#define BSP_MEMS_CS_PIN     3U

static const uint8_t bsp_led_pins[BSP_LED_COUNT] = { 12U, 13U, 14U, 15U };

static void bsp_enable_peripheral_clocks(void)
{
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
                   RCC_AHB1ENR_GPIOBEN |
                   RCC_AHB1ENR_GPIODEN |
                   RCC_AHB1ENR_GPIOEEN |
                   RCC_AHB1ENR_DMA2EN;

    RCC_APB1ENR |= RCC_APB1ENR_I2C1EN | RCC_APB1ENR_PWREN;
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_SPI1EN;

    (void)RCC_AHB1ENR;
    (void)RCC_APB1ENR;
    (void)RCC_APB2ENR;
}

static uint8_t bsp_wait_for_set(volatile uint32_t *reg, uint32_t mask)
{
    uint32_t timeout = BSP_CLOCK_TIMEOUT;

    while (((*reg) & mask) == 0U) {
        if (--timeout == 0U) {
            return 0U;
        }
    }

    return 1U;
}

static uint8_t bsp_wait_for_value(volatile uint32_t *reg, uint32_t mask, uint32_t value)
{
    uint32_t timeout = BSP_CLOCK_TIMEOUT;

    while (((*reg) & mask) != value) {
        if (--timeout == 0U) {
            return 0U;
        }
    }

    return 1U;
}

static void gpio_set_mode(uint32_t gpio, uint8_t pin, uint32_t mode)
{
    uint32_t shift = (uint32_t)pin * 2UL;
    GPIO_MODER(gpio) = (GPIO_MODER(gpio) & ~(3UL << shift)) | (mode << shift);
}

static void gpio_set_output_type(uint32_t gpio, uint8_t pin, uint32_t output_type)
{
    if (output_type == GPIO_OTYPE_OD) {
        GPIO_OTYPER(gpio) |= (1UL << pin);
    } else {
        GPIO_OTYPER(gpio) &= ~(1UL << pin);
    }
}

static void gpio_set_speed(uint32_t gpio, uint8_t pin, uint32_t speed)
{
    uint32_t shift = (uint32_t)pin * 2UL;
    GPIO_OSPEEDR(gpio) = (GPIO_OSPEEDR(gpio) & ~(3UL << shift)) | (speed << shift);
}

static void gpio_set_pull(uint32_t gpio, uint8_t pin, uint32_t pull)
{
    uint32_t shift = (uint32_t)pin * 2UL;
    GPIO_PUPDR(gpio) = (GPIO_PUPDR(gpio) & ~(3UL << shift)) | (pull << shift);
}

static void gpio_set_af(uint32_t gpio, uint8_t pin, uint32_t af)
{
    uint32_t shift = ((uint32_t)pin % 8UL) * 4UL;
    GPIO_AFR(gpio, pin) = (GPIO_AFR(gpio, pin) & ~(15UL << shift)) | (af << shift);
}

static void gpio_write(uint32_t gpio, uint8_t pin, uint8_t state)
{
    if (state != 0U) {
        GPIO_BSRR(gpio) = (1UL << pin);
    } else {
        GPIO_BSRR(gpio) = (1UL << (pin + 16U));
    }
}

static void gpio_config_af(uint32_t gpio,
                           uint8_t pin,
                           uint32_t af,
                           uint32_t output_type,
                           uint32_t pull)
{
    gpio_set_mode(gpio, pin, GPIO_MODE_AF);
    gpio_set_output_type(gpio, pin, output_type);
    gpio_set_speed(gpio, pin, GPIO_SPEED_HIGH);
    gpio_set_pull(gpio, pin, pull);
    gpio_set_af(gpio, pin, af);
}

static void gpio_config_output(uint32_t gpio, uint8_t pin)
{
    gpio_set_mode(gpio, pin, GPIO_MODE_OUTPUT);
    gpio_set_output_type(gpio, pin, GPIO_OTYPE_PP);
    gpio_set_speed(gpio, pin, GPIO_SPEED_HIGH);
    gpio_set_pull(gpio, pin, GPIO_PUPD_NONE);
}

static void gpio_config_input(uint32_t gpio, uint8_t pin, uint32_t pull)
{
    gpio_set_mode(gpio, pin, GPIO_MODE_INPUT);
    gpio_set_pull(gpio, pin, pull);
}

static void nvic_set_priority(uint8_t irqn, uint8_t priority)
{
    NVIC_IPR8(irqn) = (uint8_t)(priority << 4);
}

static void nvic_enable_irq(uint8_t irqn)
{
    NVIC_ISER(irqn) = (1UL << ((uint32_t)irqn % 32UL));
}

void bsp_clock_init(void)
{
    if ((RCC_CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL) {
        RCC_CR |= RCC_CR_HSEON;

        if (bsp_wait_for_set(&RCC_CR, RCC_CR_HSERDY) != 0U) {
            RCC_APB1ENR |= RCC_APB1ENR_PWREN;
            (void)RCC_APB1ENR;
            PWR_CR |= PWR_CR_VOS;

            FLASH_ACR = FLASH_ACR_LATENCY_5WS |
                        FLASH_ACR_PRFTEN |
                        FLASH_ACR_ICEN |
                        FLASH_ACR_DCEN;

            RCC_CFGR = (RCC_CFGR & ~(RCC_CFGR_HPRE_MASK |
                                     RCC_CFGR_PPRE1_MASK |
                                     RCC_CFGR_PPRE2_MASK)) |
                       RCC_CFGR_HPRE_DIV1 |
                       RCC_CFGR_PPRE1_DIV4 |
                       RCC_CFGR_PPRE2_DIV2;

            RCC_PLLCFGR = RCC_PLLCFGR_PLLM_8 |
                          RCC_PLLCFGR_PLLN_336 |
                          RCC_PLLCFGR_PLLP_2 |
                          RCC_PLLCFGR_PLLSRC_HSE |
                          RCC_PLLCFGR_PLLQ_7;

            RCC_CR |= RCC_CR_PLLON;

            if (bsp_wait_for_set(&RCC_CR, RCC_CR_PLLRDY) != 0U) {
                RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_PLL;

                (void)bsp_wait_for_value(&RCC_CFGR, RCC_CFGR_SWS_MASK, RCC_CFGR_SWS_PLL);
            }
        }
    }

    bsp_enable_peripheral_clocks();
}

void bsp_gpio_init(void)
{
    bsp_enable_peripheral_clocks();

    // USART1: PA9=TX, PA10=RX, AF7
    gpio_config_af(GPIOA_BASE, 9U, GPIO_AF_USART1, GPIO_OTYPE_PP, GPIO_PUPD_NONE);
    gpio_config_af(GPIOA_BASE, 10U, GPIO_AF_USART1, GPIO_OTYPE_PP, GPIO_PUPD_NONE);

    // SPI1: PA5=SCK, PA7=MOSI, AF5. MISO is unused by the TX-only driver.
    gpio_config_af(GPIOA_BASE, 5U, GPIO_AF_SPI1, GPIO_OTYPE_PP, GPIO_PUPD_NONE);
    gpio_config_af(GPIOA_BASE, 7U, GPIO_AF_SPI1, GPIO_OTYPE_PP, GPIO_PUPD_NONE);

    // Keep the onboard MEMS device deselected while SPI1 is shared.
    gpio_write(GPIOE_BASE, BSP_MEMS_CS_PIN, 1U);
    gpio_config_output(GPIOE_BASE, BSP_MEMS_CS_PIN);

    // I2C1: PB6=SCL, PB7=SDA, AF4, open-drain with pull-up.
    gpio_config_af(GPIOB_BASE, 6U, GPIO_AF_I2C1, GPIO_OTYPE_OD, GPIO_PUPD_UP);
    gpio_config_af(GPIOB_BASE, 7U, GPIO_AF_I2C1, GPIO_OTYPE_OD, GPIO_PUPD_UP);

    // User LEDs: PD12 green, PD13 orange, PD14 red, PD15 blue.
    for (uint8_t i = 0U; i < (uint8_t)BSP_LED_COUNT; i++) {
        gpio_config_output(GPIOD_BASE, bsp_led_pins[i]);
        gpio_write(GPIOD_BASE, bsp_led_pins[i], 0U);
    }

    // User button: PA0, active high on STM32F4-Discovery.
    gpio_config_input(GPIOA_BASE, 0U, GPIO_PUPD_NONE);
}

void bsp_nvic_init(void)
{
    nvic_set_priority(USART1_IRQn, USART1_PRIORITY);
    nvic_set_priority(DMA2_STREAM3_IRQn, DMA2_STREAM3_PRIORITY);

    nvic_enable_irq(USART1_IRQn);
    nvic_enable_irq(DMA2_STREAM3_IRQn);
}

void bsp_board_init(void)
{
    bsp_clock_init();
    bsp_gpio_init();
    bsp_nvic_init();
}

void bsp_led_on(bsp_led_t led)
{
    if (led < BSP_LED_COUNT) {
        gpio_write(GPIOD_BASE, bsp_led_pins[led], 1U);
    }
}

void bsp_led_off(bsp_led_t led)
{
    if (led < BSP_LED_COUNT) {
        gpio_write(GPIOD_BASE, bsp_led_pins[led], 0U);
    }
}

void bsp_led_toggle(bsp_led_t led)
{
    if (led < BSP_LED_COUNT) {
        GPIO_ODR(GPIOD_BASE) ^= (1UL << bsp_led_pins[led]);
    }
}

uint8_t bsp_button_read(bsp_button_t button)
{
    if (button != BSP_BUTTON_USER) {
        return 0U;
    }

    return (GPIO_IDR(GPIOA_BASE) & 1UL) ? 1U : 0U;
}
