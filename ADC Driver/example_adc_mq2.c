/**
 * @file    example_adc_mq2.c
 * @author  Lucifer Morningstar
 * @brief   ADC example: Reading an MQ-2 gas sensor using ADC1.
 *
 * @details
 * This example demonstrates how to configure and use the STM32F103
 * ADC driver to continuously sample the analog output of an MQ-2
 * gas sensor connected to PA0.
 *
 * The converted 12-bit ADC value is converted into millivolts and
 * transmitted over USART1 every second.
 *
 * Hardware Connections:
 * ---------------------
 * STM32F103 (Blue Pill)
 * ---------------------
 * PA0  <------ MQ-2 Analog Output (AO)
 * PA9  ------> USB-UART RX
 * PA10 <------ USB-UART TX (optional)
 * GND  <-----> Common Ground
 *
 * UART Configuration:
 * -------------------
 * Baud Rate : 9600 bps
 * Data Bits : 8
 * Stop Bits : 1
 * Parity    : None
 *
 * ADC Configuration:
 * ------------------
 * ADC Instance     : ADC1
 * Channel          : Channel 0 (PA0)
 * Resolution       : 12-bit
 * Sample Time      : 239.5 Cycles
 * Clock Prescaler  : PCLK2 / 8
 * Conversion Mode  : Continuous
 *
 * Expected Output:
 * ----------------
 * MQ-2 raw: 1583  mV: 1275
 * MQ-2 raw: 1590  mV: 1281
 * MQ-2 raw: 2014  mV: 1623   <-- Gas detected
 *
 * @note
 * The MQ-2 sensor requires several minutes of warm-up before
 * stable measurements can be obtained.
 */

#include <stdio.h>
#include "system_init.h"
#include "delay.h"
#include "gpio.h"
#include "adc.h"
#include "uart.h"

/*---------------------------------------------------------------------------
 * Global Variables
 *---------------------------------------------------------------------------*/

/** Stores the latest 12-bit ADC conversion result
 *  and the converted sensor voltage in millivolts. */

uint32_t mq2_value, mq2_mv;
int main(void){
	/*----------------------------------------------------------
	 * Initialize system clock, SysTick and delay driver.
	 * Must be called before using any peripheral drivers.
	 *---------------------------------------------------------*/
	system_init();

	 /*----------------------------------------------------------
	  * Configure USART1 GPIO pins.
	  *
	  * PA9  -> TX
	  * PA10 -> RX
	  *---------------------------------------------------------*/
	uart_gpio_pins_init(GPIOA, GPIO_PIN_9, GPIOA, GPIO_PIN_10);
	/*----------------------------------------------------------
	 * Configure USART1.
	 *---------------------------------------------------------*/
	uart_init_t uart1_cfg = {
			.instance = USART1,
			.baudrate = 9600U,
			.pclk_hz = 72000000U,
			.stop_bits = UART_STOPBITS_1,
			.flow_control = UART_FLOWCTRL_NONE
	};
	uart_init(&uart1_cfg);
	uart_printf(USART1, "1: UART OK\r\n");
	/*----------------------------------------------------------
	 * Configure PA0 as Analog Input.
	 *
	 * The MQ-2 analog output is connected to ADC Channel 0.
	 *---------------------------------------------------------*/
	gpio_pin_config(GPIOA, GPIO_PIN_0, GPIO_MODE_INPUT_ANALOG);
	uart_printf(USART1, "2: GPIO OK\r\n");

	/*----------------------------------------------------------
	 * Configure ADC1.
	 *---------------------------------------------------------*/
	adc_init_t mq2_cfg = {
	        .instance = ADC1,
	        .channel = ADC_CHANNEL_0,
	        .sample_time = ADC_SAMPLE_239_5CY,
	        .prescaler = ADC_PRESCALE_DIV8,
	        .continuous = 1
	};
	adc_init(&mq2_cfg);
	uart_printf(USART1, "3: ADC init OK\r\n");
	/*----------------------------------------------------------
	 * Start continuous ADC conversions.
	 *---------------------------------------------------------*/
	adc_start_conversion(ADC1);
	uart_printf(USART1, "4: ADC start OK\r\n");

	while(1){
	    /* Wait until a conversion completes and read
	       the 12-bit conversion result. */
	    mq2_value = adc_read_blocking(ADC1);
	    /* Convert ADC counts into millivolts assuming
	      a 3.3 V reference voltage. */
	    mq2_mv = adc_raw_to_millivolts((uint16_t)mq2_value, 3300U);
	    uart_printf(USART1, "MQ-2 raw: %lu  mV: %lu\n\r", mq2_value, mq2_mv);
	    /* Update once every second. */
	    delay_ms(1000);
	}
}
