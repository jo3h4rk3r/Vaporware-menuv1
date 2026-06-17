/* raycaster.c — CLEAN CONTROL VERSION
 *
 * Controls (FROM YOUR BUTTON SYSTEM):
 *   button_is_down()   = turn right
 *   button_is_up()     = turn left
 *   button_is_pressed()= move forward (tap or hold)
 *
 * Behavior:
 *   - Up/Down = smooth rotation
 *   - Press   = move forward (collision aware)
 *   - No raw GPIO anywhere
 */

#include "n32g031.h"
#include "display.h"
#include "system.h"
#include "battery.h"
#include "button.h"

/* ── Colors ───────────────────────────────────────────────────────── */
#define C_CEIL    COL_RGB(10, 60, 150)
#define C_FLOOR   COL_RGB(40, 40, 40)
#define C_WALL_EW COL_RGB(0, 180, 25)
#define C_WALL_NS COL_RGB(0, 90, 25)
//#define C_WALL_EW COL_RGB(10, 50, 50)
//#define C_WALL_NS COL_RGB(10, 60, 60)

static void draw_glyph(const uint8_t *bm, int x, int y, uint16_t fg, uint16_t bg)
{
    for (int row = 0; row < 5; row++)
    {
        uint8_t bits = bm[row];

        for (int col = 0; col < 3; col++)
        {
            uint16_t color = (bits & (1 << (2 - col))) ? fg : bg;
            display_fill_rect(x + col, y + row, 1, 1, color);
        }
    }
}

#define COL_EYE   0x0000U
#define COL_SCORE  0xFFFFU
#define COL_GOLD   0x07FFU

static int32_t ai_turn_timer = 0;
static uint8_t ai_turn_dir = 0; // 0 = left, 1 = right

static uint8_t autoplay = 1;


static const uint8_t font_A_Z[26][5] = {
    {0x7,0x5,0x7,0x5,0x5}, // A
    {0x6,0x5,0x6,0x5,0x6}, // B
    {0x7,0x4,0x4,0x4,0x7}, // C
    {0x7,0x5,0x5,0x5,0x7}, // D
    {0x7,0x4,0x7,0x4,0x7}, // E
    {0x7,0x4,0x7,0x4,0x4}, // F
    {0x7,0x4,0x5,0x5,0x7}, // G
    {0x5,0x5,0x7,0x5,0x5}, // H
    {0x7,0x2,0x2,0x2,0x7}, // I
    {0x1,0x1,0x1,0x5,0x7}, // J
    {0x5,0x6,0x4,0x6,0x5}, // K
    {0x4,0x4,0x4,0x4,0x7}, // L
    {0x5,0x7,0x7,0x5,0x5}, // M
    {0x5,0x7,0x7,0x7,0x5}, // N
    {0x7,0x5,0x5,0x5,0x7}, // O
    {0x7,0x5,0x7,0x4,0x4}, // P
    {0x7,0x5,0x5,0x7,0x1}, // Q
    {0x7,0x5,0x7,0x6,0x5}, // R
    {0x7,0x4,0x7,0x1,0x7}, // S
    {0x7,0x2,0x2,0x2,0x2}, // T
    {0x5,0x5,0x5,0x5,0x7}, // U
    {0x5,0x5,0x5,0x2,0x2}, // V
    {0x5,0x5,0x7,0x7,0x5}, // W
    {0x5,0x5,0x2,0x5,0x5}, // X
    {0x5,0x5,0x2,0x2,0x2}, // Y
    {0x7,0x1,0x2,0x4,0x7}, // Z
};

static const uint8_t font_0_9[10][5] = {
    {0x7,0x5,0x5,0x5,0x7}, // 0
    {0x2,0x6,0x2,0x2,0x7}, // 1
    {0x7,0x1,0x7,0x4,0x7}, // 2
    {0x7,0x1,0x7,0x1,0x7}, // 3
    {0x5,0x5,0x7,0x1,0x1}, // 4
    {0x7,0x4,0x7,0x1,0x7}, // 5
    {0x7,0x4,0x7,0x5,0x7}, // 6
    {0x7,0x1,0x1,0x1,0x1}, // 7
    {0x7,0x5,0x7,0x5,0x7}, // 8
    {0x7,0x5,0x7,0x1,0x7}, // 9
};

static const uint8_t* get_char_bitmap(char c)
{
    if (c >= '0' && c <= '9')
        return font_0_9[c - '0'];

    if (c >= 'A' && c <= 'Z')
        return font_A_Z[c - 'A'];

    if (c >= 'a' && c <= 'z')
        return font_A_Z[c - 'a'];

    return font_0_9[0]; // fallback = "0"
}

static void draw_text(const char* s, int x, int y, uint16_t fg, uint16_t bg)
{
    while (*s)
    {
        if (*s == ' ') {
            x += 8;   // space width
            s++;
            continue;
        }

        const uint8_t* glyph = get_char_bitmap(*s);

        draw_glyph(glyph, x, y, fg, bg);

        x += 8; // character spacing
        s++;
    }
}

