#ifndef DEBUG_H
#define DEBUG_H

#include "gpio.h"
#include "stm32c031xx.h"
#include <stdint.h>

#define DEBUG_CH0_PORT (GPIOD)
#define DEBUG_CH0_PIN  (0)
#define DEBUG_CH1_PORT (GPIOD)
#define DEBUG_CH1_PIN  (1)
#define DEBUG_CH2_PORT (GPIOD)
#define DEBUG_CH2_PIN  (2)

void debug_write(uint8_t debug_channel, uint8_t bit);
void debug_init(void);

#endif
