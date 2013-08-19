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

/* Comment it to disable RTC automatic calibration */
#define RTC_AUTO_CALIB

static enum lpm_mode lpm;
static int lpm_enabled;

static int rtc_synch_prediv = 250;

#ifdef RTC_AUTO_CALIB
#define RTC_CALIB_THRESHOLD (3000)
#define RTC_CALIB_DURATION_THRESHOLD (200)

static int lpm_initialized;
static uint32_t rtc_calib_start_time;
static uint32_t rtc_calib_valid_count;
#endif /* RTC_AUTO_CALIB */

void RTC_WKUP_IRQHandler(void)
{
    if (RTC_GetITStatus(RTC_IT_WUT) != RESET) {
#ifdef RTC_AUTO_CALIB

        /*
         * Sychronize RTC tick with Timer 2 by adjusting Synchronous Prescaler
         * until errors are below RTC_CALIB_THRESHOLD three times in succession.
         */
        if (!lpm_initialized) {
            uint32_t delta;
            int error;
            int ratio;

            delta = hwtimer_now() - rtc_calib_start_time;
            error = delta - 1000000;
            ratio = 1000 * 1000000 / delta;

            if (error > RTC_CALIB_THRESHOLD || error < -RTC_CALIB_THRESHOLD) {
                int old_rtc_synch_prediv = rtc_synch_prediv;

                /*
                 * Filter undesired measurement result delayed by other
                 * interrupt handler
                 */
                if (ratio - 1000 < RTC_CALIB_DURATION_THRESHOLD &&
                    ratio - 1000 > -RTC_CALIB_DURATION_THRESHOLD) {
                    rtc_synch_prediv = rtc_synch_prediv * 1000000 / delta;

                    if (old_rtc_synch_prediv == rtc_synch_prediv) {
                        rtc_synch_prediv -= (error > 0 ? 1 : -1);
                    }

                    rtc_calib_valid_count = 0;
                }

                /* Reset RTC prescaler and tick */
                RTC_InitTypeDef RTC_InitStruct;
                RTC_InitStruct.RTC_AsynchPrediv = 128 - 1;
                RTC_InitStruct.RTC_SynchPrediv = rtc_synch_prediv - 1;
                RTC_Init(&RTC_InitStruct);

                rtc_calib_start_time = hwtimer_now();
            }
            else if (rtc_calib_valid_count < 2) {
                if (ratio - 1000 < RTC_CALIB_DURATION_THRESHOLD &&
                    ratio - 1000 > -RTC_CALIB_DURATION_THRESHOLD) {
                    rtc_calib_valid_count++;
                }
            }
            else {
                RTC_WakeUpCmd(DISABLE);
                lpm_initialized = 1;
            }
        }

#endif /* RTC_AUTO_CALIB */

        RTC_ClearITPendingBit(RTC_IT_WUT);
        EXTI_ClearITPendingBit(EXTI_Line22);
    }
}

void lpm_init(void)
{
    if (!lpm_enabled) {
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
        RTC_InitStruct.RTC_SynchPrediv = rtc_synch_prediv - 1;
        RTC_Init(&RTC_InitStruct);

#ifdef RTC_AUTO_CALIB
        rtc_calib_start_time = hwtimer_now();
#endif /* RTC_AUTO_CALIB */

        RTC_WakeUpClockConfig(RTC_WakeUpClock_CK_SPRE_16bits);
        RTC_ITConfig(RTC_IT_WUT, ENABLE);

        RTC_WakeUpCmd(DISABLE);

#ifdef RTC_AUTO_CALIB
        RTC_SetWakeUpCounter(0);
        RTC_WakeUpCmd(ENABLE);
#endif /* RTC_AUTO_CALIB */

        lpm_enabled = 1;
    }
}

enum lpm_mode lpm_set(enum lpm_mode target)
{
    enum lpm_mode last_lpm = lpm;

    if (target == LPM_IDLE) {

    }
    else if (target == LPM_SLEEP) {
#ifdef RTC_AUTO_CALIB

        if (lpm_initialized) {
#else

        if (lpm_enabled) {
#endif /* RTC_AUTO_CALIB */
            uint32_t nearest_timer = 0;
            uint32_t now = hwtimer_now();
            uint32_t tmp;

            /* Find nearest timer */
            if (TIM2->CCER && TIM_Channel_1) {
                tmp = TIM2->CCR1 - now;

                if (!nearest_timer || (nearest_timer && tmp < nearest_timer)) {
                    nearest_timer = tmp;
                }
            }

            if (TIM2->CCER && TIM_Channel_2) {
                tmp = TIM2->CCR2 - now;

                if (!nearest_timer || (nearest_timer && tmp < nearest_timer)) {
                    nearest_timer = tmp;
                }
            }

            if (TIM2->CCER && TIM_Channel_3) {
                tmp = TIM2->CCR3 - now;

                if (!nearest_timer || (nearest_timer && tmp < nearest_timer)) {
                    nearest_timer = tmp;
                }
            }

            if (TIM2->CCER && TIM_Channel_4) {
                tmp = TIM2->CCR4 - now;

                if (!nearest_timer || (nearest_timer && tmp < nearest_timer)) {
                    nearest_timer = tmp;
                }
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
