#include "i2c.h"
#include "debug.h"

static i2c_config_t *i2c_config;

// typedef struct {
//	const uint8_t slave_address;
//	const uint8_t rw;
//	const uint8_t num_rx_bytes;
//	const uint8_t num_tx_bytes;
//	volatile uint8_t* rx_buf;
//	volatile uint8_t* tx_buf;
// } i2c_event_t;
//
// typedef struct {
//	uint8_t head;
//	uint8_t tail;
//	uint8_t count;
//	i2c_event_t* queue[I2C_EVENT_QUEUE_LENGTH];
// } i2c_event_queue_t;
//
// static i2c_event_queue_t my_i2c_event_queue;
// static i2c_event_queue_t* i2c_event_queue = &my_i2c_event_queue;

typedef enum {
    I2C_COMMUNICATION_MODE_POLLING,
    I2C_COMMUNICATION_MODE_INTERRUPT,
    I2C_COMMUNICATION_MODE_DMA
} i2c_communication_mode_t;

typedef enum {
    I2C_IDLE,
    I2C_SLAVE_ADDRESS_SENT,
    I2C_REGISTER_ADDRESS_SENT,
    I2C_RESTART_SENT,
    I2C_LAST_BYTE_TRANSFERRED
} i2c_state_t;

typedef struct {
    uint8_t                  rw;
    uint8_t                  num_rx_bytes;
    uint8_t                  num_tx_bytes;
    volatile uint8_t         rx_byte_idx;
    volatile uint8_t         tx_byte_idx;
    volatile i2c_state_t     i2c_state;
    volatile uint8_t        *rx_buf;
    volatile uint8_t        *tx_buf;
    i2c_communication_mode_t communication_mode;
} i2c_communication_t;

static dmamux_config_t i2c_dmamux_config = {.channel_select = DMAMUX_CHANNEL_SELECT_CH2};

static dma_config_t my_i2c_dma_config = {
    .channel_select        = DMA_CHANNEL_SELECT_CH3,
    .memory_to_memory_mode = 0,
    .channel_priority      = DMA_CHANNEL_PRIORITY_LOW,
    .memory_size           = DMA_MEMORY_SIZE_8BIT,
    .peripheral_size       = DMA_PERIPHERAL_SIZE_8BIT,
    // .memory_increment_mode initialized by I2C_start
    // .peripheral_increment_mode initialized by I2C_start
    //	.circular_mode initialized by I2C_start
    // .transfer_direction
    // .num_data initialized by I2C_start
    // .peripheral_address initialized by I2C_start
    // .memory_address initialized by I2C_start
    //	.dmamux_config = &i2c_dmamux_config
};
static dma_config_t *i2c_dma_config = &my_i2c_dma_config;

static i2c_communication_t  my_i2c_communication;
static i2c_communication_t *i2c_communication = &my_i2c_communication;

