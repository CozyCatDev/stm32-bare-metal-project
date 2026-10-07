/**
 * @file       adxl345.h
 * @brief      Driver for ADXL345 accelerometer.
 */

/**
 * @defgroup ADXL345 ADXL345
 * @brief ADXL345 driver.
 *
 * ADXL345 can operate in 10-bit mode, where scale factor varies with g range or full-resolution
 * mode where the scale factor is constant at 3.9mg/LSB and resolution varies with g range. Internal
 * FIFO can be bypassed or used in FIFO/stream/trigger modes. Communication interfaces include
 * I2C/SPI. Polling/interrupts/DMA can be used in conjunction with these interfaces to read
 * accelerometer data.
 *
 * Each "sample" referred to in this driver is 6 bytes, consisting of:
 * 1. X-axis LSB
 * 2. X-axis MSB
 * 3. Y-axis LSB
 * 4. Y-axis MSB
 * 5. Z-axis LSB
 * 6. Z-axis MSB
 *
 * So, an output data rate (ODR) of 800Hz is 800 samples/second * 6 bytes/sample * 8 bits/byte =
 * 38400 bits/second.
 */

#ifndef ADXL345_H
#define ADXL345_H

#include "i2c.h"
#include "spi.h"
#include "stm32c031xx.h"
#include <stdint.h>

/**
 * @ingroup ADXL345
 * @defgroup ADXL345_Macros Macros
 * @brief ADXL345 macros.
 * @{
 */

#define ADXL345_ADDR ((uint8_t)(0x1D)) /**< Slave address. */

#define ADXL345_CAL_SAMPLES 80 /**< Number of calibration samples to take to correct 0g offset. */

/**
 * @defgroup ADXL345_RegisterDefinitions Register Definitions
 * @brief ADXL345 register definitions.
 * @{
 */
#define ADXL345_DEVID ((uint8_t)(0x00))
/* 0x01 to 0x1C reserved */
#define ADXL345_THRESH_TAP     ((uint8_t)(0x1D))
#define ADXL345_OFSX           ((uint8_t)(0x1E))
#define ADXL345_OFSY           ((uint8_t)(0x1F))
#define ADXL345_OFSZ           ((uint8_t)(0x20))
#define ADXL345_DUR            ((uint8_t)(0x21))
#define ADXL345_LATENT         ((uint8_t)(0x22))
#define ADXL345_WINDOW         ((uint8_t)(0x23))
#define ADXL345_THRESH_ACT     ((uint8_t)(0x24))
#define ADXL345_THRESH_INACT   ((uint8_t)(0x25))
#define ADXL345_TIME_INACT     ((uint8_t)(0x26))
#define ADXL345_ACT_INACT_CTL  ((uint8_t)(0x27))
#define ADXL345_THRESH_FF      ((uint8_t)(0x28))
#define ADXL345_TIME_FF        ((uint8_t)(0x29))
#define ADXL345_TAP_AXES       ((uint8_t)(0x2A))
#define ADXL345_ACT_TAP_STATUS ((uint8_t)(0x2B))
#define ADXL345_BW_RATE        ((uint8_t)(0x2C))
#define ADXL345_POWER_CTL      ((uint8_t)(0x2D))
#define ADXL345_INT_ENABLE     ((uint8_t)(0x2E))
#define ADXL345_INT_MAP        ((uint8_t)(0x2F))
#define ADXL345_INT_SOURCE     ((uint8_t)(0x30))
#define ADXL345_DATA_FORMAT    ((uint8_t)(0x31))
#define ADXL345_DATA_X0        ((uint8_t)(0x32))
#define ADXL345_DATA_X1        ((uint8_t)(0x33))
#define ADXL345_DATA_Y0        ((uint8_t)(0x34))
#define ADXL345_DATA_Y1        ((uint8_t)(0x35))
#define ADXL345_DATA_Z0        ((uint8_t)(0x36))
#define ADXL345_DATA_Z1        ((uint8_t)(0x37))
#define ADXL345_FIFO_CTL       ((uint8_t)(0x38))
#define ADXL345_FIFO_STATUS    ((uint8_t)(0x39))
/** @} */
/** @} */

/**
 * @ingroup ADXL345
 * @defgroup ADXL345_Init Initialization
 * @brief Initialize and configure the ADXL345 accelerometer.
 */

/**
 * @ingroup ADXL345_Init
 * @brief I2C or SPI communication format.
 */
typedef enum {
    ADXL345_I2C_MODE, /**< Use ADXL345 I2C interface. */
    ADXL345_SPI_MODE  /**< Use ADXL345 SPI interface. */
} adxl345_communication_format_t;

/**
 * @ingroup ADXL345_Init
 * @brief Interrupt polarity of INT1/INT2 pins.
 */
typedef enum {
    ADXL345_INTERRUPT_POLARITY_ACTIVE_HIGH, /**< INT1/INT2 pins driven high.*/
    ADXL345_INTERRUPT_POLARITY_ACTIVE_LOW   /**< INT1/INT2 pins driven low.*/
} adxl345_interrupt_polarity_t;

/**
 * @ingroup ADXL345_Init
 * @brief Resolution mode.
 */
typedef enum {
    ADXL345_TEN_BIT_MODE, /**< Scale factor (mg/LSB) varies with g range. */
    ADXL345_FULL_RES      /**< Scale factor = 4mg/LSB as resolution varies with g range. */
} adxl345_resolution_t;

