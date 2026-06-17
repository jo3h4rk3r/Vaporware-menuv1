#include "app.h"
#include "display.h"
#include "button.h"
#include "system.h"
#include "drawtext.h"
#include "sounddriver.h"

#define SCREEN_W 128
#define SCREEN_H 160

#define PADDLE_W 24
#define PADDLE_H 4

#define BALL_SIZE 4

#define BRICK_ROWS 6
#define BRICK_COLS 8
#define BRICK_ROWS_2 8
#define BRICK_COLS_2 7

#define MAX_BRICK_ROWS 8
#define MAX_BRICK_COLS 8

#define BRICK_W 14
#define BRICK_H 6

#define BRICK_SPACING_X 2
#define BRICK_SPACING_Y 2

#define BRICK_START_Y 18

static int level;

static int paddle_x;

static int ball_x;
static int ball_y;

static int ball_vx;
static int ball_vy;


static uint8_t ball_attached;

static uint32_t score;

static int prev_ball_x;
static int prev_ball_y;
static int prev_paddle_x;

static uint8_t rainbow_t;
static uint8_t autoplay = 0;

static uint8_t bricks[MAX_BRICK_ROWS][MAX_BRICK_COLS];
//static uint8_t bricks[BRICK_ROWS][BRICK_COLS];

