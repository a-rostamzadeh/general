/* Define to prevent recursive inclusion */
#ifndef __GENERAL_H
#define __GENERAL_H

#ifdef __cplusplus
extern "C" {
#endif
 
#include "main.h"  
#include <stdio.h> 
#include <stdbool.h>  
#include <stdarg.h>  
#include <string.h>
#include "serial.h"
#include "stm32h7xx_it.h"

/*============================================================================
 *                              MACROS & DEFINES
 *============================================================================*/

/* Pin manipulation macros */
#define pin_Pin(pin)           (pin##_Pin)
#define pin_Pin_A(pin)         (pin##A_Pin) 
#define pin_Pin_B(pin)         (pin##B_Pin) 
#define Pin_state(state)       (GPIO_PIN_##state)  

#define READ_PIN(pin)           HAL_GPIO_ReadPin(pin##_GPIO_Port, pin_Pin(pin))
#define WRITE_PIN(pin, state)   HAL_GPIO_WritePin(pin##_GPIO_Port, pin_Pin(pin), Pin_state(state))
#define WRITE_PIN_S(pin, state) HAL_GPIO_WritePin(pin##_GPIO_Port, pin_Pin(pin), state)
#define WRITE_PIN_SA(pin, state) HAL_GPIO_WritePin(pin##A_GPIO_Port, pin_Pin_A(pin), state)
#define WRITE_PIN_SB(pin, state) HAL_GPIO_WritePin(pin##B_GPIO_Port, pin_Pin_B(pin), state)
#define TOGGLE_PIN(pin)         HAL_GPIO_TogglePin(pin##_GPIO_Port, pin_Pin(pin))

/* Bit manipulation macros */
#define BIT_SET(byte, nbit)     ((byte)  |=  (1U << (nbit)))
#define BIT_CLEAR(byte, nbit)   ((byte)  &= ~(1U << (nbit)))
#define BIT_FLIP(byte, nbit)    ((byte)  ^=  (1U << (nbit)))
#define BIT_CHECK_W(word, nbit) ((word)  &   ((uint16_t)1U << (nbit)))
#define BIT_CHECK_DW(dword, nbit) ((dword) & ((uint32_t)1U << (nbit)))
#define CHECK_BIT(var, pos)     ((var) & (1U << (pos)))
  
/* Timer clock frequency for STM32H7 (275 MHz) */
#define TIMER_CLOCK_FREQUENCY_HZ  275000000UL

/* Unique Device ID start address for STM32H7 */
#define UDID_START  0x1FF1E800UL

/*============================================================================
 *                         ENCODER DEFINITIONS
 *============================================================================*/

/**
 * @brief  Encoder type definitions
 * @note   These define the available rotary encoders in the system
 *         Each encoder is mapped to a specific hardware timer
 */
#define FIRST_ENC    0  /* First encoder (e.g., F control) */
#define SECOND_ENC   1  /* Second encoder (e.g., S control) */
#define THIRD_ENC    2  /* Third encoder (e.g., T control) */


/*============================================================================
 *                         LED CONTROL MACROS
 *============================================================================*/

/* LED control macros - 3 representative examples */
#define CASE_LED_WRITE(lede)     case lede: WRITE_PIN_S(lede, state); break
#define CASE_LED_WRITE_CR(lede)  case lede: WRITE_PIN_SA(lede, state); WRITE_PIN_SB(lede, nstate); break
#define CASE_LED_WRITE_CG(lede)  case lede: WRITE_PIN_SA(lede, nstate); WRITE_PIN_SB(lede, state); break
#define CASE_LED_WRITE_CN(lede)  case lede: WRITE_PIN_SA(lede, nstate); WRITE_PIN_SB(lede, nstate); break

/*============================================================================
 *                         FUNCTION PROTOTYPES
 *============================================================================*/

/**
 * @brief  Microsecond delay using DWT cycle counter (most accurate)
 * @param  us: Delay in microseconds
 * @note   Requires DWT_CYCCNT to be enabled
 *         This is the most accurate method for STM32H7
 */
void delay_us(uint32_t delay);

/**
 * @brief  Check if a specified time interval has elapsed
 * @param  tick: Pointer to the last recorded tick value (modified)
 * @param  update_rate_ms: Time interval in milliseconds
 * @return 1 if elapsed, 0 otherwise
 */
uint8_t isTimeElapsed(volatile uint32_t *tick, uint32_t update_rate_ms);

/**
 * @brief  Read rotary encoder position from timer counter
 * @param  ch: Channel number
 * @param  enc: Encoder type (FIRST_ENC, SECOND_ENC, THIRD_ENC, FOURTH_ENC)
 * @param  jitter: If true, adds artificial jitter for testing
 * @return Encoder position (counter value >> 2)
 */
uint32_t readRotaryEncoder(uint8_t ch, uint8_t enc, bool jitter);

/**
 * @brief  Get rotary encoder rotation direction
 * @param  ch: Channel number
 * @param  enc: Encoder type (FIRST_ENC, SECOND_ENC, THIRD_ENC, FOURTH_ENC)
 * @note   Updates global arrays: values_by_encoders, encoder_rotation
 */
void getRotaryEncoderDirection(uint8_t ch, uint8_t enc);

/**
 * @brief  Test key press/release state and trigger actions
 * @param  port: GPIO port
 * @param  pin: GPIO pin
 * @param  key_num: Key number (index)
 */
void testKey(GPIO_TypeDef *port, uint16_t pin, uint8_t key_num);

/**
 * @brief  Check if a string contains only whitespace characters
 * @param  str: Pointer to the string to check
 * @return true if only whitespace or empty, false otherwise
 */
bool isOnlyAsciiWhiteSpace(char *str);

/**
 * @brief  Read the unique device serial number from Flash
 * @param  address: Offset address within the UDID area (0-11)
 * @return 32-bit value from the specified address
 */
uint32_t flash_func_read_serialnumber(uint32_t address);

/**
 * @brief  Blink an LED with configurable timing and default state
 * @param  led: LED identifier to blink
 * @param  btn_blink: Pointer to blink counter (-1 for infinite, >0 for count)
 * @param  btn_blink_time: Pointer to last blink time (modified)
 * @param  btn_blink_state: Pointer to current blink state (true=ON, false=OFF)
 * @param  btn_default_state: Pointer to default LED state (true=ON, false=OFF)
 * @note   Blinks at 150ms intervals
 *         When btn_blink reaches 0, LED returns to default state
 */

void blinkLED(uint8_t led, int8_t *btn_blink, uint32_t *btn_blink_time, 
                     bool *btn_blink_state, volatile bool *btn_default_state);
					 
/**
 * @brief  Drive yellow LEDs with alternating red/green pattern at 1000Hz
 * @note   Yellow color achieved by time-division multiplexing
 *         Pattern: R G G G R G G G R G G G
 */
void driveYellowLeds(void);

/**
 * @brief  Generate a periodic pulse waveform for LED dimming
 * @param  led_state_count: Pointer to counter tracking pulse position
 * @param  led_state: Pointer to current LED state (0=OFF, 1=ON)
 * @param  change: Pointer to flag indicating state change
 * @param  switch_time: Time when LED turns OFF (pulse width)
 * @param  max_time: Total period (cycle length)
 * @note   Creates a PWM-like signal: ON for 'switch_time' counts, OFF for rest
 *         Example: switch_time=2, max_time=4 → 50% duty cycle
 */
void getLedState(uint16_t *led_state_count, uint8_t *led_state, 
                        uint8_t *change, uint16_t switch_time, uint16_t max_time);

/**
 * @brief  Update stage LEDs with dimming effect at 1000Hz
 * @note   Creates dimming effect by PWM at 25% duty cycle
 *         ON for 2 counts, OFF for 2 counts (4 count period)
 *         Called at 1000Hz for smooth dimming
 */
void updateStagesDimLEDs(void);

/**
 * @brief  Set a single LED state
 * @param  led: LED identifier
 * @param  state: GPIO_PIN_SET or GPIO_PIN_RESET
 * @param  color: Color (GREEN_COLOR, RED_COLOR, YELLOW_COLOR, NO_COLOR)
 */
void ledSet(uint8_t led, GPIO_PinState state, uint8_t color);

/**
 * @brief  Set RGY 2-pin LED to red or green
 * @param  led: LED identifier
 * @param  color: 0 = red, 1 = green
 */
void ledSetRedGreen(uint8_t led, uint8_t color);

/**
 * @brief  Set LED color and update register
 * @param  led: LED identifier
 * @param  color: Color to set (GREEN_COLOR, RED_COLOR, YELLOW_COLOR, NO_COLOR)
 */
void ledColorSet(uint8_t led, uint8_t color);

/**
 * @brief  Clear all stage LEDs (3 LEDs)
 * @param  ics: unused
 */
void clearAllStagesLEDs(uint8_t ics);

/**
 * @brief  Turn LED on with red color
 * @param  led: LED identifier
 */
void ledOnRed(uint8_t led);

/**
 * @brief  Turn LED on with green color
 * @param  led: LED identifier
 */
void ledOnGreen(uint8_t led);

/**
 * @brief  Turn LED on with yellow color
 * @param  led: LED identifier
 */
void ledOnYellow(uint8_t led);

/**
 * @brief  Set timer frequency by configuring prescaler and auto-reload
 * @param  htim: Timer handle
 * @param  frq_Hz: Desired frequency in Hz
 */
void timerSetFreq(TIM_HandleTypeDef *htim, uint32_t frq_Hz);

#ifdef DEBUG_MODE

/**
 * @brief  Convert a number to binary string (debugging)
 * @param  n: Number to convert
 * @param  bits: Number of bits to display
 * @return Pointer to static binary string
 */
char *printBinary(uint64_t n, uint8_t bits);
#endif

/*============================================================================
 *                         EXTERNAL VARIABLES
 *============================================================================*/

/* External variables used by the functions */
extern volatile uint8_t just_pressed_key[];
extern volatile uint8_t just_released_key[];
extern volatile uint8_t pressed_key_action[];
extern volatile uint8_t released_key_action[];
extern volatile uint8_t pressed_shifted_key_action[];
extern volatile uint8_t released_shifted_key_action[];
extern volatile uint8_t rotated_key_action[];
extern volatile bool secondary_btns;
extern volatile int32_t signals[];
extern volatile int32_t values_by_encoders[MAX_CHANNELS_NUM][MAX_ENCODER];
extern volatile int32_t encoder_rotation[MAX_CHANNELS_NUM][MAX_ENCODER];
extern volatile uint8_t led_reg[];
extern volatile uint8_t led_color[];
extern volatile uint8_t encoder_rotation_signal[];

#ifdef __cplusplus
}
#endif
#endif /* __GENERAL_H */
