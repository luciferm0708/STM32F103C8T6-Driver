# UART Driver for STM32F103C8T6 (Blue Pill)

A lightweight, register-level UART/USART driver for the **STM32F103C8T6 (Blue Pill)** using **CMSIS** peripheral definitions.

Unlike STM32 HAL or LL libraries, this driver communicates directly with the hardware registers, making it fast, easy to understand, and ideal for learning embedded systems or building lightweight applications.

The driver is implemented as a **single-header library**, allowing you to simply include `uart.h` in your project without managing separate source files.

---

## Features

- ✅ Register-level implementation (No HAL)
- ✅ CMSIS compatible
- ✅ Single-header library
- ✅ Supports USART1, USART2 and USART3
- ✅ Configurable baud rate
- ✅ 8-bit and 9-bit data
- ✅ Configurable parity
- ✅ Configurable stop bits
- ✅ RTS / CTS hardware flow control
- ✅ Blocking transmit
- ✅ Blocking receive
- ✅ Non-blocking polling
- ✅ Interrupt-driven reception
- ✅ User callback support
- ✅ Built-in receive ring buffer
- ✅ Line-based serial input
- ✅ `printf()` redirection
- ✅ Supports multiple UART peripherals simultaneously

---

# Repository Structure

```
UART Driver/
│
├── Inc/
│   └── uart.h
│
├── examples/
│   ├── UART Basic TX/
│   ├── UART Basic RX/
│   ├── UART Echo/
│   ├── UART Printf/
│   ├── UART LED Control/
│   ├── UART STM32 ESP32/
│   └── UART STM32 ESP32 LED Control/
│
├── LICENSE
└── README.md
```

---

# Requirements

Before using this driver, your project should contain:

- STM32F103C8T6 (Blue Pill)
- STM32CubeIDE
- CMSIS Device Header (`stm32f103x6.h`)
- GPIO Driver (`gpio.h`)
- `system_clock_config.c`

---

# Installation

Copy the following files into your project.

```
uart.h
gpio.h
stm32f103x6.h
system_clock_config.c
```

Then include the driver.

```c
#include "gpio.h"
#define UART_ENABLE_PRINTF      // Optional
#include "uart.h"
```

If you don't need `printf()` support, simply remove

```c
#define UART_ENABLE_PRINTF
```

---

# System Clock Configuration

Every example included in this repository contains a ready-to-use

```
system_clock_config.c
```

file.

Example:

```
UART Basic RX/
│
├── main.c
└── system_clock_config.c
```

The provided clock configuration sets the STM32F103C8T6 to **72 MHz**, which is the frequency used throughout every example.

Since UART baud rate generation depends on the peripheral clock, the system clock **must** be configured before initializing the UART.

Every example therefore begins with

```c
SystemClock_Config();
```

No additional clock configuration is required.

Simply copy both

```
main.c
system_clock_config.c
```

into your STM32CubeIDE project's **Src** folder.

Your project should look similar to:

```
Core/
├── Inc/
│   └── uart.h
└── Src/
    ├── main.c
    ├── system_clock_config.c
    ├── syscalls.c
    └── sysmem.c
```

---

# Quick Start

## 1. Configure UART Pins

```c
uart_gpio_pins_init(GPIOA,
                    GPIO_PIN_9,
                    GPIOA,
                    GPIO_PIN_10);
```

---

## 2. Configure UART

```c
uart_init_t uart_cfg =
{
    .instance     = USART1,
    .pclk_hz      = 72000000U,
    .baudrate     = 9600U,
    .word_length  = UART_WORDLEN_8,
    .parity       = UART_PARITY_NONE,
    .stop_bits    = UART_STOPBITS_1,
    .flow_control = UART_FLOWCTRL_NONE
};

uart_init(&uart_cfg);
```

---

## 3. Send Data

```c
uart_write_string(USART1,
                  "Hello World!\r\n");
```

---

## 4. Receive One Byte

```c
uint8_t data = uart_read_byte(USART1);
```

---

# Using printf()

Enable printf support before including the driver.

```c
#define UART_ENABLE_PRINTF
#include "uart.h"
```

Redirect stdout.

```c
uart_printf_redirect(USART1);
```

Now

```c
printf("Hello World\r\n");
```

will automatically be transmitted through USART1.

---

# Interrupt Reception

Enable interrupt-driven reception.

```c
uart_rx_buffer_enable(USART1);
```

Receive complete lines.

```c
char line[64];

if(uart_read_line(USART1, line, sizeof(line)))
{
    printf("%s\r\n", line);
}
```

---

# Included Examples

The repository contains several example projects demonstrating different UART features.

| Example | Description |
|----------|-------------|
| UART Basic TX | Basic UART transmission |
| UART Basic RX | Basic UART reception |
| UART LED Control | Control the Blue Pill onboard LED using UART commands |
| UART STM32 ESP32 | Bidirectional UART communication between STM32 and ESP32 |
| UART STM32 ESP32 LED Control | Control an ESP32 LED and synchronize LED status with the STM32 |

Each example contains:

```
main.c
system_clock_config.c
```

so no additional clock configuration is necessary.

---

# Running the Examples

1. Create a new STM32CubeIDE project for **STM32F103C8Tx**.

2. Copy

```
main.c
system_clock_config.c
```

from the desired example into the project's **Src** folder.

3. Copy

```
uart.h
gpio.h
stm32f103x6.h
```

into your include directory.

4. Build the project.

5. Flash the firmware.

6. Connect a USB-to-Serial adapter (or another UART device).

7. Open a serial terminal such as:

- RealTerm
- PuTTY
- Tera Term

using the baud rate configured in the example.

---

# API Overview

## Initialization

```c
uart_init()
uart_clock_enable()
uart_gpio_pins_init()
```

---

## Transmission

```c
uart_write_byte()
uart_write()
uart_write_string()
uart_printf()
```

---

## Reception

```c
uart_read_byte()
uart_data_ready()
uart_tx_ready()
```

---

## Interrupts

```c
uart_rx_interrupt_enable()
uart_rx_interrupt_disable()
uart_register_rx_callback()
```

---

## Ring Buffer

```c
uart_rx_buffer_enable()
uart_rx_buffer_disable()
uart_available()
uart_read_buffered_byte()
uart_read_line()
```

---

## printf Support

```c
uart_printf_redirect()
```

---

# Dependencies

This driver depends on:

- `stm32f103x6.h`
- `gpio.h`

---

# Tested On

- STM32F103C8T6 (Blue Pill)
- STM32CubeIDE
- ST-Link V2
- USB-to-Serial Adapters (CP2102 / CH340 / FT232)

---

# Future Improvements

Planned features include:

- DMA transmission
- DMA reception
- Idle-line detection
- Timeout-based receive functions
- Configurable software FIFO sizes
- Hardware RTS/CTS examples

---

# License

MIT License

Copyright (c) 2026 Lucifer Morningstar

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---

# Contributing

Contributions are welcome!

If you find a bug, have a suggestion, or would like to improve the driver, feel free to open an issue or submit a pull request.

---

If you find this project useful, consider giving it a ⭐ on GitHub.
