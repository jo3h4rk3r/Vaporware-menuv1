/* snake.c */

#include "app.h"
#include "display.h"
#include "button.h"
#include "system.h"
#include "sounddriver.h"
#include "drawtext.h"

#define SCREEN_W    128
#define SCREEN_H    160

#define TILE_SIZE   8
#define TOP_BAR     16

#define GRID_W      (SCREEN_W / TILE_SIZE)
#define GRID_H      ((SCREEN_H - TOP_BAR) / TILE_SIZE)

#define MAX_SNAKE   128
#define MOVE_DELAY  20

#define SIN_SIZE 256

static uint8_t t = 0;

static uint8_t autoplay = 1;

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

static void menu_background(void)
{
    for (int y = 0; y < SCREEN_H; y += TILE_SIZE)
    {
        for (int x = 0; x < SCREEN_W; x += TILE_SIZE)
        {
            uint8_t nx = x * 1;
            uint8_t ny = y * 1;

            int v =
                sin_fast(nx + t) +
                sin_fast(ny + t) +
                sin_fast(nx + ny + t);

                uint8_t r = 20 + sin_fast((uint8_t)v + 200);
                uint8_t g = 10 + sin_fast((uint8_t)(v + 20));
                uint8_t b = 50 + sin_fast((uint8_t)(v + 50));

               
                r >>= 2;   
                g >>= 4;
                b >>= 2;

                display_fill_rect(x, y, TILE_SIZE, TILE_SIZE, COL_RGB(r, g, b));
        }
    }
}

typedef struct
{
    uint8_t x;
    uint8_t y;
} SnakePart;

typedef struct
{
    SnakePart body[MAX_SNAKE];

    uint16_t length;
    uint16_t score;

    int8_t dirX;
    int8_t dirY;

    uint8_t foodX;
    uint8_t foodY;

    uint16_t lastMove;

    uint8_t gameOver;
} SnakeGame;

static SnakeGame gSnake;

/* -------------------------------------------------- */
/* Helpers                                             */
/* -------------------------------------------------- */

static void draw_tile(uint8_t x, uint8_t y, uint16_t color)
{
    display_fill_rect(
        x * TILE_SIZE,
        TOP_BAR + (y * TILE_SIZE),
        TILE_SIZE,
        TILE_SIZE,
        color);
}

static void erase_tile(uint8_t x, uint8_t y)
{
    //draw_tile(x, y, COL_RGB(0, 0, 0));
    //menu_background();
            uint8_t nx = x * TILE_SIZE;
            uint8_t ny = TOP_BAR + (y * TILE_SIZE);

            int v =
                sin_fast(nx + t) +
                sin_fast(ny + t) +
                sin_fast(nx + ny + t);

                uint8_t r = 20 + sin_fast((uint8_t)v + 200);
                uint8_t g = 10 + sin_fast((uint8_t)(v + 20));
                uint8_t b = 50 + sin_fast((uint8_t)(v + 50));

               
                r >>= 2;   
                g >>= 4;
                b >>= 2;

                display_fill_rect(nx, ny, TILE_SIZE, TILE_SIZE, COL_RGB(r, g, b));
}

/* -------------------------------------------------- */
/* Score                                               */
/* -------------------------------------------------- */

static void draw_score(void)
{
    char text[16];
    uint16_t value = gSnake.score;
    int pos = 0;

    text[pos++] = 'S';
    text[pos++] = 'C';
    text[pos++] = 'O';
    text[pos++] = 'R';
    text[pos++] = 'E';
    text[pos++] = ': ';

    if(value == 0)
    {
        text[pos++] = '0';
    }
    else
    {
        char temp[6];
        int len = 0;

        while(value)
        {
            temp[len++] = '0' + (value % 10);
            value /= 10;
        }

        while(len)
        {
            text[pos++] = temp[--len];
        }
    }

    text[pos] = 0;

    display_fill_rect(0,0,SCREEN_W,TOP_BAR,COL_RGB(0,0,0));

    draw_text(
        text,
        2,
        2,
        COL_RGB(255,255,255),
        COL_RGB(0,0,0));


}

/* -------------------------------------------------- */
/* Food                                                */
/* -------------------------------------------------- */

static void spawn_food(void)
{
    gSnake.foodX = (ms_now() * 37) % GRID_W;
    gSnake.foodY = (ms_now() * 17) % GRID_H;

    draw_tile(
        gSnake.foodX,
        gSnake.foodY,
        COL_RGB(255,0,0));
}

/* -------------------------------------------------- */
/* Reset                                               */
/* -------------------------------------------------- */

static void snake_reset(void)
{
    //display_fill(COL_RGB(0,0,0));
    menu_background();

    gSnake.length = 3;
    gSnake.score = 0;
    gSnake.gameOver = 0;

    gSnake.dirX = 1;
    gSnake.dirY = 0;

    gSnake.lastMove = ms_now();

    gSnake.body[0].x = GRID_W / 2;
    gSnake.body[0].y = GRID_H / 2;

    gSnake.body[1].x = gSnake.body[0].x - 1;
    gSnake.body[1].y = gSnake.body[0].y;

    gSnake.body[2].x = gSnake.body[0].x - 2;
    gSnake.body[2].y = gSnake.body[0].y;

    display_fill_rect(
        0,
        TOP_BAR - 1,
        SCREEN_W,
        1,
        COL_RGB(40,40,40));

    draw_score();

    for(uint16_t i = 0; i < gSnake.length; i++)
    {
        draw_tile(
            gSnake.body[i].x,
            gSnake.body[i].y,
            (i == 0)
                ? COL_RGB(0,255,0)
                : COL_RGB(0,180,0));
    }


    spawn_food();
}

