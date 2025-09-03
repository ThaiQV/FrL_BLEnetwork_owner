/**
 * @file led_driver.h
 * @brief Flexible multi-LED driver with configurable blink patterns and individual control
 * @author Nghia Hoang
 * @date 2025
 *
 * This driver supports:
 * - Multiple LEDs with independent configuration
 * - Hardware abstraction layer for different MCUs
 * - Individual LED on/off control
 * - Configurable blink patterns (frequency, duty cycle, count)
 * - Continuous and counted blink modes
 * - LED state management
 * - Non-blocking operation
 * - Seamless transition between modes (blink -> on/off -> blink)
 */

#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// CONFIGURATION CONSTANTS
// =============================================================================

#define MAX_LEDS            16       ///< Maximum number of LEDs supported
#define MAX_BLINK_PATTERNS  5        ///< Maximum blink patterns per LED
#define DEFAULT_BLINK_PERIOD_MS 1000 ///< Default blink period (ms)
#define DEFAULT_DUTY_CYCLE  50       ///< Default duty cycle (%)

// =============================================================================
// TYPE DEFINITIONS
// =============================================================================

/**
 * @brief LED pin definitions
 */
typedef enum {
    LED_PIN0 = 0,
    LED_PIN1 = 1,
    LED_PIN2 = 2,
    LED_PIN3 = 3,
    LED_PIN4 = 4,
    LED_PIN5 = 5,
    LED_PIN6 = 6,
    LED_PIN7 = 7,
    LED_PIN8 = 8,
    LED_PIN9 = 9,
    LED_PIN10 = 10,
    LED_PIN11 = 11,
    LED_PIN12 = 12,
    LED_PIN13 = 13,
    LED_PIN14 = 14,
    LED_PIN15 = 15,
    LED_PIN16 = 16,
    LED_PIN17 = 17,
    LED_PIN18 = 18,
    LED_PIN19 = 19,
} led_pin_t;

/**
 * @brief LED operating modes
 */
typedef enum {
    LED_MODE_OFF = 0,           ///< LED is off
    LED_MODE_ON,                ///< LED is constantly on
    LED_MODE_BLINK_CONTINUOUS,  ///< LED blinks continuously
    LED_MODE_BLINK_COUNTED,     ///< LED blinks for specified count then goes to idle
    LED_MODE_PATTERN            ///< LED follows custom pattern
} led_mode_t;

/**
 * @brief LED internal states
 */
typedef enum {
    LED_STATE_IDLE = 0,         ///< LED idle (ready for new commands)
    LED_STATE_ON,               ///< LED constantly on
    LED_STATE_BLINK_ON,         ///< LED in blink cycle - on phase
    LED_STATE_BLINK_OFF,        ///< LED in blink cycle - off phase
    LED_STATE_PATTERN_ACTIVE,   ///< LED executing pattern
    LED_STATE_COMPLETED         ///< LED completed operation, ready for new command
} led_state_t;

/**
 * @brief Hardware abstraction layer configuration
 */
typedef struct {
    /**
     * @brief Initialize all LED pins
     */
    void (*led_pin_init_all)(void);

    /**
     * @brief Initialize LED pin
     * @param pin GPIO pin number
     * @param active_high true if LED is active high, false if active low
     * @return true if initialization successful, false otherwise
     */
    bool (*led_pin_init)(led_pin_t pin, bool active_high);

    /**
     * @brief Set LED pin state
     * @param pin GPIO pin number
     * @param state true to turn LED on, false to turn off
     */
    void (*led_pin_set)(led_pin_t pin, bool state);

    /**
     * @brief Get LED pin state
     * @param pin GPIO pin number
     * @return true if LED is on, false if off
     */
    bool (*led_pin_get)(led_pin_t pin);

    /**
     * @brief Get system time in milliseconds
     * @return Current system time in milliseconds
     */
    uint64_t (*get_system_time_ms)(void);

} led_hal_t;

/**
 * @brief Blink pattern configuration
 */
typedef struct {
    uint32_t period_ms;         ///< Total blink period in milliseconds
    uint8_t duty_cycle;         ///< Duty cycle percentage (0-100)
    uint16_t blink_count;       ///< Number of blinks (0 = infinite)
    uint32_t pause_after_ms;    ///< Pause after pattern completion (ms)
    bool repeat;                ///< Repeat pattern after pause
    bool enabled;               ///< Enable/disable this pattern
} led_blink_pattern_t;

/**
 * @brief LED configuration and state
 */
