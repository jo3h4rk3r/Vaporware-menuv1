
#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

/* Init button system (reuses ADC init from battery driver) */
void button_init(void);

/* Sample ADC and update state (call once per frame) */
void button_update(void);

/* -----------------------------
   Legacy-compatible API
------------------------------*/

/* TRUE if any direction is active */
uint8_t  button_pressed(void);

/* Edge detection (any direction) */
uint8_t  button_just_pressed(void);
uint8_t  button_just_released(void);

/* Hold duration (ms) */
uint16_t button_held_ms(void);

/* Raw ADC value */
uint16_t button_raw(void);

/* -----------------------------
   Directional API
------------------------------*/

uint8_t button_is_up(void);
uint8_t button_is_down(void);
uint8_t button_is_left(void);
uint8_t button_is_right(void);
uint8_t button_is_pressed(void);

#endif /* BUTTON_H */