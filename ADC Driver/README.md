# STM32F103 ADC Driver

A lightweight **bare-metal Analog-to-Digital Converter (ADC) driver** for the **STM32F103xx** microcontroller family. 
This driver provides an easy-to-use interface for configuring and reading analog signals without relying on STM32 HAL or LL libraries.

Designed for portability, readability, and educational purposes, the driver directly accesses STM32 peripheral registers using CMSIS definitions.

---

## Features

- ✅ ADC1 and ADC2 support
- ✅ Single conversion mode
- ✅ Continuous conversion mode
- ✅ Software-triggered conversions
- ✅ Configurable sampling time
- ✅ Configurable ADC clock prescaler
- ✅ Blocking (Polling) conversion
- ✅ End Of Conversion (EOC) interrupt support
- ✅ Callback registration for interrupt-driven applications
- ✅ Internal Temperature Sensor support
- ✅ Internal Reference Voltage (VREFINT) support
- ✅ Raw ADC to millivolt conversion helper
- ✅ Pure CMSIS / Register-level implementation

---

# Supported Devices

- STM32F103C8T6
- STM32F103CBT6
- STM32F103RBT6
- Other STM32F103xx devices with compatible ADC peripherals

---

# Project Structure

```
Project
│
├── adc.h
└── example_adc_mq2.c
```

---

# Prerequisites

The example application depends on the following drivers, which are available elsewhere in this repository:

- **System Initialization Driver**
  - `system_init.h`

- **Delay Driver**
  - `delay.h`

- **GPIO Driver**
  - `gpio.h`

- **UART Driver**
  - `uart.h`

These drivers must be included in your project before building the example application.
---
# ⚠️ Hardware Caution

The STM32F103 ADC input pins are **not 5 V tolerant**. The ADC reference voltage (VREF+) is typically **3.3 V**, therefore the analog input voltage **must never exceed 3.3 V**.

Many MQ-series gas sensor modules (such as the MQ-2) are powered from **5 V** and may produce an analog output voltage greater than **3.3 V** under certain gas concentrations.

> **Directly connecting the MQ-2 analog output (AO) to an STM32 ADC pin may damage the microcontroller.**

When interfacing a 5 V MQ sensor module with the STM32F103 ADC, use a **voltage divider** to scale the analog output to a safe voltage range.

Example resistor divider:

```
            10 kΩ
AO ----/\/\/\/\/\-----+------> PA0 (ADC)
                      |
                    20 kΩ
                      |
                     GND
```

Voltage at the ADC input:

```
V_ADC = V_AO × (R2 / (R1 + R2))
```

Using:

- R1 = 10 kΩ
- R2 = 20 kΩ

gives

```
V_ADC ≈ 0.667 × V_AO
```

Therefore,

| MQ-2 Output | STM32 ADC Input |
|-------------|-----------------|
| 5.0 V | 3.33 V |
| 4.5 V | 3.00 V |
| 3.3 V | 2.20 V |

This keeps the ADC input within the STM32F103's safe operating range.

> **Note**
>
> Some MQ sensor breakout boards include onboard circuitry that may limit the analog output below the supply voltage.
> Always verify the actual output voltage with a multimeter before connecting it directly to the ADC.
---
# Driver Architecture

```
                   APB2 Clock
                        │
                        ▼
                ADC Clock Prescaler
                        │
                        ▼
               ADC Initialization
                        │
          ┌─────────────┴─────────────┐
          ▼                           ▼
   Single Conversion          Continuous Conversion
          │                           │
          ▼                           ▼
   Polling Read               Polling / Interrupt
          │                           │
          └─────────────┬─────────────┘
                        ▼
                12-bit ADC Result
                        │
                        ▼
          Raw Counts → Millivolts
```

---

# ADC Initialization Sequence

The driver performs the following operations during initialization.

1. Configure ADC clock prescaler
2. Enable ADC peripheral clock
3. Configure conversion channel
4. Configure sampling time
5. Configure conversion mode
6. Enable internal channels (if selected)
7. Power up ADC
8. Wait 1 µs stabilization time
9. Calibrate ADC

After initialization, the application only needs to call

```c
adc_start_conversion(ADC1);
```

---

# Example