void I2C1_IRQHandler(void) {
    //	uint32_t i2c_isr = I2C1->ISR;
    //	if(I2C1->ISR & (I2C_ISR_NACKF | I2C_ISR_ARLO)){
    //		I2C1->CR2 |= I2C_CR2_STOP; // send STOP condition (clears TC flag)
    //		I2C1->ICR |= (I2C_ICR_NACKCF | I2C_ICR_ARLOCF); // clear NACKF flag
    //		i2c_communication->rx_byte_idx = 0;
    //		i2c_communication->tx_byte_idx = 0;
    //		i2c_communication->i2c_state = I2C_IDLE;
    //	}
    //	debug_write(0, 1);
    if (i2c_communication->communication_mode == I2C_COMMUNICATION_MODE_DMA) {
        if (I2C1->ISR & I2C_ISR_TC) {
            /* Send RESTART condition after register address sent */
            if (i2c_communication->rw &&
                i2c_communication->i2c_state == I2C_REGISTER_ADDRESS_SENT) {
                DMA_stop();
                i2c_dmamux_config.request_id       = DMAMUX_REQUEST_ID_I2C1_RX;
                i2c_dma_config->transfer_direction = DMA_TRANSFER_DIRECTION_P2M;
                i2c_dma_config->num_data           = i2c_communication->num_rx_bytes;
                i2c_dma_config->peripheral_address = (uint32_t)&I2C1->RXDR;
                i2c_dma_config->memory_address     = (uint32_t)i2c_communication->rx_buf;
                DMA_reconfigure(i2c_dma_config);
                DMA_start();
                I2C1->CR2 &= ~(I2C_CR2_NBYTES);
                I2C1->CR2 |=
                    (i2c_communication->num_rx_bytes << I2C_CR2_NBYTES_Pos); // read/write 1 byte
                I2C1->CR2 |= I2C_CR2_RD_WRN;                                 // read mode
                I2C1->CR2 |= I2C_CR2_START; // restart I2C communication
                i2c_communication->i2c_state = I2C_RESTART_SENT;
            }
            /* Send STOP condition after write transfer complete or */
            /* after RESTART condition sent */
            else if (!i2c_communication->rw ||
                     (i2c_communication->rw && i2c_communication->i2c_state == I2C_RESTART_SENT)) {
                I2C1->CR2 |= I2C_CR2_STOP;   // send STOP condition (clears TC flag)
                I2C1->ICR |= I2C_ICR_STOPCF; // clear STOPF flag
                i2c_communication->i2c_state = I2C_IDLE;
                //				if(!i2c_communication->rw){
                DMA_stop();
                //				}
            }
        }
    } else {
        if ((I2C1->ISR & I2C_ISR_TXIS)) {
            I2C1->TXDR = i2c_communication->tx_buf[i2c_communication->tx_byte_idx];
            i2c_communication->tx_byte_idx++;
            if (i2c_communication->tx_byte_idx == 1) {
                i2c_communication->i2c_state = I2C_REGISTER_ADDRESS_SENT;
            } else if (i2c_communication->tx_byte_idx == i2c_communication->num_tx_bytes) {
                i2c_communication->i2c_state = I2C_LAST_BYTE_TRANSFERRED;
            }
        }
        if ((I2C1->ISR & I2C_ISR_RXNE) && i2c_communication->rw &&
            i2c_communication->i2c_state == I2C_RESTART_SENT) {
            i2c_communication->rx_buf[i2c_communication->rx_byte_idx] = I2C1->RXDR;
            i2c_communication->rx_byte_idx++;
            if (i2c_communication->rx_byte_idx == i2c_communication->num_rx_bytes) {
                i2c_communication->i2c_state = I2C_LAST_BYTE_TRANSFERRED;
            }
        }
        if (I2C1->ISR & I2C_ISR_TC) {
            if (i2c_communication->i2c_state == I2C_LAST_BYTE_TRANSFERRED) {
                I2C1->CR2 |= I2C_CR2_STOP;   // send STOP condition (clears TC flag)
                I2C1->ICR |= I2C_ICR_STOPCF; // clear STOPF flag
                i2c_communication->rx_byte_idx = 0;
                i2c_communication->tx_byte_idx = 0;
                i2c_communication->i2c_state   = I2C_IDLE;
            } else if (i2c_communication->rw &&
                       i2c_communication->i2c_state == I2C_REGISTER_ADDRESS_SENT) {
                I2C1->CR2 &= ~(I2C_CR2_NBYTES);
                I2C1->CR2 |=
                    (i2c_communication->num_rx_bytes << I2C_CR2_NBYTES_Pos); // read/write 1 byte
                I2C1->CR2 |= I2C_CR2_RD_WRN;                                 // read mode
                I2C1->CR2 |= I2C_CR2_START; // restart I2C communication
                i2c_communication->i2c_state = I2C_RESTART_SENT;
            }
        }
    }
    //	debug_write(0, 0);
}

// void I2C_event_push(i2c_event_t* event){
//	i2c_event_queue->queue[i2c_event_queue->tail] = event;
//	i2c_event_queue->tail++;
//	if (i2c_event_queue->tail >= I2C_EVENT_QUEUE_LENGTH){
//		i2c_event_queue->tail = 0;
//	}
//	i2c_event_queue->count++;
// }
//
// i2c_event_t* I2C_event_pop(void){
//	i2c_event_t *event;
//	if (i2c_event_queue->count == 0){
//		return (i2c_event_t*)0;
//	}
//	event = i2c_event_queue->queue[i2c_event_queue->head];
//	i2c_event_queue->queue[i2c_event_queue->head] = (i2c_event_t*)0;
//	i2c_event_queue->head++;
//	if (i2c_event_queue->head >= I2C_EVENT_QUEUE_LENGTH){
//		i2c_event_queue->head = 0;
//	}
//	i2c_event_queue->count--;
//	return event;
// }

