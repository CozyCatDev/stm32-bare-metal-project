#include "debug.h"

void debug_write(uint8_t debug_channel, uint8_t bit) {
    switch (debug_channel) {
    case 0:
        GPIO_write(DEBUG_CH0_PORT, DEBUG_CH0_PIN, bit);
        break;
    case 1:
        GPIO_write(DEBUG_CH1_PORT, DEBUG_CH1_PIN, bit);
        break;
    case 2:
        GPIO_write(DEBUG_CH2_PORT, DEBUG_CH2_PIN, bit);
        break;
    }
}

void debug_init(void) {
    gpio_config_t debug_gpio_config = {.mode              = GPIO_MODE_OUTPUT,
                                       .output_type       = GPIO_OUTPUT_TYPE_PUSH_PULL,
                                       .output_speed      = GPIO_OUTPUT_SPEED_VERY_HIGH,
                                       .internal_resistor = GPIO_INTERNAL_RESISTOR_NONE};
    debug_gpio_config.port          = DEBUG_CH0_PORT;
    debug_gpio_config.pin           = DEBUG_CH0_PIN;
    GPIO_init(&debug_gpio_config);
    debug_gpio_config.port = DEBUG_CH1_PORT;
    debug_gpio_config.pin  = DEBUG_CH1_PIN;
    GPIO_init(&debug_gpio_config);
    debug_gpio_config.port = DEBUG_CH2_PORT;
    debug_gpio_config.pin  = DEBUG_CH2_PIN;
    GPIO_init(&debug_gpio_config);
}
