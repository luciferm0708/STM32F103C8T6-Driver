/*
 * tim.h
 *
 *  Created on: Aug 3, 2026
 *      Author: Lucifer Morningstar
 */

/**
 * @file    tim.h
 * @brief   Full-featured, SINGLE-HEADER Timer driver for STM32F103C8T6 (CMSIS-style)
 *
 * USAGE - the normal case (one .c file, e.g. just main.c):
 *     #include "tim.h"
 *   That's it. Everything (declarations AND bodies) is included automatically.
 *
 * USAGE - if you include this header from MORE THAN ONE .c file:
 *   Same rule as gpio.h / uart.h / adc.h - in every EXTRA .c file, add
 *   before the include:
 *       #define TIM_DECLARATION_ONLY
 *       #include "tim.h"
 *
 * Covers: TIM1/TIM2/TIM3/TIM4, basic timebase (prescaler + auto-reload),
 * a helper to compute PSC/ARR from a target frequency, start/stop,
 * update-event interrupts, output compare (toggle/PWM1/PWM2/etc.) on any
 * of a timer's 4 channels, input capture with edge selection, and
 * capture/compare interrupts - all with automatic ISR dispatch via
 * callbacks (no TIMx_IRQHandler to write yourself).
 *
 * Depends on stm32f103x6.h for the CMSIS peripheral structs, and on
 * gpio.h for pin setup - this header does NOT configure GPIO pins
 * itself, same reasoning as adc.h: no pin-config code duplicated here.
 * Before using a channel, configure its pin yourself:
 *     - Output compare / PWM channel -> GPIO_MODE_AF_PP_50MHZ (or another AF speed)
 *     - Input capture channel        -> GPIO_MODE_INPUT_FLOATING
 *
 * Default (non-remapped) channel-to-pin map on STM32F103C8T6, for reference:
 *     TIM1: CH1=PA8  CH2=PA9  CH3=PA10 CH4=PA11
 *     TIM2: CH1=PA0  CH2=PA1  CH3=PA2  CH4=PA3
 *     TIM3: CH1=PA6  CH2=PA7  CH3=PB0  CH4=PB1
 *     TIM4: CH1=PB6  CH2=PB7  CH3=PB8  CH4=PB9
 * (Remapped alternatives exist via gpio_afio_remap() - see RM0008 AFIO_MAPR
 * if you need a channel moved off its default pin.)
 *
 * ---------------------------------------------------------------------
 * WHY TIM_CLOCK_HZ IS A SINGLE CONSTANT FOR ALL FOUR TIMERS:
 * ---------------------------------------------------------------------
 * TIM1 sits on APB2, TIM2/3/4 sit on APB1 - different buses. Normally
 * that would mean different timer clocks. But there's a quirk in RM0008:
 * whenever an APBx PRESCALER IS NOT 1, that bus's TIMER clock (not the
 * peripheral clock) is automatically DOUBLED by hardware. In your
 * SystemClock_Config(): APB2 prescaler = /1, so TIM1CLK = PCLK2 = 72 MHz
 * directly; APB1 prescaler = /2, so TIM2/3/4CLK = PCLK1*2 = 36*2 = 72 MHz.
 * Both land on 72 MHz, which is why one constant works here. If you ever
 * change the APB1 or APB2 prescaler, re-check this assumption - override
 * with `#define TIM_CLOCK_HZ <value>` before the include if it changes,
 * same pattern as DELAY_CORE_CLOCK_HZ.
 */

#ifndef TIM_H_
#define TIM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "gpio.h"
#include "stm32f103x6.h"
#include <stdint.h>

#ifndef TIM_CLOCK_HZ
#define TIM_CLOCK_HZ 72000000U
#endif

/* ----------------------------------------------------------------------- */
/*  Types                                                                    */
/* ----------------------------------------------------------------------- */

typedef enum {
    TIM_CHANNEL_1 = 0,
    TIM_CHANNEL_2 = 1,
    TIM_CHANNEL_3 = 2,
    TIM_CHANNEL_4 = 3
} tim_channel_t;

/** Output compare mode, written to CCMRx's OCxM field. PWM1/PWM2 are the
 *  ones you want for actual PWM output; TOGGLE is what your dummy driver
 *  used for a simple square-wave test pin. */
