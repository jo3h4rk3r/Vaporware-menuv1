/* template/src/main.c — Vaporware SDK application template
 *
 * Copy the entire template/ directory and rename it for your new app.
 * This file is the ONLY file you need to write.  The framework handles:
 *   - Hardware init (clocks, display, SPI, button, battery, IWDG)
 *   - ~30 fps frame loop with IWDG feeding
 *   - Auto-sleep after configurable idle timeout (MCU Stop mode, ~10-20 µA)
 *   - Hold-button-to-reset with configurable duration and callback
 *   - Persistent NV storage (survives power cycles)
 *   - Safe deep sleep: VCC cut (PA4+PA6 HIGH), MCU Stop mode via EXTI7 wake
 *
 * You implement three functions:
 *   app_init()       — called once after all hardware is up
 *   app_update()     — called every frame (~33 ms)
 *   app_wake()       — called after wake-from-sleep (optional: redraw screen)
 *
 * Available APIs:
 *   display_*()      — fill, fill_rect, draw_pixel, draw_image (display.h)
 *   button_*()       — pressed, just_pressed, held_ms            (button.h)
 *   nv_read/write()  — persistent flash storage, key/value u32   (nv.h)
 *   bat_read_raw()   — 12-bit ADC battery reading                 (battery.h)
 *   bat_level(raw)   — 0-3 charge bars                           (battery.h)
 *   delay_ms()       — blocking delay, feeds IWDG                 (system.h)
 *   ms_now()         — 16-bit millisecond counter, wraps ~65 s    (system.h)
 */
#include "app.h"
#include "display.h"
#include "button.h"
#include "battery.h"
#include "nv.h"
#include "system.h"
#include "pong.h"
#include "raycaster.h"
#include "fungraphics.h"
#include "brickbreaker.h"
#include "drawtext.h"
#include "sounddriver.h"
#include <stdio.h>

/* ── NV keys for this app ─────────────────────────────────────────────
 * Use the pre-allocated app keys (NV_KEY_APP_0..2) for your own data.
 * See nv.h for all available keys.                                    */
#define MY_KEY  NV_KEY_APP_0

typedef enum
{
    STATE_MENU,
    STATE_PONG,
    STATE_RAYCASTER,
    STATE_GRAPHICS,
    STATE_BRICKBREAKER
} AppState;

static AppState g_state;

#define MENU_PONG 0
#define MENU_RAYCASTER  1
#define MENU_GRAPHICS 2
#define MENU_BRICKBREAKER 3
#define MENU_COUNT    4

#define SCREEN_W 128
#define SCREEN_H 160


// ======================================================
// FAST SINE TABLE
// ======================================================
#define SIN_SIZE 256

static const int8_t sin_table[SIN_SIZE] = {
     0,  6, 12, 18, 24, 30, 36, 41, 47, 53, 58, 63, 68, 73, 78, 83,
    87, 91, 95, 99,102,106,109,112,114,117,119,121,123,124,126,127,127,
   127,127,127,126,124,123,121,119,117,114,112,109,106,102, 99, 95,
    91, 87, 83, 78, 73, 68, 63, 58, 53, 47, 41, 36, 30, 24, 18, 12,
     6,  0, -6,-12,-18,-24,-30,-36,-41,-47,-53,-58,-63,-68,-73,-78,
   -83,-87,-91,-95,-99,-102,-106,-109,-112,-114,-117,-119,-121,-123,
  -124,-126,-127,-127,-127,-127,-127,-127,-126,-124,-123,-121,-119,-117,
  -114,-112,-109,-106,-102,-99,-95,-91,-87,-83,-78,-73,-68,-63,-58,-53,
  -47,-41,-36,-30,-24,-18,-12,-6
};

static inline int8_t sin_fast(uint8_t x)
{
    return sin_table[x];
}

static inline uint8_t clamp_u8(int v)
{
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
}

static void clear_display(void);
static void menu_draw(void);
void menu_update(uint32_t frame);
static void menu_background(void);


static uint8_t t = 0;

static int menu_container_left = 10;
static int menu_button_width = 100;
static int menu_button_height = 20;
static int menu_button_spacing = 27;
static int menu_button_start_y = 15;




/* ── App state ──────────────────────────────────────────────────────── */
static uint32_t g_count = 0;

/* ── Hold-to-reset callback ─────────────────────────────────────────── */



/* ── Framework callbacks ────────────────────────────────────────────── */

