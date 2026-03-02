/* tft.h - TFT display header */

#ifndef TFT_H
#define TFT_H

#include <stdint.h>

void TFT_Init(void);
void TFT_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void TFT_Clear(uint16_t color);

#endif /* TFT_H */
