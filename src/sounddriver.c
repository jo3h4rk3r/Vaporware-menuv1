#include "sounddriver.h"
#include "n32g031.h"
#include "system.h"

#define COIL_PORT GPIOA
#define COIL_PIN  5

uint8_t sounddriverEnabled = 1;

void coil_set(uint8_t on) {
    if (sounddriverEnabled) {
        uint32_t m = COIL_PORT->MODER;
        COIL_PORT->MODER = (m & ~(3UL << (COIL_PIN * 2))) |
                        (1UL << (COIL_PIN * 2));
        if (on) GPIO_SET(COIL_PORT, COIL_PIN);
        else    GPIO_CLR(COIL_PORT, COIL_PIN);
    }
}

void beep(uint16_t hz, uint16_t ms) {
    if (sounddriverEnabled) {
        hz *= 10;

        uint32_t period_us = 1000000UL / hz;

        uint16_t start = ms_now();

        while ((uint16_t)(ms_now() - start) < ms)
        {
            coil_set(1);
            delay_us(period_us / 2);

            coil_set(0);
            delay_us(period_us / 2);
        }
    }
}


void delay_us(uint32_t us) {
    volatile uint32_t count;

    while(us--)
    {
        count = 8;   // tune experimentally
        while(count--);
    }
}

void play_startup(void) {
        beep(932.5, 20); 
        beep(740, 20); 
        beep(830.5, 20);  
        beep(622.5, 20);
}

void laser_sound(void) {
        for (int f = 2000; f > 300; f -= 50)
        {
            beep(f, 2);
        }
}

void coin_sound(void) {
        beep(523, 50);
        beep(659, 50);
        beep(784, 100);
}

void explosion_sound(void) {
        for (int f = 100; f < 1500; f += 40)
        {
            beep(f, 1);
        }

        for (int f = 1500; f > 50; f -= 30)
        {
            beep(f, 1);
        }
}