/* -------------------------------------------------- */
/* Input                                               */
/* -------------------------------------------------- */

static void handle_input(void)
{
    if(button_is_up() && gSnake.dirY == 0)
    {
        gSnake.dirX = 0;
        gSnake.dirY = -1;
    }

    if(button_is_down() && gSnake.dirY == 0)
    {
        gSnake.dirX = 0;
        gSnake.dirY = 1;
    }

    if(button_is_left() && gSnake.dirX == 0)
    {
        gSnake.dirX = -1;
        gSnake.dirY = 0;
    }

    if(button_is_right() && gSnake.dirX == 0)
    {
        gSnake.dirX = 1;
        gSnake.dirY = 0;
    }
}

/* -------------------------------------------------- */
/* Move                                                */
/* -------------------------------------------------- */

static void snake_ai(void)
{
    int bestDirX = gSnake.dirX;
    int bestDirY = gSnake.dirY;

    int bestScore = 999999;

    const int dirs[4][2] = {
        { 1, 0 },   // right
        {-1, 0 },   // left
        { 0, 1 },   // down
        { 0,-1 }    // up
    };

    for(int d = 0; d < 4; d++)
    {
        int nx = gSnake.body[0].x + dirs[d][0];
        int ny = gSnake.body[0].y + dirs[d][1];

        // wall check
        if(nx < 0 || ny < 0 || nx >= GRID_W || ny >= GRID_H)
            continue;

        // self collision check
        int hit = 0;
        for(uint16_t i = 0; i < gSnake.length; i++)
        {
            if(gSnake.body[i].x == nx &&
               gSnake.body[i].y == ny)
            {
                hit = 1;
                break;
            }
        }
        if(hit) continue;

        // heuristic: distance to food
        int dist =
            (nx - gSnake.foodX) * (nx - gSnake.foodX) +
            (ny - gSnake.foodY) * (ny - gSnake.foodY);

        if(dist < bestScore)
        {
            bestScore = dist;
            bestDirX = dirs[d][0];
            bestDirY = dirs[d][1];
        }
    }

    gSnake.dirX = bestDirX;
    gSnake.dirY = bestDirY;
}

static void snake_move(void)
{

    if (autoplay) {
        snake_ai();
    }

    SnakePart newHead = gSnake.body[0];
    newHead.x += gSnake.dirX;
    newHead.y += gSnake.dirY;

    if (!autoplay) {
        if(newHead.x >= GRID_W ||
        newHead.y >= GRID_H)
        {
            gSnake.gameOver = 1;
            beep(1000,5);
            beep(1500,5);
            beep(3000,5);
            return;
        }
        for(uint16_t i = 0; i < gSnake.length; i++)
        {
            if(newHead.x == gSnake.body[i].x &&
            newHead.y == gSnake.body[i].y)
            {
                gSnake.gameOver = 1;
                beep(200,200);
                return;
            }
        }
    }

    SnakePart tail =
        gSnake.body[gSnake.length - 1];

    for(int i = gSnake.length; i > 0; i--)
    {
        gSnake.body[i] = gSnake.body[i - 1];
    }

    gSnake.body[0] = newHead;

    draw_tile(
        newHead.x,
        newHead.y,
        COL_RGB(0,255,0));

    draw_tile(
        gSnake.body[1].x,
        gSnake.body[1].y,
        COL_RGB(0, 180, gSnake.score * 10));

    if(newHead.x == gSnake.foodX &&
       newHead.y == gSnake.foodY)
    {
        if(gSnake.length < MAX_SNAKE - 1)
        {
            gSnake.length++;
        }

        gSnake.score++;

        draw_score();

        beep(1800 + gSnake.score * 10, 10);

        spawn_food();
    }
    else
    {
        erase_tile(tail.x, tail.y);
    }
}

/* -------------------------------------------------- */
/* Public API                                          */
/* -------------------------------------------------- */

void snake_init(void)
{
    app_set_sleep_timeout(60000);
    snake_reset();
}

void snake_update(uint32_t frame)
{
    (void)frame;

    char buf[16];
    uint16_t v = button_raw();
    buf[0] = '0' + ((v / 1000) % 10);
    buf[1] = '0' + ((v / 100) % 10);
    buf[2] = '0' + ((v / 10) % 10);
    buf[3] = '0' + (v % 10);
    buf[4] = '\0';
    draw_text(buf, 80, 2, COL_RGB(0,0,255), COL_EYE);

    if(gSnake.gameOver)
    {
        draw_text(
            "GAME OVER",
            18,
            70,
            COL_RGB(255,0,0),
            COL_RGB(0,0,0));

        if(button_is_pressed())
        {
            snake_reset();
        }

        return;
    }

    if (!autoplay) {
        handle_input();
    }

    if((uint16_t)(ms_now() - gSnake.lastMove) < MOVE_DELAY)
    {
        return;
    }

    gSnake.lastMove = ms_now();

    snake_move();
}

void snake_wake(void)
{
    snake_reset();
}