/* ── Q12 sine table (256 entries, 1.0 = 4096) ───────────────────────  */
static const int16_t sin_lut[256] = {
        0,   101,   201,   301,   401,   501,   601,   700,   799,   897,   995,  1092,  1189,  1285,  1380,  1474,
     1567,  1660,  1751,  1842,  1931,  2019,  2106,  2191,  2276,  2359,  2440,  2520,  2598,  2675,  2751,  2824,
     2896,  2967,  3035,  3102,  3166,  3229,  3290,  3349,  3406,  3461,  3513,  3564,  3612,  3659,  3703,  3745,
     3784,  3822,  3857,  3889,  3920,  3948,  3973,  3996,  4017,  4036,  4052,  4065,  4076,  4085,  4091,  4095,
     4096,  4095,  4091,  4085,  4076,  4065,  4052,  4036,  4017,  3996,  3973,  3948,  3920,  3889,  3857,  3822,
     3784,  3745,  3703,  3659,  3612,  3564,  3513,  3461,  3406,  3349,  3290,  3229,  3166,  3102,  3035,  2967,
     2896,  2824,  2751,  2675,  2598,  2520,  2440,  2359,  2276,  2191,  2106,  2019,  1931,  1842,  1751,  1660,
     1567,  1474,  1380,  1285,  1189,  1092,   995,   897,   799,   700,   601,   501,   401,   301,   201,   101,
        0,  -101,  -201,  -301,  -401,  -501,  -601,  -700,  -799,  -897,  -995, -1092, -1189, -1285, -1380, -1474,
    -1567, -1660, -1751, -1842, -1931, -2019, -2106, -2191, -2276, -2359, -2440, -2520, -2598, -2675, -2751, -2824,
    -2896, -2967, -3035, -3102, -3166, -3229, -3290, -3349, -3406, -3461, -3513, -3564, -3612, -3659, -3703, -3745,
    -3784, -3822, -3857, -3889, -3920, -3948, -3973, -3996, -4017, -4036, -4052, -4065, -4076, -4085, -4091, -4095,
    -4096, -4095, -4091, -4085, -4076, -4065, -4052, -4036, -4017, -3996, -3973, -3948, -3920, -3889, -3857, -3822,
    -3784, -3745, -3703, -3659, -3612, -3564, -3513, -3461, -3406, -3349, -3290, -3229, -3166, -3102, -3035, -2967,
    -2896, -2824, -2751, -2675, -2598, -2520, -2440, -2359, -2276, -2191, -2106, -2019, -1931, -1842, -1751, -1660,
    -1567, -1474, -1380, -1285, -1189, -1092,  -995,  -897,  -799,  -700,  -601,  -501,  -401,  -301,  -201,  -101,
};
#define sin_q12(i) sin_lut[(uint8_t)(i)]
#define cos_q12(i) sin_lut[(uint8_t)((i) + 64)]

/* ── Map ─────────────────────────────────────────────────────────── */
static const uint8_t map[8][8] = {
    {1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,1},
    {1,0,1,1,0,1,0,1},
    {1,0,1,0,0,0,0,1},
    {1,0,0,0,1,0,0,1},
    {1,0,1,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1},
};


/* ── Player ───────────────────────────────────────────────────────── */
static int32_t px, py;
static uint8_t angle;

/* ── Controls tuning ──────────────────────────────────────────────── */
#define MOVE_SPEED 1024
#define ROT_SPEED  3

/* ── Raycast ──────────────────────────────────────────────────────── */
static int32_t cast_ray(int32_t px_q12, int32_t py_q12,
                        int32_t rdx,    int32_t rdy,
                        int    *out_side)
{
    int map_x = (int)(px_q12 >> 12);
    int map_y = (int)(py_q12 >> 12);

    int step_x = (rdx >= 0) ? 1 : -1;
    int step_y = (rdy >= 0) ? 1 : -1;

    int32_t frac_x = px_q12 - ((int32_t)map_x << 12);
    int32_t frac_y = py_q12 - ((int32_t)map_y << 12);

    int32_t delta_dist_x, side_dist_x;
    int32_t delta_dist_y, side_dist_y;

    if (rdx == 0) {
        delta_dist_x = 0x7FFFFFFF; side_dist_x = 0x7FFFFFFF;
    } else if (rdx > 0) {
        delta_dist_x = (int32_t)4096 * 4096 / rdx;
        side_dist_x  = (int32_t)(4096 - frac_x) * 4096 / rdx;
    } else {
        delta_dist_x = (int32_t)4096 * 4096 / (-rdx);
        side_dist_x  = (int32_t)frac_x * 4096 / (-rdx);
    }

    if (rdy == 0) {
        delta_dist_y = 0x7FFFFFFF; side_dist_y = 0x7FFFFFFF;
    } else if (rdy > 0) {
        delta_dist_y = (int32_t)4096 * 4096 / rdy;
        side_dist_y  = (int32_t)(4096 - frac_y) * 4096 / rdy;
    } else {
        delta_dist_y = (int32_t)4096 * 4096 / (-rdy);
        side_dist_y  = (int32_t)frac_y * 4096 / (-rdy);
    }

    int side = 0;
    for (int i = 0; i < 32; i++) {
        if (side_dist_x < side_dist_y) {
            side_dist_x += delta_dist_x;
            map_x += step_x;
            side = 0;
        } else {
            side_dist_y += delta_dist_y;
            map_y += step_y;
            side = 1;
        }
        if (map_x < 0 || map_x >= 8 || map_y < 0 || map_y >= 8)
            return 0x7FFFFFFF;
        if (map[map_y][map_x] > 0)
            break;
    }

    *out_side = side;
    return (side == 0) ? (side_dist_x - delta_dist_x)
                       : (side_dist_y - delta_dist_y);
}