void app_init(void) {



    clock_init();          

    delay_ms(50);

    /* 2. Init display at safe speed (8 MHz world) */
    //display_init();
    //display_set_backlight(80);

    //delay_ms(50);
    
    /* 3. NOW overclock CPU */
    //clock_boost_48mhz();

    /* 4. THEN increase SPI speed AFTER display is alive */
    //SPI1->CR1 = (SPI1->CR1 & ~(7UL << 3)) | SPI_CR1_BR_DIV4;

    //tim1_init();
    /* Configure framework features */

    app_set_sleep_timeout(120000);           /* sleep after 60 s idle    */
    //app_set_hold_reset(10000, on_reset);    /* hold 10 s to reset       */


    g_count = nv_read(MY_KEY, 0);

    clear_display();
    menu_background();
    menu_draw();

    //Startup sound
    play_startup();
}


static void clear_display(void) {
    display_fill(COL_RGB(0,0,0));
}

static void menu_background(void)
{
    for (int y = 0; y < SCREEN_H; y += 4)
    {
        for (int x = 0; x < SCREEN_W; x += 4)
        {
            uint8_t nx = x * 1;
            uint8_t ny = y * 1;

            int v =
                sin_fast(nx + t) +
                sin_fast(ny + t) +
                sin_fast(nx + ny + t);

                uint8_t r = 20 + sin_fast((uint8_t)v + 50);
                uint8_t g = 10 + sin_fast((uint8_t)(v + 200));
                uint8_t b = 50 + sin_fast((uint8_t)(v + 50));

               
                r >>= 2;   
                g >>= 4;
                b >>= 2;

                display_fill_rect(x, y, 2, 2, COL_RGB(r, g, b));
        }
    }
}

static void menu_draw(void)
{
    

    draw_text("Main Menu v1", 10, 10, COL_SCORE, COL_EYE);
    draw_text("Made by jo3", 10, 20, COL_SCORE, COL_EYE);


    if(g_count == MENU_PONG)
    {
        display_fill_rect(menu_container_left + 3, menu_button_start_y + menu_button_spacing + 2, menu_button_width - 5.5, menu_button_height - 4, COL_RGB(0,0,50));
    } else {
        display_fill_rect(menu_container_left + 3, menu_button_start_y + menu_button_spacing + 2, menu_button_width - 5.5, menu_button_height - 4, COL_RGB(255,0,255));
    }
    if(g_count == MENU_RAYCASTER)
    {
        display_fill_rect(menu_container_left + 3, menu_button_start_y + menu_button_spacing * 2 + 2, menu_button_width - 5.5, menu_button_height - 4, COL_RGB(0,0,50));
    } else {
        display_fill_rect(menu_container_left + 3, menu_button_start_y + menu_button_spacing * 2 + 2, menu_button_width - 5.5, menu_button_height - 4, COL_RGB(255,0,255));
    }
    if(g_count == MENU_GRAPHICS)
    {
        display_fill_rect(menu_container_left + 3, menu_button_start_y + menu_button_spacing * 3 + 2, menu_button_width - 5.5, menu_button_height - 4, COL_RGB(0,0,50));
    } else {
        display_fill_rect(menu_container_left + 3, menu_button_start_y + menu_button_spacing * 3 + 2, menu_button_width - 5.5, menu_button_height - 4, COL_RGB(255,0,255));
    }
    if(g_count == MENU_BRICKBREAKER)
    {
        display_fill_rect(menu_container_left + 3, menu_button_start_y + menu_button_spacing * 4 + 2, menu_button_width - 5.5, menu_button_height - 4, COL_RGB(0,0,50));
    } else {
        display_fill_rect(menu_container_left + 3, menu_button_start_y + menu_button_spacing * 4 + 2, menu_button_width - 5.5, menu_button_height - 4, COL_RGB(255,0,255));
    }    
   

    
    draw_text("Pong", menu_container_left * 2, menu_button_start_y + menu_button_spacing + 8, COL_SCORE, COL_EYE);

    draw_text("Raycast", menu_container_left * 2, menu_button_start_y + menu_button_spacing * 2 + 8, COL_SCORE, COL_EYE);

    draw_text("Graphics", menu_container_left * 2, menu_button_start_y + menu_button_spacing * 3 + 8, COL_SCORE, COL_EYE);

    draw_text("Brick Brkr", menu_container_left * 2, menu_button_start_y + menu_button_spacing * 4 + 8, COL_SCORE, COL_EYE);
    
}

void app_update(uint32_t frame)
{

    //Only for vape with 1 button
    /*
    if (button_held_ms() > 2500u) {
        g_state = STATE_MENU;
        clear_display();
        menu_draw();
        g_count++;
    }*/


    if (button_is_up() && button_held_ms() > 250u && g_state != STATE_MENU) {
        g_state = STATE_MENU;
        clear_display();
        menu_background();
        menu_draw();
        //g_count++;
    }

    if (g_state == STATE_MENU) {
        menu_update(frame);
    }
        
    if (g_state == STATE_PONG) {
        pong_update(frame);
    }
    
    if (g_state == STATE_RAYCASTER) {
        raycaster_update(frame);
    }

    if (g_state == STATE_GRAPHICS) {
        fun_graphics_update(frame);
    }

    if (g_state == STATE_BRICKBREAKER) {
        brickbreaker_update(frame);
    }



    
}