static void draw_rect(
    int x,
    int y,
    int w,
    int h,
    uint16_t color)
{
    display_fill_rect(x,y,w,h,color);
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

static void draw_brick(
    int row,
    int col,
    uint16_t color)
{
    int bx =
        col * (BRICK_W + BRICK_SPACING_X);

    int by =
        BRICK_START_Y +
        row * (BRICK_H + BRICK_SPACING_Y);

    draw_rect(
        bx,
        by,
        BRICK_W,
        BRICK_H,
        color);
}

static void reset_level(void)
{
    for(int r=0;r<BRICK_ROWS;r++)
    {
        for(int c=0;c<BRICK_COLS;c++)
        {
            bricks[r][c] = 1;

            uint8_t g =
                40 + (r * 30);

            draw_brick(
                r,
                c,
                COL_RGB(
                    20,
                    g,
                    120));
        }
    }
}

static void level_2(void)
{
    for(int r=0;r<BRICK_ROWS_2;r++)
    {
        for(int c=0;c<BRICK_COLS_2;c++)
        {
            bricks[r][c] = 1;

            uint8_t g =
                40 + (r * 30);

            draw_brick(
                r,
                c,
                COL_RGB(
                    120,
                    g,
                    20));
        }
    }
}

static void reset_ball(void)
{
    ball_attached = 1;

    ball_x =
        paddle_x +
        (PADDLE_W / 2) -
        (BALL_SIZE / 2);

    ball_y =
        SCREEN_H -
        20;

    ball_vx = 2;
    ball_vy = -2;

    explosion_sound();
}

static void draw_level_text(void) {
    draw_text(
        "level ",
        65,
        2,
        COL_RGB(255,0,45),
        COL_RGB(0,0,0));

    draw_rect(100,2,20,8,COL_RGB(0,0,0));

    draw_text(
        level == 0 ? "1" : "2",
        110,
        2,
        COL_RGB(0,255,0),
        COL_RGB(0,0,0));
}

void brickbreaker_init(void)
{
    //app_set_sleep_timeout(60000);

    display_fill(
        COL_RGB(0,0,0));

    score = 0;

    paddle_x =
        SCREEN_W/2 -
        PADDLE_W/2;

    rainbow_t = 0;

    level = 0;

    //reset_level();

    draw_level_text();

    if (level == 0) {
        reset_level();
    }

    if (level == 2) {
        level_2();
    }


    reset_ball();

    prev_ball_x = ball_x;
    prev_ball_y = ball_y;
    prev_paddle_x = paddle_x;
}

static void draw_score(void)
{
    char buf[12];
    uint32_t s = score;
    int i = 0;

    if (s == 0)
    {
        buf[i++] = '0';
    }
    else
    {
        char temp[12];
        int t = 0;

        while (s > 0)
        {
            temp[t++] = '0' + (s % 10);
            s /= 10;
        }

        while (t > 0)
        {
            buf[i++] = temp[--t];
        }
    }

    buf[i] = 0;

    draw_text(
        "SCORE",
        2,
        2,
        COL_RGB(255,25,255),
        COL_RGB(0,0,0));

    draw_text(
        buf,
        45,
        2,
        COL_RGB(0,255,0),
        COL_RGB(0,0,0));
}

static void erase_old_objects(void)
{
    draw_rect(
        prev_paddle_x,
        SCREEN_H - 10,
        PADDLE_W,
        PADDLE_H,
        COL_RGB(0,0,0));

    draw_rect(
        prev_ball_x,
        prev_ball_y,
        BALL_SIZE,
        BALL_SIZE,
        COL_RGB(0,0,0));
}

static void update_controls(void)
{

    if (button_is_down()) {
        autoplay = !autoplay;
    }

    if (autoplay) {
        if (ball_vy > 0) {

            int target =
                ball_x - (PADDLE_W / 2);

            if (paddle_x < target)
                paddle_x += 2;
            else if (paddle_x > target)
                paddle_x -= 2;

            if(paddle_x < 0)
                paddle_x = 0;

            if(paddle_x > SCREEN_W - PADDLE_W) {
                paddle_x = SCREEN_W - PADDLE_W;
            }
        }
    } else {

        if (button_is_left()) {
            paddle_x -= 4;
        }

        if(button_is_right()) {
            paddle_x += 4;
        }

        if(paddle_x < 0)
            paddle_x = 0;

        if(paddle_x > SCREEN_W - PADDLE_W) {
            paddle_x = SCREEN_W - PADDLE_W;
        }
    }
}

static void update_ball(void)
{
    if(ball_attached) {
        ball_x =
            paddle_x +
            (PADDLE_W/2) -
            (BALL_SIZE/2);

        ball_y =
            SCREEN_H -
            20;

        if(button_is_pressed()) {
            ball_attached = 0;
            ball_vx = 1;
            ball_vy = -2;
            laser_sound();
        }

        if (autoplay) {
            ball_attached = 0;
            ball_vx = 1;
            ball_vy = -2;
            laser_sound();
        }

        return;
    }

    ball_x += ball_vx;
    ball_y += ball_vy;

    if(ball_x <= 0)
    {
        ball_x = 0;
        ball_vx = -ball_vx;
    }

    if(ball_x >= SCREEN_W-BALL_SIZE)
    {
        ball_x =
            SCREEN_W-BALL_SIZE;

        ball_vx = -ball_vx;
    }

    if(ball_y <= 0)
    {
        ball_y = 0;
        ball_vy = -ball_vy;
    }

    if(ball_y > SCREEN_H)
    {
        reset_ball();
        return;
    }

    if(ball_vy > 0 && ball_y + BALL_SIZE >= SCREEN_H - 10 && ball_x + BALL_SIZE >= paddle_x && ball_x <= paddle_x + PADDLE_W) {
        int hit =
            (ball_x + BALL_SIZE/2)
            -
            (paddle_x + PADDLE_W/2);

        ball_vx =
            hit / 3;

        if(ball_vx == 0)
            ball_vx = -1;

        ball_vy = -ball_vy;

        ball_y =
            SCREEN_H -
            10 -
            BALL_SIZE;
    }
}

static void brick_collision(void)
{

    if (level == 0) {
        for(int r=0;r<BRICK_ROWS;r++)
        {
            for(int c=0;c<BRICK_COLS;c++)
            {
                if(!bricks[r][c])
                    continue;

                int bx =
                    c *
                    (BRICK_W + BRICK_SPACING_X);

                int by =
                    BRICK_START_Y +
                    r *
                    (BRICK_H + BRICK_SPACING_Y);

                if(
                    ball_x + BALL_SIZE >= bx &&
                    ball_x <= bx + BRICK_W &&
                    ball_y + BALL_SIZE >= by &&
                    ball_y <= by + BRICK_H
                )
                {
                    bricks[r][c] = 0;

                    draw_rect(
                        bx,
                        by,
                        BRICK_W,
                        BRICK_H,
                        COL_RGB(0,0,0));

                    ball_vy = -ball_vy;

                    beep(3000 + score * 50, 10);

                    score++;

                    return;
                }
            }
        }
    }

    if (level == 2) {
        for(int r=0;r<BRICK_ROWS_2;r++)
        {
            for(int c=0;c<BRICK_COLS_2;c++)
            {
                if(!bricks[r][c])
                    continue;

                int bx =
                    c *
                    (BRICK_W + BRICK_SPACING_X);

                int by =
                    BRICK_START_Y +
                    r *
                    (BRICK_H + BRICK_SPACING_Y);

                if(
                    ball_x + BALL_SIZE >= bx &&
                    ball_x <= bx + BRICK_W &&
                    ball_y + BALL_SIZE >= by &&
                    ball_y <= by + BRICK_H
                )
                {
                    bricks[r][c] = 0;

                    draw_rect(
                        bx,
                        by,
                        BRICK_W,
                        BRICK_H,
                        COL_RGB(0,0,0));

                    ball_vy = -ball_vy;

                    beep(3000 + score * 50, 10);

                    score++;

                    return;
                }
            }
        }      
    }
}

static uint8_t level_cleared(void)
{
    if (level == 0 && score == 48) {
        display_fill(COL_RGB(0,0,0));
        level = 2;
        draw_level_text();
        return 1;
    } else {
        if (level == 2 && score == 104) {
            display_fill(COL_RGB(0,0,0));
            level = 1;
            score = 0; 
            draw_level_text();
            reset_level();
            return 1; 
        } else {
            return 0; 
        }
        
    }

    //if (level == 0 && score == 3) {
    //    level = 2;
    //    return 1;
    //} else {
    //    return 0;
    //}

    /*
    if (level == 0) {
        for(int r=0;r<BRICK_ROWS;r++)
        {
            for(int c=0;c<BRICK_COLS;c++)
            {
                if(bricks[r][c])
                    return 0;
            }
        }
    }

    if (level == 2) {
        for(int r=0;r<BRICK_ROWS_2;r++)
        {
            for(int c=0;c<BRICK_COLS_2;c++)
            {
                if(bricks[r][c])
                    return 0;
            }
        }
    }*/

    coin_sound();

    return 1;
}

static void draw_paddle(void)
{
    draw_rect(
        paddle_x,
        SCREEN_H - 10,
        PADDLE_W,
        PADDLE_H,
        COL_RGB(255,0,0));
}

static void draw_ball(void)
{
    rainbow_t += 2;

    uint8_t r =
        50 +
        (sin_fast(rainbow_t)>>1);

    uint8_t g =
        50 +
        (sin_fast(rainbow_t+85)>>1);

    uint8_t b =
        50 +
        (sin_fast(rainbow_t+170)>>1);

    draw_rect(
        ball_x,
        ball_y,
        BALL_SIZE,
        BALL_SIZE,
        COL_RGB(r,g,b));
}

void brickbreaker_update(uint32_t frame) {
    (void)frame;

    erase_old_objects();

    update_controls();

    update_ball();

    brick_collision();

    if(level_cleared())
    {
        if (level == 0) {
            reset_level(); 
        }
        if (level == 2) {
            level_2();
        }
        
        reset_ball();
    }

    draw_paddle();

    draw_ball();

    draw_score();

    prev_paddle_x = paddle_x;

    prev_ball_x = ball_x;
    prev_ball_y = ball_y;

}

void brickbreaker_wake(void)
{
}