uint8_t I2C_start(const uint8_t     slave_address,
                  const uint8_t     rw,
                  const uint8_t     num_rx_bytes,
                  const uint8_t     num_tx_bytes,
                  volatile uint8_t *rx_buf,
                  volatile uint8_t *tx_buf,
                  const uint8_t     communication_mode) {
    /* Busy-wait polling or immediate return (interrupt) */
    if (communication_mode == I2C_COMMUNICATION_MODE_POLLING) {
        while (i2c_communication->i2c_state != I2C_IDLE);
        //		i2c_config->delay_ms(1);
    } else {
        if (i2c_communication->i2c_state != I2C_IDLE) {
            return 0;
        }
    }
    while (!(GPIOB->IDR & GPIO_IDR_ID8) || !(GPIOB->IDR & GPIO_IDR_ID9));
    /* Configure settings for current communication */
    i2c_communication->rw           = rw;
    i2c_communication->num_rx_bytes = num_rx_bytes;
    i2c_communication->num_tx_bytes = num_tx_bytes;
    i2c_communication->rx_buf       = rx_buf;
    i2c_communication->tx_buf       = tx_buf;
    i2c_communication->communication_mode =
        (i2c_communication_mode_t)(communication_mode > 2 ? 2 : communication_mode);

    if (i2c_communication->communication_mode != I2C_COMMUNICATION_MODE_DMA) {
        /* Enable I2C interrupt */
        I2C1->CR1 &= ~(I2C_CR1_TXDMAEN | I2C_CR1_RXDMAEN);
        I2C1->CR1 |= (I2C_CR1_TXIE | I2C_CR1_RXIE | I2C_CR1_TCIE);
        i2c_communication->i2c_state = I2C_SLAVE_ADDRESS_SENT;
    } else {
        /* Transmit/receive I2C data using DMA */
        I2C1->CR1 &= ~(I2C_CR1_TXIE | I2C_CR1_RXIE);
        I2C1->CR1 |= (I2C_CR1_TXDMAEN | I2C_CR1_RXDMAEN | I2C_CR1_TCIE);

        /* Configure DMA settings in read and write modes */
        i2c_dmamux_config.request_id              = DMAMUX_REQUEST_ID_I2C1_TX;
        i2c_dma_config->dmamux_config             = &i2c_dmamux_config;
        i2c_dma_config->memory_increment_mode     = 1;
        i2c_dma_config->peripheral_increment_mode = 0;
        i2c_dma_config->circular_mode             = 0;
        i2c_dma_config->transfer_direction        = DMA_TRANSFER_DIRECTION_M2P;
        i2c_dma_config->num_data                  = num_tx_bytes;
        i2c_dma_config->peripheral_address        = (uint32_t)&I2C1->TXDR;
        i2c_dma_config->memory_address            = (uint32_t)tx_buf;
        if (rw) {
            i2c_dma_config->num_data = 1;
        }
        DMA_init(i2c_dma_config);
        DMA_start();
        i2c_communication->i2c_state = I2C_REGISTER_ADDRESS_SENT;
    }
    NVIC_SetPriority(I2C1_IRQn, 2);
    NVIC_EnableIRQ(I2C1_IRQn);

    /* Start communication */
    I2C1->CR2 &= ~I2C_CR2_SADD;
    I2C1->CR2 |= (((slave_address & 0x7F) << 1U) &
                  I2C_CR2_SADD); // only SADD[7:1] used in 7-bit addressing, other bits are ignored
    I2C1->CR2 &= ~I2C_CR2_NBYTES;
    if (!i2c_communication->rw) {
        I2C1->CR2 |= (i2c_communication->num_tx_bytes
                      << I2C_CR2_NBYTES_Pos); // if write mode, configure as num_bytes to send after
                                              // slave address + W sequence
    } else {
        I2C1->CR2 |=
            (1UL << I2C_CR2_NBYTES_Pos); // if read mode, configure as 1 byte to write register
                                         // address to read from after slave address + R sequence
    }
    I2C1->CR2 &= ~I2C_CR2_RD_WRN; // write mode to send slave address + R/W
    //	while(!(GPIOB->IDR & GPIO_IDR_ID8) || !(GPIOB->IDR & GPIO_IDR_ID9)); // wait for I2C bus to
    // be released (both SDA and SCL are HIGH)
    i2c_communication->rx_byte_idx = 0;
    i2c_communication->tx_byte_idx = 0;

    I2C1->CR2 |= I2C_CR2_START; // start I2C communication
    if (communication_mode == I2C_COMMUNICATION_MODE_POLLING) {
        while (i2c_communication->i2c_state != I2C_IDLE);
        //		i2c_config->delay_ms(1);
    }
    return 1;
    //	i2c_config->delay_ms(1);
    //	while(I2C1->ISR & I2C_ISR_BUSY);
}

