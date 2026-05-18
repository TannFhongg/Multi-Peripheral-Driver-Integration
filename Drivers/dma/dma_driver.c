
#include "dma_driver.h"

#define DMA1_BASE           0x40020000UL
#define DMA1_CH3_BASE       (DMA1_BASE + 0x30)

#define DMA_CCR             (*(volatile uint32_t *)(DMA1_CH3_BASE + 0x08))
#define DMA_CNDTR           (*(volatile uint32_t *)(DMA1_CH3_BASE + 0x0C))
#define DMA_CPAR            (*(volatile uint32_t *)(DMA1_CH3_BASE + 0x10))
#define DMA_CMAR            (*(volatile uint32_t *)(DMA1_CH3_BASE + 0x14))
#define DMA_ISR             (*(volatile uint32_t *)(DMA1_BASE + 0x00))
#define DMA_IFCR            (*(volatile uint32_t *)(DMA1_BASE + 0x04))

// DMA Channel Configuration Register bits
#define CCR_EN              (1 << 0)    // Channel Enable
#define CCR_TCIE            (1 << 1)    // Transfer Complete Interrupt Enable
#define CCR_HTIE            (1 << 2)    // Half Transfer Interrupt Enable
#define CCR_TEIE            (1 << 3)    // Transfer Error Interrupt Enable
#define CCR_DIR             (1 << 4)    // Data Transfer Direction: 1=Read from memory
#define CCR_MINC            (1 << 7)    // Memory Increment Mode
#define CCR_PSIZE_8         (0 << 8)    // Peripheral Size: 8-bit
#define CCR_MSIZE_8         (0 << 10)   // Memory Size: 8-bit

// DMA Interrupt Status Register bits (for Channel 3)
#define ISR_TCIF3           (1 << 9)    // Transfer Complete Flag
#define ISR_HTIF3           (1 << 10)   // Half Transfer Flag
#define ISR_TEIF3           (1 << 11)   // Transfer Error Flag

// DMA Interrupt Flag Clear Register bits
#define IFCR_CTCIF3         (1 << 9)    // Clear Transfer Complete Flag
#define IFCR_CHTIF3         (1 << 10)   // Clear Half Transfer Flag
#define IFCR_CTEIF3         (1 << 11)   // Clear Transfer Error Flag


static dma_callback_t transfer_complete_callback = NULL;
static volatile uint32_t dma_error_count = 0;

void dma_init(uint32_t peripheral_addr, dma_callback_t callback)
{
    transfer_complete_callback = callback;
    
    // Disable DMA channel before configuration
    DMA_CCR &= ~CCR_EN;
    
    // Set peripheral address (destination: SPI_DR)
    DMA_CPAR = peripheral_addr;
    
    // Configure DMA:
    // - Memory-to-peripheral direction
    // - Memory increment (read from buffer sequentially)
    // - Peripheral no increment (always write to SPI_DR)
    // - 8-bit data size
    // - Transfer complete interrupt enabled
    // - Transfer error interrupt enabled
    DMA_CCR = CCR_DIR | CCR_MINC | CCR_PSIZE_8 | CCR_MSIZE_8 | CCR_TCIE | CCR_TEIE;
    
    // Note: NVIC configuration is done in system_init.c
}

void dma_start_tx(uint8_t *data, uint16_t len)
{
    // Disable channel before reconfiguration
    DMA_CCR &= ~CCR_EN;
    
    // Set memory address (source: pixel buffer in RAM)
    DMA_CMAR = (uint32_t)data;
    
    // Set number of data items to transfer
    DMA_CNDTR = len;
    
    // Clear any pending transfer complete flag
    DMA_IFCR = IFCR_CTCIF3;
    
    // Enable DMA channel starts immediately
    DMA_CCR |= CCR_EN;
}

uint8_t dma_is_busy(void)
{
    return (DMA_CCR & CCR_EN) ? 1 : 0;
}

void DMA1_Channel3_IRQHandler(void)
{
    uint32_t isr = DMA_ISR;  // Read interrupt status
    
    // Check for transfer error
    if (isr & ISR_TEIF3) {
        // Transfer error occurred
        dma_error_count++;
        
        // Clear error flag
        DMA_IFCR = IFCR_CTEIF3;
        
        // Disable DMA channel
        DMA_CCR &= ~CCR_EN;
    
    }
    
    // Check if transfer complete interrupt 
    if (isr & ISR_TCIF3) {
        // Clear interrupt flag
        DMA_IFCR = IFCR_CTCIF3;
        
        // Disable DMA channel
        DMA_CCR &= ~CCR_EN;
        
        // Call callback  notify transfer complete
        if (transfer_complete_callback != NULL) {
            transfer_complete_callback();
        }
    }
}

uint32_t dma_get_error_count(void)
{
    return dma_error_count;
}
