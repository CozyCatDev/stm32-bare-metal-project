#include "dma.h"

static dma_config_t *dma_config;

void DMA1_Channel1_IRQHandler(void) {
    uint32_t dma_isr = DMA1->ISR;
    if (dma_config->channel_select == DMA_CHANNEL_SELECT_CH1) {
        if (dma_isr & DMA_ISR_TCIF1) {
            if (dma_config->transfer_direction == DMA_TRANSFER_DIRECTION_P2M) {
                DMA_stop();
            }
        }
    }
}

void DMA1_Channel2_3_IRQHandler(void) {
    uint32_t dma_isr = DMA1->ISR;
    if (dma_config->channel_select == DMA_CHANNEL_SELECT_CH2) {
        if (dma_isr & DMA_ISR_TCIF2) {
        }
    } else if (dma_config->channel_select == DMA_CHANNEL_SELECT_CH3) {
        if (dma_isr & DMA_ISR_TCIF3) {
        }
    }
}

void DMA_start(void) {
    dma_config->dma->CCR |= DMA_CCR_EN;
}

void DMA_stop(void) {
    dma_config->dma->CCR &= ~DMA_CCR_EN;
}

void DMA_reconfigure(dma_config_t *config) {
    dma_config = config;
    /* Configure peripheral/memory address and number of read+write data transfers */
    dma_config->dmamux_config->dmamux->CCR &= ~DMAMUX_CxCR_DMAREQ_ID;
    dma_config->dmamux_config->dmamux->CCR |=
        (dma_config->dmamux_config->request_id << DMAMUX_CxCR_DMAREQ_ID_Pos);
    dma_config->dma->CPAR  = (uint32_t)dma_config->peripheral_address;
    dma_config->dma->CMAR  = (uint32_t)dma_config->memory_address;
    dma_config->dma->CNDTR = dma_config->num_data;
    dma_config->dma->CCR &= ~DMA_CCR_DIR;
    dma_config->dma->CCR |= (dma_config->transfer_direction << DMA_CCR_DIR_Pos);
    /* Configure DMA settings */
    //	dma_config->dma->CCR &= ~0xFFFFFFFF;
    //	dma_config->dma->CCR |= (
    //		(dma_config->memory_to_memory_mode ? 1 : 0) << DMA_CCR_MEM2MEM_Pos | // M2M mode
    //(software only) 		(dma_config->channel_priority) << DMA_CCR_PL_Pos | // channel
    // priority (4 levels) 		(dma_config->memory_size) << DMA_CCR_MSIZE_Pos | // memory
    // size (byte/half-word/word) 		(dma_config->peripheral_size) << DMA_CCR_PSIZE_Pos |
    // // peripheral
    // size (byte/half-word/word) 		(dma_config->memory_increment_mode ? 1 : 0) <<
    // DMA_CCR_MINC_Pos | // memory increment mode (increment internal pointer address to get next
    // data) 		(dma_config->peripheral_increment_mode ? 1 : 0) << DMA_CCR_PINC_Pos | //
    // peripheral increment mode (increment internal pointer address to get next data)
    //		(dma_config->circular_mode ? 1 : 0) << DMA_CCR_CIRC_Pos | // circular mode (reload
    // with CNDTR data transfers and reset pointer address to CMAR/CPAR)
    //		(dma_config->transfer_direction) << DMA_CCR_DIR_Pos // M2P or P2M
    //	);
}

void DMA_init(dma_config_t *config) {
    dma_config = config;

    /* Disable clock-gating to DMA */
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;

    /* Configure DMA registers to use based on DMA channel selected */
    switch (dma_config->channel_select) {
    case DMA_CHANNEL_SELECT_CH1:
        dma_config->dma = DMA1_Channel1;
        break;
    case DMA_CHANNEL_SELECT_CH2:
        dma_config->dma = DMA1_Channel2;
        break;
    case DMA_CHANNEL_SELECT_CH3:
        dma_config->dma = DMA1_Channel3;
        break;
    }

    /* Configure DMAMUX registers to use based on DMAMUX channel selected */
    switch (dma_config->dmamux_config->channel_select) {
    case DMAMUX_CHANNEL_SELECT_CH0:
        dma_config->dmamux_config->dmamux = DMAMUX1_Channel0;
        break;
    case DMAMUX_CHANNEL_SELECT_CH1:
        dma_config->dmamux_config->dmamux = DMAMUX1_Channel1;
        break;
    case DMAMUX_CHANNEL_SELECT_CH2:
        dma_config->dmamux_config->dmamux = DMAMUX1_Channel2;
        break;
    }

    /* Configure DMAMUX request ID from peripheral, e.g. I2C */
    dma_config->dmamux_config->dmamux->CCR &= ~DMAMUX_CxCR_DMAREQ_ID;
    dma_config->dmamux_config->dmamux->CCR |=
        (dma_config->dmamux_config->request_id << DMAMUX_CxCR_DMAREQ_ID_Pos);

    /* Configure peripheral/memory address and number of read+write data transfers */
    dma_config->dma->CPAR  = (uint32_t)dma_config->peripheral_address;
    dma_config->dma->CMAR  = (uint32_t)dma_config->memory_address;
    dma_config->dma->CNDTR = dma_config->num_data;

    /* Configure DMA settings */
    dma_config->dma->CCR &= ~0xFFFFFFFF;
    dma_config->dma->CCR |=
        ((dma_config->memory_to_memory_mode ? 1 : 0)
             << DMA_CCR_MEM2MEM_Pos |                       // M2M mode (software only)
         (dma_config->channel_priority) << DMA_CCR_PL_Pos | // channel priority (4 levels)
         (dma_config->memory_size) << DMA_CCR_MSIZE_Pos |   // memory size (byte/half-word/word)
         (dma_config->peripheral_size)
             << DMA_CCR_PSIZE_Pos | // peripheral size (byte/half-word/word)
         (dma_config->memory_increment_mode ? 1 : 0)
             << DMA_CCR_MINC_Pos | // memory increment mode (increment internal pointer address to
                                   // get next data)
         (dma_config->peripheral_increment_mode ? 1 : 0)
             << DMA_CCR_PINC_Pos | // peripheral increment mode (increment internal pointer address
                                   // to get next data)
         (dma_config->circular_mode ? 1 : 0)
             << DMA_CCR_CIRC_Pos | // circular mode (reload with CNDTR data transfers and reset
                                   // pointer address to CMAR/CPAR)
         (dma_config->transfer_direction) << DMA_CCR_DIR_Pos // M2P or P2M
        );

    //	/* Configure DMA interrupts */
    //	dma_config->dma->CCR |= DMA_CCR_TCIE; // transfer complete interrupt enable
    //
    //	if(dma_config->channel_select == DMA_CHANNEL_SELECT_CH1){
    //		NVIC_SetPriority(DMA1_Channel1_IRQn, 0);
    //		NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    //	}
    //	else if(dma_config->channel_select == DMA_CHANNEL_SELECT_CH2 || dma_config->channel_select
    //== DMA_CHANNEL_SELECT_CH3){ 		NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0);
    //		NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
    //	}
}
