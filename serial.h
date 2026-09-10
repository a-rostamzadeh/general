/**
 * @file    serial.h
 * @brief   Serial communication and debug messaging interface
 * @author  Your Name
 * @date    2024
 * @version 1.0
 * 
 * @details This module provides buffered UART transmission with debug
 *          message formatting capabilities. It supports timestamped
 *          debug output with circular buffering to prevent blocking.
 * 
 * @note    Only available when DEBUG_MODE is defined.
 */

/* Define to prevent recursive inclusion */
#ifndef __SERIAL_H
#define __SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "main.h"

#ifdef DEBUG_MODE
#include <stdarg.h>
#include <string.h>

/*============================================================================
 *                         CONFIGURATION
 *============================================================================*/

/**
 * @brief  UART port used for debug output
 * @note   Change this to match your hardware configuration
 */
#ifndef DEBUG_UART_PORT
#define DEBUG_UART_PORT huart6
#endif

/**
 * @brief  Debug buffer dimensions
 * @note   TX_BUF_COL: Maximum number of buffered messages (1-254)
 *         TX_BUF_ROW: Maximum length of each message (1-254)
 */
#define TX_BUF_COL     50   /* Max number of messages in buffer */
#define TX_BUF_ROW     150  /* Max length of each message */

/* Compile-time validation of buffer dimensions */
#if ((TX_BUF_COL < 1) || (TX_BUF_COL > 254))
#error "TX_BUF_COL is out of range! Must be 1-254"
#endif
#if ((TX_BUF_ROW < 1) || (TX_BUF_ROW > 254))
#error "TX_BUF_ROW is out of range! Must be 1-254"
#endif

/*============================================================================
 *                         EXTERNAL VARIABLES
 *============================================================================*/

/**
 * @brief  Circular buffer for debug messages
 * @note   Each row is a null-terminated string
 *         Row [0] == 0 indicates an empty slot
 */
extern volatile uint8_t tX_buff[TX_BUF_COL][TX_BUF_ROW];

/**
 * @brief  Current write position in the buffer
 * @note   Incremented after each message is added
 */
extern volatile uint32_t tx_buff_counter;

/**
 * @brief  Total number of messages sent
 * @note   Useful for statistics and debugging
 */
extern volatile uint32_t total_sent;

/*============================================================================
 *                         FUNCTION PROTOTYPES
 *============================================================================*/

/**
 * @brief  Transmit all pending messages in the debug buffer
 * @note   Sends messages from last position to current position
 *         Handles circular buffer wraparound
 *         Clears sent messages by setting first byte to 0
 * 
 * @details This function transmits all unsent messages in the buffer.
 *          It maintains a static local counter to track the last sent
 *          position and transmits messages sequentially.
 * 
 *          The function is safe to call from the main loop. For
 *          interrupt-driven transmission, ensure proper synchronization.
 */
void sendTxBuffer(void);

/**
 * @brief  Format and buffer a debug message
 * @param  debug: Enable/disable output (0 = disabled, non-zero = enabled)
 * @param  time: Add timestamp to message (0 = no, non-zero = yes)
 * @param  counter: Message counter limit (0 = unlimited, >0 = max messages)
 * @param  send_now: Send buffer immediately (0 = buffer only, non-zero = send)
 * @param  format: Printf-style format string
 * @param  ...: Variable arguments for format string
 * 
 * @details Formats a debug message using printf-style formatting and
 *          stores it in the circular buffer. Optionally prepends a
 *          timestamp and message counter.
 * 
 *          Message counter behavior:
 *          - counter == 0: No limit, all messages printed
 *          - counter > 0:  Only first 'counter' messages printed
 * 
 *          The timestamp format is: "{tick}>{buffer_index} "
 *          Example: "12345>7 Hello World\r\n"
 * 
 * @note   When DEBUG_MODE is not defined, this function is replaced
 *         with an empty macro to eliminate all debug code.
 */
void debug_msg(uint8_t debug, uint8_t time, uint16_t counter, uint8_t send_now,
               const char *format, ...);

#else
/* No-op macro when debug mode is disabled */
#define debug_msg(...)  ((void)0)
#endif /* DEBUG_MODE */

#ifdef __cplusplus
}
#endif
#endif /* __SERIAL_H */