/* ── Init ─────────────────────────────────────────────────────────── */
int raycaster_init(void)
{
    display_fill(C_FLOOR);

    display_fill_rect(
        0,
        0,
        128,
        80,
        C_CEIL
    );

    px = (1 * 4096) + 2048;
    py = (1 * 4096) + 2048;
    angle = 0;

    return 0;
}

/* ── Main loop ────────────────────────────────────────────────────── */
void raycaster_update(uint32_t frame) {

    (void)frame;    /* remove cast if you use frame for animation timing */
    /* ── INPUT (CLEAN + STABLE) ─────────────────────────────── */


    if (button_is_down()) {
        autoplay = !autoplay;
    }

    if (autoplay) {
    /* ── AI CAMERA CONTROL (screensaver mode) ───────────────────── */

        ai_turn_timer++;

        int32_t dx = (cos_q12(angle) * MOVE_SPEED) >> 12;
        int32_t dy = (sin_q12(angle) * MOVE_SPEED) >> 12;

        int32_t nx = px + dx;
        int32_t ny = py + dy;

        /* wall hit → pick new direction */
        if (map[ny >> 12][nx >> 12]) {
            ai_turn_dir = (ms_now() & 1); // random-ish
            ai_turn_timer = 0;

            angle += (ai_turn_dir ? ROT_SPEED : -ROT_SPEED) * 20;
        } else {
           px = nx;
            py = ny;
        }
    
        angle += (sin_lut[ms_now() & 255] >> 10);

        /* slow wandering turns even when not stuck */
        if ((ai_turn_timer & 127) == 0)
        {
            angle += (ms_now() & 1) ? ROT_SPEED : -ROT_SPEED;
        }
    } else {
        if (button_is_left())
            angle -= ROT_SPEED;

        if (button_is_right())
            angle += ROT_SPEED;

    
        if (button_is_pressed()) {
            int32_t dx = (cos_q12(angle) * MOVE_SPEED) >> 12;
            int32_t dy = (sin_q12(angle) * MOVE_SPEED) >> 12;

            int32_t nx = px + dx;
            int32_t ny = py + dy;

            if (!map[py >> 12][nx >> 12]) px = nx;
            if (!map[ny >> 12][px >> 12]) py = ny;
        }
    }
    


    uint16_t start = ms_now();

    /* ── CAMERA ────────────────────────────────────────────── */

    int32_t dir_x = cos_q12(angle);
    int32_t dir_y = sin_q12(angle);

    int32_t plane_x = (-sin_q12(angle) * 2700) >> 12;
    int32_t plane_y = ( cos_q12(angle) * 2700) >> 12;

    /* ── RENDER ─────────────────────────────────────────────── */

    for (uint8_t x = 0; x < 128; x += 3)
    {
        int32_t cam = (2 * x) - 127;

        int32_t rdx = dir_x + ((int32_t)plane_x * cam) / 127;
        int32_t rdy = dir_y + ((int32_t)plane_y * cam) / 127;

        int side;
        int32_t dist = cast_ray(px, py, rdx, rdy, &side);

   

        if (dist < 64)
            dist = 64;

        int32_t h = (150 * 4096) / dist;

        if (h > 150)
            h = 150;
        
        uint8_t top = (150 - h) / 2;
        uint8_t bot = top + h;

        if (bot > 150)
            bot = 150;

        uint16_t col = side ? C_WALL_NS : C_WALL_EW;

        
        display_fill_rect(x, 0, 3, top, C_CEIL);
        display_fill_rect(x, top, 3, bot - top, col);
        display_fill_rect(x, bot, 3, 150 - bot, C_FLOOR);
    }

     /* ── END TIMING ─────────────────────────────────────── */

    uint16_t elapsed = ms_now() - start;

    char buf[6];

    buf[0] = '0' + ((elapsed / 1000) % 10);
    buf[1] = '0' + ((elapsed / 100) % 10);
    buf[2] = '0' + ((elapsed / 10) % 10);
    buf[3] = '0' + ( elapsed % 10);
    buf[4] = '\0';
    /*
    if (RCC->CFGR & RCC_CFGR_SWS_PLL) {
        draw_text("48Mhz OC", 60, 0, COL_RGB(255,0,255), COL_RGB(0,0,0));
    } else {
        draw_text("8Mhz", 60, 0, COL_RGB(255,25,25), COL_RGB(0,0,0));
    }*/
    draw_text(buf, 20, 150, COL_RGB(255,0,255), C_FLOOR);




}