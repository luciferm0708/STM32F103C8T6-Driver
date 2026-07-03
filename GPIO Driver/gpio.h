/*
 * gpio.h
 *
 *  Created on: Jul 3, 2026
 *      Author: Lucifer Morningstar
 */
/** USAGE - the normal case (one .c file, e.g. just main.c):
 *     #include "gpio.h"
 *   That's it. Everything (declarations AND bodies) is included automatically.
 *
 * USAGE - if you include this header from MORE THAN ONE .c file:
 *   C has no way to define a plain function's body in a header and include
 *   it in two translation units without a "multiple definition" link error.
 *   So: in every EXTRA .c file (i.e. all but one) that includes this header,
 *   add one line before the include:
 *       #define GPIO_DECLARATION_ONLY
 *       #include "gpio.h"
 *   That file then only sees the prototypes and can still call every
 *   function normally; the actual bodies stay defined in the one file that
 *   included it without the macro.
 *
 * Covers: pin init (mode/speed/pull), read/write/toggle, atomic set/reset
 * (BSRR/BRR), alternate function remap (AFIO_MAPR), and external interrupts
 * (AFIO_EXTICR + EXTI + NVIC) with automatic ISR dispatch via callbacks -
 * you never write an EXTIx_IRQHandler yourself.
 *
 * Depends on stm32f103xx.h providing CMSIS-style peripheral structs:
 *   GPIOA..GPIOE, RCC, AFIO, EXTI, NVIC_EnableIRQ/IRQn_Type (standard CMSIS).
 */

#ifndef GPIO_H
#define GPIO_H

