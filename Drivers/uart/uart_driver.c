// uart_driver.c - UART Driver with Interrupt-Driven RX

#include "uart_driver.h"
#include "../Utils/ring_buffer.h"


#define USART1_BASE         0x40011000UL

#define USART_SR            (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART_DR            (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART_BRR           (*(volatile uint32_t *)(USART1_BASE + 0x08))
#define USART_CR1           (*(volatile uint32_t *)(USART1_BASE + 0x0C))

// Status Register bits
#define SR_RXNE             (1 << 5)    // Read Data Register Not Empty
#define SR_TXE              (1 << 7)    // Transmit Data Register Empty

// Control Register 1 bits
#define CR1_UE              (1 << 13)   // USART Enable
#define CR1_TE              (1 << 3)    // Transmitter Enable
#define CR1_RE              (1 << 2)    // Receiver Enable
#define CR1_RXNEIE          (1 << 5)    // RXNE Interrupt Enable


#define RX_BUFFER_SIZE      256         // Must be power of 2 for efficiency

static uint8_t rx_storage[RX_BUFFER_SIZE];
static ring_buffer_t rx_buffer;

void uart_init(uint32_t baudrate)
{
    // Initialize RX ring buffer
    ring_buffer_init(&rx_buffer, rx_storage, RX_BUFFER_SIZE);
    
    // Disable UART before configuration
    USART_CR1 &= ~CR1_UE;
    
    // Configure baudrate
    // BRR = fCLK / baudrate
    // Assuming APB2 clock = 84MHz for STM32F4
    // Example: 84000000 / 115200 = 729
    USART_BRR = 84000000 / baudrate;
    
    // Enable UART, transmitter, receiver, and RX interrupt
    USART_CR1 = CR1_UE | CR1_TE | CR1_RE | CR1_RXNEIE;
    
    // Note: NVIC interrupt enable should be done in startup code
}

void uart_send(const uint8_t *data, uint16_t len)
{
    uint16_t i;
    
    for (i = 0; i < len; i++) {
        // Wait until TX register is empty
        while (!(USART_SR & SR_TXE));
        
        // Write byte to data register
        USART_DR = data[i];
    }
    
    // Wait for last byte to complete transmission
    while (!(USART_SR & SR_TXE));
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
    // Check if RX interrupt occurred
    if (USART_SR & SR_RXNE) {
        // Read byte from data register (clears RXNE flag)
        uint8_t received_byte = USART_DR & 0xFF;
        
        // Store byte in ring buffer
        ring_buffer_put(&rx_buffer, received_byte);
        
        // Note: If buffer is full, ring_buffer_put() returns 0
        // In production code, you might want to handle this error
        // For demo purposes, we simply drop the byte
    }
}
