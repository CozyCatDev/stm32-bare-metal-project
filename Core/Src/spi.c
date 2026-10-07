#include "spi.h"

static spi_config_t *spi_config;

typedef enum {
    SPI_IDLE,
    SPI_FIRST_BYTE_SENT, // usually register address + R/W + multi-byte setting
    SPI_LAST_BYTE_TRANSFERRED
} spi_state_t;

typedef struct {
    uint8_t              rw;
    uint8_t              num_rx_bytes;
    uint8_t              num_tx_bytes;
    volatile uint8_t     tx_byte_idx;
    volatile uint8_t     rx_byte_idx;
    volatile spi_state_t spi_state;
    volatile uint8_t    *rx_buf;
    volatile uint8_t    *tx_buf;
} spi_communication_t;

static spi_communication_t  my_spi_communication;
static spi_communication_t *spi_communication = &my_spi_communication;

// static uint8_t spi_timeout_counter = 0;

void SPI1_IRQHandler(void) {
    //	volatile uint32_t spi_sr = SPI1->SR;
    if ((!(SPI1->SR & SPI_SR_BSY) &&
         (spi_communication->tx_byte_idx == spi_communication->num_tx_bytes) &&
         (spi_communication->rx_byte_idx == spi_communication->num_rx_bytes))) {
        SPI1->CR1 &= ~SPI_CR1_SPE; // disable SPI peripheral
        SPI1->CR2 &=
            ~(SPI_CR2_TXEIE | SPI_CR2_RXNEIE); // disable interrupts after communication is complete
        //		GPIOA->BSRR = GPIO_BSRR_BS4; // output HIGH on NSS pin
        spi_communication->tx_byte_idx = 0;
        spi_communication->rx_byte_idx = 0;
        //		spi_timeout_counter = 0;
    } else {
        if ((SPI1->SR & SPI_SR_TXE) &&
            (spi_communication->tx_byte_idx < spi_communication->num_tx_bytes)) {
            *((volatile uint8_t *)(&SPI1->DR)) =
                spi_communication->tx_buf[spi_communication->tx_byte_idx];
            spi_communication->tx_byte_idx++;
        }
        if ((SPI1->SR & SPI_SR_RXNE) &&
            (spi_communication->rx_byte_idx < spi_communication->num_rx_bytes)) {
            //			if(spi_communication->rx_byte_idx > 0){
            spi_communication->rx_buf[spi_communication->rx_byte_idx] =
                *((volatile uint8_t *)(&SPI1->DR));
            //			}
            //			else{
            //				(void)*((volatile uint8_t*)(&SPI1->DR));
            //			}
            spi_communication->rx_byte_idx++;
            //		if(!(spi_sr & SPI_SR_FRLVL)){
            //			GPIOA->BSRR = GPIO_BSRR_BS4; // output HIGH on NSS pin
            //		}
        }
    }
    //	spi_timeout_counter++;
    //	if(!(spi_sr & SPI_SR_FTLVL)){
    //		if(!(spi_sr & SPI_SR_BSY)){
    //			SPI1->CR1 &= ~SPI_CR1_SPE; // disable SPI peripheral
    //		}
    //	}
    //	else if(!(spi_sr & SPI_SR_FRLVL)){
    //
    //	}
}

uint8_t SPI_start(const uint8_t     rw,
                  const uint8_t     num_rx_bytes,
                  const uint8_t     num_tx_bytes,
                  volatile uint8_t *rx_buf,
                  volatile uint8_t *tx_buf,
                  const uint8_t     communication_mode) {
    spi_communication->rw           = rw;
    spi_communication->num_rx_bytes = num_rx_bytes;
    spi_communication->num_tx_bytes = num_tx_bytes;
    spi_communication->rx_buf       = rx_buf;
    spi_communication->tx_buf       = tx_buf;
    //	GPIOA->BRR = GPIO_BRR_BR4; // output LOW on NSS pin
    spi_communication->tx_byte_idx = 0;
    spi_communication->rx_byte_idx = 0;
    SPI1->CR2 |= (SPI_CR2_TXEIE | SPI_CR2_RXNEIE);
    SPI1->CR1 |= SPI_CR1_SPE; // enable SPI peripheral
    spi_config->delay_us(1000);
    return 1;
}

