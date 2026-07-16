#include "stm32f103x6.h"
#include "gpio.h"
#include "uart.h"

/* Configures the system clock to 72 MHz.
 * The implementation is provided elsewhere.
 */
void SystemClock_Config(void);

/**
 * @brief Simple software delay.
 *
 * Generates an approximate delay by executing NOP
 * (No Operation) instructions in a loop.
 *
 * @param count Number of loop iterations.
 *
 * @note This delay is not precise and should only be
 * used for simple demonstration programs.
 */
static void delay(volatile uint32_t count)
{
    while (count--)
    {
        __asm__("nop");
    }
}

int main(void)
{
    /*----------------------------------------------------------
     * Configure the system clock.
     *
     * The UART baud rate calculation depends on the peripheral
     * clock frequency, so initialize the clock first.
     *---------------------------------------------------------*/
    SystemClock_Config();

    /*----------------------------------------------------------
     * Configure the GPIO pins used by USART1.
     *
     * PA9  -> UART Transmit (TX)
     * PA10 -> UART Receive  (RX)
     *
     * The driver automatically configures the TX pin as
     * Alternate Function Push-Pull and the RX pin as
     * Floating Input.
     *---------------------------------------------------------*/
    uart_gpio_pins_init(GPIOA,
                        GPIO_PIN_9,
                        GPIOA,
                        GPIO_PIN_10);

    /*----------------------------------------------------------
     * Configure USART1 communication parameters.
     *---------------------------------------------------------*/
    uart_init_t uart1_cfg =
    {
        /* UART peripheral to initialize */
        .instance = USART1,

        /* USART1 receives its clock from the APB2 bus. */
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

        /* Hardware flow control is disabled. */
        .flow_control = UART_FLOWCTRL_NONE
    };

    /* Initialize USART1 using the configuration above. */
    uart_init(&uart1_cfg);

    while (1)
    {
        /* Send a text message to the connected serial terminal. */
        uart_write_string(USART1,
                          "Hello from the STM32F103C8T6\r\n");

        /* Wait before sending the next message. */
        delay(5000000);
    }
}
