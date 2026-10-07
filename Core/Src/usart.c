#include "usart.h"
#include "debug.h"

static usart_config_t *usart_config;

typedef enum { USART_IDLE, USART_BUSY } usart_state_t;

typedef enum {
    USART_COMMUNICATION_MODE_POLLING,
    USART_COMMUNICATION_MODE_INTERRUPT,
    USART_COMMUNICATION_MODE_DMA
} usart_communication_mode_t;

typedef struct {
    uint8_t                    num_rx;
    uint8_t                    num_tx;
    volatile uint8_t           rx_idx;
    volatile uint8_t           tx_idx;
    volatile usart_state_t     rx_state;
    volatile usart_state_t     tx_state;
    volatile uint16_t         *rx_buf;
    volatile uint16_t         *tx_buf;
    usart_communication_mode_t communication_mode;
} usart_communication_t;

static dmamux_config_t usart_rx_dmamux_config = {.channel_select = DMAMUX_CHANNEL_SELECT_CH0,
                                                 .request_id     = DMAMUX_REQUEST_ID_USART2_RX};

static dmamux_config_t usart_tx_dmamux_config = {.channel_select = DMAMUX_CHANNEL_SELECT_CH1,
                                                 .request_id     = DMAMUX_REQUEST_ID_USART2_TX};

static dma_config_t my_usart_rx_dma_config = {.channel_select        = DMA_CHANNEL_SELECT_CH1,
                                              .memory_to_memory_mode = 0,
                                              .channel_priority = DMA_CHANNEL_PRIORITY_VERY_HIGH,
                                              .memory_size      = DMA_MEMORY_SIZE_16BIT,
                                              .peripheral_size  = DMA_PERIPHERAL_SIZE_32BIT,
                                              .memory_increment_mode     = 1,
                                              .peripheral_increment_mode = 0,
                                              .circular_mode             = 0,
                                              .transfer_direction = DMA_TRANSFER_DIRECTION_P2M};

static dma_config_t my_usart_tx_dma_config = {.channel_select        = DMA_CHANNEL_SELECT_CH2,
                                              .memory_to_memory_mode = 0,
                                              .channel_priority = DMA_CHANNEL_PRIORITY_VERY_HIGH,
                                              .memory_size      = DMA_MEMORY_SIZE_16BIT,
                                              .peripheral_size  = DMA_PERIPHERAL_SIZE_32BIT,
                                              .memory_increment_mode     = 1,
                                              .peripheral_increment_mode = 0,
                                              .circular_mode             = 0,
                                              .transfer_direction = DMA_TRANSFER_DIRECTION_M2P};

static dma_config_t *usart_rx_dma_config = &my_usart_rx_dma_config;
static dma_config_t *usart_tx_dma_config = &my_usart_tx_dma_config;

static usart_communication_t  my_usart_communication;
static usart_communication_t *usart_communication = &my_usart_communication;

void USART2_IRQHandler(void) {
    //	uint32_t usart_isr = USART2->ISR;
    //	debug_write(0, (USART2->ISR & USART_ISR_TXE_TXFNF) ? 1 : 0);
    //	debug_write(0, 1);
    if (usart_communication->communication_mode == USART_COMMUNICATION_MODE_INTERRUPT) {
        if (USART2->ISR & USART_ISR_ORE) {
            USART2->ICR |= USART_ICR_ORECF;
        }
        /* USART2 transmission */
        if ((USART2->ISR & USART_ISR_TXE_TXFNF) && (USART2->CR1 & USART_CR1_TXEIE_TXFNFIE) &&
            (usart_communication->tx_idx < usart_communication->num_tx)) {
            USART2->TDR = usart_communication->tx_buf[usart_communication->tx_idx++];
            if (usart_communication->tx_idx == usart_communication->num_tx) {
                USART2->CR1 &= ~USART_CR1_TXEIE_TXFNFIE; // disable TXEIE/TXFNFIE interrupt
                usart_communication->tx_idx   = 0;
                usart_communication->tx_state = USART_IDLE;
            }
        }
        /* USART2 reception */
        if (USART2->ISR & USART_ISR_RXNE_RXFNE) {
            uint32_t data = USART2->RDR;
            if (usart_communication->rx_idx < usart_communication->num_rx) {
                usart_communication->rx_buf[usart_communication->rx_idx++] = data;
                if (usart_communication->rx_idx == usart_communication->num_rx) {
                    //				USART2->CR1 &= ~USART_CR1_RXNEIE_RXFNEIE; // disable
                    // RXNEIE/RXFNEIE interrupt
                    usart_communication->rx_idx   = 0;
                    usart_communication->rx_state = USART_IDLE;
                }
            }
        }
        /* Detected stop command */
        if (USART2->ISR & USART_ISR_CMF) {
            USART2->ICR |= USART_ICR_CMCF;
            usart_communication->rx_idx   = 0;
            usart_communication->rx_state = USART_IDLE;
        }
    } else if (usart_communication->communication_mode == USART_COMMUNICATION_MODE_DMA) {
        if (USART2->ISR & USART_ISR_ORE) {
            USART2->ICR |= USART_ICR_ORECF;
        }
        /* USART2 transmission */
        if (USART2->ISR & USART_ISR_TC) {
            USART2->ICR |= USART_ICR_TCCF;
            DMA_stop();
            usart_communication->tx_state = USART_IDLE;
        }
        /* Detected stop command */
        if (USART2->ISR & USART_ISR_CMF) {
            USART2->ICR |= USART_ICR_CMCF;
            DMA_stop();
            usart_communication->rx_state = USART_IDLE;
        }
    }
    //	/* USART2 transfer complete */
    //	if(USART2->ISR & USART_ISR_TC){
    //		USART2->ICR |= USART_ICR_TCCF; // clear transfer complete flag
    ////		USART2->CR1 &= ~USART_CR1_TCIE; // disable transfer complete interrupt
    //		if(usart_communication->tx_idx == usart_communication->num_tx){
    ////			USART2->CR1 &= ~USART_CR1_TXEIE_TXFNFIE; // disable TXEIE/TXFNFIE
    /// interrupt
    //			usart_communication->tx_idx = 0;
    //		}
    //	}
    //	debug_write(0, 0);
    //	debug_write(0, 0);
}

