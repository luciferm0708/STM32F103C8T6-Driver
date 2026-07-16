/**
 * @file uart.h
 * @author Lucifer Morningstar
 * @date Jul 8, 2026
 *
 * @brief
 * A lightweight and easy-to-use UART/USART driver for the STM32F103
 * (Blue Pill) using CMSIS register-level programming.
 *
 * This driver supports:
 *  - USART1, USART2 and USART3
 *  - Configurable baud rate
 *  - 8-bit and 9-bit data formats
 *  - Parity and stop-bit configuration
 *  - Hardware flow control
 *  - Blocking transmission and reception
 *  - Interrupt-driven reception
 *  - Built-in ring buffer
 *  - Non-blocking line reader
 *  - printf() redirection
 *
 * This driver is designed to work together with gpio.h.
 */

#ifndef UART_H
#define UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gpio.h"
#include "stm32f103x6.h"
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

/**
 * @brief
 * Size of the temporary buffer used by uart_printf().
 *
 * Increase this value if your formatted strings are longer than
 * the default buffer size.
 */

#ifndef UART_PRINTF_BUFSZ
#define UART_PRINTF_BUFSZ 128
#endif

/**
 * @brief
 * Number of bytes that can be stored in the receive ring buffer
 * for each UART before older data starts being discarded.
 */

#ifndef UART_RING_BUF_SZ
#define UART_RING_BUF_SZ 64
#endif

/**
 * @brief
 * Maximum number of characters that uart_read_line() can store
 * before returning a complete line.
 */
#ifndef UART_LINE_BUF_SZ
#define UART_LINE_BUF_SZ 64
#endif

// Fallback bit-definitions

#ifndef RCC_APB2ENR_USART1EN
#define RCC_APB2ENR_USART1EN (1U << 14)
#endif
#ifndef RCC_APB1ENR_USART2EN
#define RCC_APB1ENR_USART2EN (1U << 17)
#endif
#ifndef RCC_APB1ENR_USART3EN
#define RCC_APB1ENR_USART3EN (1U << 18)
#endif

/* AFIO_MAPR remap masks for USART pin remapping, handy with gpio_afio_remap() */
#define UART_AFIO_USART1_REMAP         (1U << 2)   /* 0: PA9/PA10, 1: PB6/PB7 */
#define UART_AFIO_USART2_REMAP         (1U << 3)   /* 0: PA2/PA3,  1: PD5/PD6 */
#define UART_AFIO_USART3_REMAP_MASK    (0x3U << 4) /* field mask */
#define UART_AFIO_USART3_REMAP_PARTIAL (0x1U << 4) /* PC10/PC11 */
#define UART_AFIO_USART3_REMAP_FULL    (0x3U << 4) /* PD8/PD9 */


// Types


typedef enum {
    UART_WORDLEN_8 = 0, /**< 8-bit data frame */
    UART_WORDLEN_9 = 1  /**< 9-bit data frame */
} uart_wordlen_t;

typedef enum {
    UART_PARITY_NONE = 0,
    UART_PARITY_EVEN = 1,
    UART_PARITY_ODD  = 2
} uart_parity_t;

typedef enum {
    UART_STOPBITS_1   = 0x0,
    UART_STOPBITS_0_5 = 0x1,
    UART_STOPBITS_2   = 0x2,
    UART_STOPBITS_1_5 = 0x3
} uart_stopbits_t;

typedef enum {
    UART_FLOWCTRL_NONE    = 0x0,
    UART_FLOWCTRL_RTS     = 0x1,
    UART_FLOWCTRL_CTS     = 0x2,
    UART_FLOWCTRL_RTS_CTS = 0x3
} uart_flowctrl_t;

/**
 * @brief  Signature for a UART RX callback, called from the ISR with each
 *         byte as it arrives (interrupt-driven mode only).
 */
typedef void (*uart_rx_callback_t)(USART_TypeDef *usart, uint8_t byte);

