/*
 * adc.h
 *
 *  Created on: Aug 1, 2026
 *      Author: Lucifer Morningstar
 * @brief Bare-metal STM32F103 ADC driver.
 *
 * This driver provides an interface for configuring and using
 * the STM32F103 Analog-to-Digital Converter (ADC).
 *
 * Features:
 * - ADC1 / ADC2 support
 * - Single conversion mode
 * - Continuous conversion mode
 * - Software-triggered conversions
 * - Polling-based reading
 * - End-of-Conversion interrupt support
 * - Callback registration
 * - Internal temperature sensor support
 * - Internal VREF support
 * - Raw-to-millivolt conversion helper
 *
 * Supported MCU:
 * - STM32F103xx
 */

#ifndef ADC_H_
#define ADC_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "stm32f103x6.h"
#include "delay.h"
#include "gpio.h"
#include <stdint.h>

#ifndef 	RCC_APB2ENR_ADC1EN
#define		RCC_APB2ENR_ADC1EN (1U<<9)
#endif

#ifndef 	RCC_APB2ENR_ADC2EN
#define 	RCC_APB2ENR_ADC2EN (1U<<10)
#endif

#ifndef		ADC_SR_EOC
#define 	ADC_SR_EOC 		(1U<<1)
#endif

#ifndef		ADC_CR1_EOCIE
#define 	ADC_CR1_EOCIE	(1U<<5)
#endif

#ifndef		ADC_CR2_ADON
#define 	ADC_CR2_ADON	(1U<<0)
#endif

#ifndef		ADC_CR2_CONT
#define 	ADC_CR2_CONT	(1U<<1)
#endif

#ifndef		ADC_CR2_CAL
#define		ADC_CR2_CAL		(1U<<2)
#endif

#ifndef		ADC_CR2_RSTCAL
#define		ADC_CR2_RSTCAL	(1U<<3)
#endif

#ifndef		ADC_CR2_SWSTART
#define		ADC_CR2_SWSTART	(1U<<22)
#endif

/**
 * @brief ADC regular conversion channels.
 *
 * These values correspond directly to the STM32F103 ADC channel
 * numbers used by the regular conversion sequence registers.
 *
 * External channels:
 *  - ADC_CHANNEL_0  : PA0
 *  - ADC_CHANNEL_1  : PA1
 *  ...
 *  - ADC_CHANNEL_15 : PC5
 *
 * Internal channels:
 *  - ADC_CHANNEL_TEMP : Internal temperature sensor.
 *  - ADC_CHANNEL_VREF : Internal reference voltage.
 *
 * @note Internal channels require TSVREFE to be enabled.
 */

typedef enum{
	ADC_CHANNEL_0 = 0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3,
	ADC_CHANNEL_4, ADC_CHANNEL_5, ADC_CHANNEL_6, ADC_CHANNEL_7,
	ADC_CHANNEL_8, ADC_CHANNEL_9, ADC_CHANNEL_10, ADC_CHANNEL_11,
	ADC_CHANNEL_12, ADC_CHANNEL_13, ADC_CHANNEL_14, ADC_CHANNEL_15,
	ADC_CHANNEL_TEMP = 16,
	ADC_CHANNEL_VREF = 17
}adc_channel_t;

/**
 * @brief ADC sampling time selection.
 *
 * Determines how long the ADC samples the input signal before
 * beginning the conversion.
 *
 * Longer sampling times are recommended for:
 * - High source impedance sensors
 * - MQ-series gas sensors
 * - Potentiometers with large resistance
 *
 * Shorter sampling times are suitable for:
 * - Low impedance analog sources
 * - Op-amp outputs
 */

typedef enum{
	ADC_SAMPLE_1_5CY 	= 0x0,
	ADC_SAMPLE_7_5CY 	= 0x1,
	ADC_SAMPLE_13_5CY 	= 0x2,
	ADC_SAMPLE_28_5CY 	= 0x3,
	ADC_SAMPLE_41_5CY 	= 0x4,
	ADC_SAMPLE_55_5CY 	= 0x5,
	ADC_SAMPLE_71_5CY	= 0x6,
	ADC_SAMPLE_239_5CY	= 0x7
}adc_sample_time_t;

