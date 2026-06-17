/* adc.h */

#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include "config.h"

#define ADC_BASE  VAPE_ADC_BASE

#define ADC_STS   (*(volatile uint32_t *)(ADC_BASE + 0x00))
#define ADC_CTRL2 (*(volatile uint32_t *)(ADC_BASE + 0x08))
#define ADC_SMPR2 (*(volatile uint32_t *)(ADC_BASE + 0x10))
#define ADC_RSEQ1 (*(volatile uint32_t *)(ADC_BASE + 0x30))
#define ADC_RSEQ3 (*(volatile uint32_t *)(ADC_BASE + 0x38))
#define ADC_DAT   (*(volatile uint32_t *)(ADC_BASE + 0x50))

#endif