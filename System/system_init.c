

#include "system_init.h"

#define RCC_BASE            0x40023800UL

#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x40))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x44))

// AHB1 Peripheral
#define RCC_AHB1ENR_GPIOAEN (1 << 0)    // GPIOA clock 
#define RCC_AHB1ENR_GPIOBEN (1 << 1)    // GPIOB clock 
#define RCC_AHB1ENR_DMA2EN  (1 << 22)   // DMA2 clock 

// APB1 Peripheral 
#define RCC_APB1ENR_I2C1EN  (1 << 21)   // I2C1 

// APB2 Peripheral 
#define RCC_APB2ENR_USART1EN (1 << 4)   // USART1 clock 
#define RCC_APB2ENR_SPI1EN   (1 << 12)  // SPI1 clock 


// GPIO Regist

#define GPIOA_BASE          0x40020000UL
#define GPIOB_BASE          0x40020400UL

#define GPIOA_MODER         (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_OTYPER        (*(volatile uint32_t *)(GPIOA_BASE + 0x04))
#define GPIOA_OSPEEDR       (*(volatile uint32_t *)(GPIOA_BASE + 0x08))
#define GPIOA_PUPDR         (*(volatile uint32_t *)(GPIOA_BASE + 0x0C))
#define GPIOA_AFRL          (*(volatile uint32_t *)(GPIOA_BASE + 0x20))
#define GPIOA_AFRH          (*(volatile uint32_t *)(GPIOA_BASE + 0x24))

#define GPIOB_MODER         (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_OTYPER        (*(volatile uint32_t *)(GPIOB_BASE + 0x04))
#define GPIOB_OSPEEDR       (*(volatile uint32_t *)(GPIOB_BASE + 0x08))
#define GPIOB_PUPDR         (*(volatile uint32_t *)(GPIOB_BASE + 0x0C))
#define GPIOB_AFRL          (*(volatile uint32_t *)(GPIOB_BASE + 0x20))


// NVIC Register

#define NVIC_BASE           0xE000E100UL

#define NVIC_ISER0          (*(volatile uint32_t *)(NVIC_BASE + 0x000))
#define NVIC_ISER1          (*(volatile uint32_t *)(NVIC_BASE + 0x004))
#define NVIC_ISER2          (*(volatile uint32_t *)(NVIC_BASE + 0x008))

#define NVIC_IPR_BASE       0xE000E400UL
#define NVIC_IPR(n)         (*(volatile uint32_t *)(NVIC_IPR_BASE + (4UL * (n))))

// IRQ Numbers for STM32F4

#define DMA2_Stream3_IRQn   59   // DMA2 Stream 3 global interrupt (for SPI1_TX)
#define USART1_IRQn         37   // USART1 global interrupt


// System Clock 


void system_clock_init(void)
{
    // Enable peripheral clocks
    
    // AHB1: DMA2 (for SPI1), GPIOA, GPIOB
    RCC_AHB1ENR |= RCC_AHB1ENR_DMA2EN | RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
    
    // APB1: I2C1
    RCC_APB1ENR |= RCC_APB1ENR_I2C1EN;
    
    // APB2: USART1, SPI1
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN 
                 | RCC_APB2ENR_SPI1EN;
}


void system_gpio_init(void)
{
    // USART1, PA9=TX, PA10=RX
   
    
    //  PA9 and PA10 alternate function mode
    GPIOA_MODER &= ~((3 << 18) | (3 << 20));  // Clear mode bits
    GPIOA_MODER |= (2 << 18) | (2 << 20);     // Set alternate function mode
    
    // Set high speed
    GPIOA_OSPEEDR |= (3 << 18) | (3 << 20);
    
    // No pull-up/pull-down
    GPIOA_PUPDR &= ~((3 << 18) | (3 << 20));
    
    // Set alternate function AF7 (USART1) for PA9 PA10
    GPIOA_AFRH &= ~((0xF << 4) | (0xF << 8)); // Clear AF bits
    GPIOA_AFRH |= (7 << 4) | (7 << 8);        // AF7 = USART1
    
    
    // SPI1 : PA5=SCK, PA7=MOSI
    
    // Configure PA5 PA7 alternate function mode
    GPIOA_MODER &= ~((3 << 10) | (3 << 14));  // Clear mode bits
    GPIOA_MODER |= (2 << 10) | (2 << 14);     
    
    // Set high speed
    GPIOA_OSPEEDR |= (3 << 10) | (3 << 14);
    
    // No pull-up/pull-down
    GPIOA_PUPDR &= ~((3 << 10) | (3 << 14));
    
    // Set alternate function AF5 (SPI1) for PA5 PA7
    GPIOA_AFRL &= ~((0xF << 20) | (0xF << 28)); // Clear AF bits
    GPIOA_AFRL |= (5 << 20) | (5 << 28);        // AF5 = SPI1
    
   
    // I2C1 : PB6=SCL, PB7=SDA
    
    
    // Configure PB6 and PB7 as alternate function mode
    GPIOB_MODER &= ~((3 << 12) | (3 << 14));  // Clear mode bits
    GPIOB_MODER |= (2 << 12) | (2 << 14);     // Set alternate function mode
    
    // Set output type to open-drain 
    GPIOB_OTYPER |= (1 << 6) | (1 << 7);
    
    // Set high speed
    GPIOB_OSPEEDR |= (3 << 12) | (3 << 14);
    
    // Enable pull-up resistors
    GPIOB_PUPDR &= ~((3 << 12) | (3 << 14));  // Clear bits
    GPIOB_PUPDR |= (1 << 12) | (1 << 14);     // Pull-up
    
    // Set alternate function AF4 (I2C1) for PB6 PB7
    GPIOB_AFRL &= ~((0xF << 24) | (0xF << 28)); // Clear AF bits
    GPIOB_AFRL |= (4 << 24) | (4 << 28);        // AF4 = I2C1
}


void system_nvic_init(void)
{
    // Configure Interrupt Priorities
    
    // USART1: Priority 2 
    NVIC_IPR(USART1_IRQn / 4) &= ~(0xFFUL << ((USART1_IRQn % 4) * 8));
    NVIC_IPR(USART1_IRQn / 4) |= (2UL << ((USART1_IRQn % 4) * 8 + 4));
    
    // DMA2 Stream 3: Priority 3 (for SPI1_TX)
    NVIC_IPR(DMA2_Stream3_IRQn / 4) &= ~(0xFFUL << ((DMA2_Stream3_IRQn % 4) * 8));
    NVIC_IPR(DMA2_Stream3_IRQn / 4) |= (3UL << ((DMA2_Stream3_IRQn % 4) * 8 + 4));
    
    // Enable USART1 interrupt (IRQ 37 is in ISER1)
    NVIC_ISER1 |= (1 << (USART1_IRQn - 32));
    
    // Enable DMA2 Stream 3 interrupt (IRQ 59 is in ISER1)
    NVIC_ISER1 |= (1 << (DMA2_Stream3_IRQn - 32));
}

void system_init_all(void)
{
   
    // Enable clocks 
    system_clock_init();
    
    // Configure GPIO pins
    system_gpio_init();
    
    // configure NVIC 
    system_nvic_init();
}