/**
 * @brief ADC clock prescaler.
 *
 * Selects the ADC clock frequency derived from APB2.
 *
 * ADC Clock = APB2 Clock / Prescaler
 *
 * @warning
 * Ensure the ADC clock does not exceed 14 MHz
 * for STM32F103 devices.
 */

typedef enum{
	ADC_PRESCALE_DIV2 	= 0x0,
	ADC_PRESCALE_DIV4 	= 0x1,
	ADC_PRESCALE_DIV6 	= 0x2,
	ADC_PRESCALE_DIV8 	= 0x3
}adc_prescaler_t;

/**
 * @brief ADC End-of-Conversion callback prototype.
 *
 * User callback invoked whenever an ADC conversion completes
 * while interrupts are enabled.
 *
 * @param adc ADC instance that generated the interrupt.
 * @param value Converted 12-bit ADC value.
 */

typedef void (*adc_eoc_callback_t)(ADC_TypeDef *adc, uint16_t value);

/**
 * @brief ADC initialization structure.
 *
 * This structure contains all configuration parameters required
 * to initialize an ADC peripheral.
 */
typedef struct{
	/** ADC peripheral instance (ADC1 or ADC2). */
	ADC_TypeDef 	 *instance;
	/** Regular conversion channel. */
	adc_channel_t 	  channel;
	/** Sampling time for the selected channel. */
	adc_sample_time_t sample_time;
	/** ADC clock prescaler. */
	adc_prescaler_t	  prescaler;
	/**
     * Conversion mode.
     * - 0 : Single conversion
     * - 1 : Continuous conversion
     */
	uint8_t			  continuous;
}adc_init_t;

/**
 * @brief Enables the peripheral clock for the specified ADC instance.
 *
 * This function enables the APB2 peripheral clock for the selected ADC
 * (ADC1 or ADC2). The ADC peripheral must have its clock enabled before
 * any configuration or conversion can be performed.
 *
 * @param adc Pointer to the ADC peripheral instance (e.g., ADC1 or ADC2).
 *
 * @note This function only enables the peripheral clock. It does not
 *       configure or initialize the ADC.
 */

void adc_clock_enable(ADC_TypeDef *adc);
/**
 * @brief Configures the ADC clock prescaler.
 *
 * The ADC clock is derived from the APB2 clock divided by the selected
 * prescaler value. The ADC clock must not exceed the maximum frequency
 * specified in the STM32F103 reference manual (typically 14 MHz).
 *
 * @param prescaler ADC clock prescaler selection.
 *
 * @note This setting is shared by both ADC1 and ADC2.
 */
void adc_set_prescaler(adc_prescaler_t prescaler);
/**
 * @brief Performs ADC calibration.
 *
 * Resets the previous calibration, starts a new calibration cycle,
 * and waits until the calibration process is complete.
 *
 * Calibration improves conversion accuracy by compensating for
 * internal offset errors.
 *
 * @param adc Pointer to the ADC peripheral instance.
 *
 * @note The ADC must be powered on (ADON = 1) before calibration.
 * @note Calibration is typically performed only once after ADC initialization.
 */
void adc_calibrate(ADC_TypeDef *adc);
/**
 * @brief Initializes the ADC peripheral.
 *
 * Configures the ADC clock prescaler, conversion channel,
 * sample time, conversion mode, and performs ADC calibration.
 *
 * If the selected channel is the internal temperature sensor
 * or internal voltage reference, the corresponding internal
 * measurement circuitry is automatically enabled.
 *
 * @param cfg Pointer to an ADC initialization structure.
 *
 * @note The corresponding GPIO pin should already be configured
 *       in analog mode before calling this function.
 *
 * @note A stabilization delay of at least 1 µs is inserted after
 *       enabling the ADC before calibration, as required by the
 *       STM32F1 reference manual.
 */