void I2C_init(i2c_config_t *config) {
    i2c_config = config;
    /* Disable clock-gating */
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;  // enable clock signal to GPIOB
    RCC->CCIPR &= ~RCC_CCIPR_I2C1SEL;   // use PCLK to generate I2CCLK
    RCC->APBENR1 |= RCC_APBENR1_I2C1EN; // enable clock signal to I2C1

    /* Configure SDA/SCL pins on PB9/PB8 */
    GPIOB->MODER &= ~(GPIO_MODER_MODE8 | GPIO_MODER_MODE9);
    GPIOB->MODER |= (GPIO_MODER_MODE8_1 | GPIO_MODER_MODE9_1); // alternate function
    GPIOB->OTYPER |= (GPIO_OTYPER_OT8 | GPIO_OTYPER_OT9);      // open-drain
    GPIOB->OSPEEDR &=
        ~(GPIO_OSPEEDR_OSPEED8 | GPIO_OSPEEDR_OSPEED9); // f_max = 2MHz for C=50pF, 2.7V <= V_DD
                                                        // <= 3.6V (I2C FM f_max is 400kHz)
    GPIOB->PUPDR &=
        ~(GPIO_PUPDR_PUPD8 |
          GPIO_PUPDR_PUPD9); // no pull-up/down resistors, external pull-up resistors used
    GPIOB->AFR[1] &= ~(GPIO_AFRH_AFSEL8 | GPIO_AFRH_AFSEL9);
    GPIOB->AFR[1] |= ((GPIO_AFRH_AFSEL8_2 | GPIO_AFRH_AFSEL8_1) |
                      (GPIO_AFRH_AFSEL9_2 |
                       GPIO_AFRH_AFSEL9_1)); // SCL on PB9, SDA on PB10 using AF6 for both pins

    /* Configure I2C */
    // t_PRESC = t_I2CCLK = 1/(12MHz) = 83.3ns
    // t_SCLDEL = t_PRESC = 83.3ns
    // t_SDADEL = 0 * t_PRESC = 0ns
    // t_SCLH = (14 + 1) * t_PRESC = 1.25us
    // t_SCLL = (14 + 1) * t_PRESC = 1.25us
    I2C1->TIMINGR = 0x10320309UL;
    //	I2C1->TIMINGR = 0x00B03FDBUL;
    //	I2C1->TIMINGR = 0x00000E0EUL;
    I2C1->CR1 &=
        ~(I2C_CR1_ANFOFF | I2C_CR1_DNF); // analog filter on and digital filter off by default
    I2C1->CR1 &= ~I2C_CR1_NOSTRETCH;     // SCL stretching enabled by default
    I2C1->CR1 |= I2C_CR1_PE;             // enable I2C peripheral

    /* Configure I2C communication with ADXL345 */
    I2C1->CR2 &=
        ~I2C_CR2_RELOAD; // transfer complete after NBYTES transferred (STOP or RESTART follows)
    I2C1->CR2 &= ~I2C_CR2_AUTOEND; // software end mode (TC flag set and SCL stretched low when
                                   // NBYTES transferred)
    I2C1->CR2 &= ~I2C_CR2_ADD10;   // 7-bit addressing mode
}
