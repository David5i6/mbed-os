/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stddef.h>
#include "us_ticker_api.h"
#include "cmsis.h"
#include "mbed_assert.h"
#include "em_cmu.h"
#include "em_timer.h"
#include "device_peripherals.h"

/**
 * Timer functions for microsecond ticker.
 * mbed expects a 32-bit timer. Since the EFM32 only has 16-bit timers,
 * the upper 16 bits are implemented in software.
 */

static int us_ticker_inited = 0;	// Is ticker initialized yet

static volatile uint16_t ticker_cnt = 0;
static volatile uint16_t ticker_int_rem = 0;
static volatile uint16_t ticker_int_cnt = 0;
static uint32_t ticker_freq_mhz = 0;

void us_ticker_irq_handler_internal(void)
{
    if (TIMER_IntGet(US_TICKER_TIMER) & TIMER_IF_CC0) {
        if (ticker_int_rem > 0) {
            TIMER_CompareSet(US_TICKER_TIMER, 0, ticker_int_rem);
            ticker_int_rem = 0;
            TIMER_IntClear(US_TICKER_TIMER, TIMER_IF_CC0);
        } else if (ticker_int_cnt > 0) {
            ticker_int_cnt--;
            TIMER_IntClear(US_TICKER_TIMER, TIMER_IF_CC0);
        } else {
            us_ticker_irq_handler();
        }
    }

    if (TIMER_IntGet(US_TICKER_TIMER) & TIMER_IF_OF) {
        ticker_cnt++;
        TIMER_IntClear(US_TICKER_TIMER, TIMER_IF_OF);
    }
}

void us_ticker_init(void)
{
    if (us_ticker_inited) {
        return;
    }
    us_ticker_inited = 1;

    /* Enable clock for TIMERs */
    CMU_ClockEnable(US_TICKER_TIMER_CLOCK, true);

    /* Clear TIMER counter value */
    TIMER_CounterSet(US_TICKER_TIMER, 0);

    /* Get frequency of clock in MHz for scaling ticks to microseconds */
    ticker_freq_mhz = CMU_ClockFreqGet(US_TICKER_TIMER_CLOCK) / 1000000;
    MBED_ASSERT(ticker_freq_mhz > 0);

    /*
     * Calculate maximum prescaler that gives at least 1 MHz frequency, while keeping clock as an integer multiple of 1 MHz.
     * Example: 14 MHz => prescaler = 1 (i.e. DIV2), ticker_freq_mhz = 7;
     * 			24 MHz => prescaler = 3 (i.e. DIV8), ticker_freq_mhz = 3;
     * 			48 MHz => prescaler = 4 (i.e. DIV16), ticker_freq_mhz = 3;
     * Limit prescaling to maximum prescaler value, which is 10 (DIV1024).
     */
    uint32_t prescaler = 0;
    while((ticker_freq_mhz & 1) == 0 && prescaler <= 10) {
    	ticker_freq_mhz = ticker_freq_mhz >> 1;
    	prescaler++;
    }

    /* Set prescaler */
    US_TICKER_TIMER->CTRL = (US_TICKER_TIMER->CTRL & ~_TIMER_CTRL_PRESC_MASK) | (prescaler << _TIMER_CTRL_PRESC_SHIFT);

    /* Select Compare Channel parameters */
    TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
    timerCCInit.mode = timerCCModeCompare;

    /* Configure Compare Channel 0 */
    TIMER_InitCC(US_TICKER_TIMER, 0, &timerCCInit);

    /* Enable interrupt vector in NVIC */
    TIMER_IntEnable(US_TICKER_TIMER, TIMER_IEN_OF);
    NVIC_SetVector(US_TICKER_TIMER_IRQ, (uint32_t) us_ticker_irq_handler_internal);
    NVIC_EnableIRQ(US_TICKER_TIMER_IRQ);

    /* Set top value */
    TIMER_TopSet(US_TICKER_TIMER, 0xFFFF);

    /* Start TIMER */
    TIMER_Enable(US_TICKER_TIMER, true);
}

uint32_t us_ticker_read()
{
    uint32_t volatile countH_old, countH, countL;

    if (!us_ticker_inited) {
        us_ticker_init();
    }
	
	/* If waiting in an interrupt context, update ticker_cnt upon overflow */
	if( TIMER_IntGet(US_TICKER_TIMER) & TIMER_IF_OF ) {
		ticker_cnt++;
        TIMER_IntClear(US_TICKER_TIMER, TIMER_IF_OF);
	}

    /* Avoid jumping in time by reading high bits twice */
    do {
        countH_old = ticker_cnt;
        /* If the counter overflowed while in the IRQ handler for the CC0 interrupt,
         * it hasn't had time to update ticker_cnt yet. Take this into account here. */
        if (TIMER_IntGet(US_TICKER_TIMER) & TIMER_IF_OF) {
            countH_old += 1;
        }
        countL = US_TICKER_TIMER->CNT;
        countH = ticker_cnt;
        /* If the counter overflowed while in the IRQ handler for the CC0 interrupt,
         * it hasn't had time to update ticker_cnt yet. Take this into account here. */
        if (TIMER_IntGet(US_TICKER_TIMER) & TIMER_IF_OF) {
            countH += 1;
        }
    } while (countH_old != countH);

    /* Divide by ticker_freq_mhz to get 1 MHz clock */
    return ((countH << 16) | countL) / ticker_freq_mhz;
}

void us_ticker_set_interrupt(timestamp_t timestamp)
{
    TIMER_IntDisable(US_TICKER_TIMER, TIMER_IEN_CC0);

    int delta = (int) (timestamp - us_ticker_read());
    if (delta <= 0) {
        us_ticker_irq_handler();
        return;
    }

    /* Multiply by ticker_freq_mhz to get clock ticks */
    delta = delta * ticker_freq_mhz;
    timestamp = timestamp * ticker_freq_mhz;

    /* Split delta between timers */
    ticker_int_cnt = delta >> 16;
    ticker_int_rem = timestamp & 0xFFFF;

    /* Set compare channel 0 to (current position + lower 16 bits of delta).
     * If lower 16 bits is a small number, we a do one compare of (current + lower 16 + 0x8000)
     * and then one of (current + lower 16). Else, we simply use (current + lower 16).
     *
     * When time from lower 16 bits have elapsed, run complete cycles with ticker_int_rem as
     * reference ticker_int_cnt times. */
    if ((delta & 0xFFFF) < 0x8000 && ticker_int_cnt > 0) {
        TIMER_CompareSet(US_TICKER_TIMER, 0, ticker_int_rem + 0x8000);
        ticker_int_cnt--;
    } else {
        TIMER_CompareSet(US_TICKER_TIMER, 0, ticker_int_rem);
        ticker_int_rem = 0;
    }
    TIMER_IntClear(US_TICKER_TIMER, TIMER_IFC_CC0);
    TIMER_IntEnable(US_TICKER_TIMER, TIMER_IEN_CC0);
}

void us_ticker_disable_interrupt(void)
{
    /* Disable compare channel interrupts */
    TIMER_IntDisable(US_TICKER_TIMER, TIMER_IEN_CC0);
}

void us_ticker_clear_interrupt(void)
{
    /* Clear compare channel interrupts */
    TIMER_IntClear(US_TICKER_TIMER, TIMER_IFC_CC0);
}
