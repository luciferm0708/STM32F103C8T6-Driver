# STM32F103C8T6 GPIO Driver

A lightweight, register-level **GPIO driver** for the **STM32F103** series, written in pure C without relying on STM32 HAL or LL libraries.

This driver provides a clean and reusable interface for configuring and controlling GPIO peripherals while remaining close to the hardware. 
It is intended for developers who want to learn bare-metal programming, build their own peripheral drivers, or create lightweight embedded applications.

---

## Features

- ✅ Bare-metal implementation
- ✅ CMSIS compatible
- ✅ No HAL or LL dependency
- ✅ GPIO clock management
- ✅ Input configuration
  - Analog
  - Floating
  - Pull-up
  - Pull-down
- ✅ Output configuration
  - Push-Pull
  - Open-Drain
  - 2 MHz / 10 MHz / 50 MHz
- ✅ Alternate Function modes
- ✅ Read and write individual pins
- ✅ Read and write entire GPIO ports
- ✅ Atomic pin set/reset using BSRR/BRR
- ✅ GPIO pin toggle
- ✅ GPIO configuration locking
- ✅ AFIO remapping support
- ✅ EXTI interrupt configuration
- ✅ Rising, Falling and Both-edge interrupts
- ✅ Callback-based interrupt handling

---

## Why This Driver?

The STM32 HAL is powerful but often hides the underlying hardware details.

This library is designed for developers who want to:

- Understand STM32 GPIO registers
- Learn bare-metal firmware development
- Write efficient embedded applications
- Build their own peripheral libraries
- Avoid unnecessary software overhead

The implementation closely follows the STM32F103 Reference Manual while providing a simple and intuitive API.

---

## Supported Devices

Currently tested on:

- STM32F103C8T6 (Blue Pill)

The driver is also compatible with other STM32F103 devices that share the same GPIO peripheral architecture.

---

## Project Structure

```
GPIO_Driver/
│
├── gpio.h          # Complete GPIO driver
├── pushbtn_input.c       # Usage examples
└── README.md
```

---

## Dependencies

Only CMSIS device headers are required.

```
stm32f103x6.h
```

No HAL, LL, or CubeMX-generated GPIO code is required.

---

## Getting Started

1. Copy `gpio.h` into your project.
2. Include the driver:

```c
#include "gpio.h"
```

3. Build your project.

Refer to **example.c** for complete initialization and usage examples.

---

## Supported GPIO Modes

### Input

- Analog
- Floating
- Pull-up
- Pull-down

### Output

- Push-Pull
- Open-Drain

Available output speeds:

- 2 MHz
- 10 MHz
- 50 MHz

### Alternate Function

- Alternate Function Push-Pull
- Alternate Function Open-Drain

---

## API Overview

### Initialization

- `gpio_clock_enable()`
- `gpio_init()`
- `gpio_pin_config()`

### Read Operations

- `gpio_read_pin()`
- `gpio_read_port()`

### Write Operations

- `gpio_write_pin()`
- `gpio_write_port()`
- `gpio_set_pin()`
- `gpio_reset_pin()`
- `gpio_toggle_pin()`

### GPIO Protection

- `gpio_lock_pin()`

### AFIO

- `gpio_afio_clock_enable()`
- `gpio_afio_remap()`

### External Interrupt (EXTI)

- `gpio_exti_config()`
- `gpio_exti_register_callback()`
- `gpio_exti_disable()`
- `gpio_exti_clear_flag()`
- `gpio_exti_get_flag()`

---

## Design Goals

This driver was designed with the following principles:

- Simple API
- Register-level implementation
- Minimal overhead
- Easy to understand
- Easy to extend
- No dynamic memory allocation
- Suitable for educational and production projects

---

## Examples

Example applications are provided in **example.c**, including:
- Push Button Input
---

## Future Development

This GPIO driver is the first module in a complete STM32F103 bare-metal driver library.

Upcoming drivers include:

- RCC
- USART
- SPI
- I²C
- Timers
- PWM
- ADC
- DMA
- RTC
- Watchdog

---

## Contributing

Contributions, bug reports, feature requests, and improvements are welcome.

If you find an issue or would like to add functionality, feel free to open an Issue or submit a Pull Request.

---

## License

This project is released under the MIT License.

---

## Author

**Lucifer Morningstar**

Embedded Systems | Bare-Metal Firmware | STM32 Development