#ifdef __cplusplus
extern "C"{
#endif

#include "stm32f103x6.h"
#include <stdint.h>

#ifndef RCC_APB2ENR_IOPAEN
#define RCC_APB2ENR_IOPAEN	(1U<<2)
#endif
#ifndef RCC_APB2ENR_IOPBEN
#define RCC_APB2ENR_IOPBEN	(1U<<3)
#endif
#ifndef RCC_APB2ENR_IOPCEN
#define RCC_APB2ENR_IOPCEN	(1U<<4)
#endif
#ifndef RCC_APB2ENR_IOPDEN
#define RCC_APB2ENR_IOPDEN	(1U<<5)
#endif
#ifndef RCC_APB2ENR_IOPEEN
#define RCC_APB2ENR_IOPEEN	(1U<<6)
#endif
#ifndef RCC_APB2ENR_AFIOEN
#define RCC_APB2ENR_AFIOEN   (1U << 0)
#endif
#ifndef GPIO_LCKR_LCKK
#define GPIO_LCKR_LCKK        (1U << 16)
#endif

/** Logical pin mode (combines MODE + CNF fields of CRL/CRH) */
typedef enum{
	//Input
	GPIO_MODE_INPUT_ANALOG 		= 0x0, /* MODE=00 CNF=00 */
	GPIO_MODE_INPUT_FLOATING 	= 0x1, /* MODE=00 CNF=01 (reset state) */
	GPIO_MODE_INPUT_PULL 		= 0x2, /* MODE=00 CNF=10, direction set via ODR (see gpio_set_pull) */
	//Output
	GPIO_MODE_OUTPUT_PP_10MHZ 	= 0x11,/* MODE=01 CNF=00 */
	GPIO_MODE_OUTPUT_PP_2MHZ	= 0x21,/* MODE=10 CNF=00 */
	GPIO_MODE_OUTPUT_PP_50MHZ 	= 0x31,/* MODE=11 CNF=00 */
	GPIO_MODE_OUTPUT_OD_10MHZ 	= 0x12,/* MODE=01 CNF=01 open-drain */
	GPIO_MODE_OUTPUT_OD_2MHZ 	= 0x22,
	GPIO_MODE_OUTPUT_OD_50MHZ 	= 0x32,
	//Alternate function
	GPIO_MODE_AF_PP_10MHZ	 	= 0x13,/* MODE=01 CNF=10 alt-function push-pull */
	GPIO_MODE_AF_PP_2MHZ        = 0x23,
	GPIO_MODE_AF_PP_50MHZ       = 0x33,
	GPIO_MODE_AF_OD_10MHZ       = 0x1B,/* MODE=01 CNF=11 alt-function open-drain */
	GPIO_MODE_AF_OD_2MHZ        = 0x2B,
	GPIO_MODE_AF_OD_50MHZ       = 0x3B
}gpio_mode_t;

/** Pull direction, only meaningful for GPIO_MODE_INPUT_PULL. Ignored (and
 *  may be left unset) for every other mode - floating input, analog, all
 *  outputs, and all alternate-function modes don't use ODR as a pull select. */

typedef enum{
	GPIO_PULL_DOWN = 0,
	GPIO_PULL_UP   = 1
}gpio_pull_t;

typedef enum{
	GPIO_PIN_STATE_RESET = 0,
	GPIO_PIN_STATE_SET   = 1
}gpio_pin_state_t;

typedef enum{
	GPIO_IT_RISING 		 	= 0x1,
	GPIO_IT_FALLING 		= 0x2,
	GPIO_IT_RISING_FALLING  = 0x3
}gpio_it_trigger_t;

/**
 * @brief  Signature for an EXTI pin callback.
 * @param  pin  the GPIO_PIN_x that triggered (bitmask, not index) - useful
 *              when several pins share one callback or one shared vector.
 */
typedef void (*gpio_exti_callback_t)(uint16_t pin);

/** Pin configuration descriptor used by gpio_init(). `pull` is ignored
 *  unless mode == GPIO_MODE_INPUT_PULL - leave it unset for any other mode. */
typedef struct{
	uint16_t 	pin; /* one or more of GPIO_PIN_0 .. GPIO_PIN_15 (bitmask), or GPIO_PIN_ALL */
	gpio_mode_t mode;
	gpio_pull_t	pull;/* only used when mode == GPIO_MODE_INPUT_PULL */
}gpio_init_t;

/*  Pin bitmasks*/
#define GPIO_PIN_0   ((uint16_t)0x0001)
#define GPIO_PIN_1   ((uint16_t)0x0002)
#define GPIO_PIN_2   ((uint16_t)0x0004)
#define GPIO_PIN_3   ((uint16_t)0x0008)
#define GPIO_PIN_4   ((uint16_t)0x0010)
#define GPIO_PIN_5   ((uint16_t)0x0020)
#define GPIO_PIN_6   ((uint16_t)0x0040)
#define GPIO_PIN_7   ((uint16_t)0x0080)
#define GPIO_PIN_8   ((uint16_t)0x0100)
#define GPIO_PIN_9   ((uint16_t)0x0200)
#define GPIO_PIN_10  ((uint16_t)0x0400)
#define GPIO_PIN_11  ((uint16_t)0x0800)
#define GPIO_PIN_12  ((uint16_t)0x1000)
#define GPIO_PIN_13  ((uint16_t)0x2000)
#define GPIO_PIN_14  ((uint16_t)0x4000)
#define GPIO_PIN_15  ((uint16_t)0x8000)
#define GPIO_PIN_ALL ((uint16_t)0xFFFF)

void gpio_clock_enable(GPIO_TypeDef *port);
void gpio_init(GPIO_TypeDef *port, const gpio_init_t *init);
void gpio_pin_config(GPIO_TypeDef *port, uint16_t pin, gpio_mode_t mode);
gpio_pin_state_t gpio_read_pin(GPIO_TypeDef *port, uint16_t pin);
uint16_t gpio_read_port(GPIO_TypeDef *port);
void gpio_write_pin(GPIO_TypeDef *port, uint16_t pin, gpio_pin_state_t state);
void gpio_write_port(GPIO_TypeDef *port, uint16_t value);
void gpio_toggle_pin(GPIO_TypeDef *port, uint16_t pin);
void gpio_set_pin(GPIO_TypeDef *port, uint16_t pin);
void gpio_reset_pin(GPIO_TypeDef *port, uint16_t pin);
void gpio_lock_pin(GPIO_TypeDef *port, uint16_t pin);

void gpio_afio_clock_enable(void);
void gpio_afio_remap(uint32_t mask, uint32_t value);

void gpio_exti_config(GPIO_TypeDef *port, uint16_t pin, gpio_it_trigger_t trigger);
void gpio_exti_register_callback(uint16_t pin, gpio_exti_callback_t callback);
void gpio_exti_disable(uint16_t pin);
void gpio_exti_clear_flag(uint16_t pin);
uint8_t gpio_exti_get_flag(uint16_t pin);

#ifndef GPIO_DECLARATION_ONLY

static uint8_t gpio__pin_to_index(uint16_t pin){
	uint8_t idx = 0;
	while((pin & 0x1U) == 0U && idx < 15U){
		pin >>= 1;
		idx++;
	}
	return idx;
}

static void gpio__mode_to_bits(gpio_mode_t mode,
		uint8_t *mode_bits, uint8_t *cnf_bits){
	*mode_bits = (uint8_t)((mode >> 4) & 0x3U);
	*cnf_bits  = (uint8_t)(mode & 0x3U);
}

static void gpio__write_pin_config(GPIO_TypeDef *port, uint8_t idx,
		uint8_t mode_bits, uint8_t cnf_bits){
	uint32_t cnf_mode = (uint32_t)((cnf_bits << 2) | mode_bits);

	if(idx < 8U){
		uint32_t shift = (uint32_t)idx * 4U;
		port->CRL &= ~(0xFU << shift);
		port->CRL |= (cnf_mode << shift);
	}else{
		uint32_t shift = (uint32_t)(idx - 8U) * 4U;
		port->CRH &= ~(0xFU << shift);
		port->CRH |= (cnf_mode << shift);
	}
}

static IRQn_Type gpio__exti_irqn_for_pin(uint8_t idx){
	switch (idx) {
		case 0:  return EXTI0_IRQn;
	    case 1:  return EXTI1_IRQn;
	    case 2:  return EXTI2_IRQn;
	    case 3:  return EXTI3_IRQn;
	    case 4:  return EXTI4_IRQn;
	    default: return (idx <= 9U) ? EXTI9_5_IRQn : EXTI15_10_IRQn;
	}
}

static uint32_t gpio__afio_port_code(GPIO_TypeDef *port)
{
    if (port == GPIOA) return 0x0U;
    if (port == GPIOB) return 0x1U;
    if (port == GPIOC) return 0x2U;
    if (port == GPIOD) return 0x3U;
#ifdef GPIOE
    if (port == GPIOE) return 0x4U;
#endif
    return 0x0U;
}

void gpio_clock_enable(GPIO_TypeDef *port)
{
    if (port == GPIOA)      RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    else if (port == GPIOB) RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    else if (port == GPIOC) RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    else if (port == GPIOD) RCC->APB2ENR |= RCC_APB2ENR_IOPDEN;
#ifdef GPIOE
    else if (port == GPIOE) RCC->APB2ENR |= RCC_APB2ENR_IOPEEN;
#endif
}

void gpio_init(GPIO_TypeDef *port, const gpio_init_t *init)
{
    gpio_clock_enable(port);

    uint8_t mode_bits, cnf_bits;
    gpio__mode_to_bits(init->mode, &mode_bits, &cnf_bits);

    for (uint8_t idx = 0; idx < 16U; idx++) {
        uint16_t bit = (uint16_t)(1U << idx);
        if ((init->pin & bit) == 0U) {
            continue;
        }

        gpio__write_pin_config(port, idx, mode_bits, cnf_bits);

        if (init->mode == GPIO_MODE_INPUT_PULL) {
            /* ODR bit selects pull-up (1) or pull-down (0) when CNF=10, MODE=00 */
            if (init->pull == GPIO_PULL_UP) {
                port->BSRR = bit;
            } else {
                port->BRR = bit;
            }
        }
    }
}

void gpio_pin_config(GPIO_TypeDef *port, uint16_t pin, gpio_mode_t mode){
	gpio_init_t cfg;
	cfg.pin = pin;
	cfg.mode = mode;
	cfg.pull = GPIO_PULL_UP;

	gpio_init(port, &cfg);
}

gpio_pin_state_t gpio_read_pin(GPIO_TypeDef *port, uint16_t pin){
	return ((port->IDR & pin) != 0U) ? GPIO_PIN_STATE_SET
			: GPIO_PIN_STATE_RESET;
}

uint16_t gpio_read_port(GPIO_TypeDef *port)
{
    return (uint16_t)port->IDR;
}

void gpio_write_pin(GPIO_TypeDef *port, uint16_t pin, gpio_pin_state_t state){
	if(state == GPIO_PIN_STATE_SET){
		port->BSRR = pin;
	}else{
		port->BSRR = (uint32_t)pin << 16;
	}
}

void gpio_write_port(GPIO_TypeDef *port, uint16_t value){
	port->ODR = value;
}

void gpio_toggle_pin(GPIO_TypeDef *port, uint16_t pin){
	port->ODR ^= pin;
}

void gpio_set_pin(GPIO_TypeDef *port, uint16_t pin)
{
    port->BSRR = pin;
}

void gpio_reset_pin(GPIO_TypeDef *port, uint16_t pin)
{
    port->BRR = pin;
}

void gpio_lock_pin(GPIO_TypeDef *port, uint16_t pin)
{
    volatile uint32_t tmp;
    /* LOCK key write sequence: LCKK=1 then W,R,R sequence*/
    port->LCKR = GPIO_LCKR_LCKK | pin;
    port->LCKR = pin;
    port->LCKR = GPIO_LCKR_LCKK | pin;
    tmp = port->LCKR;
    tmp = port->LCKR;
    (void)tmp;
}

void gpio_afio_clock_enable(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
}

void gpio_afio_remap(uint32_t mask, uint32_t value)
{
    gpio_afio_clock_enable();
    AFIO->MAPR &= ~mask;
    AFIO->MAPR |= (value & mask);
}

static gpio_exti_callback_t gpio__exti_callbacks[16] = { 0 };

static void gpio__exti_dispatch(uint8_t idx)
{
    uint16_t bit = (uint16_t)(1U << idx);
    if (EXTI->PR & (1U << idx)) {
        EXTI->PR = (1U << idx); /* clear pending flag (write 1 to clear) */
        if (gpio__exti_callbacks[idx] != 0) {
            gpio__exti_callbacks[idx](bit);
        }
    }
}

void gpio_exti_register_callback(uint16_t pin, gpio_exti_callback_t callback)
{
    uint8_t idx = gpio__pin_to_index(pin);
    gpio__exti_callbacks[idx] = callback;
}

void gpio_exti_config(GPIO_TypeDef *port, uint16_t pin, gpio_it_trigger_t trigger)
{
    uint8_t idx = gpio__pin_to_index(pin);

    gpio_afio_clock_enable();

    /* Route EXTIx line to this GPIO port via AFIO_EXTICR[idx/4] */
    uint32_t reg_index  = idx / 4U;
    uint32_t field_shift = (idx % 4U) * 4U;
    uint32_t port_code  = gpio__afio_port_code(port);

    AFIO->EXTICR[reg_index] &= ~(0xFU << field_shift);
    AFIO->EXTICR[reg_index] |= (port_code << field_shift);

    /* Configure edge trigger(s) */
    if (trigger & GPIO_IT_RISING) {
        EXTI->RTSR |= (1U << idx);
    } else {
        EXTI->RTSR &= ~(1U << idx);
    }

    if (trigger & GPIO_IT_FALLING) {
        EXTI->FTSR |= (1U << idx);
    } else {
        EXTI->FTSR &= ~(1U << idx);
    }

    /* Unmask the line and clear any stale pending flag */
    EXTI->PR = (1U << idx);
    EXTI->IMR |= (1U << idx);

    /* Enable the corresponding NVIC IRQ */
    NVIC_EnableIRQ(gpio__exti_irqn_for_pin(idx));
}

void gpio_exti_disable(uint16_t pin)
{
    uint8_t idx = gpio__pin_to_index(pin);
    EXTI->IMR &= ~(1U << idx);
    NVIC_DisableIRQ(gpio__exti_irqn_for_pin(idx));
}

void gpio_exti_clear_flag(uint16_t pin)
{
    uint8_t idx = gpio__pin_to_index(pin);
    EXTI->PR = (1U << idx); /* write 1 to clear */
}

uint8_t gpio_exti_get_flag(uint16_t pin)
{
    uint8_t idx = gpio__pin_to_index(pin);
    return (EXTI->PR & (1U << idx)) ? 1U : 0U;
}

void EXTI0_IRQHandler(void)     { gpio__exti_dispatch(0); }
void EXTI1_IRQHandler(void)     { gpio__exti_dispatch(1); }
void EXTI2_IRQHandler(void)     { gpio__exti_dispatch(2); }
void EXTI3_IRQHandler(void)     { gpio__exti_dispatch(3); }
void EXTI4_IRQHandler(void)     { gpio__exti_dispatch(4); }

void EXTI9_5_IRQHandler(void)
{
    for (uint8_t idx = 5U; idx <= 9U; idx++) {
        gpio__exti_dispatch(idx);
    }
}

void EXTI15_10_IRQHandler(void)
{
    for (uint8_t idx = 10U; idx <= 15U; idx++) {
        gpio__exti_dispatch(idx);
    }
}

#endif /* GPIO_DECLARATION_ONLY */

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H */