void SPI_init(spi_config_t *config) {
    spi_config = config;
    /* Disable clock-gating */
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;  // enable clock signal to GPIOB
    RCC->APBENR2 |= RCC_APBENR2_SPI1EN; // enable clock signal to I2C1

    /* Configure SCK/MOSI/NSS/MISO pins on PA1/PA2/PA4/PA6 */
    GPIOA->MODER &= ~(GPIO_MODER_MODE1 | GPIO_MODER_MODE2 | GPIO_MODER_MODE4 | GPIO_MODER_MODE6);
    GPIOA->MODER |= (GPIO_MODER_MODE1_1 | GPIO_MODER_MODE2_1 | GPIO_MODER_MODE4_1 |
                     GPIO_MODER_MODE6_1); // alternate function
    GPIOA->OTYPER &=
        ~(GPIO_OTYPER_OT1 | GPIO_OTYPER_OT2 | GPIO_OTYPER_OT4 | GPIO_OTYPER_OT6); // push-pull
    GPIOA->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED1 | GPIO_OSPEEDR_OSPEED2 | GPIO_OSPEEDR_OSPEED4 |
                        GPIO_OSPEEDR_OSPEED6);
    //	GPIOA->OSPEEDR |= (GPIO_OSPEEDR_OSPEED1_0 | GPIO_OSPEEDR_OSPEED2_0 | GPIO_OSPEEDR_OSPEED4_0
    //| GPIO_OSPEEDR_OSPEED6_0); // f_max = 10MHz for C=50pF, 2.7V <= V_DD <= 3.6V (SPI f_max is
    // f_PCLK / 2)
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD1 | GPIO_PUPDR_PUPD2 | GPIO_PUPDR_PUPD4 |
                      GPIO_PUPDR_PUPD6); // no pull-up/down resistors
    GPIOA->AFR[0] &=
        ~(GPIO_AFRL_AFSEL1 | GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL4 |
          GPIO_AFRL_AFSEL6); // SCK/MOSI/NSS/MISO pins on PA1/PA2/PA4/P6
                             //	GPIOA->BSRR = GPIO_BSRR_BS4; // output HIGH on NSS pin

    /* Configure SPI settings (CR1) */
    SPI1->CR1 &= ~SPI_CR1_BR;
    SPI1->CR1 |= (SPI_CR1_BR_2);    // f_PCLK / 16 = 750kHz
    SPI1->CR1 |= SPI_CR1_CPOL;      // idle SCLK state = 1
    SPI1->CR1 |= SPI_CR1_CPHA;      // sample on second clock edge (rising edge in this case)
    SPI1->CR1 &= ~SPI_CR1_BIDIMODE; // full-duplex
    SPI1->CR1 &= ~SPI_CR1_SSM;      // hardware NSS management
    SPI1->CR1 &= ~SPI_CR1_LSBFIRST; // MSB transmitted/received first
    SPI1->CR1 |= SPI_CR1_MSTR;      // master mode

    /* Configure SPI communication settings (CR2) */
    SPI1->CR2 &= ~SPI_CR2_DS;
    SPI1->CR2 |= (SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0); // 8-bit data size
    SPI1->CR2 |= SPI_CR2_SSOE;
    SPI1->CR2 |= SPI_CR2_FRXTH; // generate RXNE event when RXFIFO reaches 1/4 capacity (8 bits)

    /* Configure SPI TXE and RXNE interrupts */
    SPI1->CR2 |= (SPI_CR2_TXEIE | SPI_CR2_RXNEIE);
    NVIC_SetPriority(SPI1_IRQn, 1);
    NVIC_EnableIRQ(SPI1_IRQn);

    /* Initial SPI state */
    spi_communication->spi_state = SPI_IDLE;
}
