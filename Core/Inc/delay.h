#ifndef DELAY_H
#define DELAY_H

#include "stm32c031xx.h"
#include <stdint.h>

#define SYSTEM_CLOCK 12000000

#define TIMPCLK_FREQ 12000000
#define TIM1_PSC     12 // 1 clock tick = 1us (1MHz)

void delay_timer_init(void);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);
// void delay_us(uint16_t us);
// void delay_ms(uint16_t ms);

#endif
