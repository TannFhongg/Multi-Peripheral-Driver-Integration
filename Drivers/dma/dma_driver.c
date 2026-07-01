
#include "dma_driver.h"

#include <stddef.h>

// DMA2 Stream3 Channel 3 is mapp SPI1_TX

#define DMA2_BASE           0x40026400UL
#define DMA2_STREAM3_BASE   (DMA2_BASE + 0x10 + (0x18 * 3)) 

// DMA Stream Configuration Register
#define DMA_S3CR            (*(volatile uint32_t *)(DMA2_STREAM3_BASE + 0x00))
// DMA Stream Number of Data Register
#define DMA_S3NDTR          (*(volatile uint32_t *)(DMA2_STREAM3_BASE + 0x04))
// DMA Stream Peripheral Address Register
#define DMA_S3PAR           (*(volatile uint32_t *)(DMA2_STREAM3_BASE + 0x08))
// DMA Stream Memory 0 Address Register
#define DMA_S3M0AR          (*(volatile uint32_t *)(DMA2_STREAM3_BASE + 0x0C))

// DMA Low Interrupt Status Register (Streams 0-3)
#define DMA2_LISR           (*(volatile uint32_t *)(DMA2_BASE + 0x00))
// DMA Low Interrupt Flag Clear Register (Streams 0-3)
#define DMA2_LIFCR          (*(volatile uint32_t *)(DMA2_BASE + 0x08))


// DMA Stream config (DMA_SxCR) Bits

#define CR_EN               (1 << 0)     // Stream Enable
#define CR_DMEIE            (1 << 1)     // Direct Mode Error Interrupt Enable
#define CR_TEIE             (1 << 2)     // Transfer Error Interrupt Enable
#define CR_HTIE             (1 << 3)     // Half Transfer Interrupt Enable
#define CR_TCIE             (1 << 4)     // Transfer Complete Interrupt Enable
#define CR_PFCTRL           (1 << 5)     // Peripheral Flow Controller
#define CR_DIR_MEM_TO_PER   (1 << 6)     // Data Transfer Direction: Memory to Peripheral
#define CR_CIRC             (1 << 8)     // Circular Mode
#define CR_PINC             (1 << 9)     // Peripheral Increment Mode
#define CR_MINC             (1 << 10)    // Memory Increment Mode
#define CR_PSIZE_8BIT       (0 << 11)    // Peripheral Data Size: 8-bit
#define CR_MSIZE_8BIT       (0 << 13)    // Memory Data Size: 8-bit
#define CR_PL_HIGH          (2 << 16)    // Priority Level: High
#define CR_CHSEL_CH3        (3 << 25)    // Channel Selection: Channel 3 (SPI1_TX)

// DMA Interrupt Status/Clear Bits for Stream 3 (bits 22-27 in LISR/LIFCR)

#define LISR_FEIF3          (1 << 22)    // Stream 3 FIFO Error Interrupt Flag
#define LISR_DMEIF3         (1 << 24)    // Stream 3 Direct Mode Error Interrupt Flag
#define LISR_TEIF3          (1 << 25)    // Stream 3 Transfer Error Interrupt Flag
#define LISR_HTIF3          (1 << 26)    // Stream 3 Half Transfer Interrupt Flag
#define LISR_TCIF3          (1 << 27)    // Stream 3 Transfer Complete Interrupt Flag

#define LIFCR_CFEIF3        (1 << 22)    // Clear Stream 3 FIFO Error Flag
#define LIFCR_CDMEIF3       (1 << 24)    // Clear Stream 3 Direct Mode Error Flag
#define LIFCR_CTEIF3        (1 << 25)    // Clear Stream 3 Transfer Error Flag
#define LIFCR_CHTIF3        (1 << 26)    // Clear Stream 3 Half Transfer Flag
#define LIFCR_CTCIF3        (1 << 27)    // Clear Stream 3 Transfer Complete Flag
#define LIFCR_CLEAR_ALL_S3  (LIFCR_CFEIF3 | LIFCR_CDMEIF3 | LIFCR_CTEIF3 | LIFCR_CHTIF3 | LIFCR_CTCIF3)


static dma_callback_t transfer_complete_callback = NULL;
static dma_callback_t transfer_error_callback = NULL;
static volatile uint32_t dma_error_count = 0;


// DMA Init