/** Configuration descriptor used by uart_init().
 *  pclk_hz MUST be the actual clock feeding this peripheral:
 *  USART1 is on APB2 (PCLK2), USART2/USART3 are on APB1 (PCLK1).
 *  On a default un-configured Blue Pill (HSI, no PLL) both are 8 MHz;
 *  with the common 72 MHz PLL setup, PCLK2 = 72 MHz and PCLK1 = 36 MHz. */
typedef struct {
    USART_TypeDef  *instance;
    uint32_t        pclk_hz;
    uint32_t        baudrate;
    uart_wordlen_t  word_length;
    uart_parity_t   parity;
    uart_stopbits_t stop_bits;
    uart_flowctrl_t flow_control;
} uart_init_t;

/**
 * @brief Enables the peripheral clock for the selected USART.
 *
 * Every STM32 peripheral must have its clock enabled before
 * its registers can be accessed.
 *
 * @param usart Pointer to USART1, USART2 or USART3.
 */

void uart_clock_enable(USART_TypeDef *usart);

/**
 * @brief Initializes a UART peripheral.
 *
 * This function configures:
 *  - Baud rate
 *  - Word length
 *  - Stop bits
 *  - Parity
 *  - Hardware flow control
 *  - Enables transmitter and receiver
 *
 * GPIO pins must be configured separately before communication
 * can take place.
 *
 * @param cfg Pointer to UART configuration structure.
 */
void uart_init(const uart_init_t *cfg);

/**
 * @brief  Convenience helper: configure a TX pin (AF push-pull) and an RX
 *         pin (floating input) in one call, using your gpio.h driver.
 */
void uart_gpio_pins_init(GPIO_TypeDef *tx_port, uint16_t tx_pin,
                          GPIO_TypeDef *rx_port, uint16_t rx_pin);

/* --- blocking transfer --- */
void uart_write_byte(USART_TypeDef *usart, uint8_t byte);
void uart_write(USART_TypeDef *usart, const uint8_t *data, uint16_t len);
void uart_write_string(USART_TypeDef *usart, const char *str);
uint8_t uart_read_byte(USART_TypeDef *usart);

/**
 * @brief  printf-style formatted send, straight to any USART - no manual
 *         snprintf/buffer/uart_write dance needed.
 *
 *   uart_printf(USART2, "ping %lu\r\n", (unsigned long)counter);
 *
 * Formats into an internal UART_PRINTF_BUFSZ-byte buffer (128 by default,
 * override with #define UART_PRINTF_BUFSZ before #include if you need more)
 * and blocking-writes the result. If the formatted text is longer than the
 * buffer, it is truncated to fit - it will not overflow or crash.
 *
 * This is independent of uart_printf_redirect()/UART_ENABLE_PRINTF, which
 * only affects the standard C printf() -> stdout. Use uart_printf() when
 * you want formatted output on a specific USART regardless of which one
 * (if any) stdout is redirected to.
 */
void uart_printf(USART_TypeDef *usart, const char *fmt, ...);

/* --- non-blocking polling --- */
uint8_t uart_data_ready(USART_TypeDef *usart); /* RXNE flag */
uint8_t uart_tx_ready(USART_TypeDef *usart);   /* TXE flag */

/**
 * You do NOT need to write USARTx_IRQHandler yourself - this header already
 * defines USART1_IRQHandler, USART2_IRQHandler and USART3_IRQHandler and
 * dispatches to whatever you register with uart_register_rx_callback().
 */
void uart_rx_interrupt_enable(USART_TypeDef *usart);
void uart_rx_interrupt_disable(USART_TypeDef *usart);
void uart_register_rx_callback(USART_TypeDef *usart, uart_rx_callback_t callback);

