#include "stm32f103x6.h"
#include "gpio.h"

/* Enable printf() support in the UART driver.
 * This must be defined before including uart.h.
 */
#define UART_ENABLE_PRINTF

#include "uart.h"
#include <string.h>

/* Configures the system clock to 72 MHz.
 * The implementation is provided elsewhere.
 */
void SystemClock_Config(void);

/**
 * @brief Simple software delay.
 *
 * Generates an approximate delay using NOP (No Operation)
 * instructions. This delay is intended only for demonstration
 * purposes and is not timing accurate.
 *
 * @param count Number of delay iterations.
 */
static void delay(volatile uint32_t count)
{
    while (count--)
    {
        __asm__("nop");
    }
}

/* Indicates whether LED blinking mode is enabled.
 * 0 -> Blinking disabled
 * 1 -> Blinking enabled
 */
uint8_t blink = 0;

int main(void)
{
    /*----------------------------------------------------------
     * System Initialization
     *---------------------------------------------------------*/

    /* Configure the MCU to run at 72 MHz. */
    SystemClock_Config();

    /*----------------------------------------------------------
     * Configure USART1
     *
     * PA9  -> UART Transmit (TX)
     * PA10 -> UART Receive  (RX)
     *
     * USART1 is used as the serial command interface.
     *---------------------------------------------------------*/
    uart_gpio_pins_init(GPIOA,
                        GPIO_PIN_9,
                        GPIOA,
                        GPIO_PIN_10);

    /* UART communication settings */
    uart_init_t uart1_cfg =
    {
        /* UART peripheral */
        .instance = USART1,

        /* USART1 uses the APB2 peripheral clock. */
        .pclk_hz = 72000000U,

        /* Communication speed */
        .baudrate = 9600U,

        /* Standard UART frame:
         * 8 data bits
         * No parity
         * 1 stop bit
         */
        .word_length = UART_WORDLEN_8,
        .parity = UART_PARITY_NONE,
        .stop_bits = UART_STOPBITS_1,

        /* Hardware flow control disabled */
        .flow_control = UART_FLOWCTRL_NONE
    };

    /* Initialize USART1. */
    uart_init(&uart1_cfg);

    /* Display a startup message. */
    uart_write_string(USART1, "STM32 ready!!!\r\n");

    /* Enable the interrupt-driven receive buffer so complete
     * commands can be read using uart_read_line().
     */
    uart_rx_buffer_enable(USART1);

    /*----------------------------------------------------------
     * Configure the onboard LED.
     *
     * PC13 is connected to the built-in LED on most
     * STM32F103 Blue Pill boards.
     *
     * The LED is active-low:
     * RESET -> LED ON
     * SET   -> LED OFF
     *---------------------------------------------------------*/
    gpio_pin_config(GPIOC,
                    GPIO_PIN_13,
                    GPIO_MODE_OUTPUT_PP_2MHZ);

    /* Turn the LED off initially. */
    gpio_write_pin(GPIOC,
                   GPIO_PIN_13,
                   GPIO_PIN_STATE_SET);

    /*----------------------------------------------------------
     * Main Program Loop
     *---------------------------------------------------------*/
    while (1)
    {
        /* Buffer used to store one received command. */
        char line[64];

        /* Check whether a complete command has been received. */
        if (uart_read_line(USART1, line, sizeof(line)))
        {
            /* Echo the received command back to the terminal. */
            uart_printf(USART1,
                        "Received: %s\r\n",
                        line);

            /* Turn the LED on. */
            if (strcmp(line, "on") == 0)
            {
                blink = 0;

                /* PC13 is active-low. */
                gpio_write_pin(GPIOC,
                               GPIO_PIN_13,
                               GPIO_PIN_STATE_RESET);

                uart_write_string(USART1,
                                  "LED ON\r\n");
            }

            /* Turn the LED off. */
            else if (strcmp(line, "off") == 0)
            {
                blink = 0;

                gpio_write_pin(GPIOC,
                               GPIO_PIN_13,
                               GPIO_PIN_STATE_SET);

                uart_write_string(USART1,
                                  "LED OFF\r\n");
            }

            /* Enable or disable continuous blinking. */
            else if (strcmp(line, "toggle") == 0)
            {
                blink = !blink;

                uart_write_string(USART1,
                                  "Blink mode toggled\r\n");
            }

            /* Unknown command entered. */
            else
            {
                uart_write_string(
                    USART1,
                    "Unknown command. Try: on, off, toggle\r\n");
            }
        }

        /* If blink mode is enabled, continuously toggle the LED. */
        if (blink)
        {
            gpio_toggle_pin(GPIOC, GPIO_PIN_13);
            delay(2000000);
        }
    }
}
