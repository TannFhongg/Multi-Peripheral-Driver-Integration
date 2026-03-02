/* dma_driver.c - DMA Driver Implementation (Memory-to-Peripheral) */

#include "dma_driver.h"

/*=============================================================================
 * HARDWARE REGISTER DEFINITIONS (STM32-like DMA)
 * Using DMA1 Channel 3 as example (commonly used for SPI1_TX)
 *===========================================================================*/
#define DMA1_BASE           0x40020000UL
#define DMA1_CH3_BASE       (DMA1_BASE + 0x30)

#define DMA_CCR             (*(volatile uint32_t *)(DMA1_CH3_BASE + 0x08))
#define DMA_CNDTR           (*(volatile uint32_t *)(DMA1_CH3_BASE + 0x0C))
#define DMA_CPAR            (*(volatile uint32_t *)(DMA1_CH3_BASE + 0x10))
#define DMA_CMAR            (*(volatile uint32_t *)(DMA1_CH3_BASE + 0x14))
#define DMA_ISR             (*(volatile uint32_t *)(DMA1_BASE + 0x00))
#define DMA_IFCR            (*(volatile uint32_t *)(DMA1_BASE + 0x04))

/* DMA Channel Configuration Register bits */
#define CCR_EN              (1 << 0)    /* Channel Enable */
#define CCR_TCIE            (1 << 1)    /* Transfer Complete Interrupt Enable */
#define CCR_DIR             (1 << 4)    /* Data Transfer Direction: 1=Read from memory */
#define CCR_MINC            (1 << 7)    /* Memory Increment Mode */
#define CCR_PSIZE_8         (0 << 8)    /* Peripheral Size: 8-bit */
#define CCR_MSIZE_8         (0 << 10)   /* Memory Size: 8-bit */

/* DMA Interrupt Status Register bits (for Channel 3) */
#define ISR_TCIF3           (1 << 9)    /* Transfer Complete Flag */

/* DMA Interrupt Flag Clear Register bits */
#define IFCR_CTCIF3         (1 << 9)    /* Clear Transfer Complete Flag */

/*=============================================================================
 * PRIVATE VARIABLES
 *===========================================================================*/
static dma_callback_t transfer_complete_callback = NULL;

/*=============================================================================
 * PUBLIC API IMPLEMENTATION
 *===========================================================================*/

/**
 * @brief Initialize DMA for memory-to-peripheral transfer
 * 
 * Configures DMA channel for one-shot transfer from RAM to peripheral.
 * Typical use: streaming pixel data from RAM to SPI_DR for TFT display.
 * 
 * @param peripheral_addr: Address of peripheral data register (e.g., SPI_DR)
 * @param callback: Function to call when transfer completes
 */
void dma_init(uint32_t peripheral_addr, dma_callback_t callback)
{
    transfer_complete_callback = callback;
    
    /* Disable DMA channel before configuration */
    DMA_CCR &= ~CCR_EN;
    
    /* Set peripheral address (destination: SPI_DR) */
    DMA_CPAR = peripheral_addr;
    
    /* Configure DMA:
     * - Memory-to-peripheral direction
     * - Memory increment (read from buffer sequentially)
     * - Peripheral no increment (always write to SPI_DR)
     * - 8-bit data size
     * - Transfer complete interrupt enabled
     */
    DMA_CCR = CCR_DIR | CCR_MINC | CCR_PSIZE_8 | CCR_MSIZE_8 | CCR_TCIE;
    
    /* Note: NVIC interrupt enable should be done in startup code or here */
    /* For demo purposes, assume NVIC is already configured */
}

/**
 * @brief Start DMA transfer from memory to peripheral
 * 
 * Data flow: RAM buffer → DMA → SPI_DR → TFT display
 * 
 * Why DMA for TFT?
 * - TFT needs thousands of bytes (e.g., 320x240x2 = 153KB for full screen)
 * - CPU would be blocked in tight loop if using polling
 * - DMA frees CPU to do other tasks while pixels are being sent
 * 
 * @param data: Pointer to source data buffer in RAM
 * @param len: Number of bytes to transfer
 */
void dma_start_tx(uint8_t *data, uint16_t len)
{
    /* Disable channel before reconfiguration */
    DMA_CCR &= ~CCR_EN;
    
    /* Set memory address (source: pixel buffer in RAM) */
    DMA_CMAR = (uint32_t)data;
    
    /* Set number of data items to transfer */
    DMA_CNDTR = len;
    
    /* Clear any pending transfer complete flag */
    DMA_IFCR = IFCR_CTCIF3;
    
    /* Enable DMA channel - transfer starts immediately */
    DMA_CCR |= CCR_EN;
}

/**
 * @brief Check if DMA transfer is in progress
 * 
 * @return 1 if busy, 0 if idle
 */
uint8_t dma_is_busy(void)
{
    return (DMA_CCR & CCR_EN) ? 1 : 0;
}

/*=============================================================================
 * DMA INTERRUPT HANDLER
 * 
 * Called by hardware when DMA transfer completes.
 * This should be registered in the interrupt vector table.
 *===========================================================================*/
void DMA1_Channel3_IRQHandler(void)
{
    /* Check if transfer complete interrupt occurred */
    if (DMA_ISR & ISR_TCIF3) {
        /* Clear interrupt flag */
        DMA_IFCR = IFCR_CTCIF3;
        
        /* Disable DMA channel (one-shot transfer) */
        DMA_CCR &= ~CCR_EN;
        
        /* Call user callback to notify transfer complete */
        if (transfer_complete_callback != NULL) {
            transfer_complete_callback();
        }
    }
}
