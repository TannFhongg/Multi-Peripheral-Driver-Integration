/* example_tft_usage.c - SPI+DMA Usage Example for TFT Display */

#include "spi_driver.h"
#include <stdint.h>

/*=============================================================================
 * EXAMPLE: Streaming pixel data to TFT display using SPI+DMA
 *===========================================================================*/

/* Example: 16-bit RGB565 color buffer for a 100x100 pixel area */
#define PIXEL_COUNT     (100 * 100)
#define BUFFER_SIZE     (PIXEL_COUNT * 2)  /* 2 bytes per pixel (RGB565) */

static uint8_t pixel_buffer[BUFFER_SIZE];
static volatile uint8_t transfer_done = 0;

/* Transfer complete callback */
void tft_transfer_complete(void)
{
    transfer_done = 1;
    /* Can now prepare next frame or update UI */
}

/* Initialize TFT communication */
void tft_init(void)
{
    /* Initialize SPI with DMA */
    spi_init(tft_transfer_complete);
    
    /* TODO: Send TFT initialization commands here */
    /* Example: set window, set pixel format, etc. */
}

/* Fill buffer with solid color (RGB565 format) */
void fill_buffer_with_color(uint16_t color)
{
    uint16_t i;
    for (i = 0; i < PIXEL_COUNT; i++) {
        pixel_buffer[i * 2] = (color >> 8) & 0xFF;      /* High byte */
        pixel_buffer[i * 2 + 1] = color & 0xFF;         /* Low byte */
    }
}

/* Send pixel data to TFT (non-blocking) */
void tft_draw_rectangle(void)
{
    /* Fill buffer with red color (RGB565: 0xF800) */
    fill_buffer_with_color(0xF800);
    
    /* Start DMA transfer - CPU is now free! */
    transfer_done = 0;
    spi_transmit_dma(pixel_buffer, BUFFER_SIZE);
    
    /* CPU can do other work here while DMA sends data */
    /* For example: prepare next frame, handle user input, etc. */
}

/* Main application example */
void example_main(void)
{
    tft_init();
    
    while (1) {
        /* Draw red rectangle */
        tft_draw_rectangle();
        
        /* Wait for transfer to complete */
        while (!transfer_done);
        
        /* Small delay before next frame */
        for (volatile uint32_t i = 0; i < 1000000; i++);
        
        /* Can now send next frame */
    }
}

/*=============================================================================
 * PERFORMANCE COMPARISON
 *===========================================================================*/

/* WITHOUT DMA (blocking, CPU busy):
 * 
 * void spi_send_byte(uint8_t data) {
 *     SPI_DR = data;
 *     while (!(SPI_SR & SR_TXE));  // CPU waits here!
 * }
 * 
 * for (i = 0; i < 20000; i++) {
 *     spi_send_byte(buffer[i]);    // CPU blocked for entire transfer
 * }
 * 
 * Result: CPU 100% busy, cannot do anything else
 */

/* WITH DMA (non-blocking, CPU free):
 * 
 * spi_transmit_dma(buffer, 20000);
 * 
 * // CPU is free here! Can do:
 * // - Prepare next frame
 * // - Handle button input
 * // - Update sensor readings
 * // - Run other tasks
 * 
 * Result: CPU ~5% busy, can multitask efficiently
 */

/*=============================================================================
 * TYPICAL TFT COMMAND SEQUENCE
 *===========================================================================*/

/* Example: Draw image on ILI9341 TFT (320x240 pixels) */
void tft_draw_full_screen_image(uint8_t *image_data)
{
    /* Step 1: Set column address (X coordinates) */
    // Send command 0x2A, then 4 bytes: X_start_H, X_start_L, X_end_H, X_end_L
    
    /* Step 2: Set page address (Y coordinates) */
    // Send command 0x2B, then 4 bytes: Y_start_H, Y_start_L, Y_end_H, Y_end_L
    
    /* Step 3: Write to RAM */
    // Send command 0x2C
    
    /* Step 4: Stream pixel data using DMA */
    spi_transmit_dma(image_data, 320 * 240 * 2);  // 153,600 bytes!
    
    /* DMA handles the heavy lifting - CPU is free */
}
