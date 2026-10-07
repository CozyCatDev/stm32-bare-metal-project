#include "bme280_intf.h"

static struct bme280_dev           my_dev;
static volatile struct bme280_data my_data;
static bme280_intf_config_t       *bme280_intf_config;

static volatile uint8_t tx_buf[32];

/* Constant ADXL345 I2C parameters */
static i2c_config_t  my_bme280_i2c_config = {};
static i2c_config_t *bme280_i2c_config    = &my_bme280_i2c_config;

static spi_config_t  my_bme280_spi_config = {};
static spi_config_t *bme280_spi_config    = &my_bme280_spi_config;

static uint8_t (*communication_start)(const uint8_t     slave_address,
                                      const uint8_t     rw,
                                      const uint8_t     num_rx_bytes,
                                      const uint8_t     num_tx_bytes,
                                      volatile uint8_t *rx_buf,
                                      volatile uint8_t *tx_buf,
                                      const uint8_t     communication_mode);

static uint8_t bme280_intf_I2CStart(const uint8_t     slave_address,
                                    const uint8_t     rw,
                                    const uint8_t     num_rx_bytes,
                                    const uint8_t     num_tx_bytes,
                                    volatile uint8_t *rx_buf,
                                    volatile uint8_t *tx_buf,
                                    const uint8_t     communication_mode) {
    return I2C_start(slave_address, rw, num_rx_bytes, num_tx_bytes, rx_buf, tx_buf,
                     communication_mode);
}

static uint8_t bme280_intf_SPIStart(const uint8_t     slave_address,
                                    const uint8_t     rw,
                                    const uint8_t     num_rx_bytes,
                                    const uint8_t     num_tx_bytes,
                                    volatile uint8_t *rx_buf,
                                    volatile uint8_t *tx_buf,
                                    const uint8_t     communication_mode) {
    (void)slave_address;
    return SPI_start(rw, num_rx_bytes, num_tx_bytes, rx_buf, tx_buf, communication_mode);
}

void bme280_intf_delay_us(uint32_t period, void *intf_ptr) {
    delay_us(period);
}

BME280_INTF_RET_TYPE
bme280_intf_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    //	const uint8_t communication_mode = (((reg_addr == BME280_REG_DATA) && init_complete) ? 1 :
    // 0);
    tx_buf[0] = (volatile uint8_t)reg_addr;

    return !communication_start(BME280_ADDR, 1, (uint8_t)len, 1, (volatile uint8_t *)reg_data,
                                tx_buf, 0);
}

BME280_INTF_RET_TYPE
bme280_intf_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    //	const uint8_t communication_mode = (((reg_addr == BME280_REG_DATA) && init_complete) ? 1 :
    // 0);
    tx_buf[0] = (volatile uint8_t)reg_addr;

    // e.g. len = 3 => data1, addr2, data2, addr3, data3
    for (uint8_t i = 1; i <= len; i++) {
        tx_buf[i] = reg_data[i - 1];
    }

    return !communication_start(BME280_ADDR, 0, 0, (uint8_t)(len + 1), (volatile uint8_t *)0,
                                tx_buf, 0);
}

void bme280_intf_setSensorSettings(uint8_t desired_settings) {
    bme280_set_sensor_settings(desired_settings, &bme280_intf_config->settings,
                               bme280_intf_config->dev);
}

void bme280_intf_setSensorMode(uint8_t sensor_mode) {
    bme280_set_sensor_mode(sensor_mode, bme280_intf_config->dev);
}

void bme280_intf_calMeasDelay(void) {
    bme280_cal_meas_delay(bme280_intf_config->max_delay, &bme280_intf_config->settings);
}

uint8_t bme280_intf_getSensorData(uint8_t sensor_comp) {
    bme280_get_sensor_data(sensor_comp, bme280_intf_config->data, bme280_intf_config->dev);
    return !bme280_intf_config->dev->intf_rslt;
}

void bme280_intf_init(bme280_intf_config_t *config) {
    bme280_intf_config                = config;
    bme280_intf_config->dev           = &my_dev;
    bme280_intf_config->data          = &my_data;
    bme280_intf_config->dev->delay_us = bme280_intf_delay_us;
    bme280_intf_config->dev->read     = bme280_intf_read;
    bme280_intf_config->dev->write    = bme280_intf_write;
    bme280_intf_config->dev->intf_ptr = bme280_intf_config;
    bme280_intf_config->settings      = config->settings;
    bme280_intf_config->data          = config->data;
    bme280_intf_config->max_delay     = config->max_delay;
    bme280_intf_config->dev->intf     = config->intf;
    if (config->intf == BME280_I2C_INTF) {
        communication_start = bme280_intf_I2CStart;
        /* Application-dependent I2C configuration, e.g. delay function to use */
        bme280_i2c_config->delay_us = delay_us;
        I2C_init(bme280_i2c_config);
    } else if (config->intf == BME280_SPI_INTF) {
        communication_start         = bme280_intf_SPIStart;
        bme280_spi_config->delay_us = delay_us;
        SPI_init(bme280_spi_config);
    }

    bme280_init(bme280_intf_config->dev);
    bme280_intf_setSensorSettings(BME280_SEL_ALL_SETTINGS);
    bme280_intf_calMeasDelay();
    bme280_intf_setSensorMode(BME280_POWERMODE_NORMAL);
}