```c
gpio_pin_config(GPIOA,
                GPIO_PIN_0,
                GPIO_MODE_INPUT_ANALOG);

adc_init_t adc_cfg =
{
    .instance      = ADC1,
    .channel       = ADC_CHANNEL_0,
    .sample_time   = ADC_SAMPLE_239_5CY,
    .prescaler     = ADC_PRESCALE_DIV8,
    .continuous    = 1
};

adc_init(&adc_cfg);

adc_start_conversion(ADC1);

while (1)
{
    uint16_t value = adc_read_blocking(ADC1);

    uint32_t mv = adc_raw_to_millivolts(value, 3300);

    printf("%u  %lu mV\n", value, mv);
}
```

---

# API Overview

## Initialization

```c
adc_clock_enable()
adc_set_prescaler()
adc_calibrate()
adc_init()
```

---

## Conversion

```c
adc_start_conversion()
adc_stop_continuous()
adc_read_blocking()
adc_data_ready()
```

---

## Utilities

```c
adc_raw_to_millivolts()
```

---

## Interrupts

```c
adc_interrupt_enable()
adc_interrupt_disable()
adc_register_eoc_callback()
```

---

# ADC Configuration Structure

```c
typedef struct
{
    ADC_TypeDef         *instance;
    adc_channel_t        channel;
    adc_sample_time_t    sample_time;
    adc_prescaler_t      prescaler;
    uint8_t              continuous;
} adc_init_t;
```

---

# Sampling Time

| Enum | Sampling Time |
|-------|---------------|
| ADC_SAMPLE_1_5CY | 1.5 Cycles |
| ADC_SAMPLE_7_5CY | 7.5 Cycles |
| ADC_SAMPLE_13_5CY | 13.5 Cycles |
| ADC_SAMPLE_28_5CY | 28.5 Cycles |
| ADC_SAMPLE_41_5CY | 41.5 Cycles |
| ADC_SAMPLE_55_5CY | 55.5 Cycles |
| ADC_SAMPLE_71_5CY | 71.5 Cycles |
| ADC_SAMPLE_239_5CY | 239.5 Cycles |

Long sampling times are recommended for high impedance sensors such as:

- MQ Gas Sensors
- LDR Modules
- Potentiometers
- Thermistors

---

# ADC Clock Prescaler

| Enum | Division |
|------|----------|
| ADC_PRESCALE_DIV2 | PCLK2 / 2 |
| ADC_PRESCALE_DIV4 | PCLK2 / 4 |
| ADC_PRESCALE_DIV6 | PCLK2 / 6 |
| ADC_PRESCALE_DIV8 | PCLK2 / 8 |

> **Important**

The STM32F103 ADC clock must not exceed **14 MHz**.

---

# Interrupt Example

```c
void adc_callback(ADC_TypeDef *adc, uint16_t value)
{
    printf("%u\n", value);
}

adc_register_eoc_callback(ADC1, adc_callback);

adc_interrupt_enable(ADC1);

adc_start_conversion(ADC1);
```

---

# Voltage Conversion

Convert a raw ADC value into millivolts.

```c
uint32_t voltage =
    adc_raw_to_millivolts(raw, 3300);
```

Formula

```
Voltage = (Raw × VREF) / 4095
```

---

# Example Output

```
MQ-2 Raw: 1581   Voltage: 1274 mV
MQ-2 Raw: 1587   Voltage: 1279 mV
MQ-2 Raw: 1593   Voltage: 1284 mV

Gas Detected

MQ-2 Raw: 2178   Voltage: 1755 mV
MQ-2 Raw: 2481   Voltage: 1999 mV
MQ-2 Raw: 2860   Voltage: 2305 mV
```

---

# Tested Hardware

Board

- STM32 Blue Pill (STM32F103C8T6)

Sensor

- MQ-2 Gas Sensor Module

Compiler

- arm-none-eabi-gcc

IDE

- STM32CubeIDE

---

# Dependencies

This driver depends on

- CMSIS Device Header
- GPIO Driver
- Delay Driver
- System Initialization Driver

---

# Future Improvements

- DMA Support
- Scan Mode
- Injected Channels
- Analog Watchdog
- Multi-channel Conversion
- Calibration Helper APIs
- Oversampling Support
- Averaging Helper Functions

---

# License

This project is released under the MIT License.

---

# Author

**Lucifer Morningstar** aka **Faiyaz Khan Sami** 

Bare-Metal Embedded Systems Developer

STM32 • ESP32 • Embedded C • IoT
