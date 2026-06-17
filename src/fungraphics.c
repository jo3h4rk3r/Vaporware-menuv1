#include "app.h"
#include "display.h"
#include "button.h"
#include "system.h"
#include <stdint.h>

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

// ======================================================
// MODE SYSTEM
// ======================================================
typedef enum {
    MODE_PLASMA = 0,
    MODE_TUNNEL,
    MODE_FIRE,
    MODE_COUNT
} app_mode_t;

static app_mode_t mode = MODE_PLASMA;
static uint8_t last_button = 0;
static uint8_t t = 0;



// ======================================================
// UTIL
// ======================================================
static void draw_rect(int x, int y, int w, int h, uint16_t color)
{
    display_fill_rect(x, y, w, h, color);
}

// ======================================================
// BUTTON MODE SWITCH
// ======================================================

static void clear_if_needed(void) {
    draw_rect(0, 0, SCREEN_W, SCREEN_H, COL_RGB(0,0,0));
}

static void handle_button(void)
{
    uint8_t pressed = button_pressed();

    if (pressed && !last_button)
    {
        clear_if_needed();
        mode = (mode + 1) % MODE_COUNT;
    }

    last_button = pressed;
}

// ======================================================
// PLASMA
// ======================================================
static void mode_plasma(void)
{
    for (int y = 0; y < SCREEN_H; y += 1)
    {
        for (int x = 0; x < SCREEN_W; x += 1)
        {
            uint8_t nx = x * 1;
            uint8_t ny = y * 1;

            int v =
                sin_fast(nx + t) +
                sin_fast(ny + t) +
                sin_fast(nx + ny + t);

            uint8_t r = 128 + sin_fast((uint8_t)v);
            uint8_t g = 128 + sin_fast((uint8_t)(v + 85));
            uint8_t b = 128 + sin_fast((uint8_t)(v + 170));

            draw_rect(x, y, 1, 1, COL_RGB(r,g,b));
        }
    }
}

// ======================================================
// TUNNEL
// ======================================================
static void mode_tunnel(void)
{
    for (int y = 0; y < SCREEN_H; y += 10)
    {
        for (int x = 0; x < SCREEN_W; x += 10)
        {
            int nx = x - SCREEN_W / 2;
            int ny = y - SCREEN_H / 2;

            int dist = (nx * nx + ny * ny) >> 5;

            uint8_t a = (uint8_t)(nx + ny + t);

            int v = sin_fast(a) + sin_fast((uint8_t)(dist + t));


            uint8_t r = 128 + sin_fast((uint8_t)v);
            uint8_t g = 128 + sin_fast((uint8_t)(v + 50));
            uint8_t b = 128 + sin_fast((uint8_t)(v + 100));

            draw_rect(x, y, 10, 10, COL_RGB(r,g,b));
        }
    }
}

// ======================================================
// FIRE (NO BUFFER VERSION - RAM SAFE)
// ======================================================

#define STAR_COUNT 40

static int16_t star_x[STAR_COUNT];
static int16_t star_y[STAR_COUNT];
static int16_t star_z[STAR_COUNT];

static int16_t prev_x[STAR_COUNT];
static int16_t prev_y[STAR_COUNT];

static void stars_init(void)
{
    for (int i = 0; i < STAR_COUNT; i++)
    {
        star_x[i] = ((i * 97) % 256) - 128;
        star_y[i] = ((i * 53) % 256) - 128;
        star_z[i] = (i * 13) % 256 + 1;

        prev_x[i] = SCREEN_W / 2;
        prev_y[i] = SCREEN_H / 2;
    }
}

static void mode_fire(void)
{
    int cx = SCREEN_W / 2;
    int cy = SCREEN_H / 2;

    for (int i = 0; i < STAR_COUNT; i++)
    {
        /* erase old */
        draw_rect(
            prev_x[i],
            prev_y[i],
            3,
            3,
            COL_RGB(0,0,0)
        );

        /* fly toward camera */
        star_z[i] -= 6;

        if (star_z[i] <= 1)
        {
            star_x[i] = ((t * 17 + i * 91) & 255) - 128;
            star_y[i] = ((t * 29 + i * 57) & 255) - 128;
            star_z[i] = 255;
        }

        /* perspective projection */
        int sx = cx + ((star_x[i] * 128) / star_z[i]);
        int sy = cy + ((star_y[i] * 128) / star_z[i]);

        /* off screen -> respawn */
        if (sx < 0 || sx >= SCREEN_W ||
            sy < 0 || sy >= SCREEN_H)
        {
            star_x[i] = ((t * 23 + i * 41) & 255) - 128;
            star_y[i] = ((t * 11 + i * 67) & 255) - 128;
            star_z[i] = 255;
            continue;
        }

        uint8_t brightness = 255 - star_z[i];

        uint8_t size = 2;

        if (star_z[i] < 96) size = 2;
        if (star_z[i] < 48) size = 3;

        draw_rect(
            sx,
            sy,
            size,
            size,
            COL_RGB(
                brightness >> 3,
                brightness >> 2,
                brightness
            )
        );

        prev_x[i] = sx;
        prev_y[i] = sy;
    }
}

// ======================================================
// APP LIFECYCLE
// ======================================================
void fun_graphics_init(void)
{
    stars_init();
}

void fun_graphics_update(uint32_t frame)
{
    (void)frame;

    handle_button();

    switch (mode)
    {
        case MODE_PLASMA: mode_plasma(); break;
        case MODE_TUNNEL: mode_tunnel(); break;
        case MODE_FIRE:   mode_fire();   break;
        default: break;
    }
	
    t++;
}

void fun_graphics_wake(void)
{
}