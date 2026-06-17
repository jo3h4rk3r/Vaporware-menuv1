/* vaporware/src/button.c — ADC directional button driver
 *
 * IMPORTANT:
 * Uses bat_read_raw() from battery.c as ADC backend.
 * No direct ADC registers used here (matches your N32 firmware design).
 */

#include "button.h"
#include "battery.h"
#include "config.h"
#include "system.h"
#include "display.h"
#include "adc.h"


#define BTN_GPIO_PORT GPIOA
#define BTN_GPIO_PIN  7

/* -----------------------------
   Button state enum
------------------------------*/

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
   Decode resistor ladder
   (YOU WILL TUNE THESE VALUES)


UP      ~0.00V
LEFT    ~0.30V
DOWN    ~0.60V
RIGHT   ~0.90V
PRESSED ~1.30V
NONE    ~2.82V
------------------------------*/

static button_t decode_adc(uint16_t v)
{
    
    
    /*
    if (v < 150)   return BTN_DOWN;     // ~0.6V
    if (v < 450)  return BTN_RIGHT;    // ~0.9V
    if (v < 900)   return BTN_UP;        // ~0V
    if (v < 1400)   return BTN_LEFT;     // ~0.3V
    if (v < 2000)  return BTN_PRESSED;  // ~1.3V
    */

    
    if (v < 150)   return BTN_LEFT;     // ~0.6V
    if (v < 450)  return BTN_DOWN;    // ~0.9V
    if (v < 900)   return BTN_RIGHT;        // ~0V
    if (v < 1400)   return BTN_UP;     // ~0.3V
    if (v < 2000)  return BTN_PRESSED;  // ~1.3V
    
    return BTN_NONE; // ~2.8V idle
}

/* -----------------------------
   Init
------------------------------*/

uint16_t button_read_raw(void)
{
    /* Save GPIOA MODER, then set PA6 (BAT_GPIO_PIN) to analog mode (MODER=11).
     * Analog mode disconnects the digital output driver and Schmitt trigger,
     * which is required for accurate ADC readings — floating digital inputs
     * inject noise into the sample.
     *
     * PA6 is normally output-LOW (display VCC enable).  The saved MODER
     * restores output-LOW on exit, so the display P-FET stays on for all but
     * the ~21-84 µs conversion window.  The 239.5-cycle sample time exceeds
     * the RC settling time of the battery divider by >100×, so the reading
     * is accurate even though PA6 was at 0 V immediately before. */
    uint32_t saved_moder = BTN_GPIO_PORT->MODER;
    //BTN_GPIO_PORT->MODER |= (3UL << (BTN_GPIO_PIN * 2));  /* MODER=11 = analog */

    /* Trigger conversion via SWSTRRCH (CTRL2 bit 22) and poll EOC (STS bit 1).
     * At 48 MHz PLL the tight loop below runs at ~10 ns/iter; 20 000 iters =
     * ~200 µs — ample margin for the longest ADC conversion (239.5 + 12.5
     * cycles at any supported ADC clock).  IWDG is fed once before and after
     * rather than inside the loop: each IWDG_FEED() write is a slow peripheral
     * bus transaction that inflates per-iteration time and defeats the purpose
     * of counting iterations as a time proxy at PLL speeds. */
    IWDG_FEED();
    ADC_STS    = 0;
    ADC_CTRL2 |= (1UL << 22);  /* SWSTRRCH: start conversion */
    for (uint32_t i = 0; i < 20000u && !(ADC_STS & 2U); i++);
    IWDG_FEED();

    /* Read 12-bit result from data register (bits[11:0]) */
    uint16_t r = (uint16_t)(ADC_DAT & 0xFFFU);

    /* Restore PA6 GPIO mode (may be input, output, or AF depending on caller) */
    //BAT_GPIO_PORT->MODER = saved_moder;


    return r;
}

void button_init(void) {
        /* Enable ADC1 clock: AHBENR bit 12 (not APB2ENR — ADC1 is on AHB on this device) */
    *(volatile uint32_t *)0x40021014UL |= (1UL << 12);

    /* CFGR2: ADC prescaler — 0x00003804 selects a divider that keeps
     * the ADC input clock within spec at 8 MHz HSI */
    //*(volatile uint32_t *)0x4002102CUL = 0x00003804UL;

    /* Enable GPIOA clock (PA6 is the battery sense pin / ADC channel 6) */
   // RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    /* NOTE: PA6 GPIO mode is NOT changed to analog here.
     * bat_read_raw() saves GPIOA->MODER, temporarily switches PA6 to analog
     * (MODER=11) for the ADC conversion, then restores the saved mode so PA6
     * returns to output-LOW (display VCC enable).  Keeping PA6 as output-LOW
     * between readings ensures the display P-FET stays on at all times except
     * during the brief (~21-84 µs) conversion window.                     */

    /* SMPR2: set bits[20:18]=0b111 for channel 6 → 239.5-cycle sample time.
     * The long sample time ensures the ~0.71 divider resistor network fully
     * charges the ADC sample capacitor before conversion starts.            */
    //ADC_SMPR2 |= (7UL << 18);  /* ch6 = bits[20:18] = 239.5 cycles */

    /* RSEQ1: regular sequence length = 0 → 1 conversion */
    ADC_RSEQ1  = 0x00000000UL;

    /* RSEQ3: first (and only) conversion = BAT_ADC_CHANNEL (6 = PA6) */
   // ADC_RSEQ3  = (uint32_t)BAT_ADC_CHANNEL;
    ADC_RSEQ3  = 7;
    
    /* CTRL2: ADON (bit0) = 1 → power on ADC */
    ADC_CTRL2  = 0x00000001UL;

    /* CTRL2: EXTSEL[3:1]=bits[19:17]=7 (software trigger), EXTTRIG=bit20=1
     * Required to allow SWSTRRCH (bit22) to start a conversion */
    ADC_CTRL2 |= (7UL << 17) | (1UL << 20);
    /*
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    GPIOA->MODER &= ~(3U << (7 * 2)); // input
    GPIOA->PUPDR &= ~(3U << (7 * 2));
    GPIOA->PUPDR |=  (1U << (7 * 2)); // pull-up

    ADC_CTRL2 |= (1UL << 22); // enable ADC trigger setup

    button_read_raw();
    button_read_raw();
*/
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
   Update (call every frame)
------------------------------*/

void button_update(void)
{
    g_prev_button = g_button;

    //uint16_t val = bat_read_raw();
    uint16_t val = button_read_raw();
    g_button = decode_adc(val);


    //display_fill_rect(val / 16, 70, 2, 10, COL_RGB(0,0,255));

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