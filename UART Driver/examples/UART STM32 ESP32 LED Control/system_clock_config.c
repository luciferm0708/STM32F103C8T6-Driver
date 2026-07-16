/*
 * system_clock_config.c
 *
 *  Created on: Jul 9, 2026
 *      Author: Lucifer Morningstar
 */


#include "stm32f103x6.h"

/**
 * @brief  Bring the STM32F103C8T6 up from its power-on-reset default
 *         (internal 8 MHz HSI, no PLL) to the standard Blue Pill 72 MHz
 *         configuration, using the board's 8 MHz HSE crystal.
 *
 *   HSE (8 MHz) -> PLL x9 -> SYSCLK 72 MHz
 *   AHB  (HCLK)  = SYSCLK / 1 = 72 MHz
 *   APB2 (PCLK2) = HCLK   / 1 = 72 MHz   <- matches USART1's pclk_hz
 *   APB1 (PCLK1) = HCLK   / 2 = 36 MHz   <- matches USART2/3's pclk_hz
 *
 * Call this as the very first thing in main(), before any peripheral init.
 * Every register/bit here is verified against RM0008 (RCC and FLASH
 * chapters) - not guessed.
 */
void SystemClock_Config(void)
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
