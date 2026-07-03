/**
 * @file  pushbtn_input.c
 * @brief Example usage of the single-header gpio.h driver on STM32F103C8T6 (Blue Pill)
 *
 * Demo: PC13 = onboard LED (active-low),
 *       PA0  = push button pin.
 * When the push button state is high, the LED state is high, and vice-versa.
 * Note there is no gpio.c to add to the project - just this one file plus
 * gpio.h and your existing stm32f103x6.h.
 */
#include "stm32f103x6.h"
#include "gpio.h"

int main(void){
	gpio_init_t led = {
			.pin = GPIO_PIN_13,
			.mode = GPIO_MODE_OUTPUT_PP_2MHZ
	};
	gpio_init(GPIOC, &led);
	gpio_init_t btn = {
			.pin = GPIO_PIN_1,
			.mode = GPIO_MODE_INPUT_PULL,
			.pull = GPIO_PULL_UP
	};
	gpio_init(GPIOA, &btn);
	while(1){
		if(gpio_read_pin(GPIOA, GPIO_PIN_1) == GPIO_PIN_STATE_SET){
			gpio_write_pin(GPIOC, GPIO_PIN_13, GPIO_PIN_STATE_SET);
		}else{
			gpio_write_pin(GPIOC, GPIO_PIN_13, GPIO_PIN_STATE_RESET);
		}
	}


}
