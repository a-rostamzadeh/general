/**
 * @file    serial.c
 * @brief   Serial communication and debug messaging implementation
 * @author  Abolfazl Rostamzadeh
 * @date    2024
 * @version 1.0
 * 
 * @details This file contains the implementation of the debug messaging
 *          system with circular buffering and UART transmission.
 */

#include "serial.h"

#ifdef DEBUG_MODE

/*============================================================================
 *                         GLOBAL VARIABLES
 *============================================================================*/

/**
 * @brief  Circular buffer for debug messages
 * @note   Initialized to zero, indicating empty slots
 */
volatile uint8_t tX_buff[TX_BUF_COL][TX_BUF_ROW];

/**
 * @brief  Current write position in buffer
 * @note   Wraps around at TX_BUF_COL - 1
 */
volatile uint32_t tx_buff_counter = 0;

/**
 * @brief  Total number of messages sent
 * @note   Incremented after successful transmission
 */
volatile uint32_t total_sent = 0;

/*============================================================================
 *                         FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief  Transmit all pending messages in the debug buffer
 * 
 * @details Sends all unsent messages from the buffer in FIFO order.
 *          The function:
 *          1. Reads from 'last_tx_buff_counter' position
 *          2. Transmits message via UART
 *          3. Clears the message (sets first byte to 0)
 *          4. Advances the read position with wraparound
 *          5. Continues until reaching 'tx_buff_counter'
 * 
 *          The static local variable 'last_tx_buff_counter' persists
 *          between calls, tracking the read position.
 * 
 * @note   This function should be called periodically from the main loop
 *         or after adding new messages to the buffer.
 * 
 * @warning Ensure DEBUG_UART_PORT is properly initialized before calling
 */
void sendTxBuffer(void)
{
    /* Check if debug output is enabled */
    if (!debug_send) {
        return;
    }
    
    /* Static variable tracks last sent position */
    static uint32_t last_tx_buff_counter = 0;
    
    /* Transmit all pending messages */
    while (last_tx_buff_counter != tx_buff_counter) {
        
        /* Check if this slot has a message (non-zero first byte) */
        if (tX_buff[last_tx_buff_counter][0] != 0) {
            
            /* Optional: Send via SWO for debugging */
            #ifdef DEBUG_MODE_SWO
            printf("%s", (const char *)tX_buff[last_tx_buff_counter]);
            #endif
            
            /* Transmit message via UART */
            HAL_UART_Transmit(&DEBUG_UART_PORT,
                              (uint8_t *)tX_buff[last_tx_buff_counter],
                              strlen((const char *)tX_buff[last_tx_buff_counter]),
                              1000);
            
            /* Clear the message (mark slot as empty) */
            tX_buff[last_tx_buff_counter][0] = 0;
            
            /* Increment statistics */
            total_sent++;
        }
        
        /* Advance read position with wraparound */
        if (++last_tx_buff_counter > TX_BUF_COL - 1) {
            last_tx_buff_counter = 0;
        }
    }
}

/**
 * @brief  Format and buffer a debug message
 * 
 * @details Creates a formatted debug message with optional timestamp
 *          and counter filtering. The message is stored in a circular
 *          buffer for later transmission.
 * 
 *          Message format (with timestamp):
 *          "{tick}>{buffer_index} {formatted_message}"
 *          Example: "12345>7 Sensor value: 42\r\n"
 * 
 *          Counter behavior:
 *          - counter == 0: No limit (unlimited messages)
 *          - counter > 0:  Message counter increments, only messages
 *                          where counter <= limit are printed
 * 
 *          Buffer overflow protection:
 *          - If format string >= TX_BUF_ROW, an error message is printed
 *          - The buffer index wraps around at TX_BUF_COL
 * 
 * @param  debug: Enable/disable output (0 = disabled)
 * @param  time: Add timestamp (0 = no timestamp)
 * @param  counter: Message count limit (0 = unlimited)
 * @param  send_now: Send immediately (0 = buffer only, non-zero = send)
 * @param  format: Printf-style format string
 * @param  ...: Variable arguments
 * 
 * @note   Uses vsprintf which doesn't check buffer bounds, so ensure
 *         format strings and arguments produce output < TX_BUF_ROW.
 */
void debug_msg(uint8_t debug, uint8_t time, uint16_t counter, uint8_t send_now,
               const char *format, ...)
{
    /* Exit early if debug is disabled */
    if (!debug) {
        return;
    }
    
    /* Static counter for message limiting */
    static uint16_t debug_msg_counter = 0;
    
    /* Update message counter based on mode */
    if (counter) {
        /* Limited mode: increment counter */
        debug_msg_counter++;
    } else {
        /* Unlimited mode: reset counter */
        debug_msg_counter = 0;
    }
    
    /* Check if we should output this message */
    if (debug_msg_counter > counter) {
        return;
    }
    
    /* Get current buffer position (local copy for atomicity) */
    uint8_t tc = (uint8_t)tx_buff_counter;
    
    /* Initialize variable argument list */
    va_list args;
    va_start(args, format);
    
    /* Track current position in buffer row */
    int space = 0;
    
    /* Add timestamp if requested */
    if (time) {
        sprintf((char *)tX_buff[tc], "%lu>%u ",
                (unsigned long)HAL_GetTick(), tc);
        space = strlen((const char *)tX_buff[tc]);
    }
    
    /* Format the message with bounds checking */
    if (strlen(format) < TX_BUF_ROW) {
        vsprintf((char *)tX_buff[tc] + space, format, args);
    } else {
        /* Format string too long - output error message */
        vsprintf((char *)tX_buff[tc] + space, ">=TX_BUF_ROW strlen error", args);
    }
    
    /* Clean up variable argument list */
    va_end(args);
    
    /* Advance buffer write position with wraparound */
    if (++tx_buff_counter > TX_BUF_COL - 1) {
        tx_buff_counter = 0;
    }
    
    /* Send immediately if requested */
    if (send_now != 0) {
        sendTxBuffer();
    }
}

#endif /* DEBUG_MODE */
