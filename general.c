#include "general.h"

/*============================================================================
 *                         STATIC VARIABLES
 *============================================================================*/

/* Static variables for functions that need persistence */
static uint32_t rotary_encoder_position[MAX_CHANNELS_NUM][MAX_ENCODER] = {0};
static int32_t rotary_encoder_last_position[MAX_CHANNELS_NUM][MAX_ENCODER] = {0};
static bool key_press_active[MAX_N_S_BTNS_NUM] = {0};
static bool key_release_active[MAX_N_S_BTNS_NUM] = {0};
static bool rotation_active[MAX_ENCODER] = {0};

#ifdef DEBUG_MODE
static uint16_t debug_msg_counter = 0;
#endif

/*============================================================================
 *                         FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief  Microsecond delay using DWT cycle counter (most accurate)
 * @param  us: Delay in microseconds
 * @note   Requires DWT_CYCCNT to be enabled
 *         This is the most accurate method for STM32H7
 */
void delay_us(uint32_t us)
{
    // Get CPU frequency (HAL defined)
    uint32_t cpu_freq_mhz = HAL_RCC_GetSysClockFreq() / 1000000UL;
    
    // Calculate required cycles
    uint32_t cycles = us * cpu_freq_mhz;
    
    // Get current cycle count
    uint32_t start = DWT->CYCCNT;
    
    // Wait for required cycles
    while ((DWT->CYCCNT - start) < cycles) {
        // Busy wait
        __NOP();
    }
}

/**
 * @brief  Initialize DWT for cycle counting
 * @note   Call this once at startup
 */
