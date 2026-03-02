/* uart_driver.c - UART Driver with Interrupt-Driven RX */

#include "uart_driver.h"
#include "../Utils/ring_buffer.h"

/*=============================================================================
 * HARDWARE REGISTER DEFINITIONS (STM32-like USART1)
 *===========================================================================*/
#define USART1_BASE         0x40011000UL

#define USART_SR            (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART_DR            (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART_BRR           (*(volatile uint32_t *)(USART1_BASE + 0x08))
#define USART_CR1           (*(volatile uint32_t *)(USART1_BASE + 0x0C))

/* Status Register bits */
#define SR_RXNE             (1 << 5)    /* Read Data Register Not Empty */
#define SR_TXE              (1 << 7)    /* Transmit Data Register Empty */

/* Control Register 1 bits */
#define CR1_UE              (1 << 13)   /* USART Enable */
#define CR1_TE              (1 << 3)    /* Transmitter Enable */
#define CR1_RE              (1 << 2)    /* Receiver Enable */
#define CR1_RXNEIE          (1 << 5)    /* RXNE Interrupt Enable */

/*=============================================================================
 * PRIVATE VARIABLES
 *===========================================================================*/
#define RX_BUFFER_SIZE      256         /* Must be power of 2 for efficiency */

static uint8_t rx_storage[RX_BUFFER_SIZE];
static ring_buffer_t rx_buffer;

/*=============================================================================
 * PUBLIC API IMPLEMENTATION
 *===========================================================================*/

/**
 * @brief Initialize UART with specified baudrate
 * 
 * Configuration:
 * - 8 data bits, 1 stop bit, no parity (8N1)
 * - RX interrupt enabled
 * - TX polling mode (no interrupt)
 * 
 * Why interrupt for RX but not TX?
 * - RX: Data arrives asynchronously, must be captured immediately
 * - TX: Application controls when to send, polling is simpler
 * 
 * @param baudrate: Desired baudrate (e.g., 9600, 115200)
 */
void uart_init(uint32_t baudrate)
{
    /* Initialize RX ring buffer */
    ring_buffer_init(&rx_buffer, rx_storage, RX_BUFFER_SIZE);
    
    /* Disable UART before configuration */
    USART_CR1 &= ~CR1_UE;
    
    /* Configure baudrate
     * BRR = fCLK / baudrate
     * Assuming APB2 clock = 84MHz for STM32F4
     * Example: 84000000 / 115200 = 729
     */
    USART_BRR = 84000000 / baudrate;
    
    /* Enable UART, transmitter, receiver, and RX interrupt */
    USART_CR1 = CR1_UE | CR1_TE | CR1_RE | CR1_RXNEIE;
    
    /* Note: NVIC interrupt enable should be done in startup code */
}

/**
 * @brief Send data via UART (blocking, polling mode)
 * 
 * Simple polling approach suitable for:
 * - Sending AT commands to GSM module
 * - Debug output
 * - Response messages
 * 
 * @param data: Pointer to data buffer
 * @param len: Number of bytes to send
 */
void uart_send(const uint8_t *data, uint16_t len)
{
    uint16_t i;
    
    for (i = 0; i < len; i++) {
        /* Wait until TX register is empty */
        while (!(USART_SR & SR_TXE));
        
        /* Write byte to data register */
        USART_DR = data[i];
    }
    
    /* Wait for last byte to complete transmission */
    while (!(USART_SR & SR_TXE));
}

/**
 * @brief Read one byte from RX buffer (non-blocking)
 * 
 * Application calls this to retrieve received data.
 * Data was previously stored by RX interrupt handler.
 * 
 * @param byte: Pointer to store retrieved byte
 * @return 1 if byte read, 0 if buffer empty
 */
uint8_t uart_read_byte(uint8_t *byte)
{
    return ring_buffer_get(&rx_buffer, byte);
}

/**
 * @brief Get number of bytes available in RX buffer
 * 
 * Useful for:
 * - Checking if data is ready before reading
 * - Determining if complete AT response received
 * 
 * @return Number of bytes available
 */
uint16_t uart_available(void)
{
    return ring_buffer_available(&rx_buffer);
}

/**
 * @brief Clear RX buffer
 * 
 * Useful for:
 * - Discarding incomplete/corrupted data
 * - Resetting state before sending new AT command
 */
void uart_clear_rx(void)
{
    ring_buffer_clear(&rx_buffer);
}

/*=============================================================================
 * UART RX INTERRUPT HANDLER
 * 
 * Called by hardware when byte is received (RXNE flag set).
 * This should be registered in the interrupt vector table.
 * 
 * Data flow:
 * 1. GSM module sends byte → UART hardware receives it
 * 2. UART sets RXNE flag → CPU jumps to this ISR
 * 3. ISR reads byte from USART_DR (clears RXNE flag)
 * 4. ISR stores byte in ring buffer
 * 5. Application later reads from ring buffer at its own pace
 * 
 * Why this approach?
 * - ISR is fast (just store byte and return)
 * - No data loss even if application is busy
 * - Application can process AT responses when ready
 *===========================================================================*/
void USART1_IRQHandler(void)
{
    /* Check if RX interrupt occurred */
    if (USART_SR & SR_RXNE) {
        /* Read byte from data register (clears RXNE flag) */
        uint8_t received_byte = USART_DR & 0xFF;
        
        /* Store byte in ring buffer */
        ring_buffer_put(&rx_buffer, received_byte);
        
        /* Note: If buffer is full, ring_buffer_put() returns 0
         * In production code, you might want to handle this error
         * For demo purposes, we simply drop the byte
         */
    }
}
