

#include "spi_driver.h"
#include "../dma/dma_driver.h"

#include <stddef.h>

#define SPI1_BASE           0x40013000UL

#define SPI_CR1             (*(volatile uint32_t *)(SPI1_BASE + 0x00))
#define SPI_CR2             (*(volatile uint32_t *)(SPI1_BASE + 0x04))
#define SPI_SR              (*(volatile uint32_t *)(SPI1_BASE + 0x08))
#define SPI_DR              (*(volatile uint32_t *)(SPI1_BASE + 0x0C))

// SPI Control Register 1 bits
#define CR1_CPHA            (1 << 0)    // Clock Phase
#define CR1_CPOL            (1 << 1)    // Clock Polarity
#define CR1_MSTR            (1 << 2)    // Master Selection
#define CR1_BR_DIV16        (3 << 3)    // Baud Rate: fPCLK/16
#define CR1_SPE             (1 << 6)    // SPI Enable
#define CR1_SSM             (1 << 9)    // Software Slave Management
#define CR1_SSI             (1 << 8)    // Internal Slave Select
#define CR1_DFF             (1 << 11)   // Data Frame Format: 0=8bit, 1=16bit

// SPI Control Register 2 bits
#define CR2_TXDMAEN         (1 << 1)    // TX DMA Enable

// SPI Status Register bits
#define SR_BSY              (1 << 7)    // Busy Flag
#define SR_TXE              (1 << 1)    // TX Empty Flag

// SPI Data Register address (for DMA)
#define SPI1_DR_ADDR        (SPI1_BASE + 0x0C)


static spi_callback_t user_callback = NULL;
static volatile uint8_t dma_transfer_complete = 0;


static void spi_dma_complete_handler(void);
static void spi_dma_error_handler(void);

void spi_init(spi_callback_t callback)
{
    user_callback = callback;
    
    // Disable SPI before configuration
    SPI_CR1 &= ~CR1_SPE;
    
    // Configure SPI CR1:
    // - Master mode
    // - 8-bit data frame
    // - Software slave management (no NSS pin)
    // - Clock: Mode 0 (CPOL=0, CPHA=0)
    // - Baud rate: fPCLK/16
    SPI_CR1 = CR1_MSTR | CR1_SSM | CR1_SSI | CR1_BR_DIV16;
    
    // Enable TX DMA request
    SPI_CR2 = CR2_TXDMAEN;
    
    // Initialize DMA for SPI TX
    dma_init(SPI1_DR_ADDR, spi_dma_complete_handler);
    dma_set_error_callback(spi_dma_error_handler);
    
    // Enable SPI peripheral
    SPI_CR1 |= CR1_SPE;
}

void spi_transmit_dma(uint8_t *data, uint16_t len)
{
    // Clear completion flag
    dma_transfer_complete = 0;
    
    // Start DMA transfer - DMA will feed data to SPI_DR automatically
    dma_start_tx(data, len);
}

uint8_t spi_is_busy(void)
{
    // Check DMA busy flag
    if (dma_is_busy()) {
        return 1;
    }
    
    // If DMA complete but SPI still transmitting last byte
    if (dma_transfer_complete && (SPI_SR & SR_BSY)) {
        return 1;
    }
    
    return 0;
}

static void spi_dma_complete_handler(void)
{
    // DMA transfer complete set flag for polling
    dma_transfer_complete = 1;
    
    //  poll spi_is_busy() to ensure SPI BSY clear
   
    if (user_callback != NULL) {
        user_callback();
    }
}

static void spi_dma_error_handler(void)
{
    dma_transfer_complete = 1;

    if (user_callback != NULL) {
        user_callback();
    }
}
