#include "adxl345.h"

static adxl345_config_t *adxl345_config;

/* Constant ADXL345 I2C parameters */
static i2c_config_t  my_adxl345_i2c_config = {};
static i2c_config_t *adxl345_i2c_config    = &my_adxl345_i2c_config;

static spi_config_t  my_adxl345_spi_config = {};
static spi_config_t *adxl345_spi_config    = &my_adxl345_spi_config;

static uint8_t ADXL345_I2C_start(const uint8_t     slave_address,
                                 const uint8_t     rw,
                                 const uint8_t     num_rx_bytes,
                                 const uint8_t     num_tx_bytes,
                                 volatile uint8_t *rx_buf,
                                 volatile uint8_t *tx_buf,
                                 const uint8_t     communication_mode) {
    return I2C_start(slave_address, rw, num_rx_bytes, num_tx_bytes, rx_buf, tx_buf,
                     communication_mode);
}

static uint8_t ADXL345_SPI_start(const uint8_t     slave_address,
                                 const uint8_t     rw,
                                 const uint8_t     num_rx_bytes,
                                 const uint8_t     num_tx_bytes,
                                 volatile uint8_t *rx_buf,
                                 volatile uint8_t *tx_buf,
                                 const uint8_t     communication_mode) {
    (void)slave_address;
    return SPI_start(rw, num_rx_bytes, num_tx_bytes, rx_buf, tx_buf, communication_mode);
}

static uint8_t ADXL345_getFirstByte(uint8_t register_address, uint8_t rw, uint8_t mb) {
    uint8_t first_byte = 0;
    switch (adxl345_config->communication_format) {
    case ADXL345_I2C_MODE:
        first_byte = register_address;
        break;
    case ADXL345_SPI_MODE:
        first_byte = ((register_address & 0x3F) | (rw << 7) | (mb << 6));
        break;
    }
    return first_byte;
}

static void ADXL345_setMeasurementMode() {
    adxl345_config->tx_buf[0] = ADXL345_getFirstByte(ADXL345_POWER_CTL, 0, 0);
    adxl345_config->tx_buf[1] = 0x08;
    adxl345_config->communication_start(ADXL345_ADDR, 0, 0, 2, adxl345_config->rx_buf,
                                        adxl345_config->tx_buf, 0);
}

static void ADXL345_setBWRate() {
    adxl345_config->tx_buf[0] = ADXL345_getFirstByte(ADXL345_BW_RATE, 0, 0);
    adxl345_config->tx_buf[1] = adxl345_config->rate & 0xF;
    adxl345_config->communication_start(ADXL345_ADDR, 0, 0, 2, adxl345_config->rx_buf,
                                        adxl345_config->tx_buf, 0);
}

static void ADXL345_setDataFormat() {
    adxl345_config->tx_buf[0] = ADXL345_getFirstByte(ADXL345_DATA_FORMAT, 0, 0);
    adxl345_config->tx_buf[1] = (adxl345_config->resolution << 3) | (adxl345_config->justify << 2) |
                                (adxl345_config->range & 0x3);
    adxl345_config->communication_start(ADXL345_ADDR, 0, 0, 2, adxl345_config->rx_buf,
                                        adxl345_config->tx_buf, 0);
}

static void ADXL345_setFIFOCtl() {
    adxl345_config->tx_buf[0] = ADXL345_getFirstByte(ADXL345_FIFO_CTL, 0, 0);
    adxl345_config->tx_buf[1] =
        (adxl345_config->fifo_mode << 6) | (adxl345_config->fifo_samples & 0x1F);
    adxl345_config->communication_start(ADXL345_ADDR, 0, 0, 2, adxl345_config->rx_buf,
                                        adxl345_config->tx_buf, 0);
}

static void ADXL345_setIntEnable() {
    adxl345_config->tx_buf[0] = ADXL345_getFirstByte(ADXL345_INT_ENABLE, 0, 0);
    adxl345_config->tx_buf[1] = adxl345_config->interrupt_enable;
    adxl345_config->communication_start(ADXL345_ADDR, 0, 0, 2, adxl345_config->rx_buf,
                                        adxl345_config->tx_buf, 0);
}

static void ADXL345_setIntMap() {
    adxl345_config->tx_buf[0] = ADXL345_getFirstByte(ADXL345_INT_MAP, 0, 0);
    adxl345_config->tx_buf[1] = adxl345_config->interrupt_map;
    adxl345_config->communication_start(ADXL345_ADDR, 0, 0, 2, adxl345_config->rx_buf,
                                        adxl345_config->tx_buf, 0);
}