typedef enum {
    TIM_OCMODE_FROZEN         = 0x0,
    TIM_OCMODE_ACTIVE         = 0x1, /* forces output high on match */
    TIM_OCMODE_INACTIVE       = 0x2, /* forces output low on match */
    TIM_OCMODE_TOGGLE         = 0x3,
    TIM_OCMODE_FORCE_INACTIVE = 0x4,
    TIM_OCMODE_FORCE_ACTIVE   = 0x5,
    TIM_OCMODE_PWM1           = 0x6, /* high while CNT < CCR, low after */
    TIM_OCMODE_PWM2           = 0x7  /* low while CNT < CCR, high after */
} tim_ocmode_t;

/** Which edge triggers an input capture. GP timers on F1 only expose one
 *  polarity bit per channel, so "both edges" isn't directly selectable -
 *  use two channels (one rising, one falling) on the same input if you
 *  need that. */
typedef enum {
    TIM_IC_EDGE_RISING  = 0,
    TIM_IC_EDGE_FALLING = 1
} tim_ic_edge_t;

typedef void (*tim_update_callback_t)(TIM_TypeDef *tim);
typedef void (*tim_cc_callback_t)(TIM_TypeDef *tim, tim_channel_t channel, uint16_t value);

/** Configuration descriptor used by tim_init() for a basic timebase.
 *  prescaler and period are given as the actual divide-by counts you
 *  want (e.g. prescaler=7200, period=10000 for a 1 Hz tick at 72 MHz) -
 *  tim_init() subtracts 1 internally when writing PSC/ARR, so you don't
 *  have to remember to do that yourself. */
typedef struct {
    TIM_TypeDef *instance;
    uint32_t     prescaler;            /* 1-65536, divides TIM_CLOCK_HZ (uint32_t so 65536 itself, the true max, is representable - it would overflow a uint16_t) */
    uint32_t     period;               /* 1-65536, counts per update event (same reason) */
    uint8_t      auto_reload_preload;  /* 1 = ARPE on (buffered ARR, glitch-free updates), 0 = off */
} tim_init_t;

/* ----------------------------------------------------------------------- */
/*  Public API - declarations (always visible)                              */
/* ----------------------------------------------------------------------- */

void tim_clock_enable(TIM_TypeDef *tim);

/** Basic bring-up: enables the clock, sets PSC/ARR/ARPE from cfg, resets
 *  the counter, and forces an update event (EGR|UG) so PSC/ARR take
 *  effect immediately. Does NOT start the timer - call tim_start() after. */
void tim_init(const tim_init_t *cfg);

/**
 * @brief  Convenience alternative to tim_init(): pick PSC/ARR automatically
 *         for an approximate target frequency in Hz (e.g. 1 for 1 Hz,
 *         1000 for 1 kHz). Enables the clock, resets the counter, and
 *         forces an update event, same as tim_init(). Good for "I just
 *         want roughly N Hz" cases; use tim_init() directly if you need
 *         an exact PSC/ARR pair.
 * @note   The search favours the smallest prescaler that gives an exact
 *         division; frequencies that don't divide TIM_CLOCK_HZ evenly
 *         will be approximated to the nearest achievable rate.
 */
void tim_set_frequency(TIM_TypeDef *tim, uint32_t target_hz);

void tim_start(TIM_TypeDef *tim); /* CEN = 1 */
void tim_stop(TIM_TypeDef *tim);  /* CEN = 0 */

void tim_set_counter(TIM_TypeDef *tim, uint16_t value);
uint16_t tim_get_counter(TIM_TypeDef *tim);

/* --- update (overflow/reload) interrupt --- */

/**
 * You do NOT need to write TIMx_IRQHandler yourself - this header
 * already defines TIM1_UP_IRQHandler, TIM2_IRQHandler, TIM3_IRQHandler
 * and TIM4_IRQHandler, dispatching to whatever you register below.
 * (TIM1's update and capture/compare events use SEPARATE NVIC lines on
 * this chip - TIM1_UP_IRQn and TIM1_CC_IRQn - unlike TIM2/3/4, which
 * share one line for everything. This header handles that split for you
 * automatically; you don't need to know which line is which.)
 */
void tim_update_interrupt_enable(TIM_TypeDef *tim);
void tim_update_interrupt_disable(TIM_TypeDef *tim);
void tim_register_update_callback(TIM_TypeDef *tim, tim_update_callback_t callback);

/* --- output compare / PWM --- */

/**
 * @brief  Configure one channel as an output compare (or PWM) channel and
 *         enable its output. Also forces an update event so the mode/CCR
 *         take effect immediately, and leaves output-compare preload
 *         (OCxPE) enabled so later tim_pwm_set_duty() calls only take
 *         effect on the next update event - glitch-free duty changes.
 * @param  pulse  initial CCRx value (compare/duty threshold, in counter ticks)
 */