uint8_t
USART_read(const uint8_t num_rx, volatile uint16_t *rx_buf, const uint8_t communication_mode) {
    if (communication_mode == USART_COMMUNICATION_MODE_POLLING) {
        while (usart_communication->rx_state != USART_IDLE);
    } else {
        if (usart_communication->rx_state != USART_IDLE) {
            return 0;
        }
    }
    /* Initialize USART2 communication object */
    usart_communication->rx_state = USART_BUSY;
    usart_communication->num_rx   = num_rx;
    usart_communication->rx_buf   = rx_buf;
    usart_communication->rx_idx   = 0;
    usart_communication->communication_mode =
        (usart_communication_mode_t)(communication_mode > 2 ? 2 : communication_mode);
    if (communication_mode == USART_COMMUNICATION_MODE_INTERRUPT) {
        /* Enable USART2 interrupt flags */
        USART2->CR1 |= (USART_CR1_RXNEIE_RXFNEIE | USART_CR1_RE | USART_CR1_CMIE);
        USART2->CR2 &= ~(USART_CR2_ADD);
        USART2->CR2 |= (usart_config->character_match << USART_CR2_ADD_Pos);
    } else if (communication_mode == USART_COMMUNICATION_MODE_DMA) {
        USART2->CR1 &= ~USART_CR1_RXNEIE_RXFNEIE;
        USART2->CR1 |= (USART_CR1_CMIE | USART_CR1_RE);
        USART2->CR2 &= ~(USART_CR2_ADD);
        USART2->CR2 |= (usart_config->character_match << USART_CR2_ADD_Pos);
        USART2->CR3 |= USART_CR3_DMAR;
        usart_rx_dma_config->num_data           = num_rx;
        usart_rx_dma_config->memory_address     = (uint32_t)rx_buf;
        usart_rx_dma_config->peripheral_address = (uint32_t)&USART2->RDR;
        usart_rx_dma_config->dmamux_config      = &usart_rx_dmamux_config;
        DMA_init(usart_rx_dma_config);
        DMA_start();
    }
    USART2->CR1 |= USART_CR1_UE;
    if (communication_mode == USART_COMMUNICATION_MODE_POLLING) {
        while (usart_communication->rx_state != USART_IDLE);
    }
    return 1;
}

