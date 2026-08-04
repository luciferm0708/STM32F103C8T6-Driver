/**
 * @file    main.c
 * @brief   Example application demonstrating the Delay Driver.
 *
 * This example blinks the onboard LED connected to **PC13** using the
 * delay driver. The LED toggles every 1000 ms (1 second), resulting in
 * a continuous 1 Hz blink.
 *
 * Initialization sequence:
 * 1. Configure the system clock to 72 MHz using `system_init()`.
 * 2. Configure PC13 as a push-pull output.
 * 3. Toggle the LED with `delay_ms()`.
 *
 * @note
 * `system_init()` must be called before using the delay driver, as
 * `delay_ms()` and `delay_us()` assume the MCU is running at 72 MHz.
 *
 * Hardware:
 * - Board: STM32F103C8T6 (Blue Pill)
 * - LED  : PC13 (Active-Low)
 */


#include "system_init.h"
#include "gpio.h"
#include "delay.h"

int main(void){

	/**
	 * @brief Application entry point.
	 *
	 * Initializes the system clock and GPIO before repeatedly toggling
	 * the onboard LED with a 1-second delay between state changes.
	 *
	 * @return This function never returns.
	 */

	system_init();
	gpio_pin_config(GPIOC, GPIO_PIN_13, GPIO_MODE_OUTPUT_PP_50MHZ);

	while(1){
		gpio_write_pin(GPIOC, GPIO_PIN_13, GPIO_PIN_STATE_RESET);
		delay_ms(1000);
		gpio_write_pin(GPIOC, GPIO_PIN_13, GPIO_PIN_STATE_SET);
		delay_ms(1000);
	}
}
