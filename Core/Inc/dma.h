#ifndef DMA_H
#define DMA_H

#include "stm32c031xx.h"
#include <stdint.h>

typedef enum {
    DMA_CHANNEL_SELECT_CH1 = 1,
    DMA_CHANNEL_SELECT_CH2 = 2,
    DMA_CHANNEL_SELECT_CH3 = 3
} dma_channel_select_t;

typedef enum {
    DMA_CHANNEL_PRIORITY_LOW,
    DMA_CHANNEL_PRIORITY_MEDIUM,
    DMA_CHANNEL_PRIORITY_HIGH,
    DMA_CHANNEL_PRIORITY_VERY_HIGH
} dma_channel_priority_t;

typedef enum {
    DMA_MEMORY_SIZE_8BIT,
    DMA_MEMORY_SIZE_16BIT,
    DMA_MEMORY_SIZE_32BIT
} dma_memory_size_t;

typedef enum {
    DMA_PERIPHERAL_SIZE_8BIT,
    DMA_PERIPHERAL_SIZE_16BIT,
    DMA_PERIPHERAL_SIZE_32BIT
} dma_peripheral_size_t;

typedef enum { DMA_TRANSFER_DIRECTION_P2M, DMA_TRANSFER_DIRECTION_M2P } dma_transfer_direction_t;

typedef enum {
    DMAMUX_CHANNEL_SELECT_CH0 = 0,
    DMAMUX_CHANNEL_SELECT_CH1 = 1,
    DMAMUX_CHANNEL_SELECT_CH2 = 2
} dmamux_channel_select_t;

typedef enum {
    DMAMUX_REQUEST_ID_DMAMUX_GEN0 = 1,
    DMAMUX_REQUEST_ID_DMAMUX_GEN1 = 2,
    DMAMUX_REQUEST_ID_DMAMUX_GEN2 = 3,
    DMAMUX_REQUEST_ID_DMAMUX_GEN3 = 4,
    DMAMUX_REQUEST_ID_ADC1        = 5,
    DMAMUX_REQUEST_ID_I2C1_RX     = 10,
    DMAMUX_REQUEST_ID_I2C1_TX     = 11,
    DMAMUX_REQUEST_ID_SPI2S1_RX   = 16,
    DMAMUX_REQUEST_ID_SPI2S1_TX   = 17,
    DMAMUX_REQUEST_ID_USART1_RX   = 50,
    DMAMUX_REQUEST_ID_USART1_TX   = 51,
    DMAMUX_REQUEST_ID_USART2_RX   = 52,
    DMAMUX_REQUEST_ID_USART2_TX   = 53
} dmamux_request_id_t;

typedef struct {
    DMAMUX_Channel_TypeDef *dmamux;
    dmamux_channel_select_t channel_select;
    dmamux_request_id_t     request_id;
} dmamux_config_t;

typedef struct {
    DMA_Channel_TypeDef     *dma;
    dma_channel_select_t     channel_select;
    uint8_t                  memory_to_memory_mode;
    dma_channel_priority_t   channel_priority;
    dma_memory_size_t        memory_size;
    dma_peripheral_size_t    peripheral_size;
    uint8_t                  memory_increment_mode;
    uint8_t                  peripheral_increment_mode;
    uint8_t                  circular_mode;
    dma_transfer_direction_t transfer_direction;
    uint16_t                 num_data;
    uint32_t                 peripheral_address;
    uint32_t                 memory_address;
    dmamux_config_t         *dmamux_config;
} dma_config_t;

void DMA_start(void);
void DMA_stop(void);
void DMA_reconfigure(dma_config_t *config);
void DMA_init(dma_config_t *dma_config);

#endif