void DWT_Init(void)
{
    // Enable DWT cycle counter
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
 * @brief  Check if a specified time interval has elapsed
 * @param  tick: Pointer to the last recorded tick value (modified)
 * @param  update_rate_ms: Time interval in milliseconds
 * @return 1 if elapsed, 0 otherwise
 */
uint8_t isTimeElapsed(volatile uint32_t *tick, uint32_t update_rate_ms)
{
    uint32_t now = HAL_GetTick();
    if ((*tick > now) || ((now - *tick) > update_rate_ms)) {
        *tick = now;
        return 1U;
    }
    return 0U;
}

/**
 * @brief  Read rotary encoder position from timer counter
 * @param  ch: Channel number
 * @param  enc: Encoder type (FIRST_ENC, SECOND_ENC, THIRD_ENC, FOURTH_ENC)
 * @param  jitter: If true, adds artificial jitter for testing
 * @return Encoder position (counter value >> 2)
 * @note   Encoder to timer mapping:
 *         FIRST_ENC  -> TIM4  (F control)
 *         SECOND_ENC -> TIM3  (S control)
 *         THIRD_ENC  -> TIM23 (T control)
 */
uint32_t readRotaryEncoder(uint8_t ch, uint8_t enc, bool jitter)
{
    volatile uint32_t timer_value = 0;
    
    /* Select the appropriate timer based on encoder type */
    switch(enc) {
        case FIRST_ENC:   timer_value = TIM4->CNT;  break;
        case SECOND_ENC:  timer_value = TIM3->CNT;  break;
        case THIRD_ENC:   timer_value = TIM23->CNT; break;
        default: return 0U;
    }
    
    /* Artificial jitter for testing */
    if (jitter) {
        rotary_encoder_position[ch][enc] += 4U;
        return 0U;
    }
    
    /* Update position with hysteresis to filter noise */
    if (timer_value >= rotary_encoder_position[ch][enc] + 4U) {
        rotary_encoder_position[ch][enc] = timer_value;
    } else if (timer_value + 4U <= rotary_encoder_position[ch][enc]) {
        rotary_encoder_position[ch][enc] = timer_value;
    }
    
    return (rotary_encoder_position[ch][enc] >> 2U);
}

/**
 * @brief  Get rotary encoder rotation direction
 * @param  ch: Channel number
 * @param  enc: Encoder type (FIRST_ENC, SECOND_ENC, THIRD_ENC, FOURTH_ENC)
 * @note   Updates global arrays: values_by_encoders, encoder_rotation
 */
void getRotaryEncoderDirection(uint8_t ch, uint8_t enc)
{
    /* Read current position and invert direction */
    int32_t current_position = ((int32_t)readRotaryEncoder(ch, enc, false)) * -1;
    
    /* Update current value */
    values_by_encoders[ch][enc] = current_position;
    
    /* Calculate rotation amount (positive = clockwise, negative = counter-clockwise) */
    encoder_rotation[ch][enc] = rotary_encoder_last_position[ch][enc] - current_position;
    rotary_encoder_last_position[ch][enc] = current_position;
    
    /* Handle rotation event signaling (edge detection) */
    if (encoder_rotation[ch][enc] != 0) {
        if (!rotation_active[enc]) {
            rotation_active[enc] = true;
            rotated_key_action[enc] = 1;
            ++signals[ROTATION_SIG];
        }
    } else {
        rotation_active[enc] = false;
    }
}

/**
 * @brief  Test key press/release state and trigger actions
 * @param  port: GPIO port
 * @param  pin: GPIO pin
 * @param  key_num: Key number (index)
 * @note   Updates global arrays for key events
 */
void testKey(GPIO_TypeDef *port, uint16_t pin, uint8_t key_num)
{
    /* Read the key state (active low) */
    if (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) {
        just_pressed_key[key_num] = 1;
        just_released_key[key_num] = 0;
    } else {
        just_released_key[key_num] = 1;
        just_pressed_key[key_num] = 0;
    }
    
    /* Handle key press detection (rising edge) */
    if (just_pressed_key[key_num]) {
        if (!key_press_active[key_num]) {
            key_press_active[key_num] = true;
            if (secondary_btns && (key_num < MAX_SHIFTED_BTN_NUM)) {
                pressed_shifted_key_action[key_num] = 1;
                ++signals[SHIFTED_KEY_PRESS_SIG];
            } else {
                pressed_key_action[key_num] = 1;
                ++signals[KEY_PRESS_SIG];
            }
        }
    } else {
        key_press_active[key_num] = false;
    }
    
    /* Handle key release detection (falling edge) */
    if (just_released_key[key_num]) {
        if (!key_release_active[key_num]) {
            key_release_active[key_num] = true;
            if (secondary_btns && (key_num < MAX_SHIFTED_BTN_NUM)) {
                released_shifted_key_action[key_num] = 1;
                ++signals[SHIFTED_KEY_RELEASE_SIG];
            } else {
                released_key_action[key_num] = 1;
                ++signals[KEY_RELEASE_SIG];
            }
        }
    } else {
        key_release_active[key_num] = false;
    }
}

/*============================================================================
 *                         STRING UTILITY FUNCTIONS
 *============================================================================*/

/**
 * @brief  Check if a string contains only whitespace characters
 * @param  str: Pointer to the string to check
 * @return true if only whitespace or empty, false otherwise
 * @note   Whitespace is defined as space character only (ASCII 0x20)
 *         This function does not check for tab, newline, etc.
 */
bool isOnlyAsciiWhiteSpace(char *str)
{
    if (str == NULL) {
        return true;  /* NULL string considered whitespace-only */
    }
    
    char it = str[0];
    do {
        if (it == 0) {
            return true;  /* End of string - only whitespace found */
        }
        it = *(str++);
    } while (it == ' ');  /* Check for space character only */
    
    return false;  /* Found a non-whitespace character */
}

/*============================================================================
 *                         HARDWARE ID FUNCTIONS
 *============================================================================*/

/**
 * @brief  Read the unique device serial number from Flash
 * @param  address: Offset address within the UDID area (0-11)
 * @return 32-bit value from the specified address
 * @note   The UDID (Unique Device ID) is 12 bytes starting at 0x1FF1E800
 *         The STM32H7 has a 96-bit unique ID
 */
uint32_t flash_func_read_serialnumber(uint32_t address)
{
    /* Read a 32-bit word from the UDID area at the specified offset */
    return *(uint32_t *)(UDID_START + address);
}

/*============================================================================
 *                         LED CONTROL FUNCTIONS
 *============================================================================*/

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
                     bool *btn_blink_state, volatile bool *btn_default_state)
{
    uint32_t current_time = HAL_GetTick();
    
    /* Check if blinking is active and time to toggle */
    if ((*btn_blink > 0 || *btn_blink == -1) && (current_time > *btn_blink_time)) {
        
        /* Decrement counter if not infinite */
        if (*btn_blink > 0) {
            (*btn_blink)--;
        }
        
        /* Update next toggle time (150ms interval) */
        *btn_blink_time = current_time + 150U;
        
        /* Toggle LED state */
        ledSet(led, (GPIO_PinState)(*btn_blink_state), RED_COLOR);
        *btn_blink_state = !(*btn_blink_state);
        
        /* Check if blinking ended */
        if (*btn_blink == 0) {
            /* Return to default state */
            ledSet(led, (GPIO_PinState)(*btn_default_state), RED_COLOR);
        }
    }
}

/**
 * @brief  Drive yellow LEDs with alternating red/green pattern at 1000Hz
 * @note   Yellow color achieved by time-division multiplexing
 *         Pattern: R G G G R G G G R G G G
 */
