/* spi_driver.c - SPI Master Driver with DMA (Transmit-Only) */

#include "spi_driver.h"
#include "../dma/dma_driver.h"

/*=============================================================================
 * HARDWARE REGISTER DEFINITIONS (STM32-like SPI1)
 *===========================================================================*/
#define SPI1_BASE           0x40013000UL

#define SPI_CR1             (*(volatile uint32_t *)(SPI1_BASE + 0x00))
#define SPI_CR2             (*(volatile uint32_t *)(SPI1_BASE + 0x04))
#define SPI_SR              (*(volatile uint32_t *)(SPI1_BASE + 0x08))
#define SPI_DR              (*(volatile uint32_t *)(SPI1_BASE + 0x0C))

/* SPI Control Register 1 bits */
#define CR1_CPHA            (1 << 0)    /* Clock Phase */
#define CR1_CPOL            (1 << 1)    /* Clock Polarity */
#define CR1_MSTR            (1 << 2)    /* Master Selection */
#define CR1_BR_DIV16        (3 << 3)    /* Baud Rate: fPCLK/16 */
#define CR1_SPE             (1 << 6)    /* SPI Enable */
#define CR1_SSM             (1 << 9)    /* Software Slave Management */
#define CR1_SSI             (1 << 8)    /* Internal Slave Select */
#define CR1_DFF             (1 << 11)   /* Data Frame Format: 0=8bit, 1=16bit */

/* SPI Control Register 2 bits */
#define CR2_TXDMAEN         (1 << 1)    /* TX DMA Enable */

/* SPI Status Register bits */
#define SR_BSY              (1 << 7)    /* Busy Flag */

/* SPI Data Register address (for DMA) */
#define SPI1_DR_ADDR        (SPI1_BASE + 0x0C)

/*=============================================================================
 * PRIVATE VARIABLES
 *===========================================================================*/
static spi_callback_t user_callback = NULL;

/*=============================================================================
 * PRIVATE FUNCTION PROTOTYPES
 *===========================================================================*/
static void spi_dma_complete_handler(void);

/*=============================================================================
 * PUBLIC API IMPLEMENTATION
 *===========================================================================*/

/**
 * @brief Initialize SPI in Master mode with DMA support
 * 
 * Configuration:
 * - Master mode
 * - 8-bit data frame
 * - Clock polarity/phase: Mode 0 (CPOL=0, CPHA=0)
 * - Baud rate: fPCLK/16 (e.g., 84MHz/16 = 5.25MHz for STM32F4)
 * - Transmit-only (suitable for TFT display)
 * 
 * Why this configuration for TFT?
 * - TFT only needs data IN (MOSI), no data OUT (MISO)
 * - 5MHz+ is fast enough for smooth graphics
 * - DMA handles bulk pixel data transfer
 * 
 * @param callback: Function to call when SPI+DMA transfer completes
 */
void spi_init(spi_callback_t callback)
{
    user_callback = callback;
    
    /* Disable SPI before configuration */
    SPI_CR1 &= ~CR1_SPE;
    
    /* Configure SPI CR1:
     * - Master mode
     * - 8-bit data frame
     * - Software slave management (no NSS pin)
     * - Clock: Mode 0 (CPOL=0, CPHA=0)
     * - Baud rate: fPCLK/16
     */
    SPI_CR1 = CR1_MSTR | CR1_SSM | CR1_SSI | CR1_BR_DIV16;
    
    /* Enable TX DMA request */
    SPI_CR2 = CR2_TXDMAEN;
    
    /* Initialize DMA for SPI TX */
    dma_init(SPI1_DR_ADDR, spi_dma_complete_handler);
    
    /* Enable SPI peripheral */
    SPI_CR1 |= CR1_SPE;
}

/**
 * @brief Transmit data via SPI using DMA (non-blocking)
 * 
 * Data flow diagram:
 * 
 *   [RAM Buffer] → [DMA Controller] → [SPI_DR] → [MOSI Pin] → [TFT Display]
 *        ↑              ↑                 ↑
 *     User data    Auto transfer    Hardware shift register
 * 
 * Process:
 * 1. DMA reads byte from RAM buffer
 * 2. DMA writes byte to SPI_DR
 * 3. SPI hardware shifts byte out on MOSI pin
 * 4. Repeat until all bytes sent
 * 5. DMA interrupt fires → callback called
 * 
 * CPU is free during steps 1-4!
 * 
 * @param data: Pointer to data buffer (e.g., pixel array)
 * @param len: Number of bytes to send
 */
void spi_transmit_dma(uint8_t *data, uint16_t len)
{
    /* Start DMA transfer - DMA will feed data to SPI_DR automatically */
    dma_start_tx(data, len);
}

/**
 * @brief Check if SPI+DMA transfer is in progress
 * 
 * @return 1 if busy, 0 if idle
 */
uint8_t spi_is_busy(void)
{
    /* Check both DMA busy and SPI busy flags */
    return (dma_is_busy() || (SPI_SR & SR_BSY));
}

/*=============================================================================
 * PRIVATE CALLBACK HANDLER
 *===========================================================================*/

/**
 * @brief Internal callback called by DMA when transfer completes
 * 
 * Sequence of events:
 * 1. DMA finishes moving all bytes from RAM to SPI_DR
 * 2. DMA interrupt fires
 * 3. DMA driver calls this function
 * 4. This function waits for SPI to finish shifting last byte
 * 5. User callback is called to notify "transfer complete"
 * 
 * Why wait for SPI busy flag?
 * - DMA completes when last byte is written to SPI_DR
 * - But SPI might still be shifting that byte out on MOSI
 * - We wait for SPI_SR.BSY=0 to ensure transmission is truly done
 */
static void spi_dma_complete_handler(void)
{
    /* Wait for SPI to finish transmitting last byte */
    while (SPI_SR & SR_BSY);
    
    /* Notify user that transfer is complete */
    if (user_callback != NULL) {
        user_callback();
    }
}
