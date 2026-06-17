#include "app.h"
#include "display.h"
#include "button.h"
#include "system.h"
#include "sounddriver.h"
#include "drawtext.h"


#define SCREEN_W 128
#define SCREEN_H 160

#define BLOCK_W 4
#define BLOCK_H 4

#define BALL_SIZE 4

static int player_y;

static int block_x;
static int block_y;

static int block_vx;
static int block_vy;

static uint32_t score;

static int prev_player_y;
static int prev_ai_y;
static int prev_block_x;
static int prev_block_y;

static uint8_t rainbow_t = 0;

static void reset_blocks(void)
{

}

static void draw_rect(int x, int y, int w, int h, uint16_t color)
{
    display_fill_rect(x, y, w, h, color);
}

void blocks_init(void)
{
    app_set_sleep_timeout(60000);

    display_fill_rect(SCREEN_W / 2, 10, w, h, color);

}



void pong_update(uint32_t frame)
{
    (void)frame;


}


void pong_wake(void) {

}