void driveYellowLeds(void)
{
    #define YELLOW_PATTERN_SIZE 12
    
    /* Yellow color pattern: 0 = Red, 1 = Green */
    static const uint8_t yellow_pattern[YELLOW_PATTERN_SIZE] = 
        {0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1};
    static uint8_t pattern_index = 0;
    
    uint8_t color = yellow_pattern[pattern_index];
    pattern_index = (pattern_index + 1) % YELLOW_PATTERN_SIZE;
    
    for (uint8_t led = STAGE1_LED; led <= STAGE3_LED; led++) {
        if (led_color[led] == YELLOW_COLOR) {
            ledSetRedGreen(led, color);
        }
    }
}

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
                        uint8_t *change, uint16_t switch_time, uint16_t max_time)
{
    /* Increment counter for each cycle */
    (*led_state_count)++;
    
    /* Start of pulse: Turn LED ON */
    if (*led_state_count == 1U) {
        *led_state = 1U;
        *change = 1U;
    }
    
    /* Switch point: Turn LED OFF */
    if (*led_state_count == switch_time) {
        *led_state = 0U;
        *change = 1U;
    }
    
    /* End of period: Reset counter and ensure OFF state */
    if (*led_state_count >= max_time) {
        *led_state_count = 0U;
        *led_state = 0U;
        *change = 1U;
    }
}

/**
 * @brief  Update stage LEDs with dimming effect at 1000Hz
 * @note   Creates dimming effect by PWM at 25% duty cycle
 *         ON for 2 counts, OFF for 2 counts (4 count period)
 *         Called at 1000Hz for smooth dimming
 */
void updateStagesDimLEDs(void)
{
    static uint16_t led_state_counter = 0;
    uint8_t state_changed = 0;
    uint8_t current_state = 0;
    
    /* Generate PWM signal: 25% duty cycle (2 ON, 4 total) */
    getLedState(&led_state_counter, &current_state, &state_changed, 2U, 4U);
    
    /* Only update LEDs when state changes */
    if (state_changed) {
        GPIO_PinState pin_state = (GPIO_PinState)current_state;
        
        /* Update all stage LEDs that are in DIM mode */
        for (uint8_t led = STAGE1_LED; led <= STAGE8_LED; led++) {
            if (led_reg[led] == LED_DIM) {
                /* Set LED state: ON with color, OFF with NO_COLOR */
                ledSet(led, pin_state, current_state ? led_color[led] : NO_COLOR);
            }
        }
    }
}

/**
 * @brief  Set a single LED state
 * @param  led: LED identifier
 * @param  state: GPIO_PIN_SET or GPIO_PIN_RESET
 * @param  color: Color (GREEN_COLOR, RED_COLOR, YELLOW_COLOR, NO_COLOR)
 */
void ledSet(uint8_t led, GPIO_PinState state, uint8_t color)
{
    GPIO_PinState nstate = (GPIO_PinState)(1U - (uint8_t)state);
    
    switch(led) {
        /* Simple single-pin LEDs (on/off only) */
        CASE_LED_WRITE(STATUS_LED);
        CASE_LED_WRITE(POWER_LED);
        CASE_LED_WRITE(TEST_LED);
        
    default:
        /* RGY 2-pin  LEDs (stage LEDs) - Green color */
        if (color == GREEN_COLOR) {
            switch(led) {
                CASE_LED_WRITE_CG(STAGE1_LED);
                CASE_LED_WRITE_CG(STAGE2_LED);
                CASE_LED_WRITE_CG(STAGE3_LED);
                default: break;
            }
        } 
        /* RGY 2-pin  LEDs (stage LEDs) - Red color */
        else if (color == RED_COLOR) {
            switch(led) {
                CASE_LED_WRITE_CR(STAGE1_LED);
                CASE_LED_WRITE_CR(STAGE2_LED);
                CASE_LED_WRITE_CR(STAGE3_LED);
                default: break;
            }
        } 
        /* RGY 2-pin  LEDs (stage LEDs) - Off */
        else if (color == NO_COLOR) {
            switch(led) {
                CASE_LED_WRITE_CN(STAGE1_LED);
                CASE_LED_WRITE_CN(STAGE2_LED);
                CASE_LED_WRITE_CN(STAGE3_LED);
                default: break;
            }
        }
        break;
    }
}

/**
 * @brief  Set RGY 2-pin LED to red or green
 * @param  led: LED identifier
 * @param  color: 0 = red, 1 = green
 */
void ledSetRedGreen(uint8_t led, uint8_t color)
{
    if (color) {
        ledSet(led, GPIO_PIN_SET, GREEN_COLOR);
    } else {
        ledSet(led, GPIO_PIN_SET, RED_COLOR);
    }
}

/**
 * @brief  Set LED color and update register
 * @param  led: LED identifier
 * @param  color: Color to set (GREEN_COLOR, RED_COLOR, YELLOW_COLOR, NO_COLOR)
 */