void menu_update(uint32_t frame) {
    (void)frame;    /* remove cast if you use frame for animation timing */
    
    char buf[16];
    uint16_t v = button_raw();
    buf[0] = '0' + ((v / 1000) % 10);
    buf[1] = '0' + ((v / 100) % 10);
    buf[2] = '0' + ((v / 10) % 10);
    buf[3] = '0' + (v % 10);
    buf[4] = '\0';
    draw_text(buf, 10, 30, COL_RGB(0,0,255), COL_EYE);


    char buf1[16];
    uint16_t b = bat_read_raw();
    buf1[0] = '0' + ((b / 1000) % 10);
    buf1[1] = '0' + ((b / 100) % 10);
    buf1[2] = '0' + ((b / 10) % 10);
    buf1[3] = '0' + (b % 10);
    buf1[4] = '\0';
    draw_text(buf1, 45, 30, COL_RGB(255,0,255), COL_EYE);



    char buf2[8];

    uint32_t psc = TIM1->ARR;

    buf2[0]='0'+((psc/10000)%10);
    buf2[1]='0'+((psc/1000)%10);
    buf2[2]='0'+((psc/100)%10);
    buf2[3]='0'+((psc/10)%10);
    buf2[4]='0'+(psc%10);
    buf2[5]=0;

    draw_text(buf2,80,30,COL_RGB(0,255,0),0);

    static uint8_t last_down = 0;
    static uint8_t last_up = 0;

    uint8_t down = button_is_down();
    uint8_t up   = button_is_up();

    
    if (button_just_pressed() && g_state == STATE_MENU) {
    if (down && !last_down) {
        g_count++;
        if (g_count >= MENU_COUNT)
            g_count = 0;

        nv_write(MY_KEY, g_count);
        menu_draw();
        beep(700, 20);
    }

    if (up && !last_up)
    {
        if (g_count == 0)
            g_count = MENU_COUNT - 1;
        else
            g_count--;

        nv_write(MY_KEY, g_count);
        menu_draw();
        beep(500, 20);
    }
    }

    last_down = down;
    last_up = up;

    if (button_is_right() && g_state == STATE_MENU) {
        if (sounddriverEnabled) {
            sounddriverEnabled = 0;
            beep(4000,50);
        }
    }

    if (button_is_left() && g_state == STATE_MENU) {
        if (!sounddriverEnabled) {
            sounddriverEnabled = 1;
            beep(6000,50);
        }
    }

    
    
    if (button_is_pressed() && g_state == STATE_MENU) {
        
        beep(2000,50);
        
        if (g_count == MENU_PONG) {
            g_state = STATE_PONG;
            clear_display();
            pong_init(); 
            return;
        }

        if (g_count == MENU_RAYCASTER) {
            g_state = STATE_RAYCASTER;
            clear_display();
            raycaster_init();
            return;
        }

        if (g_count == MENU_GRAPHICS) {
            g_state = STATE_GRAPHICS;
            clear_display();
            fun_graphics_init();
            return;
        }
        if (g_count == MENU_BRICKBREAKER) {
            g_state = STATE_BRICKBREAKER;
            clear_display();
            brickbreaker_init();
            return;
        }        
    }

    //Only for vape with 1 button
    /*
    if (g_state == STATE_MENU) {
        if (button_held_ms() > 1500u) {
            if (g_count == MENU_PONG) {
                g_state = STATE_PONG;
                clear_display();
                pong_init(); 
                return;
            }

            if (g_count == MENU_RAYCASTER) {
                g_state = STATE_RAYCASTER;
                clear_display();
                raycaster_init();
                return;
            }

            if (g_count == MENU_GRAPHICS) {
                g_state = STATE_GRAPHICS;
                clear_display();
                fun_graphics_init();
                return;
            }
            if (g_count == MENU_BRICKBREAKER) {
                g_state = STATE_BRICKBREAKER;
                clear_display();
                brickbreaker_init();
                return;
            }
        }
    }*/
    
}

void app_wake(void) {
    /* Called after the device wakes from Stop-mode sleep.
     * display_init() already ran (GRAM cleared to black) — redraw your full UI.
     * Also reset any physics timers (e.g. g_phys_t = ms_now()) so there is no
     * catch-up burst of logic ticks from the sleep gap.                        */
    /* TODO: redraw your UI here */
    menu_draw();
}
