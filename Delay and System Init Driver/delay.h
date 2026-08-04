/*
 * delay.h
 *
 *  Created on: Aug 2, 2026
 *      Author: Lucifer Morningstar
 */

 /* ---------------------------------------------------------------------------
 * Delay Driver for STM32F103C8T6
 * ---------------------------------------------------------------------------
 *
 * This driver provides:
 *
 *  • Millisecond delays using the Cortex-M SysTick timer.
 *  • Microsecond delays using the Cortex-M3 DWT Cycle Counter.
 *  • A continuously incrementing millisecond system tick.
 *  • Simple timeout utilities for non-blocking applications.
 *
 * IMPORTANT
 * ---------
 * This driver assumes the MCU is already running at 72 MHz.
 *
 * Therefore, call:
 *
 *      system_init();
 *
 * before using any delay function.
 *
 * Internally, the driver automatically initializes SysTick and the
 * DWT cycle counter the first time a delay function is called.
 *
 */

#ifndef DELAY_H_
#define DELAY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f103x6.h"
#include <stdint.h>

#ifndef CoreDebug_DEMCR_TRCENA_Msk
#define CoreDebug_DEMCR_TRCENA_Msk (1U<<24)
#endif

/* Core clock frequency used for all delay calculations.
 *
 * Default:
 *      72 MHz
 *
 * If your application runs at a different system clock,
 * redefine DELAY_CORE_CLOCK_HZ before including this header.
 */

#ifndef DELAY_CORE_CLOCK_HZ
#define DELAY_CORE_CLOCK_HZ 72000000U
#endif

/**
 * @brief Delay execution for the specified number of milliseconds.
 *
 * Uses the SysTick timer interrupt.
 *
 * @param ms Number of milliseconds to wait.
 */

void delay_ms(uint32_t ms);

/**
 * @brief Delay execution for the specified number of microseconds.
 *
 * Uses the Cortex-M3 DWT cycle counter for high precision.
 *
 * @param us Number of microseconds to wait.
 */

void delay_us(uint32_t us);

/**
 * @brief Returns the current millisecond system tick.
 *
 * @return Elapsed milliseconds since the delay driver was initialized.
 */

uint32_t delay_get_tick(void);

/**
 * @brief Returns the current tick value.
 *
 * Used together with delay_elapsed() for implementing timeouts.
 *
 * @return Current tick count.
 */

uint32_t delay_start(void);

/**
 * @brief Checks whether a timeout has elapsed.
 *
 * @param start_tick Tick returned by delay_start().
 * @param timeout_ms Timeout duration in milliseconds.
 *
 * @return
 *      1 -> timeout expired
 *      0 -> timeout not yet expired
 */

uint8_t delay_elapsed(uint32_t start_tick, uint32_t timout_ms);

#ifndef DELAY_DECLARATION_ONLY

/* Internal millisecond counter.
 *
 * Incremented every SysTick interrupt.
 */

static volatile uint32_t delay__tick_ms = 0U;

/* Indicates whether the delay driver has already been initialized.
 *
 * Prevents configuring SysTick and DWT more than once.
 */

static volatile uint8_t delay__ready = 0U;

/*
 * Performs one-time initialization of the delay driver.
 *
 * This function:
 *
 * 1. Configures SysTick to generate an interrupt every 1 ms.
 * 2. Enables the Cortex-M3 DWT cycle counter.
 * 3. Starts the millisecond software tick.
 *
 * The initialization is performed automatically on the first
 * call to any public delay function.
 */

static void delay__ensure_ready(void){
	if(delay__ready){
		return;
	}

	SysTick->CTRL = 0U; /* Disable SysTick while configuring it. */

	/* Reload value for a 1 ms interrupt.
	 *
	 * Example:
	 *
	 * 72 MHz / 1000 = 72000 cycles
	 *
	 * SysTick counts from LOAD down to zero.
	 */

	SysTick->LOAD = (DELAY_CORE_CLOCK_HZ / 1000U) - 1U;

	/* Clear the current counter value. */
	SysTick->VAL = 0U;

	/*
	 * Enable SysTick.
	 *
	 * Bit 2 : CLKSOURCE = 1
	 *          Use processor clock.
	 *
	 * Bit 1 : TICKINT = 1
	 *          Enable SysTick interrupt.
	 *
	 * Bit 0 : ENABLE = 1
	 *          Start SysTick.
	 */
	SysTick->CTRL = (1U << 2) | (1U << 1) | (1U<<0);

	/*
	 * Enable access to the DWT unit.
	 *
	 * DEMCR.TRCENA must be set before the
	 * DWT cycle counter can be used.
	 */
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

	/* Reset the DWT cycle counter. */
	DWT->CYCCNT = 0U;

	/*
	 * Enable the DWT cycle counter.
	 *
	 * After enabling, CYCCNT increments
	 * once every CPU clock cycle.
	 */
	DWT->CTRL |= (1U<<0);
	delay__ready = 1U;
}
/*
 * Wait until the required number of
 * millisecond ticks has elapsed.
 *
 * Unsigned subtraction is used so the
 * calculation remains correct even if
 * the tick counter overflows.
 */
void delay_ms(uint32_t ms){
	delay__ensure_ready();
	uint32_t start = delay__tick_ms;
	while((uint32_t)(delay__tick_ms - start) < ms){}
}


void delay_us(uint32_t us){
	delay__ensure_ready();
	uint32_t start = DWT->CYCCNT;
	/*
	 * Convert microseconds into CPU cycles.
	 *
	 * Example:
	 *
	 * 72 MHz
	 * = 72 cycles/us
	 *
	 * 10 us
	 * = 720 CPU cycles
	 */
	uint32_t cycles = us * (DELAY_CORE_CLOCK_HZ / 1000000U);
	while ((uint32_t)(DWT->CYCCNT - start) < cycles) { }

}

uint32_t delay_get_tick(void){
	delay__ensure_ready();
	return delay__tick_ms;
}

uint32_t delay_start(void){
	delay__ensure_ready();
	return delay__tick_ms;
}

/*
 * Overflow-safe timeout comparison.
 *
 * Works correctly even when the
 * millisecond counter wraps around.
 */
uint8_t delay_elapsed(uint32_t start_tick, uint32_t timeout_ms){
    return ((uint32_t)(delay__tick_ms - start_tick) >= timeout_ms) ? 1U : 0U;
}

/*
 * SysTick Interrupt Service Routine.
 *
 * Executed every 1 millisecond.
 *
 * Updates the internal software tick
 * used by all millisecond timing functions.
 */
void SysTick_Handler(void){
    delay__tick_ms++;
}

#endif
#ifdef __cplusplus
}
#endif

#endif /* DELAY_H_ */