typedef struct {
    // Hardware configuration
    uint8_t gpio_pin;           ///< GPIO pin number
    bool active_high;           ///< true: active high, false: active low

    // Current mode and pattern
    led_mode_t mode;            ///< Current operating mode
    led_blink_pattern_t patterns[MAX_BLINK_PATTERNS]; ///< Blink patterns
    uint8_t pattern_count;      ///< Number of configured patterns
    uint8_t active_pattern;     ///< Currently active pattern index

    // State callbacks
    void (*on_state_change)(uint8_t led_id, bool led_on); ///< State change callback
    void (*on_blink_complete)(uint8_t led_id);            ///< Blink sequence complete callback
    void (*on_pattern_complete)(uint8_t led_id, uint8_t pattern_id); ///< Pattern complete callback

    // Internal state (do not modify directly)
    led_state_t state;          ///< Current LED internal state
    uint32_t last_change_time;  ///< Last state change time
    uint32_t cycle_start_time;  ///< Current cycle start time
    uint16_t current_blink_count; ///< Current blink count in pattern
    uint16_t target_blink_count; ///< Target blink count for current operation
    bool physical_state;        ///< Current physical LED state (on/off)
    bool enabled;               ///< LED enable/disable flag
    bool initialized;           ///< Hardware initialization flag
    bool operation_complete;    ///< Flag indicating operation completion

} led_config_t;

/**
 * @brief LED manager handle
 */
typedef struct {
    led_config_t leds[MAX_LEDS];    ///< Array of LED configurations
    uint8_t led_count;              ///< Number of registered LEDs
    led_hal_t hal;                  ///< Hardware abstraction layer
    bool initialized;               ///< Manager initialization flag
} led_manager_t;

// =============================================================================
// API FUNCTION DECLARATIONS
// =============================================================================

/**
 * @brief Initialize LED manager with hardware abstraction layer
 * @param hal Pointer to hardware abstraction layer configuration
 * @return true if initialization successful, false otherwise
 */
bool led_manager_init(const led_hal_t *hal);

/**
 * @brief Deinitialize LED manager and all registered LEDs
 */
void led_manager_deinit(void);

/**
 * @brief Add a new LED to the manager
 * @param gpio_pin GPIO pin number
 * @param active_high true if LED is active high, false if active low
 * @return LED ID (0-15) on success, 0xFF on failure
 */
uint8_t led_add(uint8_t gpio_pin, bool active_high);

/**
 * @brief Remove an LED from the manager
 * @param led_id LED ID to remove
 * @return true if successful, false otherwise
 */
bool led_remove(uint8_t led_id);

/**
 * @brief Turn LED on (interrupts any ongoing operation)
 * @param led_id LED ID
 * @return true if successful, false otherwise
 */
bool led_on(uint8_t led_id);

/**
 * @brief Turn LED off (interrupts any ongoing operation)
 * @param led_id LED ID
 * @return true if successful, false otherwise
 */
bool led_off(uint8_t led_id);

/**
 * @brief Toggle LED state (interrupts any ongoing operation)
 * @param led_id LED ID
 * @return true if successful, false otherwise
 */
bool led_toggle(uint8_t led_id);

/**
 * @brief Start simple blink (50% duty cycle, continuous)
 * @param led_id LED ID
 * @param period_ms Blink period in milliseconds
 * @return true if successful, false otherwise
 */
bool led_blink(uint8_t led_id, uint32_t period_ms);

/**
 * @brief Start blink with custom duty cycle (continuous)
 * @param led_id LED ID
 * @param period_ms Blink period in milliseconds
 * @param duty_cycle Duty cycle percentage (0-100)
 * @return true if successful, false otherwise
 */
bool led_blink_duty(uint8_t led_id, uint32_t period_ms, uint8_t duty_cycle);

/**
 * @brief Start counted blink sequence
 * @param led_id LED ID
 * @param period_ms Blink period in milliseconds
 * @param duty_cycle Duty cycle percentage (0-100)
 * @param count Number of blinks (must be > 0)
 * @return true if successful, false otherwise
 */
bool led_blink_count(uint8_t led_id, uint32_t period_ms, uint8_t duty_cycle, uint16_t count);

/**
 * @brief Stop all LED activity and turn off
 * @param led_id LED ID
 * @return true if successful, false otherwise
 */
bool led_stop(uint8_t led_id);

/**
 * @brief Add a blink pattern to an LED
 * @param led_id LED ID
 * @param period_ms Blink period in milliseconds
 * @param duty_cycle Duty cycle percentage (0-100)
 * @param blink_count Number of blinks (0 = infinite)
 * @param pause_after_ms Pause after pattern completion
 * @param repeat Repeat pattern after pause
 * @return Pattern ID (0-4) on success, 0xFF on failure
 */
