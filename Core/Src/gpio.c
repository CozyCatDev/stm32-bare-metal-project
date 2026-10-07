#include "gpio.h"

void GPIO_write(GPIO_TypeDef *gpio_port, uint8_t gpio_pin, uint8_t bit) {
    uint8_t pin = gpio_pin & 0xF;
    if (bit) {
        gpio_port->BSRR = (0x1UL << pin);
    } else {
        gpio_port->BRR = (0x1UL << pin);
    }
}

void GPIO_init(gpio_config_t *gpio_config) {
    uint8_t pin = gpio_config->pin & 0xF;

    switch ((uint32_t)gpio_config->port) {
    case (uint32_t)GPIOA:
        RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
        break;
    case (uint32_t)GPIOB:
        RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
        break;
    case (uint32_t)GPIOC:
        RCC->IOPENR |= RCC_IOPENR_GPIOCEN;
        break;
    case (uint32_t)GPIOD:
        RCC->IOPENR |= RCC_IOPENR_GPIODEN;
        break;
    case (uint32_t)GPIOF:
        RCC->IOPENR |= RCC_IOPENR_GPIOFEN;
        break;
    }

    /* GPIO mode: input, output, alternate function, analog */
    gpio_config->port->MODER &= ~(0x3UL << (pin * 2U));
    gpio_config->port->MODER |= (gpio_config->mode << (pin * 2U));

    /* GPIO mode: push-pull, open-drain */
    gpio_config->port->OTYPER |= (gpio_config->output_type << pin);

    /* GPIO output speed: very low, low, high, very high */
    gpio_config->port->OSPEEDR &= ~(0x3UL << (pin * 2U));
    gpio_config->port->OSPEEDR |= (gpio_config->output_speed << (pin * 2U));

    /* GPIO internal resistor settings: none, pull-up, pull-down */
    gpio_config->port->PUPDR &= ~(0x3UL << (gpio_config->pin * 2U));
    gpio_config->port->PUPDR |= (gpio_config->internal_resistor << (pin * 2U));

    /* GPIO alternate functions: AF0-AF15 */
    uint8_t afr_idx = gpio_config->pin < 8 ? 0 : 1;
    gpio_config->port->AFR[afr_idx] &= ~(0xFUL << (gpio_config->pin * 4U));
    gpio_config->port->AFR[afr_idx] |= (gpio_config->alternate_function << (gpio_config->pin * 4U));
}
