/*
 * stm32f407vgt6-lpm.c
 *
 *  Created on: 09.08.2012
 *      Author: pfeiffer
 */
#include "stm32f4xx.h"

#include <stdio.h>
#include <stdint.h>
#include "lpm.h"
#include "hwtimer.h"

static enum lpm_mode lpm;

void RTC_WKUP_IRQHandler(void)
{
    if (RTC_GetITStatus(RTC_IT_WUT) != RESET) {
        RTC_ClearITPendingBit(RTC_IT_WUT);
        EXTI_ClearITPendingBit(EXTI_Line22);
    }
}

void lpm_init(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    RTC_InitTypeDef RTC_InitStruct;

    EXTI_ClearITPendingBit(EXTI_Line22);
    EXTI_InitStructure.EXTI_Line = EXTI_Line22;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    PWR_BackupAccessCmd(ENABLE);

    /* Setup RTC clock */
    RCC_LSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
    RCC_RTCCLKCmd(ENABLE);
    RTC_WaitForSynchro();

    RTC_InitStruct.RTC_AsynchPrediv = 128 - 1;
    // TODO: Fix synchronized prescaler
    RTC_InitStruct.RTC_SynchPrediv = 250 - 1;
    RTC_Init(&RTC_InitStruct);

    RTC_WakeUpClockConfig(RTC_WakeUpClock_CK_SPRE_16bits);
    RTC_ITConfig(RTC_IT_WUT, ENABLE);
}

enum lpm_mode lpm_set(enum lpm_mode target)
{
    enum lpm_mode last_lpm = lpm;

    if (target == LPM_IDLE) {

    }
    else if (target == LPM_SLEEP) {
        uint32_t nearest_timer = 0;
        uint32_t now = hwtimer_now();
        uint32_t tmp;

        /* Find nearest timer */
        if (TIM2->CCER && TIM_Channel_1) {
            tmp = TIM2->CCR1 - now;
            if (!nearest_timer || (nearest_timer && tmp < nearest_timer))
                nearest_timer = tmp;
        }

        if (TIM2->CCER && TIM_Channel_2) {
            tmp = TIM2->CCR2 - now;
            if (!nearest_timer || (nearest_timer && tmp < nearest_timer))
                nearest_timer = tmp;
        }

        if (TIM2->CCER && TIM_Channel_3) {
            tmp = TIM2->CCR3 - now;
            if (!nearest_timer || (nearest_timer && tmp < nearest_timer))
                nearest_timer = tmp;
        }

        if (TIM2->CCER && TIM_Channel_4) {
            tmp = TIM2->CCR4 - now;
            if (!nearest_timer || (nearest_timer && tmp < nearest_timer))
                nearest_timer = tmp;
        }

        /* Use RTC and deep sleep for long term timer */
        if (nearest_timer > 1000000) {
            uint32_t seconds = nearest_timer / 1000000;

            RTC_SetWakeUpCounter(seconds - 1);

            RTC_WakeUpCmd(ENABLE);

            PWR_EnterSTOPMode(PWR_Regulator_ON, PWR_STOPEntry_WFI);

            RTC_WakeUpCmd(DISABLE);

            /* Recover system clocks */
            RCC_HSEConfig(RCC_HSE_ON);
            while (RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET);
            RCC_PLLCmd(ENABLE);
            while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
            RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
            while (RCC_GetSYSCLKSource() != 0x08);

            TIM2->CNT += seconds * 1000000;
        }

        __WFI();
    }

    lpm = target;

    return last_lpm;
}

void lpm_awake(void)
{
    lpm = LPM_ON;
}