uint8_t
USART_write(const uint8_t num_tx, volatile uint16_t *tx_buf, const uint8_t communication_mode) {
    if (communication_mode == USART_COMMUNICATION_MODE_POLLING) {
        while (usart_communication->tx_state != USART_IDLE);
    } else {
        if (usart_communication->tx_state != USART_IDLE) {
            return 0;
        }
    }
    /* Initialize USART2 communication object */
    usart_communication->tx_state = USART_BUSY;
    usart_communication->num_tx   = num_tx;
    usart_communication->tx_buf   = tx_buf;
    usart_communication->tx_idx   = 0;
    usart_communication->communication_mode =
        (usart_communication_mode_t)(communication_mode > 2 ? 2 : communication_mode);
    if (communication_mode == USART_COMMUNICATION_MODE_INTERRUPT) {
        /* Enable USART2 interrupt flags */
        USART2->CR1 |= (USART_CR1_TXEIE_TXFNFIE | USART_CR1_TE);
    } else if (communication_mode == USART_COMMUNICATION_MODE_DMA) {
        USART2->CR1 &= ~USART_CR1_TXEIE_TXFNFIE;
        USART2->CR1 |= (USART_CR1_TCIE | USART_CR1_TE);
        USART2->CR3 |= USART_CR3_DMAT;
        usart_tx_dma_config->num_data           = num_tx;
        usart_tx_dma_config->memory_address     = (uint32_t)tx_buf;
        usart_tx_dma_config->peripheral_address = (uint32_t)&USART2->TDR;
        usart_tx_dma_config->dmamux_config      = &usart_tx_dmamux_config;
        DMA_init(usart_tx_dma_config);
        DMA_start();
    }
    USART2->CR1 |= USART_CR1_UE;
    if (communication_mode == USART_COMMUNICATION_MODE_POLLING) {
        while (usart_communication->tx_state != USART_IDLE);
    }
    return 1;
}

// uint8_t USART_start(
//	const uint8_t num_rx,
//	const uint8_t num_tx,
//	volatile uint16_t* rx_buf,
//	volatile uint16_t* tx_buf,
//	const uint8_t communication_mode
//){
//	if(usart_communication->usart_state != USART_IDLE){
//		return 0;
//	}
//	/* Initialize USART2 communication object */
//	usart_communication->num_rx = num_rx;
//	usart_communication->num_tx = num_tx;
//	usart_communication->rx_buf = rx_buf;
//	usart_communication->tx_buf = tx_buf;
//	usart_communication->rx_idx = 0;
//	usart_communication->tx_idx = 0;
//	usart_communication->communication_mode = (usart_communication_mode_t)(communication_mode >
// 2 ? 2 : communication_mode); 	if(communication_mode ==
// USART_COMMUNICATION_MODE_INTERRUPT){
//		/* Enable USART2 interrupt flags */
//		if(usart_communication->num_rx > 0){
//			USART2->CR1 |= (USART_CR1_RXNEIE_RXFNEIE | USART_CR1_RE | USART_CR1_CMIE);
//			USART2->CR2 &= ~(USART_CR2_ADD);
//			USART2->CR2 |= (usart_config->character_match << USART_CR2_ADD_Pos);
//		}
//		if(usart_communication->num_tx > 0){
//			USART2->CR1 |= (USART_CR1_TXEIE_TXFNFIE | USART_CR1_TE);
//		}
////		USART2->CR1 |= USART_CR1_TCIE;
//	}
//	else if(communication_mode == USART_COMMUNICATION_MODE_DMA){
//		if(usart_communication->num_rx > 0){
//			USART2->CR1 &= ~USART_CR1_RXNEIE_RXFNEIE;
//			USART2->CR1 |= (USART_CR1_CMIE | USART_CR1_RE);
//			USART2->CR2 &= ~(USART_CR2_ADD);
//			USART2->CR2 |= (usart_config->character_match << USART_CR2_ADD_Pos);
//			USART2->CR3 |= USART_CR3_DMAR;
//			usart_rx_dma_config->num_data = num_rx;
//			usart_rx_dma_config->memory_address = (uint32_t)rx_buf;
//			usart_rx_dma_config->peripheral_address = (uint32_t)&USART2->RDR;
//			usart_rx_dma_config->dmamux_config = &usart_rx_dmamux_config;
//			DMA_init(usart_rx_dma_config);
//		}
//		if(usart_communication->num_tx > 0){
//			USART2->CR1 &= ~USART_CR1_TXEIE_TXFNFIE;
//			USART2->CR1 |= (USART_CR1_TCIE | USART_CR1_TE);
//			USART2->CR3 |= USART_CR3_DMAT;
//			usart_tx_dma_config->num_data = num_tx;
//			usart_tx_dma_config->memory_address = (uint32_t)tx_buf;
//			usart_tx_dma_config->peripheral_address = (uint32_t)&USART2->TDR;
//			usart_tx_dma_config->dmamux_config = &usart_tx_dmamux_config;
//			DMA_init(usart_tx_dma_config);
//		}
//		DMA_start();
//	}
//	usart_communication->usart_state = USART_BUSY;
//	USART2->CR1 |= USART_CR1_UE;
//	NVIC_SetPriority(USART2_IRQn, 1);
//	NVIC_EnableIRQ(USART2_IRQn);
//	return 1;
//}

