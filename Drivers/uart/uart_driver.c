

#include "uart_driver.h"
#include "../../Utils/ring_buffer.h"

#define USART1_BASE 0x40011000UL

#define USART_SR (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART_DR (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART_BRR (*(volatile uint32_t *)(USART1_BASE + 0x08))
#define USART_CR1 (*(volatile uint32_t *)(USART1_BASE + 0x0C))

// Status Register bits
#define SR_RXNE (1 << 5) // Read Data Register Not Empty
#define SR_TXE (1 << 7)  // Transmit Data Register Empty
#define SR_ORE (1 << 3)  // Overrun Error
#define SR_FE (1 << 1)   // Framing Error
#define SR_NE (1 << 2)   // Noise Error

// Control Register 1 bits
#define CR1_UE (1 << 13)    // USART Enable
#define CR1_TE (1 << 3)     // Transmitter Enable
#define CR1_RE (1 << 2)     // Receiver Enable
#define CR1_RXNEIE (1 << 5) // RXNE Interrupt Enable

#define RX_BUFFER_SIZE 256       // Must be power of 2 for efficiency
#define APB2_CLOCK_HZ 84000000UL // STM32F4 APB2 clock

static uint8_t rx_storage[RX_BUFFER_SIZE];
static ring_buffer_t rx_buffer;

// Error counters for diagnostics
static volatile uint32_t uart_overrun_count = 0;
static volatile uint32_t uart_framing_error_count = 0;
static volatile uint32_t uart_noise_error_count = 0;

void uart_init(uint32_t baudrate)
{
    //  RX ring buffer
    ring_buffer_init(&rx_buffer, rx_storage, RX_BUFFER_SIZE);

    // Disable UART
    USART_CR1 &= ~CR1_UE;

    // BRR = (USARTDIV × 16) where USARTDIV = fCLK / (16 × baudrate)

    uint32_t usartdiv = (APB2_CLOCK_HZ * 25) / (4 * baudrate);
    uint32_t mantissa = usartdiv / 100;
    uint32_t fraction = ((usartdiv - (mantissa * 100)) * 16 + 50) / 100;
    USART_BRR = (mantissa << 4) | (fraction & 0x0F);

    // Enable UART, transmitter, receiver, and RX interrupt
    USART_CR1 = CR1_UE | CR1_TE | CR1_RE | CR1_RXNEIE;
}

void uart_send(const uint8_t *data, uint16_t len)
{
    uint16_t i;

    for (i = 0; i < len; i++)
    {
        // Wait until TX register empty
        while (!(USART_SR & SR_TXE))
            ;

        // Write byte to data register
        USART_DR = data[i];
    }

    // Wait for last byte to complete transmission
    while (!(USART_SR & SR_TXE))
        ;
}

uint8_t uart_read_byte(uint8_t *byte)
{
    return ring_buffer_get(&rx_buffer, byte);
}

uint16_t uart_available(void)
{
    return ring_buffer_available(&rx_buffer);
}

void uart_clear_rx(void)
{
    ring_buffer_clear(&rx_buffer);
}

void USART1_IRQHandler(void)
{
    uint32_t sr = USART_SR; // Read status register
    uint8_t received_byte = 0;
    uint8_t has_data = 0;

    if (sr & (SR_RXNE | SR_ORE | SR_FE | SR_NE))
    {
        received_byte = (uint8_t)(USART_DR & 0xFF);
        has_data = (sr & SR_RXNE) ? 1 : 0;
    }

    // Check for errors first
    if (sr & SR_ORE)
    {
        // Overrun error - data lost
        uart_overrun_count++;
    }

    if (sr & SR_FE)
    {

        uart_framing_error_count++;
    }

    if (sr & SR_NE)
    {

        uart_noise_error_count++;
    }

    // Check if RX interrupt occurred
    if (has_data)
    {
        // Store byte in ring buffer

        ring_buffer_put(&rx_buffer, received_byte);
    }
}