void adc_init(const adc_init_t *cfg);
/**
 * @brief Starts an ADC conversion using software trigger.
 *
 * Initiates a conversion by setting the SWSTART bit.
 *
 * In continuous mode, this starts the first conversion and the ADC
 * automatically continues subsequent conversions.
 *
 * @param adc Pointer to the ADC peripheral instance.
 */
void adc_start_conversion(ADC_TypeDef *adc);
/**
 * @brief Stops continuous conversion mode.
 *
 * Clears the CONT bit, preventing the ADC from automatically
 * starting a new conversion after the current one finishes.
 *
 * @param adc Pointer to the ADC peripheral instance.
 *
 * @note This does not abort an ongoing conversion.
 */
void adc_stop_continuous(ADC_TypeDef *adc);
/**
 * @brief Checks whether a conversion result is available.
 *
 * Tests the End Of Conversion (EOC) flag.
 *
 * @param adc Pointer to the ADC peripheral instance.
 *
 * @return
 *         - 1 : Conversion completed.
 *         - 0 : Conversion still in progress.
 */
uint8_t adc_data_ready(ADC_TypeDef *adc);

/**
 * @brief Reads an ADC conversion result using polling.
 *
 * Waits until the End Of Conversion (EOC) flag is set,
 * then returns the converted 12-bit digital value.
 *
 * @param adc Pointer to the ADC peripheral instance.
 *
 * @return 12-bit ADC conversion result (0 - 4095).
 *
 * @note This function blocks execution until the conversion
 *       has completed.
 */
uint16_t adc_read_blocking(ADC_TypeDef *adc);
/**
 * @brief Converts a raw ADC reading into millivolts.
 *
 * Uses the supplied reference voltage to convert a 12-bit ADC
 * reading into its equivalent voltage.
 *
 * Formula:
 * @code
 * Voltage(mV) = (Raw × VREF) / 4095
 * @endcode
 *
 * @param raw Raw ADC conversion result (0 - 4095).
 * @param vref_mv ADC reference voltage in millivolts.
 *
 * @return Converted voltage in millivolts.
 *
 * @note For STM32 Blue Pill operating at 3.3 V,
 *       use vref_mv = 3300.
 */
uint32_t adc_raw_to_millivolts(uint16_t raw, uint32_t vref_mv);
/**
 * @brief Enables the ADC End Of Conversion interrupt.
 *
 * Enables the EOC interrupt within the ADC peripheral and
 * enables the shared ADC interrupt in the NVIC.
 *
 * @param adc Pointer to the ADC peripheral instance.
 *
 * @note A callback should be registered using
 *       adc_register_eoc_callback() before enabling interrupts.
 */
void adc_interrupt_enable(ADC_TypeDef *adc);
/**
 * @brief Disables the ADC End Of Conversion interrupt.
 *
 * Clears the ADC EOC interrupt enable bit.
 *
 * @param adc Pointer to the ADC peripheral instance.
 *
 * @note This function only disables the ADC interrupt source.
 *       It does not disable the NVIC interrupt.
 */
void adc_interrupt_disable(ADC_TypeDef *adc);
/**
 * @brief Registers a callback function for ADC conversion complete events.
 *
 * The registered callback is invoked whenever an End Of Conversion (EOC)
 * interrupt occurs for the specified ADC.
 *
 * @param adc Pointer to the ADC peripheral instance.
 * @param callback Pointer to the user callback function.
 *
 * @note Passing NULL removes the previously registered callback.
 */
void adc_register_eoc_callback(ADC_TypeDef *adc, adc_eoc_callback_t callback);

//*************************Implement*********************************//
#ifndef ADC_DECLARATION_ONLY

static uint8_t adc__index(ADC_TypeDef *adc){
	if (adc == ADC1) return 0U;
	return 1U;
}