void dma_init(uint32_t peripheral_addr, dma_callback_t callback)
{
    transfer_complete_callback = callback;
    transfer_error_callback = NULL;
    
    // 1. Disable DMA stream before configuration
    DMA_S3CR &= ~CR_EN;
    
    // 2. Wait until stream is fully disabled (EN bit reads 0)
    while (DMA_S3CR & CR_EN);
    
    // 3. Clear all interrupt flags for Stream 3
    DMA2_LIFCR = LIFCR_CLEAR_ALL_S3;
    
    // 4. Set peripheral address (destination: SPI1_DR)
    DMA_S3PAR = peripheral_addr;
    
    // 5. Configure DMA Stream Control Register:
    //    - Channel 3 (SPI1_TX on DMA2 Stream 3)
    //    - Memory-to-Peripheral direction
    //    - Memory increment enabled (read buffer sequentially)
    //    - Peripheral increment disabled (always write to SPI_DR)
    //    - 8-bit data size for both memory and peripheral
    //    - High priority
    //    - Transfer complete interrupt enabled
    //    - Transfer error interrupt enabled
    DMA_S3CR = CR_CHSEL_CH3        |  // Channel 3 for SPI1_TX
               CR_DIR_MEM_TO_PER   |  // Memory to Peripheral
               CR_MINC             |  // Memory increment mode
               CR_PSIZE_8BIT       |  // Peripheral size: 8-bit
               CR_MSIZE_8BIT       |  // Memory size: 8-bit
               CR_PL_HIGH          |  // Priority: High
               CR_TCIE             |  // Transfer Complete Interrupt Enable
               CR_DMEIE            |  // Direct Mode Error Interrupt Enable
               CR_TEIE;               // Transfer Error Interrupt Enable
    
    // Note: NVIC configuration for DMA2_Stream3_IRQn is done in system_init.c
}

void dma_set_error_callback(dma_callback_t callback)
{
    transfer_error_callback = callback;
}

// Start DMA Transfer

void dma_start_tx(uint8_t *data, uint16_t len)
{
    // 1. Disable stream before reconfiguration (mandatory)
    DMA_S3CR &= ~CR_EN;
    
    // 2. Wait until stream is fully disabled
    while (DMA_S3CR & CR_EN);
    
    // 3. Clear all interrupt flags
    DMA2_LIFCR = LIFCR_CLEAR_ALL_S3;
    
    // 4. Set memory address (source: pixel buffer in RAM)
    DMA_S3M0AR = (uint32_t)data;
    
    // 5. Set number of data items to transfer
    DMA_S3NDTR = len;
    
    // 6. Enable DMA stream - transfer starts immediately
    DMA_S3CR |= CR_EN;
}

// Check if DMA is Busy

uint8_t dma_is_busy(void)
{
    return (DMA_S3CR & CR_EN) ? 1 : 0;
}

static void notify_dma_error(void)
{
    if (transfer_error_callback != NULL) {
        transfer_error_callback();
    } else if (transfer_complete_callback != NULL) {
        transfer_complete_callback();
    }
}

// DMA2 Stream 3 Interrupt Handler (STM32F4)

// IRQ Handler: DMA2_Stream3_IRQHandler
void DMA2_Stream3_IRQHandler(void)
{
    uint32_t lisr = DMA2_LISR;  // Read Low Interrupt Status Register
    uint32_t error_clear_flags = 0;
    
    // Check for transfer error
    if (lisr & LISR_TEIF3) {
        // Transfer error occurred
        dma_error_count++;
        error_clear_flags |= LIFCR_CTEIF3;
    }
    
    // Check for direct mode error
    if (lisr & LISR_DMEIF3) {
        // Direct mode error occurred
        dma_error_count++;
        error_clear_flags |= LIFCR_CDMEIF3;
    }

    if (error_clear_flags != 0) {
        // Clear error flags
        DMA2_LIFCR = error_clear_flags;

        // Disable DMA stream
        DMA_S3CR &= ~CR_EN;

        notify_dma_error();
        return;
    }
    
    // Check if transfer complete interrupt
    if (lisr & LISR_TCIF3) {
        // Clear interrupt flag
        DMA2_LIFCR = LIFCR_CTCIF3;
        
        // Disable DMA stream
        DMA_S3CR &= ~CR_EN;
        
        // Call callback to notify transfer complete
        if (transfer_complete_callback != NULL) {
            transfer_complete_callback();
        }
    }
}

uint32_t dma_get_error_count(void)
{
    return dma_error_count;
}
