#ifndef SOUNDDRIVER_H
#define SOUNDDRIVER_H

#include <stdint.h>

extern uint8_t sounddriverEnabled;

void coil_set(uint8_t on);
void delay_us(uint32_t us);
void play_startup(void);
void laser_sound(void);
void coin_sound(void);
void explosion_sound(void);
void beep(uint16_t hz, uint16_t ms);

#endif