void tim_oc_channel_init(TIM_TypeDef *tim, tim_channel_t channel, tim_ocmode_t mode, uint16_t pulse);

/** Update a channel's compare/duty value at runtime (just writes CCRx).
 *  Safe to call while the timer is running - takes effect on the next
 *  update event thanks to OCxPE set by tim_oc_channel_init(). */
void tim_pwm_set_duty(TIM_TypeDef *tim, tim_channel_t channel, uint16_t pulse);

/** Convert a 0-100 percent duty cycle to a CCRx value for the timer's
 *  CURRENT ARR - call after tim_init()/tim_set_frequency() so ARR is
 *  already set. percent is clamped to [0,100]. */
uint16_t tim_pwm_duty_from_percent(TIM_TypeDef *tim, uint8_t percent);

/* --- input capture --- */

/** Configure one channel as an input capture channel on the given edge,
 *  with no input filter/prescaler (captures every valid edge). Enables
 *  the capture (CCxE=1). */
void tim_ic_channel_init(TIM_TypeDef *tim, tim_channel_t channel, tim_ic_edge_t edge);

/** 1 if a new capture is waiting (CCxIF flag) on this channel, 0 otherwise. */
uint8_t tim_ic_data_ready(TIM_TypeDef *tim, tim_channel_t channel);

/** Read the last captured counter value for this channel. Reading CCRx
 *  clears CCxIF automatically in capture mode, matching this driver
 *  family's "read clears the flag" convention (see uart_read_byte(),
 *  adc_read_blocking()). */
uint16_t tim_ic_read(TIM_TypeDef *tim, tim_channel_t channel);

/* --- capture/compare interrupt (covers both IC captures and OC matches) --- */

void tim_cc_interrupt_enable(TIM_TypeDef *tim, tim_channel_t channel);
void tim_cc_interrupt_disable(TIM_TypeDef *tim, tim_channel_t channel);
void tim_register_cc_callback(TIM_TypeDef *tim, tim_cc_callback_t callback);

/* ----------------------------------------------------------------------- */
/*  Implementation                                                           */
/* ----------------------------------------------------------------------- */

#ifndef TIM_DECLARATION_ONLY

static uint8_t tim__index(TIM_TypeDef *tim)
{
    if (tim == TIM1) return 0U;
    if (tim == TIM2) return 1U;
    if (tim == TIM3) return 2U;
    return 3U; /* TIM4 */
}

static volatile uint32_t *tim__ccmr_for_channel(TIM_TypeDef *tim, tim_channel_t ch, uint8_t *shift)
{
    if (ch == TIM_CHANNEL_1) { *shift = 0U; return &tim->CCMR1; }
    if (ch == TIM_CHANNEL_2) { *shift = 8U; return &tim->CCMR1; }
    if (ch == TIM_CHANNEL_3) { *shift = 0U; return &tim->CCMR2; }
    *shift = 8U; return &tim->CCMR2; /* TIM_CHANNEL_4 */
}

static volatile uint32_t *tim__ccr_for_channel(TIM_TypeDef *tim, tim_channel_t ch)
{
    switch (ch) {
        case TIM_CHANNEL_1: return &tim->CCR1;
        case TIM_CHANNEL_2: return &tim->CCR2;
        case TIM_CHANNEL_3: return &tim->CCR3;
        default:            return &tim->CCR4; /* TIM_CHANNEL_4 */
    }
}

static uint8_t tim__ccer_shift(tim_channel_t ch)
{
    return (uint8_t)(((uint8_t)ch) * 4U); /* CCxE at bit (ch*4), CCxP at bit (ch*4+1) */
}

void tim_clock_enable(TIM_TypeDef *tim)
{
    if (tim == TIM1)      RCC->APB2ENR |= (1U << 11); /* TIM1EN */
    else if (tim == TIM2) RCC->APB1ENR |= (1U << 0);  /* TIM2EN */
    else if (tim == TIM3) RCC->APB1ENR |= (1U << 1);  /* TIM3EN */
#ifdef TIM4
    else if (tim == TIM4) RCC->APB1ENR |= (1U << 2);  /* TIM4EN */
#endif
}

