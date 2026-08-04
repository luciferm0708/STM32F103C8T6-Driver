# System Initialization (`system_init.h`)

Every driver in this repository assumes that the STM32F103C8T6 is running at its intended operating frequency (**72 MHz**). 
To ensure every peripheral operates correctly, **`system_init()` must be the first function called in `main()`**.

```c
#include "system_init.h"

int main(void)
{
    system_init();

    // Initialize peripherals here
    ...
}
```

---

## Why is `system_init()` necessary?

Many STM32 peripherals derive their timing directly from the system clock. Without configuring the clock first, these peripherals cannot operate as expected.

For example:

- **UART** baud rate calculations depend on the APB peripheral clocks.
- **Timers (TIM)** generate frequencies based on the timer clock.
- **ADC** conversion timing depends on the ADC clock.
- **Delay Driver** configures SysTick and the DWT cycle counter assuming the CPU is running at **72 MHz**.

If the system clock is not configured before initializing these drivers:

- UART communication will use incorrect baud rates.
- Timer periods and PWM frequencies will be inaccurate.
- ADC timing will not match the expected sampling rate.
- `delay_ms()` and `delay_us()` will produce incorrect delays.

Therefore, every project should begin with:

```c
system_init();
```

before initializing **GPIO, UART, ADC, TIM, SPI, I2C, Delay**, or any other peripheral driver.

---

## What does `system_init()` configure?

`system_init()` performs the complete clock configuration required for the STM32F103C8T6.

It performs the following steps:

1. Enables the external **8 MHz HSE crystal oscillator**
2. Waits until the oscillator becomes stable
3. Configures Flash latency and enables the prefetch buffer
4. Sets the AHB and APB bus prescalers
5. Configures the PLL (8 MHz × 9)
6. Enables the PLL and waits for it to lock
7. Switches the system clock source to the PLL

Final clock configuration:

| Clock | Frequency |
|--------|----------:|
| SYSCLK | 72 MHz |
| HCLK | 72 MHz |
| APB2 | 72 MHz |
| APB1 | 36 MHz |

---

## Why use `system_init()` instead of calling the clock configuration directly?

The function is intentionally implemented as a **`static inline`** wrapper.

This provides several advantages:

- One consistent initialization function for every project.
- No need to remember which clock configuration function must be called first.
- Future board-level initialization (watchdog, cache, etc.) can be added in one place.
- **Zero runtime overhead** — the compiler optimizes it exactly as if the code were written directly inside `main()`.

---

# Delay Driver (`delay.h`)

The delay driver provides accurate **millisecond** and **microsecond** delays without using any general-purpose timer peripherals.

```c
#include "delay.h"

delay_ms(500);
delay_us(100);
```

---

## Features

- Accurate millisecond delay (`delay_ms()`)
- Accurate microsecond delay (`delay_us()`)
- Millisecond system tick counter
- Timeout helper functions
- Automatic initialization (no separate init function required)
- Uses only **SysTick** and the Cortex-M3 **DWT Cycle Counter**

---

## How it works

The driver combines two hardware features of the STM32F103C8T6.

### 1. SysTick (Millisecond Timing)

The driver configures SysTick to generate an interrupt every **1 ms**.

Each interrupt increments an internal millisecond counter.

```
SysTick Interrupt
        │
        ▼
delay__tick_ms++
```

This counter is used by:

- `delay_ms()`
- `delay_get_tick()`
- `delay_start()`
- `delay_elapsed()`

---

### 2. DWT Cycle Counter (Microsecond Timing)

For microsecond delays, the driver uses the Cortex-M3 **Data Watchpoint and Trace (DWT)** cycle counter (`DWT->CYCCNT`).

Since the counter increments every CPU clock cycle:

```
72 MHz
= 72 cycles per microsecond
```

The requested delay is converted into CPU cycles, and the driver waits until the required number of cycles has elapsed.

This provides significantly higher precision than software loops or millisecond timers.

---

## Automatic Initialization

The delay driver initializes itself automatically the first time any delay function is called.

Internally it:

- Configures SysTick for a 1 ms interrupt
- Enables the Cortex-M3 DWT cycle counter
- Starts the internal millisecond tick counter

No initialization function is required.

---

## Why `system_init()` must be called first

The delay driver assumes the processor is already running at **72 MHz**.

```c
#define DELAY_CORE_CLOCK_HZ 72000000U
```

Both SysTick and the DWT cycle counter use this frequency when calculating delays.

If the MCU is still running from its default clock configuration:

- `delay_ms()` will no longer represent real milliseconds.
- `delay_us()` will no longer represent real microseconds.
- Timeout calculations will become inaccurate.

Always configure the system clock before using the delay driver.

```c
int main(void)
{
    system_init();

    delay_ms(100);
    delay_us(10);

    while (1)
    {

    }
}
```

---

## Timeout Utilities

The driver includes helper functions for implementing non-blocking timeouts.

```c
uint32_t start = delay_start();

while (!delay_elapsed(start, 1000))
{
    // Execute other tasks
}
```

This is useful when waiting for peripherals or implementing cooperative multitasking without blocking the CPU for long periods.

---

## Available Functions

| Function | Description |
|----------|-------------|
| `delay_ms(uint32_t ms)` | Blocking delay in milliseconds using SysTick |
| `delay_us(uint32_t us)` | High-precision blocking delay in microseconds using the DWT cycle counter |
| `delay_get_tick()` | Returns the number of milliseconds since the delay driver started |
| `delay_start()` | Returns the current tick value for timeout measurements |
| `delay_elapsed(start, timeout)` | Checks whether the specified timeout has expired |

---

# Initialization Order

For every project in this driver library, the recommended initialization sequence is:

```text
system_init()
      │
      ├── GPIO
      ├── UART
      ├── ADC
      ├── TIM
      ├── SPI
      ├── I2C
      └── Delay
```

Calling `system_init()` first ensures that every driver operates with the correct clock configuration, resulting in accurate timing, reliable peripheral operation, 
and consistent behavior across all projects.