/**
 * @brief  Built-in, per-USART receive ring buffer - lives entirely inside
 *         this header, so it's always available, works correctly no
 *         matter which USART(s) you use it on simultaneously, and never
 *         corrupts data: each USART gets its own independent buffer, the
 *         ISR only ever writes the head index and the byte, your code
 *         only ever writes the tail index, and a full buffer just drops
 *         incoming bytes rather than overwriting unread ones or spilling
 *         into other memory.
 *
 * Usage:
 *   uart_rx_buffer_enable(USART1);          // once, during setup
 *   ...
 *   while (1) {
 *       int16_t b = uart_read_buffered_byte(USART1);
 *       if (b >= 0) {
 *           // got one byte, 0-255
 *       }
 *
 *       char line[64];
 *       if (uart_read_line(USART1, line, sizeof(line))) {
 *           // got one full line, already null-terminated, no \r\n
 *       }
 *   }
 *
 * Note: this occupies that USART's one available RX callback slot (see
 * uart_register_rx_callback() above) - do not also register your own
 * callback for the same USART while using the ring buffer for it.
 */
void uart_rx_buffer_enable(USART_TypeDef *usart);
void uart_rx_buffer_disable(USART_TypeDef *usart); /* stops new bytes; buffered data is kept */
uint8_t uart_available(USART_TypeDef *usart);       /* 1 if at least one byte is waiting */
int16_t uart_read_buffered_byte(USART_TypeDef *usart); /* -1 if empty, else 0-255 */

/**
 * @brief  Non-blocking line reader built on top of the ring buffer above.
 *         Call this repeatedly (e.g. every main-loop iteration); it drains
 *         whatever bytes have arrived so far and returns 1 exactly once
 *         per complete line (terminated by \r, \n, or \r\n - all treated
 *         as one line ending, and the terminator itself is not included).
 * @param  out_line  destination buffer, always null-terminated on return
 * @param  out_size  size of out_line; longer lines are truncated to fit
 * @return 1 if out_line now holds a complete line, 0 otherwise
 */
uint8_t uart_read_line(USART_TypeDef *usart, char *out_line, uint16_t out_size);

/**
 * @brief  Route stdio (printf, puts, etc.) through a given USART.
 *
 * Opt-in: only takes effect if UART_ENABLE_PRINTF is #defined before this
 * header is included (in the one .c file with the implementation). This
 * overrides the weak _write() syscall stub that STM32CubeIDE generates in
 * syscalls.c - no conflict, since that stub is weak and this is strong.
 *
 * Usage:
 *   #define UART_ENABLE_PRINTF
 *   #include "uart.h"
 *   ...
 *   uart_init(&cfg);
 *   uart_printf_redirect(USART1);
 *   printf("value = %d\r\n", 42);   // now goes out over USART1
 *
 * Make sure your project is linked with --specs=nano.specs (or the regular
 * newlib specs) so printf exists at all on a bare-metal target, and that
 * you actually call uart_init() on the chosen USART before printf-ing.
 */
#ifdef UART_ENABLE_PRINTF
void uart_printf_redirect(USART_TypeDef *usart);
#endif

/* ----------------------------------------------------------------------- */
/*  Implementation                                                           */
/* ----------------------------------------------------------------------- */

#ifndef UART_DECLARATION_ONLY

/* --- internal helpers --- */

static uint8_t uart__index(USART_TypeDef *usart)
{
    if (usart == USART1) return 0U;
    if (usart == USART2) return 1U;
    return 2U; /* USART3, if present */
}

/* ST's standard integer-only BRR calculation (avoids float on Cortex-M3
 * without an FPU). USARTDIV = pclk / (16 * baud), stored as Q12.4. */
static uint16_t uart__compute_brr(uint32_t pclk_hz, uint32_t baud)
{
    uint32_t integer_div = (25U * pclk_hz) / (4U * baud);
    uint32_t tmp = (integer_div / 100U) << 4U;
    uint32_t fractional_div = integer_div - (100U * (tmp >> 4U));
    tmp |= (((fractional_div * 16U) + 50U) / 100U) & 0x0FU;
    return (uint16_t)tmp;
}

/* --- core API --- */

