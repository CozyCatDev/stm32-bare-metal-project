#ifndef GPIO_H
#define GPIO_H

#include "stm32c031xx.h"
#include <stdint.h>

typedef enum {
    GPIO_MODE_INPUT,
    GPIO_MODE_OUTPUT,
    GPIO_MODE_ALTERNATE_FUNCTION,
    GPIO_MODE_ANALOG,
} gpio_mode_t;

typedef enum { GPIO_OUTPUT_TYPE_PUSH_PULL, GPIO_OUTPUT_TYPE_OPEN_DRAIN } gpio_output_type_t;

typedef enum {
    GPIO_OUTPUT_SPEED_VERY_LOW,
    GPIO_OUTPUT_SPEED_LOW,
    GPIO_OUTPUT_SPEED_HIGH,
    GPIO_OUTPUT_SPEED_VERY_HIGH
} gpio_output_speed_t;

typedef enum {
    GPIO_INTERNAL_RESISTOR_NONE,
    GPIO_INTERNAL_RESISTOR_PULL_UP,
    GPIO_INTERNAL_RESISTOR_PULL_DOWN
} gpio_internal_resistor_t;

typedef enum {
    GPIO_ALTERNATE_FUNCTION_AF0,
    GPIO_ALTERNATE_FUNCTION_AF1,
    GPIO_ALTERNATE_FUNCTION_AF2,
    GPIO_ALTERNATE_FUNCTION_AF3,
    GPIO_ALTERNATE_FUNCTION_AF4,
    GPIO_ALTERNATE_FUNCTION_AF5,
    GPIO_ALTERNATE_FUNCTION_AF6,
    GPIO_ALTERNATE_FUNCTION_AF7,
    GPIO_ALTERNATE_FUNCTION_AF8,
    GPIO_ALTERNATE_FUNCTION_AF9,
    GPIO_ALTERNATE_FUNCTION_AF10,
    GPIO_ALTERNATE_FUNCTION_AF11,
    GPIO_ALTERNATE_FUNCTION_AF12,
    GPIO_ALTERNATE_FUNCTION_AF13,
    GPIO_ALTERNATE_FUNCTION_AF14,
    GPIO_ALTERNATE_FUNCTION_AF15
} gpio_alternate_function_t;

typedef struct {
    GPIO_TypeDef             *port;
    uint8_t                   pin;
    gpio_mode_t               mode;
    gpio_output_type_t        output_type;
    gpio_output_speed_t       output_speed;
    gpio_internal_resistor_t  internal_resistor;
    gpio_alternate_function_t alternate_function;
} gpio_config_t;

void GPIO_write(GPIO_TypeDef *gpio_port, uint8_t gpio_pin, uint8_t bit);
void GPIO_init(gpio_config_t *gpio_config);

#endif
