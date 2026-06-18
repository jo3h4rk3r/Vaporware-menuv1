
#include "button.h"
#include "battery.h"
#include "config.h"
#include "system.h"
#include "display.h"
#include "adc.h"


#define BTN_GPIO_PORT GPIOA
#define BTN_GPIO_PIN  7


typedef enum {
    BTN_NONE = 0,
    BTN_UP,
    BTN_LEFT,
    BTN_DOWN,
    BTN_RIGHT,
    BTN_PRESSED
} button_t;

static button_t g_button;
static button_t g_prev_button;

static uint8_t  g_just_pressed;
static uint8_t  g_just_released;
static uint16_t g_held_since;
static uint8_t  g_holding;


/* -----------------------------
   resistor ladder
UP      ~0.00V
LEFT    ~0.30V
DOWN    ~0.60V
RIGHT   ~0.90V
PRESSED ~1.30V
NONE    ~2.82V
------------------------------*/

static button_t decode_adc(uint16_t v)
{
    
    
    
    if (v < 150)   return BTN_DOWN;     // ~0.6V
    if (v < 450)  return BTN_RIGHT;    // ~0.9V
    if (v < 900)   return BTN_UP;        // ~0V
    if (v < 1400)   return BTN_LEFT;     // ~0.3V
    if (v < 2000)  return BTN_PRESSED;  // ~1.3V
    

    /*
    if (v < 150)   return BTN_LEFT;     // ~0.6V
    if (v < 450)  return BTN_DOWN;    // ~0.9V
    if (v < 900)   return BTN_RIGHT;        // ~0V
    if (v < 1400)   return BTN_UP;     // ~0.3V
    if (v < 2000)  return BTN_PRESSED;  // ~1.3V*/
    
    return BTN_NONE; // ~2.8V idle
}

/* -----------------------------
   Init
------------------------------*/

uint16_t button_read_raw(void)
{
    
    uint16_t r = (uint16_t)(ADC_DAT & 0xFFFU);

    return r;
}

void button_init(void) {

    ADC_RSEQ3  = 7;
    
    GPIOA->MODER &= ~(3U << (7 * 2));
    GPIOA->PUPDR &= ~(3U << (7 * 2));
    GPIOA->PUPDR |= (1U << (7 * 2));

    g_button = BTN_NONE;
    g_prev_button = BTN_NONE;

    g_just_pressed = 0;
    g_just_released = 0;
    g_holding = 0;
    g_held_since = 0;
}



/* -----------------------------
   Update (calls every frame)
------------------------------*/

void button_update(void)
{
    g_prev_button = g_button;

    uint16_t val = button_read_raw();
    g_button = decode_adc(val);

    /* edge detection */
    g_just_pressed =
        (g_button != BTN_NONE && g_prev_button == BTN_NONE);

    g_just_released =
        (g_button == BTN_NONE && g_prev_button != BTN_NONE);

    /* hold tracking */
    if (g_just_pressed) {
        g_held_since = ms_now();
        g_holding = 1;
    }
    else if (g_button == BTN_NONE) {
        g_holding = 0;
    }
}

/* -----------------------------
   Legacy API
------------------------------*/

uint8_t button_pressed(void)
{
    return g_button != BTN_NONE;
}

uint8_t button_just_pressed(void)
{
    return g_just_pressed;
}

uint8_t button_just_released(void)
{
    return g_just_released;
}

uint16_t button_held_ms(void)
{
    if (!g_holding) return 0;
    return (uint16_t)(ms_now() - g_held_since);
}

uint16_t button_raw(void)
{
    return button_read_raw();
}

/* -----------------------------
   Direction helpers
------------------------------*/

uint8_t button_is_up(void)    { return g_button == BTN_UP; }
uint8_t button_is_down(void)  { return g_button == BTN_DOWN; }
uint8_t button_is_left(void)  { return g_button == BTN_LEFT; }
uint8_t button_is_right(void) { return g_button == BTN_RIGHT; }
uint8_t button_is_pressed(void) { return g_button == BTN_PRESSED; }