void USART_init(usart_config_t *config) {
    /* Initialize USART object */
    usart_config = config;

    /* Disable clock-gating to USART2 and USART2 GPIO port */
    RCC->APBENR1 |= RCC_APBENR1_USART2EN;
    switch ((uint32_t)USART2_GPIO_PORT) {
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

    /* GPIO configuration */
    USART2_GPIO_PORT->MODER &=
        ~((0x3UL << (USART2_TX_GPIO_PIN * 2U)) | (0x3UL << (USART2_RX_GPIO_PIN * 2U)));
    USART2_GPIO_PORT->MODER |= ((0x2UL << (USART2_TX_GPIO_PIN * 2U)) |
                                (0x2UL << (USART2_RX_GPIO_PIN * 2U))); // alternate function
    USART2_GPIO_PORT->OTYPER &=
        ~((0x1UL << USART2_TX_GPIO_PIN) | (0x1UL << USART2_RX_GPIO_PIN)); // push-pull
    USART2_GPIO_PORT->OSPEEDR &= ~((0x3UL << (USART2_TX_GPIO_PIN * 2U)) |
                                   (0x3UL << (USART2_RX_GPIO_PIN * 2U))); // very low speed
    USART2_GPIO_PORT->PUPDR &= ~((0x3UL << (USART2_TX_GPIO_PIN * 2U)) |
                                 (0x3UL << (USART2_RX_GPIO_PIN * 2U))); // no pull-up/down resistors
    USART2_GPIO_PORT->AFR[0] &=
        ~((0xFUL << (USART2_TX_GPIO_PIN * 4U)) | (0xFUL << (USART2_RX_GPIO_PIN * 4U)));
    USART2_GPIO_PORT->AFR[0] |= ((0x1UL << (USART2_TX_GPIO_PIN * 4U)) |
                                 (0x1UL << (USART2_RX_GPIO_PIN * 4U))); // set TX and RX pins

    /* FIFO mode and parity bit settings */
    USART2->CR1 &= ~0xFFFFFFFFUL;
    USART2->CR1 |=
        (usart_config->fifo_mode << USART_CR1_FIFOEN_Pos |          // FIFO enable
         usart_config->parity_control_enable << USART_CR1_PCE_Pos | // parity control enable
         usart_config->parity_selection << USART_CR1_PS_Pos         // even or odd parity
        );

    /* Word length (inclusive of parity bit) */
    switch (usart_config->word_length) {
    case USART_WORD_LENGTH_8BIT:
        break; // 7 data bits + parity bit
    case USART_WORD_LENGTH_9BIT:
        USART2->CR1 |= USART_CR1_M0; // 8 data bits + parity bit
        break;
    case USART_WORD_LENGTH_7BIT:
        USART2->CR1 |= USART_CR1_M1; // 9 data bits + parity bit
        break;
    }

    /* Oversampling (16x or 8x at bit center) and baud rate settings */
    switch (usart_config->oversampling_mode) {
    case USART_OVERSAMPLING_MODE_16X: // 16x oversampling at bit center
        USART2->CR1 &= ~USART_CR1_OVER8;
        USART2->BRR = usart_config->baud_rate;
        break;
    case USART_OVERSAMPLING_MODE_8X: // 8x oversampling at bit center
        USART2->CR1 |= USART_CR1_OVER8;
        USART2->BRR =
            ((usart_config->baud_rate & 0xFFF0) | ((usart_config->baud_rate & 0xF) >> 1U));
        break;
    }

    /* Data transmission settings */
    USART2->CR2 &= ~0xFFFFFFFFUL;
    USART2->CR2 |=
        (usart_config->stop_bits << USART_CR2_STOP_Pos |      // number of stop bits
         usart_config->msb_first << USART_CR2_MSBFIRST_Pos |  // LSB/MSB first
         usart_config->clock_polarity << USART_CR2_CPOL_Pos | // clock idle state
         usart_config->clock_phase << USART_CR2_CPHA_Pos |    // sample on first/second clock edge
         usart_config->last_bit_clock_pulse << USART_CR2_LBCL_Pos // send clock pulse on last bit
        );

    /* FIFO threshold settings to generate interrupt */
    if (usart_config->fifo_mode) {
        USART2->CR3 &= ~0xFFFFFFFFUL;
        USART2->CR3 |= (usart_config->txfifo_threshold << USART_CR3_TXFTCFG_Pos |
                        usart_config->rxfifo_threshold << USART_CR3_RXFTCFG_Pos);
    }

    NVIC_SetPriority(USART2_IRQn, 1);
    NVIC_EnableIRQ(USART2_IRQn);
}
