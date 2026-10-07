#include "adc.h"

static adc_config_t *adc_config;

void ADC_init(adc_config_t *config) {
    adc_config = config;
    // Configuration settings required before calibration
    ADC1->CR &= ~ADC_CR_ADEN;           // ADC disabled
    ADC1->CR |= ADC_CR_ADVREGEN;        // voltage regulator enabled
    ADC1->CFGR1 &= ~(ADC_CFGR1_AUTOFF | // Auto-off mode disabled
                     ADC_CFGR1_DMAEN    // DMA disabled
    );

    // Calibration
    ADC1->CR |= ADC_CR_ADCAL;        // ADCAL bit must be set before calibration
    while (ADC1->CR & ADC_CR_ADCAL); // wait until ADCAL = 0 (calibration data bits ready)
    if (ADC1->DR <= 0x7F) {
        ADC1->CALFACT = ((ADC1->DR & 0x7F) + 1U) &
                        ADC_CALFACT_CALFACT_Msk; // calibration data + 1 written into CALFACT
    } else {
        ADC1->CALFACT = 0x7F; // prevent overflow if calibration bits > 0x7F
    }

    // Select clock source
    RCC->APBENR2 |= RCC_APBENR2_ADCEN; // enable clock signal to ADC
    if (adc_config->adcclk_source == HSIKER) {
        if (adc_config->clock_source == ADCCLK) {
            RCC->CCIPR |= RCC_CCIPR_ADCSEL_1; // use HSIKER derived from HSI48 as ADC clock source
            RCC->CR |= RCC_CR_HSION;          // enable HSI48 clock
            RCC->CR &= ~RCC_CR_HSIKERDIV;
            RCC->CR |= RCC_CR_HSIKERDIV_1; // HSIKER pre-scaler (reset value = '010', i.e. 3 so 48/3
                                           // = 16MHz)
            while ((RCC->CR & RCC_CR_HSIRDY) == 0); // wait for HSI48 to be ready
            ADC1->CFGR2 &=
                ~ADC_CFGR2_CKMODE; // use asynchronous clock source (HSIKER in this case) instead of
                                   // clock source derived from APB clock (PCLK)
        }
        // TODO: add config for other clock sources (PCLK2, PCLK4, PCLK)
    }

    // Enable
    ADC1->ISR |= ADC_ISR_ADRDY;               // clear ADC ready bit (write-clear)
    ADC1->CR |= ADC_CR_ADEN;                  // set ADC enable bit
    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0); // wait for ADC ready bit to be set

    // Configure sampling time
    ADC1->SMPR &= ~0x7FFFFF00;
    ADC1->SMPR |= (adc_config->sampling_channel_bitmask << 8U) & 0x7FFFFF00;
    /* NOTE: min. value of t_{S_temp}, i.e. min. sampling time for temperature sensor is 5us */

    ADC1->SMPR &= ~(ADC_SMPR_SMP1 | ADC_SMPR_SMP2);

    // use 160.5 ADC clock cycles (160.5 / 16MHz = 10.03125us), i.e. longest sampling time for SMP1
    // and SMP2
    ADC1->SMPR |= (adc_config->sampling_time1 << ADC_SMPR_SMP1_Pos | adc_config->sampling_time2
                                                                         << ADC_SMPR_SMP2_Pos);

    // Configure conversion mode
    ADC1->CFGR1 &= ~(ADC_CFGR1_CHSELRMOD | ADC_CFGR1_DISCEN | ADC_CFGR1_CONT | ADC_CFGR1_ALIGN |
                     ADC_CFGR1_RES | ADC_CFGR1_SCANDIR);
    ADC1->CFGR1 |=
        (adc_config->chselr_mode << ADC_CFGR1_CHSELRMOD_Pos |     // scan in either forward/backward
                                                                  // direction (no arbitrary order)
         adc_config->discontinuous_mode << ADC_CFGR1_DISCEN_Pos | // one software/hardware trigger
                                                                  // completes entire sequence scan
         adc_config->conversion_mode << ADC_CFGR1_CONT_Pos |      // no automatic restart
         adc_config->data_alignment << ADC_CFGR1_ALIGN_Pos | // right alignment for data register
         adc_config->resolution << ADC_CFGR1_RES_Pos |       // 12-bit resolution
         adc_config->scan_direction << ADC_CFGR1_SCANDIR_Pos // upward scan direction
        );

    switch (adc_config->chselr_mode) {
    case FIXED_SEQUENCE:
        ADC1->CHSELR &= ~0x007FFFFF;
        ADC1->CHSELR |= (adc_config->channel_select & 0x007FFFFF);
        break;
    case ARBITRARY_SEQUENCE:
        ADC1->CHSELR &= ~0xFFFFFFFF;
        for (uint8_t i = 0; i < 8; i++) {
            ADC1->CHSELR |= (adc_config->channel_sequence[i] << (i * 4));
        }
        break;
    }

    if (adc_config->eoc_interrupt_state == EOC_ENABLED) {
        ADC1->IER |= ADC_IER_EOCIE; // enable end-of-conversion interrupt
    }
    if (adc_config->eos_interrupt_state == EOS_ENABLED) {
        ADC1->IER |= ADC_IER_EOSIE; // enable end-of-sequence interrupt
    }
    if (ADC1->IER) {
        NVIC_SetPriority(ADC1_IRQn, 0);
        NVIC_EnableIRQ(ADC1_IRQn);
    }

    if (adc_config->temp_sensor_state == TS_ENABLED) {
        ADC->CCR |= ADC_CCR_TSEN; // wake up temperature sensor from power down mode
    }
    if (adc_config->voltage_reference_state == VR_ENABLED) {
        ADC->CCR |= ADC_CCR_VREFEN; // enable voltage reference required by temperature sensor
    }
    /* NOTE: max. value of t_START, i.e. max. startup time for temperature sensor is 120us */
    adc_config->delay_us(200); // wait for 1ms
}

void ADC_enable() {
    ADC1->ISR |= ADC_ISR_ADRDY;               // clear ADC ready bit (write-clear)
    ADC1->CR |= ADC_CR_ADEN;                  // set ADC enable bit
    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0); // wait for ADC ready bit to be set
}

void ADC_disable() {
    while (ADC1->CR & ADC_CR_ADSTART); // wait for ADSTART to be cleared
    ADC1->CR |= ADC_CR_ADDIS;          // disable ADC
    while (ADC1->CR & ADC_CR_ADEN);    // wait for ADEN to be cleared
    ADC1->ISR |= ADC_ISR_ADRDY;        // clear ADRDY bit (write-clear)
}

void ADC_start() {
    ADC1->CR |= ADC_CR_ADSTART;
}

void ADC_stop() {
    ADC1->CR |= ADC_CR_ADSTP;
}

void ADC_read(volatile uint16_t *data) {
    if (adc_config->data_alignment == RIGHT) {
        *data = (uint16_t)(ADC1->DR);
    } else if (adc_config->data_alignment == LEFT) {
        if (adc_config->resolution == TWELVE_BIT) {
            *data = (uint16_t)(ADC1->DR >> 4U);
        } else if (adc_config->resolution == TEN_BIT) {
            *data = (uint16_t)(ADC1->DR >> 6U);
        } else if (adc_config->resolution == EIGHT_BIT) {
            *data = (uint16_t)(ADC1->DR >> 8U);
        } else if (adc_config->resolution == SIX_BIT) {
            *data = (uint16_t)(ADC1->DR >> 2U);
        }
    }
    //	ADC1->ISR |= ADC_ISR_EOC;
}