/**
 * @brief Enables the clock for the selected ADC peripheral.
 *
 * This function enables the APB2 peripheral clock required by the ADC.
 * The ADC must be clocked before any register can be accessed.
 *
 * Registers Modified:
 * - RCC->APB2ENR
 *   - ADC1EN (Bit 9)
 *   - ADC2EN (Bit 10)
 *
 * @param adc Pointer to the ADC peripheral (ADC1 or ADC2).
 *
 * @note This function only enables the peripheral clock.
 *       It does not configure or power up the ADC.
 */
void adc_clock_enable(ADC_TypeDef *adc){
	if (adc == ADC1) 	 RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
#ifdef ADC2
	else if(adc == ADC2) RCC->APB2ENR |= RCC_APB2ENR_ADC2EN;
#endif
}

void adc_set_prescaler(adc_prescaler_t prescaler){
	RCC->CFGR &= ~(0x3U << 14);
	RCC->CFGR |= ((uint32_t)prescaler & 0x3U) << 14;
}

void adc_calibrate(ADC_TypeDef *adc){
	adc->CR2 |= ADC_CR2_RSTCAL;
	while(adc->CR2 & ADC_CR2_RSTCAL){}
	adc->CR2 |= ADC_CR2_CAL;
	while(adc->CR2 & ADC_CR2_CAL){}
}

/**
 * @brief Initializes an ADC peripheral.
 *
 * Configures the ADC operating parameters including conversion
 * channel, sampling time, conversion mode, clock prescaler,
 * powers up the ADC and performs calibration.
 *
 * Registers Modified:
 * - RCC->CFGR
 *      - ADCPRE[15:14] : ADC clock prescaler
 *
 * - ADCx->SQR1
 *      - L[23:20] : Regular sequence length
 *
 * - ADCx->SQR3
 *      - SQ1[4:0] : First conversion channel
 *
 * - ADCx->SMPR1 / ADCx->SMPR2
 *      - SMPx[2:0] : Channel sampling time
 *
 * - ADCx->CR2
 *      - TSVREFE
 *      - CONT
 *      - ADON
 *
 * Performs:
 * 1. ADC clock configuration
 * 2. ADC peripheral clock enable
 * 3. Channel selection
 * 4. Sampling time configuration
 * 5. Continuous/single conversion configuration
 * 6. Internal sensor enable (if required)
 * 7. ADC power-up
 * 8. 1 µs stabilization delay
 * 9. ADC calibration
 *
 * @param cfg Pointer to ADC configuration structure.
 *
 * @pre The corresponding GPIO pin must already be configured
 *      in Analog Input mode.
 *
 * @post ADC is calibrated and ready to start conversions.
 */

void adc_init(const adc_init_t *cfg){
	ADC_TypeDef *adc = cfg->instance;

	adc_set_prescaler(cfg->prescaler);
	adc_clock_enable(adc);
	adc->SQR1 &= ~(0xFU << 20);
	adc->SQR3 = ((uint32_t)cfg->channel) & 0x1FU;

	if(cfg->channel <= ADC_CHANNEL_9) {
		uint32_t shift = ((uint32_t)cfg->channel)*3U;
		adc->SMPR2 &= ~(0x7U << shift);
		adc->SMPR2 |= ((uint32_t)cfg->sample_time)<<shift;
	}else{
		uint32_t shift = ((uint32_t)cfg->channel - 10U)*3U;
		adc->SMPR1 &= ~(0x7U << shift);
		adc->SMPR1 |= ((uint32_t)cfg->sample_time)<<shift;
	}

	if(cfg->channel == ADC_CHANNEL_VREF || cfg->channel == ADC_CHANNEL_TEMP){
		adc->CR2 |= (1U<<23);
	}

	if(cfg->continuous){
		adc->CR2 |= ADC_CR2_CONT;
	}else{
		adc->CR2 &= ~ADC_CR2_CONT;
	}
	adc->CR2 |= ADC_CR2_ADON;

	delay_us(1);

	adc_calibrate(adc);
}

