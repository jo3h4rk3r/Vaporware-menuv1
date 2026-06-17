#include "app.h"
#include "display.h"
#include "button.h"
#include "system.h"
#include "sounddriver.h"
#include "drawtext.h"


#define SCREEN_W 128
#define SCREEN_H 160

#define PADDLE_W 4
#define PADDLE_H 24

#define BALL_SIZE 4

static int player_y;
static int ai_y;

static int ball_x;
static int ball_y;

static int ball_vx;
static int ball_vy;

static uint32_t score;

static int prev_player_y;
static int prev_ai_y;
static int prev_ball_x;
static int prev_ball_y;

static uint8_t rainbow_t = 0;

static uint8_t autoplay = 1;

static void reset_ball(void)
{
    ball_x = SCREEN_W / 2;
    ball_y = SCREEN_H / 2;

    ball_vx = -2;
    ball_vy = 1;
    player_y = 75;
}

static void draw_rect(int x, int y, int w, int h, uint16_t color)
{
    display_fill_rect(x, y, w, h, color);
}

void pong_init(void)
{

    app_set_sleep_timeout(60000);

    player_y = SCREEN_H / 2 - PADDLE_H / 2;
    ai_y = player_y;

    score = 0;

    reset_ball();

    prev_player_y = player_y;
    prev_ai_y = ai_y;
    prev_ball_x = ball_x;
    prev_ball_y = ball_y;

    // Draw center line once
    for (int y = 0; y < SCREEN_H; y += 8)
    {
        draw_rect(
            SCREEN_W / 2 - 1,
            y,
            2,
            4,
            COL_RGB(64,0,64)
        );
    }

    // Draw initial paddles and ball
    draw_rect(
        2,
        player_y,
        PADDLE_W,
        PADDLE_H,
        COL_RGB(0,0,255)
    );

    draw_rect(
        SCREEN_W - PADDLE_W - 2,
        ai_y,
        PADDLE_W,
        PADDLE_H,
        COL_RGB(255,0,0)
    );

    draw_rect(
        ball_x,
        ball_y,
        BALL_SIZE,
        BALL_SIZE,
        COL_RGB(0,0,255)
    );
}

static inline int16_t sin_fast(uint8_t x)
{
    int16_t v;

    if (x < 64)
        v = x * 2;
    else if (x < 128)
        v = 127 - ((x - 64) * 2);
    else if (x < 192)
        v = -((x - 128) * 2);
    else
        v = -127 + ((x - 192) * 2);

    return v;
}

