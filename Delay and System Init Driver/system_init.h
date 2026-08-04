/*
 * system_init.h
 *
 *  Created on: Aug 2, 2026
 *      Author: Lucifer Morningstar
 */

/**
 * @file    system_init.h
 * @brief   Single, consistent entry point for one-time board bring-up.
 *
 * USAGE:
 *     #include "system_init.h"
 *     int main(void){
 *         system_init();   // always first, every project
 *         ...
 *     }
 *
 * WHY THIS EXISTS:
 * Every project in this driver family needs SystemClock_Config() called
 * before anything else (GPIO/UART/ADC timing and delay.h's SysTick/DWT
 * setup all assume the real clock is already running). Rather than
 * relying on remembering the exact function name as the first line of
 * every main.c, system_init() gives one consistent call to reach for.
 * If a future project ever needs another one-time boot step (e.g. a
 * watchdog config), it's added inside system_init() once, and every
 * main() that already calls it picks the change up automatically.
 *
 * This is intentionally a thin, `static inline` wrapper - it compiles
 * away to exactly the same code as calling SystemClock_Config() directly,
 * it just gives that call a stable, memorable name across projects.
 *
 * Requires system_clock_config.c (providing SystemClock_Config()) to be
 * part of the build - this header only declares/wraps it, it doesn't
 * define it.
 */

#ifndef SYSTEM_INIT_H_
#define SYSTEM_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f103x6.h"

static inline void system_init(void)
{
	/* 1. Start the external 8 MHz crystal and wait for it to stabilize */
	    RCC->CR |= (1U << 16); /* HSEON */
	    while ((RCC->CR & (1U << 17)) == 0U) { /* wait HSERDY */
	    }

	    /* 2. Flash: 2 wait states required for 48 MHz < SYSCLK <= 72 MHz,
	     *    plus keep the prefetch buffer enabled (required whenever an AHB
	     *    prescaler other than /1 could be used - harmless to enable here). */
	    FLASH->ACR = (FLASH->ACR & ~0x7U) | 0x02U; /* LATENCY = 2 wait states */
	    FLASH->ACR |= (1U << 4); /* PRFTBE: prefetch buffer enable */

	    /* 3. Bus prescalers: AHB /1 (72 MHz), APB2 /1 (72 MHz), APB1 /2 (36 MHz) */
	    RCC->CFGR &= ~(0xFU << 4);   /* HPRE  = 0xxx -> SYSCLK not divided */
	    RCC->CFGR &= ~(0x7U << 11);  /* PPRE2 = 0xx  -> HCLK not divided (72 MHz) */
	    RCC->CFGR = (RCC->CFGR & ~(0x7U << 8)) | (0x4U << 8); /* PPRE1 = 100 -> /2 (36 MHz) */

	    /* 4. Configure the PLL: source = HSE (not divided), multiplier = x9
	     *    8 MHz * 9 = 72 MHz. PLL must be OFF while changing these bits. */
	    RCC->CR &= ~(1U << 24); /* PLLON = 0 (disable PLL before reconfiguring) */
	    RCC->CFGR &= ~(1U << 16); RCC->CFGR |= (1U << 16); /* PLLSRC = 1 -> HSE */
	    RCC->CFGR &= ~(1U << 17); /* PLLXTPRE = 0 -> HSE not divided */
	    RCC->CFGR = (RCC->CFGR & ~(0xFU << 18)) | (0x7U << 18); /* PLLMUL = 0111 -> x9 */

	    /* 5. Turn the PLL on and wait for it to lock */
	    RCC->CR |= (1U << 24); /* PLLON */
	    while ((RCC->CR & (1U << 25)) == 0U) { /* wait PLLRDY */
	    }

	    /* 6. Switch SYSCLK over to the PLL and wait for the switch to take effect */
	    RCC->CFGR = (RCC->CFGR & ~0x3U) | 0x2U; /* SW = 10 -> PLL selected */
	    while ((RCC->CFGR & (0x3U << 2)) != (0x2U << 2)) { /* wait SWS == PLL */
	    }
}

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_INIT_H_ */
