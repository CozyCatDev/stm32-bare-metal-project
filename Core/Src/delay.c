#include "delay.h"

// static volatile uint32_t delay_tick = 0UL;

// void SysTick_Handler(void){
//	delay_tick++;
// }

// void delay_timer_init(void)
//{
//	SysTick->LOAD  = 0xFFFFFFFFUL;                         /* set reload register */
////	NVIC_SetPriority (SysTick_IRQn, 3UL); /* set Priority for Systick Interrupt */
//	SysTick->VAL   = 0UL;                                             /* Load the SysTick
// Counter Value */ 	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; /*
// Enable SysTick IRQ and SysTick Timer */
////	NVIC_EnableIRQ(SysTick_IRQn);
//}
//
// void delay_us(uint32_t us)
//{
//	uint32_t ticks = (SYSTEM_CLOCK / 1000000UL) * us;
//	uint32_t start = SysTick->VAL;
//
//	/* Add a freq to guarantee minimum wait */
//	//  if (wait < HAL_MAX_DELAY)
//	//  {
//	//    wait += (uint32_t)(uwTickFreq);
//	//  }
//
//	while ((uint32_t)(start - SysTick->VAL) < ticks)
//	{
//	}
//}
//
// void delay_ms(uint32_t ms){
//	delay_us(ms * 1000U);
//}

void delay_timer_init(void) {
    /* Enable clock signal to TIM1 */
    RCC->APBENR2 |= RCC_APBENR2_TIM1EN;

    /* Configure TIM1 */
    TIM1->CR1 &= ~TIM_CR1_CMS; // edge-aligned mode
    TIM1->CR1 &= ~TIM_CR1_DIR; // upcounter
    TIM1->PSC = TIM1_PSC - 1;  // 1MHz timer
    TIM1->ARR = 0xFFFF;
    TIM1->EGR |= TIM_EGR_UG; // generate update event (UEV) to transfer preload register values to
                             // shadow registers

    /* Enable TIM1 */
    TIM1->CR1 |= TIM_CR1_CEN;
}

void delay_us(uint32_t us) {
    while (us) {
        uint16_t delay = us > 65535U ? 65535U : (uint16_t)us;
        uint16_t start = TIM1->CNT;
        // e.g. start = 50000, us = 30000
        //     +15535     +1  +14464      = 30000us
        // 50000 -> 65535 -> 0 -> 14464
        // So, TIM1->CNT - start = 14464 - 50000 = -35536
        // Decreasing by 35536 by 65536 is the same as increasing by 30000 from 0 (65536), i.e.
        // wrap-around -35536 is outside range of int16_t, so it's first casted to int32_t (w/ sign
        // extension) -35536 = 0xFFFF7530 (int32_t) Then casted to uint16_t (upper 16 bits
        // discarded) 0x7530 = 30000 (MSB is not sign bit as it's uint16_t) Same result can be
        // obtained with -35536 + 65536 = 30000
        while ((uint16_t)(TIM1->CNT - start) < delay);
        us -= delay;
    }
}

void delay_ms(uint32_t ms) {
    while (ms--) {
        delay_us(1000);
    }
}