/**
 * @brief Starts a software-triggered ADC conversion.
 *
 * Sets the SWSTART bit to begin a regular conversion.
 *
 * Register Modified:
 * - ADCx->CR2
 *      - SWSTART (Bit 22)
 *
 * @param adc Pointer to the ADC peripheral.
 *
 * @note In Continuous Conversion mode, only the first conversion
 *       needs to be started manually. Subsequent conversions are
 *       automatically triggered by hardware.
 */

void adc_start_conversion(ADC_TypeDef *adc){
	adc->CR2 |= ADC_CR2_SWSTART;
}

void adc_stop_continuous(ADC_TypeDef *adc){
	adc->CR2 &= ~ADC_CR2_CONT;
}

uint8_t adc_data_ready(ADC_TypeDef *adc){
	return(adc->SR & ADC_SR_EOC) ? 1U:0U;
}

/**
 * @brief Reads a conversion result using polling.
 *
 * Waits until the End Of Conversion (EOC) flag becomes set,
 * then returns the 12-bit conversion result.
 *
 * Registers Accessed:
 * - ADCx->SR
 *      - EOC (Bit 1)
 *
 * - ADCx->DR
 *      - DATA[11:0]
 *
 * @param adc Pointer to the ADC peripheral.
 *
 * @return 12-bit ADC conversion result (0-4095).
 *
 * @note Reading ADCx->DR automatically clears the EOC flag.
 */

uint16_t adc_read_blocking(ADC_TypeDef *adc){
	while (!(adc->SR & ADC_SR_EOC)) { }
	return (uint16_t)(adc->DR & 0x0FFFU);
}

uint32_t adc_raw_to_millivolts(uint16_t raw, uint32_t vref_mv){
    return ((uint32_t)raw * vref_mv) / 4095U;
}

/* --- EOC interrupts --- */

static adc_eoc_callback_t adc__eoc_callbacks[2] = { 0, 0 };

/**
 * @brief Dispatches ADC End-of-Conversion events.
 *
 * Reads the conversion result, clears the EOC flag,
 * and invokes the registered user callback.
 *
 * Registers Accessed:
 * - ADCx->SR
 * - ADCx->DR
 *
 * @param adc ADC instance.
 *
 * @note Internal helper.
 */

static void adc__dispatch(ADC_TypeDef *adc){
    if (adc->SR & ADC_SR_EOC) {
        uint16_t value = (uint16_t)(adc->DR & 0x0FFFU); /* reading DR clears EOC */
        uint8_t idx = adc__index(adc);
        if (adc__eoc_callbacks[idx] != 0) {
            adc__eoc_callbacks[idx](adc, value);
        }
    }
}

/**
 * @brief Enables ADC End-Of-Conversion interrupt.
 *
 * Enables generation of an interrupt whenever a regular conversion
 * completes and enables the shared ADC interrupt in the NVIC.
 *
 * Registers Modified:
 * - ADCx->CR1
 *      - EOCIE (Bit 5)
 *
 * - NVIC
 *      - ADC1_2_IRQn Enable
 *
 * @param adc Pointer to the ADC peripheral.
 *
 * @pre Register a callback using adc_register_eoc_callback().
 */

void adc_interrupt_enable(ADC_TypeDef *adc){
    adc->CR1 |= ADC_CR1_EOCIE;
    NVIC_EnableIRQ(ADC1_2_IRQn);
}

void adc_interrupt_disable(ADC_TypeDef *adc){
    adc->CR1 &= ~ADC_CR1_EOCIE;
}

void adc_register_eoc_callback(ADC_TypeDef *adc, adc_eoc_callback_t callback){
    adc__eoc_callbacks[adc__index(adc)] = callback;
}

void ADC1_2_IRQHandler(void){
    adc__dispatch(ADC1);
#ifdef ADC2
    adc__dispatch(ADC2);
#endif
}

#endif
#ifdef __cplusplus
}
#endif

#endif /* ADC_H_ */
