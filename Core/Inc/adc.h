/**
 * @file       adc.h
 * @brief      ADC driver for STM32C031.
 *
 * Provides configuration and control functions for the STM32C031C6 ADC peripheral.
 */

/**
 * @defgroup ADC ADC
 * @brief ADC peripheral driver.
 */

#ifndef ADC_H
#define ADC_H

#include "stm32c031xx.h"
#include <stdint.h>

/**
 * @ingroup ADC
 * @defgroup ADC_Init Initialization
 * @brief Initialize and configure the ADC peripheral.
 */

/**
 * @ingroup ADC_Init
 * @brief ADC clock source.
 */
typedef enum {
    SYSCLK, /**< System clock. */
    HSIKER  /**< HSI kernel clock. */
} adc_adcclk_source_t;

/**
 * @ingroup ADC_Init
 * @brief ADC is either clocked asynchronously by ADCCLK or by the APB peripheral clock (PCLK).
 * @see adc_adcclk_source_t (ADCCLK source)
 */
typedef enum {
    ADCCLK, /**< ADC clock. */
    PCLK2,  /**< PCLK divided by 2. */
    PCLK4,  /**< PCLK divided by 4. */
    PCLK    /**< APB peripheral clock. */
} adc_ckmode_t;

/**
 * @ingroup ADC_Init
 * @brief ADC channel selection mode. Either a fixed sequence (upwards/backwards) or arbitrary
 * sequence can be used.
 * @note Only 15 ADC channels (CH0 - CH14) can be used in arbitrary sequence mode. The arbitrary
 * sequence length is 8 channels, e.g. CH0->CH2->CH4->CH5->CH7->CH12->CH13->CH14.
 */
typedef enum {
    FIXED_SEQUENCE,    /**< Use a fixed channel sequence. */
    ARBITRARY_SEQUENCE /**< Use an arbitrary channel sequence. */
} adc_chselrmod_t;

/**
 * @ingroup ADC_Init
 * @brief ADC discontinuous conversion mode.
 */
typedef enum {
    SINGLE_TRIGGER, /**< Perform one conversion per trigger. */
    MULTI_TRIGGER   /**< Perform multiple conversions per trigger. */
} adc_discen_t;

/**
 * @ingroup ADC_Init
 * @brief ADC conversion mode.
 */
typedef enum {
    SINGLE,    /**< Perform one conversion sequence. */
    CONTINUOUS /**< Conversion sequences are repeated indefinitely by hardware until ADC_stop() is
                  called. */
} adc_cont_t;

/**
 * @ingroup ADC_Init
 * @brief ADC data alignment.
 */
typedef enum {
    RIGHT, /**< Right-align conversion results. */
    LEFT   /**< Left-align conversion results. */
} adc_align_t;

/**
 * @ingroup ADC_Init
 * @brief ADC resolution.
 */
typedef enum {
    TWELVE_BIT, /**< 12-bit resolution. */
    TEN_BIT,    /**< 10-bit resolution. */
    EIGHT_BIT,  /**< 8-bit resolution. */
    SIX_BIT     /**< 6-bit resolution. */
} adc_resolution_t;

/**
 * @ingroup ADC_Init
 * @brief ADC scan direction.
 */
typedef enum {
    UPWARD,  /**< Scan channels in ascending order. */
    BACKWARD /**< Scan channels in descending order. */
} adc_scandir_t;

/**
 * @ingroup ADC_Init
 * @brief ADC temperature sensor state.
 */
typedef enum {
    TS_DISABLED, /**< Disable internal temperature sensor. */
    TS_ENABLED   /**< Enable internal temperature sensor. */
} adc_temp_sensor_t;

/**
 * @ingroup ADC_Init
 * @brief ADC voltage reference state.
 */
typedef enum {
    VR_DISABLED, /**< Disable internal voltage reference. */
    VR_ENABLED   /**< Enable internal voltage reference. */
} adc_voltage_reference_t;

/**
 * @ingroup ADC_Init
 * @brief ADC end-of-conversion interrupt state.
 */
typedef enum {
    EOC_DISABLED, /**< Disable EOC interrupt. */
    EOC_ENABLED   /**< Enable EOC interrupt. */
} adc_eoc_interrupt_t;

/**
 * @ingroup ADC_Init
 * @brief ADC end-of-sequence interrupt state.
 */
typedef enum {
    EOS_DISABLED, /**< Disable EOS interrupt. */
    EOS_ENABLED   /**< Enable EOS interrupt. */
} adc_eos_interrupt_t;

/**
 * @ingroup ADC_Init
 * @brief ADC peripheral configuration.
 *
 * Contains the parameters required to initialize and configure
 * the ADC peripheral.
 */