void pong_update(uint32_t frame)
{
    (void)frame;

    rainbow_t += 2;   
    uint16_t start = ms_now();

    // Erase previous paddle positions
    draw_rect(
        2,
        prev_player_y,
        PADDLE_W,
        PADDLE_H,
        COL_RGB(0,0,0)
    );

    draw_rect(
        SCREEN_W - PADDLE_W - 2,
        prev_ai_y,
        PADDLE_W,
        PADDLE_H,
        COL_RGB(0,0,0)
    );

    // Erase previous ball
    draw_rect(
        prev_ball_x,
        prev_ball_y,
        BALL_SIZE,
        BALL_SIZE,
        COL_RGB(0,0,0)
    );

    // Restore center line if ball crossed it
    draw_rect(
        SCREEN_W / 2 - 1,
        prev_ball_y,
        2,
        BALL_SIZE,
        COL_RGB(64,0,64)
    );

    // =========================
    // PLAYER INPUT (FIXED)
    // =========================

    if (button_is_pressed()) {
        autoplay = !autoplay;
    }

    if (autoplay) {
        int target_y = ball_y - (PADDLE_H / 2);
        if (player_y < target_y) player_y += 1;
    else if (player_y > target_y) player_y -= 1;
    } else {
        int speed = 0;
        if (button_is_right()) speed -= 4;
        if (button_is_left())  speed += 4;
        player_y += speed;
    }



    /*
    int speed = 0;

    if (button_is_right())
        speed -= 4;

    if (button_is_left())
        speed += 4;

    player_y += speed;
*/
    // clamp
    if (player_y < 0) player_y = 0;
    if (player_y > SCREEN_H - 24) player_y = SCREEN_H - 24;

    // AI follows ball
    int ai_center = ai_y + PADDLE_H / 2;

    if (ball_y < ai_center)
        ai_y -= 1;
    else if (ball_y > ai_center)
        ai_y += 1;

    if (ai_y < 0)
        ai_y = 0;

    if (ai_y > SCREEN_H - PADDLE_H)
        ai_y = SCREEN_H - PADDLE_H;

    // Move ball
    ball_x += ball_vx;
    ball_y += ball_vy;

    // Top wall
    if (ball_y <= 0)
    {
        ball_y = 0;
        ball_vy = -ball_vy;
        beep(1000, 10);
    }

    // Bottom wall
    if (ball_y >= SCREEN_H - BALL_SIZE)
    {
        ball_y = SCREEN_H - BALL_SIZE;
        ball_vy = -ball_vy;
        beep(1000, 10);
    }

    // Player paddle collision
    if (
        ball_vx < 0 &&
        ball_x <= PADDLE_W + 2 &&
        ball_y + BALL_SIZE >= player_y &&
        ball_y <= player_y + PADDLE_H
    )
    {
        ball_x = PADDLE_W + 3;
        ball_vx = -ball_vx;
        score++;
        beep(1000, 10);
    }

    // AI paddle collision
    if (
        ball_vx > 0 &&
        ball_x + BALL_SIZE >= SCREEN_W - PADDLE_W - 2 &&
        ball_y + BALL_SIZE >= ai_y &&
        ball_y <= ai_y + PADDLE_H
    )
    {
        ball_x = SCREEN_W - PADDLE_W - BALL_SIZE - 3;
        ball_vx = -ball_vx;
        beep(1000, 10);
    }

    /*
    // Missed ball (player loses)
    if (ball_x < 0)
    {
        score = 0;

        draw_rect(
            0,
            0,
            SCREEN_W,
            SCREEN_H,
            COL_RGB(0,0,0)
        );

        // Redraw center line
        for (int y = 0; y < SCREEN_H; y += 8)
        {
            draw_rect(
                SCREEN_W / 2 - 1,
                y,
                2,
                4,
                COL_RGB(64,0,64)
            );
        }

        reset_ball();
    }

    // Missed ball (AI loses)


    if (ball_x > SCREEN_W)
    {
        reset_ball();
    }*/

    draw_rect(
        2,
        player_y,
        PADDLE_W,
        PADDLE_H,
        COL_RGB(255,0,0)
    );

    draw_rect(
        SCREEN_W - PADDLE_W - 2,
        ai_y,
        PADDLE_W,
        PADDLE_H,
        COL_RGB(255,0,0)
    );

    uint8_t rainbow_r = 50 + (sin_fast(rainbow_t) >> 1);
    uint8_t rainbow_g = 10 + (sin_fast(rainbow_t + 85) >> 1);
    uint8_t rainbow_b = 50 + (sin_fast(rainbow_t + 170) >> 1);

    draw_rect(
        ball_x,
        ball_y,
        BALL_SIZE,
        BALL_SIZE,
        COL_RGB(rainbow_r,rainbow_g,rainbow_b)
    );

  //  if (score > 0) {
  //      draw_text("Score", 40, 48, COL_SCORE, COL_EYE);
  //  }
    //draw_text("HIGH SCORE", 20, 78, COL_GOLD, COL_EYE);

    // Save previous positions
    prev_player_y = player_y;
    prev_ai_y = ai_y;
    prev_ball_x = ball_x;
    prev_ball_y = ball_y;

    uint16_t elapsed = ms_now() - start;

    char buf[6];

    buf[0] = '0' + ((elapsed / 1000) % 10);
    buf[1] = '0' + ((elapsed / 100) % 10);
    buf[2] = '0' + ((elapsed / 10) % 10);
    buf[3] = '0' + ( elapsed % 10);
    buf[4] = '\0';

    draw_text(buf, 0, 0, COL_RGB(255,255,255), COL_RGB(0,0,0));
}


void pong_wake(void) {

}