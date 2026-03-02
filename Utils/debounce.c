/* debounce.c - Button debounce implementation */

#include "debounce.h"

void Debounce_Init(Debounce_t *db, uint16_t delay)
{
    db->state = 0;
    db->lastState = 0;
    db->counter = 0;
    db->delay = delay;
}

uint8_t Debounce_Update(Debounce_t *db, uint8_t input)
{
    if (input != db->lastState) {
        db->counter = 0;
        db->lastState = input;
    } else {
        if (db->counter < db->delay) {
            db->counter++;
        } else {
            db->state = input;
        }
    }
    return db->state;
}
