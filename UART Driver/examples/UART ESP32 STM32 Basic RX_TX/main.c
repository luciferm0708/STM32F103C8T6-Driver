/**
 * @brief   A two-way serial bridge between an STM32F103C8T6 (Blue Pill) and
 *          an ESP32, with a live debug console on the side so you can watch
 *          everything happen in real time.
 *
 * WHAT THIS DOES
 * ---------------
 * The STM32 has two independent serial ports running at once, each doing a
 * different job:
 *
 *   USART1 (PA9 = TX, PA10 = RX)  -- your PC, via a USB-serial (FTDI) adapter.
 *       This is your "debug console." Open it in RealTerm, PuTTY, or any
 *       serial terminal. Type a line and hit Enter, and the STM32 will:
 *         1. Print back what you typed, so you know it was received.
 *         2. Forward that exact line over to the ESP32.
 *         3. Print whatever the ESP32 sends back, as it arrives.
 *
 *   USART2 (PA2 = TX, PA3 = RX)  -- the ESP32, as a normal full-duplex link.
 *       Anything you type on your PC shows up here; anything the ESP32
 *       sends back gets relayed straight to your PC's terminal.
 *
 * In short: type on your PC -> STM32 relays it to the ESP32 -> ESP32's
 * reply gets relayed back to your PC. It's a simple, working example of
 * getting two microcontrollers talking to each other while still being
 * able to watch the conversation happen.
 *
 * WIRING
 * ------
 *   STM32 PA2  (USART2 TX)  --->  ESP32 GPIO16 (RX2)
 *   STM32 PA3  (USART2 RX)  <---  ESP32 GPIO17 (TX2)
 *   STM32 GND               ----  ESP32 GND        <- see note below!
 *
 *   STM32 PA9  (USART1 TX)  --->  FTDI RX
 *   STM32 PA10 (USART1 RX)  <---  FTDI TX
 *   STM32 GND               ----  FTDI GND
 *
 * A FEW THINGS THAT WILL SAVE YOU TIME
 * -------------------------------------
 *   - TX always connects to RX on the other device, never TX-to-TX. If you
 *     swap them by accident, that side simply won't receive anything.
 *
 *   - A solid, shared ground connection between every device is not
 *     optional - it's just as important as the TX/RX wires themselves.
 *     Without a good common ground, you'll typically see corrupted or
 *     garbled text instead of clean messages, even if TX/RX are wired
 *     correctly. If you're getting garbage on your terminal, check your
 *     ground wires (and how solid the physical connections are) before
 *     you suspect the code.
 *
 *   - If your board seems to receive unreliably even with everything wired
 *     correctly, a 10k pull-up resistor from the RX pin to 3.3V can help.
 *     UART lines idle high, and a pull-up gives a floating/noisy line
 *     something solid to rest at when nothing is actively driving it.
 *
 *   - Both boards here run on 3.3V logic, so you can connect them directly
 *     with no level shifter in between. That would NOT be true if you were
 *     wiring to a 5V board like a classic Arduino Uno - you'd risk damaging
 *     the STM32's pins in that case.
 *
 *   - Avoid the ESP32's default UART0 (GPIO1/GPIO3) for this kind of link.
 *     Those pins double as its USB/flashing console and spam boot-up logs
 *     over them, so use a second hardware UART (like UART2 here) instead.
 *
 *   - Both sides of any UART link must agree on the same baud rate. This
 *     example uses 9600 on both the PC-facing and ESP32-facing links, but
 *     feel free to raise it (e.g. 115200) once you've confirmed everything
 *     works - just make sure you update it everywhere it's set, on both
 *     the STM32 code and the ESP32 sketch.
 */

#include "stm32f103x6.h"
#include "gpio.h"

/* This flag switches on printf() support inside uart.h - it has to be
 * defined before uart.h is included, or the feature silently won't build. */
#define UART_ENABLE_PRINTF
#include "uart.h"

#include <stdio.h>
#include <string.h>

/* Brings the chip up from its default, sluggish 8 MHz internal clock to a
 * full 72 MHz using the board's external crystal and the PLL. Defined in
 * system_clock_config.c - see that file for the full explanation of what
 * it does and why every peripheral clock speed below depends on it. */
void SystemClock_Config(void);

int main(void)
{
    /* Always do this first. Every baud rate calculation below assumes the
     * chip is already running at 72 MHz - if this is skipped or fails
     * silently, nothing that follows will communicate at the right speed. */
    SystemClock_Config();

    /* ----------------------------------------------------------------
     * Set up USART1 as our debug console, talking to the PC over FTDI.
     * ---------------------------------------------------------------- */
    uart_gpio_pins_init(GPIOA, GPIO_PIN_9, GPIOA, GPIO_PIN_10);

    uart_init_t dbg_cfg = {
        .instance     = USART1,
        .pclk_hz      = 72000000U, /* USART1 sits on the 72 MHz APB2 bus */
        .baudrate     = 9600U,
        .word_length  = UART_WORDLEN_8,
        .parity       = UART_PARITY_NONE,
        .stop_bits    = UART_STOPBITS_1,
        .flow_control = UART_FLOWCTRL_NONE
    };
    uart_init(&dbg_cfg);

    /* From this point on, ordinary printf() calls are sent out over
     * USART1 instead of going nowhere - handy for quick debug messages
     * without having to think about which UART function to call. */
    uart_printf_redirect(USART1);

    /* ----------------------------------------------------------------
     * Set up USART2 as our link to the ESP32.
     * ---------------------------------------------------------------- */
    uart_gpio_pins_init(GPIOA, GPIO_PIN_2, GPIOA, GPIO_PIN_3);

    uart_init_t esp_cfg = {
        .instance     = USART2,
        .pclk_hz      = 36000000U, /* USART2 sits on the 36 MHz APB1 bus */
        .baudrate     = 9600U,
        .word_length  = UART_WORDLEN_8,
        .parity       = UART_PARITY_NONE,
        .stop_bits    = UART_STOPBITS_1,
        .flow_control = UART_FLOWCTRL_NONE
    };
    uart_init(&esp_cfg);

    /* Both USARTs get their own receive buffer here, so incoming text
     * from your PC and from the ESP32 can arrive at any time, in any
     * order, without either one interfering with the other. Once this is
     * on, you don't have to babysit incoming bytes yourself - just ask
     * uart_read_line() whether a full line has shown up yet. */
    uart_rx_buffer_enable(USART1);
    uart_rx_buffer_enable(USART2);

    printf("STM32 ready - type a message in your terminal, it'll be sent to the ESP32\r\n");

    char line[64];

    while (1) {
        /* Did the person typing on the PC finish a line? If so, show it
         * back to them (so they know it was received) and pass it along
         * to the ESP32 exactly as typed. */
        if (uart_read_line(USART1, line, sizeof(line))) {
            printf("You typed: %s\r\n", line);
            uart_printf(USART2, "%s\r\n", line);
        }

        /* Did the ESP32 send anything back? If so, relay it straight to
         * the PC's terminal so you can see the reply. */
        if (uart_read_line(USART2, line, sizeof(line))) {
            printf("From ESP32: %s\r\n", line);
        }
    }
}
