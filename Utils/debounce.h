/* debounce.h - Button debounce header */

#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdint.h>

typedef struct {
    uint8_t state;
    uint8_t lastState;
    uint16_t counter;
    uint16_t delay;
} Debounce_t;

void Debounce_Init(Debounce_t *db, uint16_t delay);
uint8_t Debounce_Update(Debounce_t *db, uint8_t input);

#endif /* DEBOUNCE_H */