void uart_clock_enable(USART_TypeDef *usart)
{
    if (usart == USART1)      RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    else if (usart == USART2) RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
#ifdef USART3
    else if (usart == USART3) RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
#endif
}

void uart_init(const uart_init_t *cfg)
{
    USART_TypeDef *usart = cfg->instance;

    uart_clock_enable(usart);

    usart->BRR = uart__compute_brr(cfg->pclk_hz, cfg->baudrate);

    uint32_t cr1 = 0;
    cr1 |= (1U << 3); /* TE: transmitter enable */
    cr1 |= (1U << 2); /* RE: receiver enable */

    if (cfg->word_length == UART_WORDLEN_9) {
        cr1 |= (1U << 12); /* M: 9-bit word */
    }

    if (cfg->parity != UART_PARITY_NONE) {
        cr1 |= (1U << 10); /* PCE: parity control enable */
        if (cfg->parity == UART_PARITY_ODD) {
            cr1 |= (1U << 9); /* PS: odd parity */
        }
    }

    usart->CR1 = cr1;
    usart->CR2 = ((uint32_t)cfg->stop_bits & 0x3U) << 12; /* STOP[1:0] */

    uint32_t cr3 = 0;
    if (cfg->flow_control & UART_FLOWCTRL_RTS) cr3 |= (1U << 8); /* RTSE */
    if (cfg->flow_control & UART_FLOWCTRL_CTS) cr3 |= (1U << 9); /* CTSE */
    usart->CR3 = cr3;

    /* Enable the USART peripheral.
     * This must be done after all configuration registers have been set.
     */

    usart->CR1 |= (1U << 13); /* UE: USART enable*/
}

void uart_gpio_pins_init(GPIO_TypeDef *tx_port, uint16_t tx_pin,
                          GPIO_TypeDef *rx_port, uint16_t rx_pin)
{
	/* Configure the TX pin as an Alternate Function Push-Pull output
	 * so the USART peripheral can drive the pin.
	 */

    gpio_pin_config(tx_port, tx_pin, GPIO_MODE_AF_PP_50MHZ);

    /* Configure the RX pin as a floating input so incoming serial
     * data can be received by the USART peripheral.
     */

    gpio_pin_config(rx_port, rx_pin, GPIO_MODE_INPUT_FLOATING);
}

void uart_write_byte(USART_TypeDef *usart, uint8_t byte)
{
	/* Wait until the transmit data register becomes empty,
	 * meaning the next byte can be written safely.
	 */
    while ((usart->SR & (1U << 7)) == 0U) {
    }
    usart->DR = byte;
}

void uart_write(USART_TypeDef *usart, const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        uart_write_byte(usart, data[i]);
    }
}

void uart_write_string(USART_TypeDef *usart, const char *str)
{
    while (*str != '\0') {
        uart_write_byte(usart, (uint8_t)(*str));
        str++;
    }
}

uint8_t uart_read_byte(USART_TypeDef *usart)
{
	/* Wait until a new byte has been received. */
    while ((usart->SR & (1U << 5)) == 0U) {
    }
    return (uint8_t)usart->DR;
}

void uart_printf(USART_TypeDef *usart, const char *fmt, ...)
{
    char buf[UART_PRINTF_BUFSZ];

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len <= 0) {
    	/* Formatting failed.
    	 * Nothing will be transmitted.
    	 */
        return;
    }
    if ((uint32_t)len >= sizeof(buf)) {
    	/* The formatted string exceeded the buffer size.
    	 * Only the portion that fits will be transmitted.
    	 */
        len = (int)sizeof(buf) - 1;
    }

    uart_write(usart, (const uint8_t *)buf, (uint16_t)len);
}

uint8_t uart_data_ready(USART_TypeDef *usart)
{
    return (usart->SR & (1U << 5)) ? 1U : 0U; /* RXNE */
}

uint8_t uart_tx_ready(USART_TypeDef *usart)
{
    return (usart->SR & (1U << 7)) ? 1U : 0U; /* TXE */
}