/**
 * @ingroup ADXL345_Init
 * @brief Data alignment.
 */
typedef enum {
    ADXL345_RIGHT_ALIGNED, /**< Right-aligned data w/ sign extension. */
    ADXL345_LEFT_ALIGNED   /**< Left-aligned data. */
} adxl345_justify_t;

/**
 * @ingroup ADXL345_Init
 * @brief FIFO modes.
 */
typedef enum {
    ADXL345_BYPASS_MODE, /**< FIFO bypassed. */
    ADXL345_FIFO_MODE,   /**< FIFO collects up to 32 samples and stops collecting when full. */
    ADXL345_STREAM_MODE, /**< FIFO holds last 32 samples. Oldest data is overwritten when newer data
                            is available and FIFO is full. */
    ADXL345_TRIGGER_MODE /**< FIFO holds last 32 samples. FIFO keeps last n samples after a trigger
                            event and continues operating in ADXL345_FIFO_MODE. */
} adxl345_fifo_mode_t;

/**
 * @ingroup ADXL345_Init
 * @brief ADXL345 configuration object.
 */
typedef struct {
    /**
     * @brief User-defined microsecond delay function.
     *
     * @param[in] us Delay duration in microseconds.
     */
    void (*delay_us)(uint32_t us);
    /**
     * @brief User-defined I2C/SPI communication function.
     *
     * @param[in] slave_address I2C slave address.
     * @param[in] rw '1' = Read, '0' = Write.
     * @param[in] num_rx_bytes Number of bytes to receive in rx_buf.
     * @param[in] num_tx_bytes Number of bytes to transmite from tx_buf.
     * @param[out] rx_buf Receive buffer for storing data read from I2C/SPI bus.
     * @param[in] tx_buf Transmit buffer for transmitting data to I2C/SPI bus.
     * @param[in] communication_mode '0' = Polling, '1' = Interrupt, '2' = DMA.
     */
    uint8_t (*communication_start)(const uint8_t     slave_address,
                                   const uint8_t     rw,
                                   const uint8_t     num_rx_bytes,
                                   const uint8_t     num_tx_bytes,
                                   volatile uint8_t *rx_buf,
                                   volatile uint8_t *tx_buf,
                                   const uint8_t     communication_mode);

    adxl345_communication_format_t communication_format; /**< I2C/SPI communication format. */

    /* Data transfer settings */
    uint8_t rate; /**< 0.10Hz to 3200Hz (0000 to 1111, doubles per LSB) */
    adxl345_resolution_t
        resolution;            /**< 10-bit resolution for all data ranges or dynamic resolution */
    adxl345_justify_t justify; /**< right justified (with sign extension) or left-justified */
    uint8_t           range;   /**< +-2g to +-16g (00 to 11, doubles per LSB) */

    /* Interrupt settings */
    /* Bitmask order: OVERRUN, WATERMARK, FREE_FALL, INACTIVITY, ACTIVITY, DOUBLE_TAP, SINGLE_TAP,
     * DATA_READY */
    adxl345_interrupt_polarity_t interrupt_polarity; /**< Active high/low interrupts. */
    uint8_t                      interrupt_enable;   /**< '1' = enable, '0' = disable. */
    uint8_t  interrupt_map;    /**< '0' = map to INT1 pin, '1' = map to INT2 pin. */
    uint8_t *interrupt_status; /**< '0' = no interrupt occurred, '1' = interrupt occurred. */

    /* FIFO settings */
    adxl345_fifo_mode_t fifo_mode; /**< Bypass/FIFO/Stream/Trigger modes. */
    uint8_t fifo_samples; /**< Number of FIFO samples for WATERMARK interrupt or number of samples
                             to keep before trigger event in Trigger mode. */

    /* I2C variables */
    volatile uint8_t *rx_buf; /**< I2C receive buffer. */
    volatile uint8_t *tx_buf; /**< I2C transmit buffer. */
} adxl345_config_t;

/**
 * @ingroup ADXL345_Init
 * @brief Initialize the ADXL345 accelerometer.
 *
 * Performs the following actions in order:
 * 1. Configures the ADXL345 according to the configuration object.
 * 2. Puts ADXL345 in measurement mode.
 * 3. Calibrates ADXL345 by taking ADXL345_CAL_SAMPLES measurements, averaging them and storing in
 * offset correction registers to correct 0g offset.
 *
 * @param[in] adxl345_config Pointer to the ADXL345 configuration object.
 *
 * @warning This function must be called before any other driver functions.
 *
 * @see adxl345_config_t
 */
void ADXL345_init(adxl345_config_t *config);

/**
 * @ingroup ADXL345
 * @defgroup ADXL345_Data Data Acquisition
 * @ingroup ADXL345_Data
 * @brief Read 2's complement accelerometer data into user-provided buffers.
 *
 * @param[out] x X-axis accelerometer data.
 * @param[out] y Y-axis accelerometer data.
 * @param[out] z Z-axis accelerometer data.
 * @param[in] communication_mode '0' = Polling, '1' = Interrupt, '2' = DMA
 */
uint8_t ADXL345_readData(volatile int16_t *x,
                         volatile int16_t *y,
                         volatile int16_t *z,
                         const uint8_t     communication_mode);

#endif