void tim_init(const tim_init_t *cfg)
{
    TIM_TypeDef *tim = cfg->instance;

    tim_clock_enable(tim);

    tim->PSC = (uint16_t)(cfg->prescaler - 1U);
    tim->ARR = (uint16_t)(cfg->period - 1U);
    tim->CNT = 0U;

    if (cfg->auto_reload_preload) {
        tim->CR1 |= (1U << 7); /* ARPE */
    } else {
        tim->CR1 &= ~(1U << 7);
    }

    tim->EGR |= (1U << 0); /* UG: force update, loads PSC/ARR immediately */
}

void tim_set_frequency(TIM_TypeDef *tim, uint32_t target_hz)
{
    tim_clock_enable(tim);

    uint32_t psc = 1U;
    uint32_t arr = TIM_CLOCK_HZ / (psc * target_hz);

    while ((arr < 1U || arr > 65536U) && psc <= 65536U) {
        psc++;
        arr = TIM_CLOCK_HZ / (psc * target_hz);
    }
    if (arr < 1U)     arr = 1U;
    if (arr > 65536U) arr = 65536U;

    tim->PSC = (uint16_t)(psc - 1U);
    tim->ARR = (uint16_t)(arr - 1U);
    tim->CNT = 0U;
    tim->EGR |= (1U << 0);
}

void tim_start(TIM_TypeDef *tim) { tim->CR1 |= (1U << 0); }
void tim_stop(TIM_TypeDef *tim)  { tim->CR1 &= ~(1U << 0); }

void tim_set_counter(TIM_TypeDef *tim, uint16_t value) { tim->CNT = value; }
uint16_t tim_get_counter(TIM_TypeDef *tim) { return (uint16_t)tim->CNT; }

/* --- output compare / PWM --- */

void tim_oc_channel_init(TIM_TypeDef *tim, tim_channel_t channel, tim_ocmode_t mode, uint16_t pulse)
{
    uint8_t shift;
    volatile uint32_t *ccmr = tim__ccmr_for_channel(tim, channel, &shift);

    *ccmr &= ~(0xFFU << shift); /* clear CCxS + OCxFE/PE/M/CE for this channel */
    *ccmr |= ((uint32_t)mode & 0x7U) << (shift + 4U); /* OCxM */
    if (mode == TIM_OCMODE_PWM1 || mode == TIM_OCMODE_PWM2)
    {
        *ccmr |= (1U << (shift + 3U)); // OCxPE
    }

    tim->CCER &= ~(0x3U << tim__ccer_shift(channel));
    /* CCxE: enable output, active-high polarity default */
    tim->CCER |= (1U << tim__ccer_shift(channel));

    *tim__ccr_for_channel(tim, channel) = pulse;

    tim->EGR |= (1U << 0); /* force update so mode/CCR apply immediately */
}

void tim_pwm_set_duty(TIM_TypeDef *tim, tim_channel_t channel, uint16_t pulse)
{
    *tim__ccr_for_channel(tim, channel) = pulse;
}

uint16_t tim_pwm_duty_from_percent(TIM_TypeDef *tim, uint8_t percent)
{
    if (percent > 100U) percent = 100U;
    uint32_t arr = tim->ARR + 1U;
    return (uint16_t)((arr * percent) / 100U);
}

/* --- input capture --- */

void tim_ic_channel_init(TIM_TypeDef *tim, tim_channel_t channel, tim_ic_edge_t edge)
{
    uint8_t shift;
    volatile uint32_t *ccmr = tim__ccmr_for_channel(tim, channel, &shift);
    /* clear CCxS + ICxPSC + ICxF for this channel */
    *ccmr &= ~(0xFFU << shift);
    /* CCxS = 01: IC mapped directly to its own timer input (TIx) */
    *ccmr |= (0x1U << shift);

    tim->CCER &= ~(0x3U << tim__ccer_shift(channel));
    if (edge == TIM_IC_EDGE_FALLING) {
    	/* CCxP = 1: falling edge */
        tim->CCER |= (1U << (tim__ccer_shift(channel) + 1U));
    }
    tim->CCER |= (1U << tim__ccer_shift(channel)); /* CCxE: enable capture */
}

uint8_t tim_ic_data_ready(TIM_TypeDef *tim, tim_channel_t channel)
{
    uint32_t bit = (1U << (((uint8_t)channel) + 1U)); /* CC1IF=bit1 .. CC4IF=bit4 */
    return (tim->SR & bit) ? 1U : 0U;
}

