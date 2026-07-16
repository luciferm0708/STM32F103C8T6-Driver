/**
 * @brief   Control an ESP32's LED from your PC, and watch the STM32's own
 *          LED mirror whatever the ESP32's LED is doing - a small,
 *          complete example of two microcontrollers staying in sync over
 *          a plain UART link.
 *
 * WHAT THIS DOES
 * ---------------
 * Open a serial terminal on the STM32's debug UART (USART1, via FTDI) and
 * type one of two commands, then press Enter:
 *
 *     led_on
 *     led_off
 *
 * The STM32 forwards that command to the ESP32 over USART2. The ESP32
 * turns its own LED on or off accordingly, and - separately - reports
 * back over the same link whenever ITS LED state changes (whether that
 * change came from your command, or from anywhere else in the ESP32's own
 * code, like a physical button - see esp32_led_sync.ino for an example of
 * that). The STM32 listens for that report and drives its own onboard LED
 * (PC13) to match. End result: press a button or send a command from
 * either side, and both boards' LEDs end up showing the same state.
 *
 * WIRING
 * ------
 *   STM32 PA9  (USART1 TX)  ----->  FTDI RX
 *   STM32 PA10 (USART1 RX)  <-----  FTDI TX
 *   STM32 GND                -----  FTDI GND
 *
 *   STM32 PA2  (USART2 TX)  ----->  ESP32 GPIO16 (RX2)
 *   STM32 PA3  (USART2 RX)  <-----  ESP32 GPIO17 (TX2)
 *   STM32 GND                -----  ESP32 GND
 *
 * See example_stm32_esp32.c for the fuller write-up on why the ground
 * connections matter as much as the TX/RX wires do, why a 10k pull-up on
 * an RX pin can help on a noisy breadboard, and why the ESP32's own
 * UART0 (GPIO1/3) isn't a good choice for this kind of link.
 *
 * Both sides talk at 9600 baud here - keep the STM32 code and the ESP32
 * sketch's Serial2.begin() in agreement if you change it.
 */

#include "stm32f103x6.h"
#include "gpio.h"

/* Must come before uart.h is included, or printf() support silently
 * doesn't get compiled in. */
#define UART_ENABLE_PRINTF
#include "uart.h"

#include <stdio.h>
#include <string.h>

/* Brings the chip up to 72 MHz - see system_clock_config.c. Every
 * pclk_hz value below assumes this has already run. */
void SystemClock_Config(void);

int main(void)
{
    SystemClock_Config();

    /* ----------------------------------------------------------------
     * USART1: your PC, via FTDI - this is where you type commands and
     * see status messages.
     * ---------------------------------------------------------------- */
    uart_gpio_pins_init(GPIOA, GPIO_PIN_9, GPIOA, GPIO_PIN_10);

    uart_init_t dbg_cfg = {
        .instance     = USART1,
        .pclk_hz      = 72000000U, /* USART1 lives on the 72 MHz APB2 bus */
        .baudrate     = 9600U,
        .word_length  = UART_WORDLEN_8,
        .parity       = UART_PARITY_NONE,
        .stop_bits    = UART_STOPBITS_1,
        .flow_control = UART_FLOWCTRL_NONE
    };
    uart_init(&dbg_cfg);
    uart_printf_redirect(USART1); /* printf() now goes out over USART1 */

    /* ----------------------------------------------------------------
     * USART2: the link to the ESP32.
     * ---------------------------------------------------------------- */
    uart_gpio_pins_init(GPIOA, GPIO_PIN_2, GPIOA, GPIO_PIN_3);

    uart_init_t esp_cfg = {
        .instance     = USART2,
        .pclk_hz      = 36000000U, /* USART2 lives on the 36 MHz APB1 bus */
        .baudrate     = 9600U,
        .word_length  = UART_WORDLEN_8,
        .parity       = UART_PARITY_NONE,
        .stop_bits    = UART_STOPBITS_1,
        .flow_control = UART_FLOWCTRL_NONE
    };
    uart_init(&esp_cfg);

    /* Independent receive buffers for each USART - lets us read a
     * complete typed command from the PC and a complete status line from
     * the ESP32, whichever arrives first, without either one blocking or
     * corrupting the other. */
    uart_rx_buffer_enable(USART1);
    uart_rx_buffer_enable(USART2);

    /* Onboard LED on most Blue Pill boards: PC13, wired active-LOW (a
     * LOW output turns it on, HIGH turns it off - the opposite of what
     * you'd expect at first glance). Start with it off. */
    gpio_pin_config(GPIOC, GPIO_PIN_13, GPIO_MODE_OUTPUT_PP_2MHZ);
    gpio_write_pin(GPIOC, GPIO_PIN_13, GPIO_PIN_STATE_SET);

    printf("STM32 ready. Type led_on / led_off in RealTerm to control ESP32's LED.\r\n");

    char line[64];

    while (1) {
        /* Command typed on the PC -> forward to the ESP32. */
        if (uart_read_line(USART1, line, sizeof(line))) {
            if (strcmp(line, "led_on") == 0) {
                uart_write_string(USART2, "led_on\r\n");
                printf("Sent to ESP32: led_on\r\n");
            } else if (strcmp(line, "led_off") == 0) {
                uart_write_string(USART2, "led_off\r\n");
                printf("Sent to ESP32: led_off\r\n");
            } else {
                printf("Unknown command. Try: led_on, led_off\r\n");
            }
        }

        /* Status line from the ESP32 -> mirror it on the STM32's own LED. */
        if (uart_read_line(USART2, line, sizeof(line))) {
            printf("From ESP32: %s\r\n", line);

            if (strcmp(line, "led_on") == 0) {
                gpio_write_pin(GPIOC, GPIO_PIN_13, GPIO_PIN_STATE_RESET); /* active-low: RESET = on */
                printf("STM32 LED ON\r\n");
            } else if (strcmp(line, "led_off") == 0) {
                gpio_write_pin(GPIOC, GPIO_PIN_13, GPIO_PIN_STATE_SET); /* active-low: SET = off */
                printf("STM32 LED OFF\r\n");
            }
        }
    }
}