/*=============================================================
 * UART Receive Interrupt Handling
 *
 * These functions enable interrupt-driven reception and call
 * the user-registered callback whenever a new byte arrives.
 *=============================================================*/

static uart_rx_callback_t uart__rx_callbacks[3] = { 0, 0, 0 };

static void uart__dispatch(USART_TypeDef *usart)
{
    if (usart->SR & (1U << 5)) { /* RXNE */
        uint8_t byte = (uint8_t)usart->DR; /* reading DR clears RXNE (and ORE) */
        uint8_t idx = uart__index(usart);
        if (uart__rx_callbacks[idx] != 0) {
            uart__rx_callbacks[idx](usart, byte);
        }
    }
}

void uart_rx_interrupt_enable(USART_TypeDef *usart)
{
    usart->CR1 |= (1U << 5); /* RXNEIE */

    if (usart == USART1)      NVIC_EnableIRQ(USART1_IRQn);
    else if (usart == USART2) NVIC_EnableIRQ(USART2_IRQn);
#ifdef USART3
    else if (usart == USART3) NVIC_EnableIRQ(USART3_IRQn);
#endif
}

void uart_rx_interrupt_disable(USART_TypeDef *usart)
{
    usart->CR1 &= ~(1U << 5); /* RXNEIE */

    if (usart == USART1)      NVIC_DisableIRQ(USART1_IRQn);
    else if (usart == USART2) NVIC_DisableIRQ(USART2_IRQn);
#ifdef USART3
    else if (usart == USART3) NVIC_DisableIRQ(USART3_IRQn);
#endif
}

void uart_register_rx_callback(USART_TypeDef *usart, uart_rx_callback_t callback)
{
    uart__rx_callbacks[uart__index(usart)] = callback;
}

/*=============================================================
 * Receive Ring Buffer
 *
 * Each UART has its own independent circular buffer for storing
 * incoming bytes. This allows received data to be processed
 * later without losing characters while the CPU is busy.
 *=============================================================*/

typedef struct {
    volatile uint8_t  buf[UART_RING_BUF_SZ];
    volatile uint16_t head; /**< Next write position (updated by ISR) */
    volatile uint16_t tail; /**< Next read position (updated by user code) */
} uart__ring_t;

static uart__ring_t uart__rx_ring[3]; /* index 0=USART1, 1=USART2, 2=USART3 */

typedef struct {
    char    buf[UART_LINE_BUF_SZ];
    uint8_t idx;
    uint8_t just_saw_cr;
} uart__line_t;

static uart__line_t uart__rx_line[3];

/* ISR-side: called via the normal callback mechanism, so it runs with the
 * same guarantees as any other RX callback - fast, no blocking calls. */
static void uart__ring_push(USART_TypeDef *usart, uint8_t byte)
{
    uart__ring_t *r = &uart__rx_ring[uart__index(usart)];
    uint16_t next = (uint16_t)((r->head + 1U) % UART_RING_BUF_SZ);
    if (next != r->tail) {
    	/* If the buffer is full, the new byte is discarded.
    	 * Existing unread data is preserved.
    	 */
        r->buf[r->head] = byte;
        r->head = next;
    }
}

void uart_rx_buffer_enable(USART_TypeDef *usart)
{
    uint8_t idx = uart__index(usart);
    uart__rx_ring[idx].head = 0;
    uart__rx_ring[idx].tail = 0;
    uart__rx_line[idx].idx  = 0;
    uart__rx_line[idx].just_saw_cr = 0;

    uart_register_rx_callback(usart, uart__ring_push);
    uart_rx_interrupt_enable(usart);
}
void uart_rx_buffer_disable(USART_TypeDef *usart)
{
    uart_rx_interrupt_disable(usart); /* buffered bytes are left intact */
}
/**
 * @brief Checks whether unread data exists in the receive buffer.
 *
 * @return
 *      1 -> Data available
 *      0 -> Buffer empty
 */
