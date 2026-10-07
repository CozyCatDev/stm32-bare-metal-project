#ifndef USART_H
#define USART_H

#include "dma.h"
#include "gpio.h"
#include "stm32c031xx.h"
#include <stdint.h>

#define USART2_TX_GPIO_PIN 2
#define USART2_RX_GPIO_PIN 3
#define USART1_TX_GPIO_PIN 6
#define USART1_RX_GPIO_PIN 7

#define USART2_GPIO_PORT (GPIOA)
#define USART1_GPIO_PORT (GPIOB)

#define USART_DEBUG

#ifdef USART_DEBUG
    #include "debug.h"
#endif

/* USART TXFIFO size: 7/8/9 data bits x 8 levels */
/* USART RXFIFO size: (7/8/9 data bits + parity error bit + noise error bit + framing error bit) x 8
 * levels */

typedef enum {
    USART_WORD_LENGTH_8BIT,
    USART_WORD_LENGTH_9BIT,
    USART_WORD_LENGTH_7BIT
} usart_word_length_t;

typedef enum {
    USART_STOP_BITS_1BIT,
    USART_STOP_BITS_HALFBIT,
    USART_STOP_BITS_2BIT,
    USART_STOP_BITS_ONEHALFBIT
} usart_stop_bits_t;

typedef enum { USART_OVERSAMPLING_MODE_16X, USART_OVERSAMPLING_MODE_8X } usart_oversampling_mode_t;

typedef enum { USART_PARITY_SELECTION_EVEN, USART_PARITY_SELECTION_ODD } usart_parity_selection_t;

typedef enum {
    USART_CLOCK_POLARITY_ACTIVE_LOW,
    USART_CLOCK_POLARITY_ACTIVE_HIGH
} usart_clock_polarity_t;

typedef enum { USART_CLOCK_PHASE_FIRST_EDGE, USART_CLOCK_PHASE_SECOND_EDGE } usart_clock_phase_t;

typedef enum {
    USART_CLOCK_PRESCALER_NONE,
    USART_CLOCK_PRESCALER_DIV2,
    USART_CLOCK_PRESCALER_DIV4,
    USART_CLOCK_PRESCALER_DIV6,
    USART_CLOCK_PRESCALER_DIV8,
    USART_CLOCK_PRESCALER_DIV10,
    USART_CLOCK_PRESCALER_DIV12,
    USART_CLOCK_PRESCALER_DIV16,
    USART_CLOCK_PRESCALER_DIV32,
    USART_CLOCK_PRESCALER_DIV64,
    USART_CLOCK_PRESCALER_DIV128,
    USART_CLOCK_PRESCALER_DIV256,
} usart_clock_prescaler_t;

typedef enum {
    USART_TXFIFO_THRESHOLD_1LEVEL,
    USART_TXFIFO_THRESHOLD_2LEVEL,
    USART_TXFIFO_THRESHOLD_4LEVEL,
    USART_TXFIFO_THRESHOLD_6LEVEL,
    USART_TXFIFO_THRESHOLD_7LEVEL,
    USART_TXFIFO_THRESHOLD_EMPTY
} usart_txfifo_threshold_t;

typedef enum {
    USART_RXFIFO_THRESHOLD_1LEVEL,
    USART_RXFIFO_THRESHOLD_2LEVEL,
    USART_RXFIFO_THRESHOLD_4LEVEL,
    USART_RXFIFO_THRESHOLD_6LEVEL,
    USART_RXFIFO_THRESHOLD_7LEVEL,
    USART_RXFIFO_THRESHOLD_FULL
} usart_rxfifo_threshold_t;

typedef struct {
    usart_word_length_t       word_length;
    uint16_t                  baud_rate;
    usart_stop_bits_t         stop_bits;
    usart_oversampling_mode_t oversampling_mode;
    uint8_t                   fifo_mode;
    uint8_t                   parity_control_enable;
    usart_parity_selection_t  parity_selection;
    uint8_t                   msb_first;
    usart_clock_polarity_t    clock_polarity;
    usart_clock_phase_t       clock_phase;
    uint8_t                   last_bit_clock_pulse;
    usart_txfifo_threshold_t  txfifo_threshold;
    usart_rxfifo_threshold_t  rxfifo_threshold;
    usart_clock_prescaler_t   clock_prescaler;
    uint8_t                   character_match;
} usart_config_t;

uint8_t
USART_read(const uint8_t num_rx, volatile uint16_t *rx_buf, const uint8_t communication_mode);

uint8_t
USART_write(const uint8_t num_tx, volatile uint16_t *tx_buf, const uint8_t communication_mode);

// uint8_t USART_start(
//	const uint8_t num_rx,
//	const uint8_t num_tx,
//	volatile uint16_t* rx_buf,
//	volatile uint16_t* tx_buf,
//	const uint8_t communication_mode
//);

void USART_init(usart_config_t *config);

#endif