uint8_t led_add_pattern(uint8_t led_id, uint32_t period_ms, uint8_t duty_cycle,
                       uint16_t blink_count, uint32_t pause_after_ms, bool repeat);

/**
 * @brief Remove a blink pattern from an LED
 * @param led_id LED ID
 * @param pattern_id Pattern ID to remove
 * @return true if successful, false otherwise
 */
bool led_remove_pattern(uint8_t led_id, uint8_t pattern_id);

/**
 * @brief Start executing a specific pattern
 * @param led_id LED ID
 * @param pattern_id Pattern ID to execute
 * @return true if successful, false otherwise
 */
bool led_start_pattern(uint8_t led_id, uint8_t pattern_id);

/**
 * @brief Enable/disable a blink pattern
 * @param led_id LED ID
 * @param pattern_id Pattern ID
 * @param enabled true to enable, false to disable
 * @return true if successful, false otherwise
 */
bool led_set_pattern_enabled(uint8_t led_id, uint8_t pattern_id, bool enabled);

/**
 * @brief Set state change callback
 * @param led_id LED ID
 * @param callback Callback function (NULL to disable)
 * @return true if successful, false otherwise
 */
bool led_set_state_callback(uint8_t led_id, void (*callback)(uint8_t, bool));

/**
 * @brief Set blink complete callback
 * @param led_id LED ID
 * @param callback Callback function (NULL to disable)
 * @return true if successful, false otherwise
 */
bool led_set_blink_complete_callback(uint8_t led_id, void (*callback)(uint8_t));

/**
 * @brief Set pattern complete callback
 * @param led_id LED ID
 * @param callback Callback function (NULL to disable)
 * @return true if successful, false otherwise
 */
bool led_set_pattern_complete_callback(uint8_t led_id, void (*callback)(uint8_t, uint8_t));

/**
 * @brief Enable/disable an LED
 * @param led_id LED ID
 * @param enabled true to enable, false to disable
 * @return true if successful, false otherwise
 */
bool led_set_enabled(uint8_t led_id, bool enabled);

/**
 * @brief Get LED current mode
 * @param led_id LED ID
 * @return Current LED mode
 */
led_mode_t led_get_mode(uint8_t led_id);

/**
 * @brief Get LED current state
 * @param led_id LED ID
 * @return Current LED internal state
 */
led_state_t led_get_state(uint8_t led_id);

/**
 * @brief Check if LED is currently on
 * @param led_id LED ID
 * @return true if LED is physically on, false otherwise
 */
bool led_is_on(uint8_t led_id);

/**
 * @brief Get current blink count in active pattern
 * @param led_id LED ID
 * @return Current blink count (0 if not blinking)
 */
uint16_t led_get_blink_count(uint8_t led_id);

/**
 * @brief Check if LED operation is complete and ready for new command
 * @param led_id LED ID
 * @return true if ready for new command, false otherwise
 */
bool led_is_ready(uint8_t led_id);

/**
 * @brief Check if LED blink sequence is complete
 * @param led_id LED ID
 * @return true if blink sequence completed, false otherwise
 */
bool led_is_blink_complete(uint8_t led_id);

/**
 * @brief Get blink progress as percentage (0.0 - 1.0)
 * @param led_id LED ID
 * @return Progress percentage (0.0 = not started, 1.0 = complete)
 */
float led_get_blink_progress(uint8_t led_id);

/**
 * @brief Clear completion flag and reset LED to idle state
 * @param led_id LED ID
 * @return true if successful, false otherwise
 */
bool led_clear_complete_flag(uint8_t led_id);

/**
 * @brief Process all registered LEDs (call this in main loop)
 * This function should be called regularly (every few milliseconds)
 */
void led_process_all(void);

/**
 * @brief Process a specific LED
 * @param led_id LED ID to process
 * @return true if successful, false otherwise
 */
bool led_process_single(uint8_t led_id);

/**
 * @brief Get LED manager statistics
 * @param total_leds Pointer to store total LED count
 * @param active_leds Pointer to store active LED count
 */
void led_get_stats(uint8_t *total_leds, uint8_t *active_leds);

/**
 * @brief Turn all LEDs off
 */
void led_all_off(void);

/**
 * @brief Turn all LEDs on
 */
void led_all_on(void);

/**
 * @brief Stop all LED activities
 */
void led_all_stop(void);

#ifdef __cplusplus
}
#endif

#endif // LED_DRIVER_H