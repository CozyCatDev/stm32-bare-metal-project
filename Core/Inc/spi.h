#ifndef SPI_H
#define SPI_H

#include "stm32c031xx.h"

typedef enum { FULL_DUPLEX, HALF_DUPLEX, RXONLY, TXONLY } spi_communication_format_t;

typedef struct {
    /* User-defined delay function */
    void (*delay_us)(uint32_t us);
    spi_communication_format_t spi_communication_format;
} spi_config_t;

void    SPI_init(spi_config_t *spi_config);
uint8_t SPI_start(const uint8_t     rw,
                  const uint8_t     num_rx_bytes,
                  const uint8_t     num_tx_bytes,
                  volatile uint8_t *rx_buf,
                  volatile uint8_t *tx_buf,
                  const uint8_t     communication_mode);

#endif