uint16_t tim_ic_read(TIM_TypeDef *tim, tim_channel_t channel)
{
	/* reading CCRx clears CCxIF in capture mode */
    return (uint16_t)(*tim__ccr_for_channel(tim, channel));
}

/* --- capture/compare interrupts --- */

static tim_update_callback_t tim__update_cb[4] = { 0, 0, 0, 0 };
static tim_cc_callback_t     tim__cc_cb[4]     = { 0, 0, 0, 0 };

static void tim__dispatch_update(TIM_TypeDef *tim)
{
    if (tim->SR & (1U << 0)) { /* UIF */
    	/* UIF is NOT cleared by reading anything - must write 0 explicitly */
        tim->SR &= ~(1U << 0);
        tim_update_callback_t cb = tim__update_cb[tim__index(tim)];
        if (cb != 0) {
            cb(tim);
        }
    }
}

static void tim__dispatch_cc(TIM_TypeDef *tim)
{
    tim_cc_callback_t cb = tim__cc_cb[tim__index(tim)];
    for (uint8_t ch = 0; ch < 4U; ch++) {
        uint32_t bit = (1U << (ch + 1U));
        if (tim->SR & bit) {
            uint16_t value = (uint16_t)(*tim__ccr_for_channel(tim, (tim_channel_t)ch));
            /* explicit clear - valid in both capture and compare mode */
            tim->SR &= ~bit;
            if (cb != 0) {
                cb(tim, (tim_channel_t)ch, value);
            }
        }
    }
}

void tim_update_interrupt_enable(TIM_TypeDef *tim)
{
    tim->DIER |= (1U << 0); /* UIE */
    if (tim == TIM1)      NVIC_EnableIRQ(TIM1_UP_IRQn);
    else if (tim == TIM2) NVIC_EnableIRQ(TIM2_IRQn);
    else if (tim == TIM3) NVIC_EnableIRQ(TIM3_IRQn);
#ifdef TIM4
    else if (tim == TIM4) NVIC_EnableIRQ(TIM4_IRQn);
#endif
}

void tim_update_interrupt_disable(TIM_TypeDef *tim)
{
    tim->DIER &= ~(1U << 0);
    /* NVIC line left enabled on TIM2/3/4 - it's shared with capture/compare
     * interrupts on those timers, so disabling it here could silence a CC
     * interrupt you still want. Only the DIER bit above is needed to stop
     * update interrupts specifically. */
}

void tim_register_update_callback(TIM_TypeDef *tim, tim_update_callback_t callback)
{
    tim__update_cb[tim__index(tim)] = callback;
}

void tim_cc_interrupt_enable(TIM_TypeDef *tim, tim_channel_t channel)
{
    tim->DIER |= (1U << (((uint8_t)channel) + 1U)); /* CCxIE */
    /* TIM1 CC has its own separate line */
    if (tim == TIM1)      NVIC_EnableIRQ(TIM1_CC_IRQn);
    else if (tim == TIM2) NVIC_EnableIRQ(TIM2_IRQn);
    else if (tim == TIM3) NVIC_EnableIRQ(TIM3_IRQn);
#ifdef TIM4
    else if (tim == TIM4) NVIC_EnableIRQ(TIM4_IRQn);
#endif
}

void tim_cc_interrupt_disable(TIM_TypeDef *tim, tim_channel_t channel)
{
    tim->DIER &= ~(1U << (((uint8_t)channel) + 1U));
}

void tim_register_cc_callback(TIM_TypeDef *tim, tim_cc_callback_t callback)
{
    tim__cc_cb[tim__index(tim)] = callback;
}

/* --- IRQ vectors --- */
/* TIM1's update and capture/compare events live on SEPARATE NVIC lines on
 * this chip; TIM2/3/4 share one line for everything, so their handlers
 * check both. You never write any of these yourself. */

void TIM1_UP_IRQHandler(void) { tim__dispatch_update(TIM1); }
void TIM1_CC_IRQHandler(void) { tim__dispatch_cc(TIM1); }

void TIM2_IRQHandler(void) { tim__dispatch_update(TIM2); tim__dispatch_cc(TIM2); }
void TIM3_IRQHandler(void) { tim__dispatch_update(TIM3); tim__dispatch_cc(TIM3); }
#ifdef TIM4_IRQn
void TIM4_IRQHandler(void) { tim__dispatch_update(TIM4); tim__dispatch_cc(TIM4); }
#endif
#endif /* TIM_DECLARATION_ONLY */

#ifdef __cplusplus
}
#endif

#endif /* TIM_H_ */
