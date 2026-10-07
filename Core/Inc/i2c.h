#ifndef I2C_H
#define I2C_H

#include "dma.h"
#include "stm32c031xx.h"

#define I2C_EVENT_QUEUE_LENGTH 5

typedef enum { I2C_SEVEN_BIT_ADDRESSING, I2C_TEN_BIT_ADDRESSING } i2c_addressing_mode_t;

typedef struct {
    /* Configure prescaler */
    uint8_t presc; // f_PRESC = f_I2CCLK / (PRESC + 1)
    /* Configure SCL low stretch times */
    uint8_t scldel; // inserted delay for SDA setup time
    uint8_t sdadel; // inserted delay for SDA hold time
    /* Configure SCL high and low times */
    uint8_t sclh; // SCL high time
    uint8_t scll; // SCL low time
} i2c_timing_config_t;

typedef struct {
    /* User-defined delay function */
    void (*delay_us)(uint32_t us);
    /* Configure SDA and SCL pins */
    GPIO_TypeDef *sda_port;
    GPIO_TypeDef *scl_port;
    uint8_t       sda_pin;
    uint8_t       scl_pin;
    /* Configure I2C timing */
    i2c_timing_config_t *timing_config;
    /* Configure CR1 settings */
    uint8_t analog_filter_on;
    uint8_t digital_filter_on;
    uint8_t clock_stretching_on;
    /* Configure CR2 settings */
    i2c_addressing_mode_t i2c_addressing_mode;
} i2c_config_t;

void    I2C_init(i2c_config_t *config);
uint8_t I2C_start(const uint8_t     slave_address,
                  const uint8_t     rw,
                  const uint8_t     num_rx_bytes,
                  const uint8_t     num_tx_bytes,
                  volatile uint8_t *rx_buf,
                  volatile uint8_t *tx_buf,
                  const uint8_t     communication_mode);

#endif