typedef struct {
    /**
     * @brief User-defined microsecond delay function.
     *
     * @param[in] us Delay duration in microseconds.
     */
    void (*delay_us)(uint32_t us);

    adc_adcclk_source_t adcclk_source;      /**< ADC clock source. */
    adc_ckmode_t        clock_source;       /**< ADC clock mode. */
    adc_chselrmod_t     chselr_mode;        /**< Channel selection mode. */
    adc_discen_t        discontinuous_mode; /**< Discontinuous mode. */
    adc_cont_t          conversion_mode;    /**< Conversion mode. */
    adc_align_t         data_alignment;     /**< Result data alignment. */
    adc_resolution_t    resolution;         /**< ADC resolution. */
    adc_scandir_t       scan_direction;     /**< Channel scan direction. */

    /**
     * @brief Bitmask specifying which sampling time to use ('0' = use `sampling_time1`, '1' = use
     * `sampling_time2`).
     * @note Only bits[22:0] considered as there are only 23 ADC channels.
     */
    uint32_t sampling_channel_bitmask;

    /**
     * @brief Sampling time configuration 1 in terms of ADC clock cycles.
     * @details
     * | Value | Sampling time |
     * | :---  | :---:         |
     * | 000   | 1.5 ADC clock cycles |
     * | 001   | 3.5 ADC clock cycles |
     * | 010   | 7.5 ADC clock cycles |
     * | 011   | 12.5 ADC clock cycles |
     * | 100   | 19.5 ADC clock cycles |
     * | 101   | 39.5 ADC clock cycles |
     * | 110   | 79.5 ADC clock cycles |
     * | 111   | 160.5 ADC clock cycles |
     */
    uint8_t sampling_time1;

    /**
     * @brief Sampling time configuration 2 in terms of ADC clock cycles.
     * @details
     * | Value | Sampling time |
     * | :---  | :---:         |
     * | 000   | 1.5 ADC clock cycles |
     * | 001   | 3.5 ADC clock cycles |
     * | 010   | 7.5 ADC clock cycles |
     * | 011   | 12.5 ADC clock cycles |
     * | 100   | 19.5 ADC clock cycles |
     * | 101   | 39.5 ADC clock cycles |
     * | 110   | 79.5 ADC clock cycles |
     * | 111   | 160.5 ADC clock cycles |
     */
    uint8_t sampling_time2;

    /**
     * @brief ADC channel selection configuration.
     * @note Only bits[22:0] considered as there are only 23 ADC channels.
     */
    uint32_t channel_select;

    /**
     * @brief ADC conversion sequence.
     * Contains up to 8 ADC channels in conversion order. To convert
     * CH0->CH2->CH4->CH5->CH7->CH12->CH13->CH14, do:
     * @code
     * uint16_t channel_sequence[8] = {0, 2, 4, 5, 7, 12, 13, 14};
     * @endcode
     * @note Only used when arbitrary sequence mode is selected.
     */
    uint16_t channel_sequence[8];

    adc_temp_sensor_t       temp_sensor_state;       /**< Temperature sensor state. */
    adc_voltage_reference_t voltage_reference_state; /**< Voltage reference state. */
    adc_eoc_interrupt_t     eoc_interrupt_state;     /**< EOC interrupt state. */
    adc_eos_interrupt_t     eos_interrupt_state;     /**< EOS interrupt state. */

} adc_config_t;

/**
 * @ingroup ADC_Init
 * @brief Initialize the ADC peripheral.
 *
 * Configures the ADC according to the supplied configuration structure.
 *
 * @param[in] adc_config Pointer to the ADC configuration structure.
 *
 * @pre The ADC configuration structure must be fully initialized.
 *
 * @see adc_config_t
 */
void ADC_init(adc_config_t *adc_config);

/**
 * @ingroup ADC
 * @defgroup ADC_Control Peripheral Control
 * @brief Control ADC operation.
 */

/**
 * @ingroup ADC_Control
 * @brief Enable the ADC peripheral.
 *
 * @pre ADC_init() must have been called successfully.
 */
void ADC_enable(void);

/**
 * @ingroup ADC_Control
 * @brief Disable the ADC peripheral.
 *
 * @pre No ADC conversion must be ongoing.
 */
void ADC_disable(void);

/**
 * @ingroup ADC_Control
 * @brief Start ADC conversion.
 *
 * @pre ADC_init() must have been called successfully.
 */
void ADC_start(void);

/**
 * @ingroup ADC_Control
 * @brief Stop ADC conversion.
 */
void ADC_stop(void);

/**
 * @ingroup ADC
 * @defgroup ADC_Data Data Acquisition
 * @brief Read ADC conversion results.
 */

/**
 * @ingroup ADC_Data
 * @brief Read ADC conversion results.
 *
 * @param[out] data Destination buffer for ADC conversion results.
 *
 * @note The required buffer size depends on the configured conversion sequence (fixed/arbitrary).
 * Buffer size must be at least 23 in fixed sequence mode and at least 15 in arbitrary sequence
 * mode.
 */
void ADC_read(volatile uint16_t *data);

#endif /* ADC_H */