uint8_t uart_available(USART_TypeDef *usart)
{
    uart__ring_t *r = &uart__rx_ring[uart__index(usart)];
    return (r->head != r->tail) ? 1U : 0U;
}

/**
 * @brief Reads one byte from the receive buffer.
 *
 * @return
 *      0-255 : Received byte
 *      -1    : Buffer empty
 */

int16_t uart_read_buffered_byte(USART_TypeDef *usart)
{
    uart__ring_t *r = &uart__rx_ring[uart__index(usart)];
    if (r->head == r->tail) {
        return -1; /* nothing waiting */
    }
    uint8_t byte = r->buf[r->tail];
    r->tail = (uint16_t)((r->tail + 1U) % UART_RING_BUF_SZ);
    return (int16_t)byte;
}

uint8_t uart_read_line(USART_TypeDef *usart, char *out_line, uint16_t out_size)
{
    uart__ring_t *r = &uart__rx_ring[uart__index(usart)];
    uart__line_t *l = &uart__rx_line[uart__index(usart)];

    while (r->head != r->tail) {
        uint8_t byte = r->buf[r->tail];
        r->tail = (uint16_t)((r->tail + 1U) % UART_RING_BUF_SZ);

        if (byte == '\r' || byte == '\n') {
            /* Any of \r, \n, or \r\n ends a line. To avoid a \r\n pair
             * being seen as two line endings (one real + one blank),
             * skip a lone '\n' that immediately follows a '\r' we just
             * consumed as a terminator. */
            if (byte == '\n' && l->just_saw_cr) {
                l->just_saw_cr = 0U;
                continue;
            }
            l->just_saw_cr = (byte == '\r') ? 1U : 0U;

            uint8_t got_line = (l->idx > 0U) ? 1U : 0U;
            if (got_line && out_size > 0U) {
                uint16_t n = l->idx;
                if (n >= out_size) {
                    n = out_size - 1U; /* truncate to fit caller's buffer */
                }
                for (uint16_t i = 0; i < n; i++) {
                    out_line[i] = l->buf[i];
                }
                out_line[n] = '\0';
            }
            l->idx = 0;
            if (got_line) {
                return 1U; /* one line delivered - stop and let the caller
                             * come back for the next one on a later call */
            }
            /* Ignore empty lines and continue waiting for valid data. */
            continue;
        }

        l->just_saw_cr = 0U; /* any non-terminator byte breaks a \r\n pair */

        if (l->idx < (UART_LINE_BUF_SZ - 1U)) {
            l->buf[l->idx++] = (char)byte;
        }
        /* else: line too long for UART_LINE_BUF_SZ - extra bytes dropped
         * until the next line terminator, rather than corrupting memory */
    }
    /* No complete line has been received yet. */
    return 0U;
}

/* --- USART interrupt vectors --- */
/* These interrupt handlers are provided by the driver.
 * The application only needs to register a callback using
 * uart_register_rx_callback().
 */

void USART1_IRQHandler(void) { uart__dispatch(USART1); }
void USART2_IRQHandler(void) { uart__dispatch(USART2); }
#ifdef USART3
void USART3_IRQHandler(void) { uart__dispatch(USART3); }
#endif

/* --- printf retargeting (opt-in) --- */

#ifdef UART_ENABLE_PRINTF

static USART_TypeDef *uart__printf_instance = 0;

void uart_printf_redirect(USART_TypeDef *usart)
{
    uart__printf_instance = usart;
}

/* Redirects the standard C library output functions
 * (printf, puts, putchar, etc.) to the selected UART.
 *
 * This replaces the weak _write() function generated by
 * STM32CubeIDE.
 */
int _write(int file, char *ptr, int len)
{
    (void)file;
    if (uart__printf_instance != 0) {
        uart_write(uart__printf_instance, (const uint8_t *)ptr, (uint16_t)len);
    }
    return len;
}

#endif /* UART_ENABLE_PRINTF */

#endif /* UART_DECLARATION_ONLY */

#ifdef __cplusplus
}
#endif

#endif /* UART_H */