void ledColorSet(uint8_t led, uint8_t color)
{
    led_color[led] = color;
    if (color != YELLOW_COLOR) {
        ledSet(led, (GPIO_PinState)(color != NO_COLOR), color);
    }
}

/**
 * @brief  Clear all stage LEDs (3 LEDs)
 * @param  ics: unused
 */
void clearAllStagesLEDs(uint8_t ics)
{
    (void)ics;  /* Suppress unused parameter warning */
    
    for (uint8_t i = STAGE1_LED; i <= STAGE3_LED; ++i) {
        led_reg[i] = LED_OFF;
        ledColorSet(i, NO_COLOR);
    }
}

/**
 * @brief  Turn LED on with red color
 * @param  led: LED identifier
 */
void ledOnRed(uint8_t led)
{
    led_reg[led] = LED_ON;
    led_color[led] = RED_COLOR;
    ledSet(led, GPIO_PIN_SET, RED_COLOR);
}

/**
 * @brief  Turn LED on with green color
 * @param  led: LED identifier
 */
void ledOnGreen(uint8_t led)
{
    led_reg[led] = LED_ON;
    led_color[led] = GREEN_COLOR;
    ledSet(led, GPIO_PIN_SET, GREEN_COLOR);
}

/**
 * @brief  Turn LED on with yellow color
 * @param  led: LED identifier
 */
void ledOnYellow(uint8_t led)
{
    led_reg[led] = LED_ON;
    led_color[led] = YELLOW_COLOR;
    ledSet(led, GPIO_PIN_SET, YELLOW_COLOR);
}

/*============================================================================
 *                         TIMER FUNCTIONS
 *============================================================================*/

/**
 * @brief  Set timer frequency by calculating optimal prescaler and auto-reload
 * @param  htim: Timer handle
 * @param  frq_Hz: Desired frequency in Hz
 * @note   Uses TIMER_CLOCK_FREQUENCY_HZ (275 MHz for STM32H7)
 *         Automatically adjusts prescaler if auto-reload exceeds 16-bit limit
 */
void timerSetFreq(TIM_HandleTypeDef *htim, uint32_t frq_Hz)
{
    const uint32_t clock_frq_hz = TIMER_CLOCK_FREQUENCY_HZ;
    
    debug_msg(DEBUG_SETUP, 1, 1, true, 
              "timer_clock_frq=%luMHz\r\n", clock_frq_hz / 1000000UL);
    
    uint32_t prescaler = 0;
    uint32_t auto_reload;
    
    /* Calculate prescaler and auto-reload values */
    while (frq_Hz != 0) {
        /* Calculate auto-reload: atr = clock / (frq * (prescaler + 1)) */
        auto_reload = clock_frq_hz / frq_Hz / (prescaler + 1);
        
        /* Round up based on remainder (for better accuracy) */
        if (((clock_frq_hz * 10UL) % (frq_Hz * (prescaler + 1))) >= 5) {
            auto_reload++;
        }
        
        /* Check if auto-reload fits in 16-bit timer */
        if (auto_reload < 65536UL) {
            auto_reload--;  /* Auto-reload is zero-based */
            __HAL_TIM_SET_PRESCALER(htim, prescaler);
            __HAL_TIM_SET_AUTORELOAD(htim, auto_reload);
            
            debug_msg(DEBUG_SETUP, 0, 3, true,
                      "timerSetFreq=%luHz, prescaler=%lu, auto_reload=%lu\r\n",
                      frq_Hz, prescaler, auto_reload);
            return;
        } else {
            /* Increase prescaler if auto-reload exceeds 16-bit limit */
            prescaler++;
        }
    }
}

/*============================================================================
 *                         DEBUG FUNCTIONS
 *============================================================================*/

#ifdef DEBUG_MODE

/**
 * @brief  Convert a 64-bit number to binary string representation
 * @param  n: Number to convert (up to 64 bits)
 * @param  bits: Number of bits to display (1-64)
 * @return Pointer to static buffer containing binary string
 * @note   Skips leading zeros for cleaner output
 */
char *printBinary(uint64_t n, uint8_t bits)
{
    static char binaryStr[64 + 1];  /* 64 bits + null terminator */
    binaryStr[64] = '\0';
    
    int i = 0;
    bool significantPartStarted = false;
    
    /* Convert from MSB to LSB */
    for (int bit = bits - 1; bit >= 0; bit--) {
        uint64_t mask = 1ULL << bit;
        if ((n & mask) != 0 || significantPartStarted) {
            binaryStr[i++] = (char)('0' + ((n & mask) != 0));
            significantPartStarted = true;
        }
    }
    
    /* If all bits were zero, output a single zero */
    if (!significantPartStarted) {
        binaryStr[i++] = '0';
    }
    
    binaryStr[i] = '\0';
    return binaryStr;
}

#endif /* DEBUG_MODE */
