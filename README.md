# STM32F103C8T6 Bare-Metal Driver Library

A lightweight, modular, and register-level peripheral driver library for the **STM32F103C8T6 (Blue Pill)**, developed entirely in **Embedded C** without using STM32 HAL or LL libraries.

The goal of this project is to provide a clean and educational implementation of STM32 peripheral drivers while maintaining a simple and reusable API. Each driver is implemented directly from the STM32F103 Reference Manual and Datasheet, making this repository suitable for learning bare-metal programming as well as developing lightweight embedded applications.

---

## Why This Project?

Most STM32 projects rely on the Hardware Abstraction Layer (HAL), which simplifies development but often hides the underlying hardware details.

This repository focuses on:

- Understanding STM32 peripherals at the register level
- Learning how peripheral registers work internally
- Writing efficient and reusable embedded drivers
- Building applications without HAL dependencies
- Creating a complete bare-metal firmware framework from scratch

If you want to understand **what happens behind HAL**, this project is for you.

---

## Repository Structure

```
STM32F103C8T6-Driver/
│
├── GPIO Driver/
│   ├── gpio.h
│   ├── example.c
│   └── README.md
│
├── USART Driver/          (Coming Soon)
├── SPI Driver/            (Coming Soon)
├── I2C Driver/            (Coming Soon)
├── TIMER Driver/          (Coming Soon)
├── ADC Driver/            (Coming Soon)
├── PWM Driver/            (Coming Soon)
│
├── LICENSE
└── README.md
```

---

## Implemented Drivers

| Driver | Status |
|---------|:------:|
| GPIO | ✅ Completed |
| USART | 🚧 In Progress |
| SPI | ⏳ Planned |
| I²C | ⏳ Planned |
| Timer | ⏳ Planned |
| PWM | ⏳ Planned |
| ADC | ⏳ Planned |
| DMA | ⏳ Planned |
| RTC | ⏳ Planned |

---

## Current Features

### GPIO Driver

- GPIO initialization
- Input modes
  - Analog
  - Floating
  - Pull-up
  - Pull-down
- Output modes
  - Push-Pull
  - Open-Drain
- Alternate Function modes
- GPIO read/write
- GPIO toggle
- GPIO locking
- AFIO remapping
- EXTI interrupt support
- Callback-based interrupt handling

📖 See the **GPIO Driver/README.md** for complete documentation and examples.

---

## Design Philosophy

This project follows a few simple principles:

- Bare-metal implementation
- CMSIS compatible
- Modular driver architecture
- Minimal memory footprint
- Easy to understand
- Easy to extend
- No dynamic memory allocation
- No HAL dependency

Each peripheral driver is self-contained and can be added to a project independently.

---

## Development Environment

- MCU: STM32F103C8T6 (Blue Pill)
- Language: C (C11)
- IDE: STM32CubeIDE
- Compiler: arm-none-eabi-gcc
- Programming Interface: ST-Link V2
- Architecture: ARM Cortex-M3

---

## Getting Started

1. Clone this repository

```bash
https://github.com/luciferm0708/STM32F103C8T6-Driver.git
```

2. Open the driver folder you want to use.

3. Follow the documentation inside that driver's README.

4. Build and flash your project.

---

## Roadmap

- ✅ GPIO Driver
- 🔄 USART Driver
- 🔄 SPI Driver
- 🔄 I²C Driver
- 🔄 Timer Driver
- 🔄 PWM Driver
- 🔄 ADC Driver
- 🔄 DMA Driver
- 🔄 RTC Driver
- 🔄 Watchdog Driver

---

## Contributing

Contributions are welcome.

If you find a bug, have an improvement, or would like to add a new peripheral driver, feel free to open an Issue or submit a Pull Request.

---

## License

This project is licensed under the MIT License.

---

## Author

**Faiyaz Khan Sami** aka **Lucifer Morningstar** 

Computer Science Engineer | Embedded Systems Enthusiast

GitHub: https://github.com/luciferm0708

---

⭐ If you find this repository useful, consider giving it a star.
