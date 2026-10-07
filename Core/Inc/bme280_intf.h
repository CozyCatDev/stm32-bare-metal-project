/**
 * @file       bme280_intf.h
 * @brief      Interface driver that uses BME280 API by Bosch.
 *
 * @note This is not the official BME280 API by Bosch. It is a driver specifically written for this
 * project to interface with the BME280 API provided by Bosch, e.g. to provide user-defined read
 * and write functions, configure sensor settings, etc. Refer to bme280.h and bme280_defs.h for the
 * API provided by Bosch, which includes (but not limited to) register definitions, data readout,
 * compensation formulas, etc.
 */

/**
 * @defgroup BME280_intf BME280 Interface
 * @brief BME280 interface driver.
 *
 * The BME280 sensor can operate in 3 modes, i.e. sleep, normal and forced modes. In sleep mode, no
 * measurements can be made but all registers are accessible. Sensor settings should be configured
 * in this mode. In normal mode, perpetual cycling between active measurement periods and inactive
 * standby periods are performed. Temperature, pressure and humidity measurements are performed
 * during active periods while no measurements are performed in standby periods. In forced mode, a
 * single measurement cycle is performed, followed by the sensor entering sleep mode. An optional
 * hardware IIR filter is implemented for pressure and temperature measurements. I2C/SPI interfaces
 * are available. Polling/interrupts/DMA can be used with these interfaces.
 *
 * Each "sample" referred to in this driver is 8 bytes, consisting of:
 * 1. Humidity LSB
 * 2. Humidity MSB
 * 3. Temperature XLSB (dependent on resolution, which itself is dependent on IIR filter and
 * oversampling settings)
 * 4. Temperature LSB
 * 5. Temperature MSB
 * 6. Pressure XLSB (dependent on resolution, which itself is dependent on IIR filter and
 * oversampling settings)
 * 7. Pressure LSB
 * 8. Pressure MSB
 */

#ifndef BME280_INTF_H
#define BME280_INTF_H

#include "bme280.h"
#include "bme280_defs.h"
#include "debug.h"
#include "delay.h"
#include "i2c.h"
#include "spi.h"
#include <string.h>

/**
 * @ingroup BME280_intf
 * @defgroup BME280_Macros Macros
 * @brief BME280 macros.
 * @{
 */
#define BME280_ADDR ((uint8_t)(0x77)) /**< Slave address. */
/** @} */

/**
 * @ingroup BME280_intf
 * @brief BME280 configuration object to be passed into bme280_intf_init().
 *
 * Struct members follow definitions from bme280_defs.h.
 */
typedef struct {
    struct bme280_dev     *dev;      /**< Dev object for defining callback functions. */
    struct bme280_settings settings; /**< Oversampling, filter, standby time settings. */
    volatile struct bme280_data
             *data; /**< Data object to store pressure, temperature and humidity measurements. */
    uint32_t *max_delay;   /**< Stores maximum possible measurement delay which is dependent on
                              oversampling settings. */
    enum bme280_intf intf; /**< I2C/SPI interface. */
} bme280_intf_config_t;

/**
 * @ingroup BME280_intf
 * @defgroup BME280_intf_ReadWrite Read/Write Functions
 * @brief Read/write callback functions to be defined in dev object.
 * @note The body of these functions implement polling. Interrupts and DMA cannot be used as the
 * Bosch API expects data to be fully read/written from/to registers when these functions return.
 */

/**
 * @ingroup BME280_intf_ReadWrite
 * @brief Callback function for reading data from BME280 registers.
 * @param[in] reg_addr Register address to read from.
 * @param[out] reg_data Receive buffer to store data read from reg_addr.
 * @param[in] len Number of bytes to read into reg_data.
 * @param[in] intf_ptr Pointer to interface descriptor, i.e. to a bme280_intf_config_t object in
 * this case.
 *
 * @retval 0 Success.
 * @retval != 0 Fail.
 */
BME280_INTF_RET_TYPE
bme280_intf_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);

/**
 * @ingroup BME280_intf_ReadWrite
 * @brief Callback function for writing data to BME280 registers.
 * @param[in] reg_addr Register address to write to.
 * @param[in] reg_data Transmit buffer containing data to transmit to reg_addr.
 * @param[in] len Number of bytes to transmit from reg_data.
 * @param[in] intf_ptr Pointer to interface descriptor, i.e. to a bme280_intf_config_t object in
 * this case.
 *
 * @retval 0 Success.
 * @retval != 0 Fail.
 */
BME280_INTF_RET_TYPE
bme280_intf_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);

/**
 * @ingroup BME280_intf_ReadWrite
 * @brief Function to read sensor data into `volatile struct bme280_data *data` of configuration
 * object.
 * @param[in] sensor_comp Measurement bitmask, <P><T><H>, e.g. '101' enables compensated
 * measurements for pressure and humidity only.
 *
 * @retval 0 Success.
 * @retval != 0 Fail.
 */
uint8_t bme280_intf_getSensorData(uint8_t sensor_comp);

/**
 * @ingroup BME280_intf
 * @defgroup BME280_intf_Control Control Functions
 * @brief Functions to configure BME280 sensor settings.
 */

/**
 * @ingroup BME280_intf_Control
 * @brief Set oversampling, filter and standby settings.
 * @param[in] sensor_comp Desired settings bitmask, <P><T><H>, e.g. '101' configures oversampling,
 * filter and standby settings for pressure and humidity only.
 */
void bme280_intf_setSensorSettings(uint8_t desired_settings);

/**
 * @ingroup BME280_intf_Control
 * @brief Set sensor mode.
 * @param[in] sensor_mode Sensor mode bitmask. '00' = Sleep mode, '01' or '10' = Forced mode, '11' =
 * Normal mode.
 */
void bme280_intf_setSensorMode(uint8_t sensor_mode);

/**
 * @ingroup BME280_intf_Control
 * @brief Calculates measurement delay based on oversampling settings. Result is stored in `uint32_t
 * *max_delay` of bme280_intf_config_t object.
 */
void bme280_intf_calMeasDelay(void);

/**
 * @ingroup BME280_intf
 * @defgroup BME280_intf_Init Initialization
 * @brief Initialization function for BME280 interface driver.
 */

/**
 * @ingroup BME280_intf_Init
 * @brief Initialize the BME280 environmental sensor.
 *
 * Performs the following actions in order:
 * 1. Configures the callback functions, settings and data structs, interface pointer required by
 * Bosch API.
 * 2. Configures I2C/SPI interface functions.
 * 3. Calls Bosch API bme280_init() function.
 * 4. Sets sensor oversampling, filter and standby settings according to config.
 * 5. Calculates measurement delay based on oversampling settings.
 * 6. Configure sensor in normal mode.
 *
 * @param[in] config Pointer to the bme280_intf_config_t configuration object.
 *
 * @warning This function must be called before any other driver functions.
 *
 * @see bme280_intf_config_t
 */
void bme280_intf_init(bme280_intf_config_t *config);

#endif