static void ADXL345_setCalibrationOffset(int8_t ofsx, int8_t ofsy, int8_t ofsz) {
    adxl345_config->tx_buf[0] = ADXL345_getFirstByte(ADXL345_OFSX, 0, 0);
    adxl345_config->tx_buf[1] = (uint8_t)ofsx;
    adxl345_config->communication_start(ADXL345_ADDR, 0, 0, 2, adxl345_config->rx_buf,
                                        adxl345_config->tx_buf, 0);

    adxl345_config->tx_buf[0] = ADXL345_getFirstByte(ADXL345_OFSY, 0, 0);
    adxl345_config->tx_buf[1] = (uint8_t)ofsy;
    adxl345_config->communication_start(ADXL345_ADDR, 0, 0, 2, adxl345_config->rx_buf,
                                        adxl345_config->tx_buf, 0);

    adxl345_config->tx_buf[0] = ADXL345_getFirstByte(ADXL345_OFSZ, 0, 0);
    adxl345_config->tx_buf[1] = (uint8_t)ofsz;
    adxl345_config->communication_start(ADXL345_ADDR, 0, 0, 2, adxl345_config->rx_buf,
                                        adxl345_config->tx_buf, 0);
}

uint8_t ADXL345_readData(volatile int16_t *x,
                         volatile int16_t *y,
                         volatile int16_t *z,
                         const uint8_t     communication_mode) {
    uint8_t data_ready         = 0;
    uint8_t num_rx_dummy_bytes = adxl345_config->communication_format == ADXL345_SPI_MODE ? 1 : 0;
    uint8_t num_tx_dummy_bytes = adxl345_config->communication_format == ADXL345_SPI_MODE ? 6 : 0;
    adxl345_config->tx_buf[0]  = ADXL345_getFirstByte(ADXL345_DATA_X0, 1, 1);
    for (uint8_t i = 1; i <= num_tx_dummy_bytes; i++) {
        adxl345_config->tx_buf[i] = 0x40;
    }
    data_ready = adxl345_config->communication_start(ADXL345_ADDR, 1, 6 + num_rx_dummy_bytes,
                                                     1 + num_tx_dummy_bytes, adxl345_config->rx_buf,
                                                     adxl345_config->tx_buf, communication_mode);
    if (!data_ready) {
        return 0;
    }
    __disable_irq();
    *x = (int16_t)((uint16_t)(adxl345_config->rx_buf[0 + num_rx_dummy_bytes]) |
                   ((uint16_t)adxl345_config->rx_buf[1 + num_rx_dummy_bytes] << 8U)) *
         39 / 10;
    *y = (int16_t)((uint16_t)(adxl345_config->rx_buf[2 + num_rx_dummy_bytes]) |
                   ((uint16_t)adxl345_config->rx_buf[3 + num_rx_dummy_bytes] << 8U)) *
         39 / 10;
    *z = (int16_t)((uint16_t)(adxl345_config->rx_buf[4 + num_rx_dummy_bytes]) |
                   ((uint16_t)adxl345_config->rx_buf[5 + num_rx_dummy_bytes] << 8U)) *
         39 / 10;
    __enable_irq();
    return 1;
}

static void ADXL345_calibrate() {
    volatile int16_t x = 0, y = 0, z = 0;
    int32_t          x_sum = 0, y_sum = 0, z_sum = 0;
    int8_t           ofsx = 0, ofsy = 0, ofsz = 0;

    for (uint8_t i = 0; i < ADXL345_CAL_SAMPLES; i++) {
        ADXL345_readData(&x, &y, &z, 0);
        x_sum += (int32_t)x;
        y_sum += (int32_t)y;
        z_sum += (int32_t)z;
    }
    ofsx = (int8_t)(-x_sum * 10 / 39 / ADXL345_CAL_SAMPLES / 4);
    ofsy = (int8_t)(-y_sum * 10 / 39 / ADXL345_CAL_SAMPLES / 4);
    ofsz = (int8_t)(-(z_sum * 10 / 39 / ADXL345_CAL_SAMPLES - 256) / 4);
    ADXL345_setCalibrationOffset(ofsx, ofsy, ofsz);
}

void ADXL345_init(adxl345_config_t *config) {
    /* Save ADXL345 configuration object */
    adxl345_config = config;
    /* Turn-on time = 1/ODR + 1.1 = 1.25 + 1.1 = 2.35ms */
    //	adxl345_config->delay_ms(3);
    /* Configure I2C communication */
    if (adxl345_config->communication_format == ADXL345_I2C_MODE) {
        adxl345_config->communication_start = ADXL345_I2C_start;
        /* Application-dependent I2C configuration, e.g. delay function to use */
        adxl345_i2c_config->delay_us = adxl345_config->delay_us;
        I2C_init(adxl345_i2c_config);
    } else if (adxl345_config->communication_format == ADXL345_SPI_MODE) {
        adxl345_config->communication_start = ADXL345_SPI_start;
        adxl345_spi_config->delay_us        = adxl345_config->delay_us;
        SPI_init(adxl345_spi_config);
    }

    /* Configure ADXL345 settings */
    ADXL345_setBWRate();
    ADXL345_setDataFormat();
    ADXL345_setFIFOCtl();
    ADXL345_setIntEnable();
    ADXL345_setIntMap();
    ADXL345_setMeasurementMode();
    ADXL345_calibrate();
}
