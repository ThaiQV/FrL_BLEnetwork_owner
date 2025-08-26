/**
 * @file button_driver.h
 * @brief Flexible multi-button driver with configurable hold levels and multi-click detection
 * @author Nghia Hoang
 * @date 2025
 *
 * This driver supports:
 * - Multiple buttons with independent configuration
 * - Hardware abstraction layer for different MCUs
 * - Configurable hold levels (1s, 2s, 3s, etc.)
 * - Press, release, click, and hold events
 * - Multi-click detection (single, double, triple, etc.)
 * - Debounce filtering
 * - Configurable multi-click timeout
 */

#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// CONFIGURATION CONSTANTS
// =============================================================================

#define MAX_BUTTONS         16       ///< Maximum number of buttons supported
#define MAX_HOLD_LEVELS     5        ///< Maximum hold levels per button
#define MAX_MULTI_CLICK     10       ///< Maximum multi-click count supported
#define DEFAULT_DEBOUNCE_TIME   50   ///< Default debounce time (ms)
#define DEFAULT_CLICK_TIMEOUT   500  ///< Default click timeout (ms)
#define DEFAULT_MULTICLICK_TIMEOUT  500  ///< Default multi-click timeout (ms)
#define DEFAULT_CLICK_SEPARATION 150   ///< Default time between clicks (ms)

// =============================================================================
// TYPE DEFINITIONS
// =============================================================================

/**
 * @brief Button event types
 */
typedef enum {
    BUTTON_EVENT_NONE = 0,      ///< No event
    BUTTON_EVENT_PRESS,         ///< Button pressed down
    BUTTON_EVENT_RELEASE,       ///< Button released
    BUTTON_EVENT_CLICK,         ///< Quick press and release (deprecated - use MULTI_CLICK)
    BUTTON_EVENT_MULTI_CLICK,   ///< Multi-click event (1=single, 2=double, etc.)
    BUTTON_EVENT_HOLD_START,    ///< Hold started
    BUTTON_EVENT_HOLD_LEVEL,    ///< Hold level reached
    BUTTON_EVENT_HOLD_END       ///< Hold ended
} button_event_t;

/**
 * @brief Button internal states
 */
typedef enum {
    BUTTON_STATE_IDLE = 0,      ///< Button not pressed
    BUTTON_STATE_DEBOUNCE,      ///< Debouncing
    BUTTON_STATE_PRESSED,       ///< Button confirmed pressed
    BUTTON_STATE_HOLD,          ///< Button being held
    BUTTON_STATE_WAIT_MULTICLICK ///< Waiting for additional clicks
} button_state_t;

/**
 * @brief Button number pin
 */
typedef enum {
	BUTTON_PIN0 = 0,
	BUTTON_PIN1 = 1,
	BUTTON_PIN2 = 2,
	BUTTON_PIN3 = 3,
	BUTTON_PIN4 = 4,
	BUTTON_PIN5 = 5,
	BUTTON_PIN6 = 6,
	BUTTON_PIN7 = 7,
	BUTTON_PIN8 = 8,
	BUTTON_PIN9 = 9,
	BUTTON_PIN10 = 10,
	BUTTON_PIN11 = 11,  // Fixed typo
	BUTTON_PIN12 = 12,
	BUTTON_PIN13 = 13,
	BUTTON_PIN14 = 14,
	BUTTON_PIN15 = 15,
	BUTTON_PIN16 = 16,  // Fixed typo
	BUTTON_PIN17 = 17,
	BUTTON_PIN18 = 18,
	BUTTON_PIN19 = 19,
} button_pin_t;

/**
 * @brief Hardware abstraction layer configuration
 */
typedef struct {

	void (*button_pin_init_all)(void);
    /**
     * @brief Initialize button pin
     * @param pin GPIO pin number
     * @param active_low true if button is active low, false if active high
     * @return true if initialization successful, false otherwise
     */
    bool (*button_pin_init)(button_pin_t pin, bool active_low);

    /**
     * @brief Read button pin state
     * @param pin GPIO pin number
     * @return true if button is pressed (logic level after considering active_low)
     */
    bool (*button_pin_read)(button_pin_t pin);

    /**
     * @brief Get system time in milliseconds
     * @return Current system time in milliseconds
     */
    uint32_t (*get_system_time_ms)(void);

} button_hal_t;

/**
 * @brief Hold level configuration
 */
typedef struct {
    uint32_t hold_time;         ///< Hold time in milliseconds
    void (*callback)(uint8_t button_id, uint32_t actual_hold_time);  ///< Callback function
    bool enabled;               ///< Enable/disable this hold level
} button_hold_level_t;

/**
 * @brief Multi-click configuration
 */
typedef struct {
    uint8_t click_count;        ///< Click count (1=single, 2=double, etc.)
    void (*callback)(uint8_t button_id, uint8_t click_count);  ///< Callback function
    bool enabled;               ///< Enable/disable this multi-click level
} button_multiclick_t;

/**
 * @brief Button configuration and state
 */
typedef struct {
    // Hardware configuration
    uint8_t gpio_pin;           ///< GPIO pin number
    bool active_low;            ///< true: active low, false: active high

    // Timing configuration
    uint32_t debounce_time;     ///< Debounce time in milliseconds
    uint32_t click_timeout;     ///< Click detection timeout in milliseconds
    uint32_t multiclick_timeout; ///< Multi-click timeout in milliseconds

    // Hold levels
    button_hold_level_t hold_levels[MAX_HOLD_LEVELS];  ///< Hold level configurations
    uint8_t hold_level_count;   ///< Number of configured hold levels

    // Multi-click levels
    button_multiclick_t multiclick_levels[MAX_MULTI_CLICK]; ///< Multi-click configurations
    uint8_t multiclick_level_count; ///< Number of configured multi-click levels

    // Event callbacks
    void (*on_press)(uint8_t button_id);               ///< Press event callback
    void (*on_release)(uint8_t button_id);             ///< Release event callback
    void (*on_click)(uint8_t button_id);               ///< Legacy click event callback
    void (*on_multiclick)(uint8_t button_id, uint8_t click_count); ///< Multi-click callback
    void (*on_hold_start)(uint8_t button_id);          ///< Hold start callback
    void (*on_hold_end)(uint8_t button_id);            ///< Hold end callback

    // Internal state (do not modify directly)
    button_state_t state;       ///< Current button state
    uint32_t last_change_time;  ///< Last state change time
    uint32_t press_start_time;  ///< Time when press started
    uint32_t release_time;      ///< Time when button was released
    uint8_t last_hold_level;    ///< Last triggered hold level
    uint8_t click_count;        ///< Current click count
    bool raw_state;             ///< Raw GPIO state
    bool stable_state;          ///< Debounced stable state
    bool enabled;               ///< Button enable/disable flag
    bool initialized;           ///< Hardware initialization flag

} button_config_t;

/**
 * @brief Button manager handle
 */
typedef struct {
    button_config_t buttons[MAX_BUTTONS];   ///< Array of button configurations
    uint8_t button_count;                   ///< Number of registered buttons
    button_hal_t hal;                       ///< Hardware abstraction layer
    bool initialized;                       ///< Manager initialization flag
} button_manager_t;

// =============================================================================
// API FUNCTION DECLARATIONS
// =============================================================================

/**
 * @brief Initialize button manager with hardware abstraction layer
 * @param hal Pointer to hardware abstraction layer configuration
 * @return true if initialization successful, false otherwise
 */
bool button_manager_init(const button_hal_t *hal);

/**
 * @brief Deinitialize button manager and all registered buttons
 */
void button_manager_deinit(void);

/**
 * @brief Add a new button to the manager
 * @param gpio_pin GPIO pin number
 * @param active_low true if button is active low, false if active high
 * @return Button ID (0-15) on success, 0xFF on failure
 */
uint8_t button_add(uint8_t gpio_pin, bool active_low);

/**
 * @brief Remove a button from the manager
 * @param button_id Button ID to remove
 * @return true if successful, false otherwise
 */
bool button_remove(uint8_t button_id);

/**
 * @brief Set press event callback
 * @param button_id Button ID
 * @param callback Callback function (NULL to disable)
 * @return true if successful, false otherwise
 */
bool button_set_press_callback(uint8_t button_id, void (*callback)(uint8_t));

/**
 * @brief Set release event callback
 * @param button_id Button ID
 * @param callback Callback function (NULL to disable)
 * @return true if successful, false otherwise
 */
bool button_set_release_callback(uint8_t button_id, void (*callback)(uint8_t));

/**
 * @brief Set legacy click event callback (single click only)
 * @param button_id Button ID
 * @param callback Callback function (NULL to disable)
 * @return true if successful, false otherwise
 */
bool button_set_click_callback(uint8_t button_id, void (*callback)(uint8_t));

/**
 * @brief Set multi-click event callback
 * @param button_id Button ID
 * @param callback Callback function (NULL to disable)
 * @return true if successful, false otherwise
 */
bool button_set_multiclick_callback(uint8_t button_id,
                                   void (*callback)(uint8_t button_id, uint8_t click_count));

/**
 * @brief Set hold start/end event callbacks
 * @param button_id Button ID
 * @param on_start Hold start callback (NULL to disable)
 * @param on_end Hold end callback (NULL to disable)
 * @return true if successful, false otherwise
 */
bool button_set_hold_callbacks(uint8_t button_id,
                               void (*on_start)(uint8_t),
                               void (*on_end)(uint8_t));

/**
 * @brief Add a hold level to a button
 * @param button_id Button ID
 * @param hold_time_ms Hold time in milliseconds
 * @param callback Callback function when hold level is reached
 * @return true if successful, false otherwise (max levels reached)
 */
bool button_add_hold_level(uint8_t button_id,
                          uint32_t hold_time_ms,
                          void (*callback)(uint8_t, uint32_t));

/**
 * @brief Remove a hold level from a button
 * @param button_id Button ID
 * @param level_index Hold level index (0-4)
 * @return true if successful, false otherwise
 */
bool button_remove_hold_level(uint8_t button_id, uint8_t level_index);

/**
 * @brief Enable/disable a specific hold level
 * @param button_id Button ID
 * @param level_index Hold level index (0-4)
 * @param enabled true to enable, false to disable
 * @return true if successful, false otherwise
 */
bool button_set_hold_level_enabled(uint8_t button_id, uint8_t level_index, bool enabled);

/**
 * @brief Add a multi-click level to a button
 * @param button_id Button ID
 * @param click_count Click count (1=single, 2=double, 3=triple, etc.)
 * @param callback Callback function when multi-click is detected
 * @return true if successful, false otherwise (max levels reached)
 */
bool button_add_multiclick_level(uint8_t button_id,
                                uint8_t click_count,
                                void (*callback)(uint8_t, uint8_t));

/**
 * @brief Remove a multi-click level from a button
 * @param button_id Button ID
 * @param level_index Multi-click level index (0-9)
 * @return true if successful, false otherwise
 */
bool button_remove_multiclick_level(uint8_t button_id, uint8_t level_index);

/**
 * @brief Enable/disable a specific multi-click level
 * @param button_id Button ID
 * @param level_index Multi-click level index (0-9)
 * @param enabled true to enable, false to disable
 * @return true if successful, false otherwise
 */
bool button_set_multiclick_level_enabled(uint8_t button_id, uint8_t level_index, bool enabled);

/**
 * @brief Set debounce time for a button
 * @param button_id Button ID
 * @param debounce_ms Debounce time in milliseconds
 * @return true if successful, false otherwise
 */
bool button_set_debounce_time(uint8_t button_id, uint32_t debounce_ms);

/**
 * @brief Set click timeout for a button
 * @param button_id Button ID
 * @param timeout_ms Click timeout in milliseconds
 * @return true if successful, false otherwise
 */
bool button_set_click_timeout(uint8_t button_id, uint32_t timeout_ms);

/**
 * @brief Set multi-click timeout for a button
 * @param button_id Button ID
 * @param timeout_ms Multi-click timeout in milliseconds
 * @return true if successful, false otherwise
 */
bool button_set_multiclick_timeout(uint8_t button_id, uint32_t timeout_ms);

/**
 * @brief Enable/disable a button
 * @param button_id Button ID
 * @param enabled true to enable, false to disable
 * @return true if successful, false otherwise
 */
bool button_set_enabled(uint8_t button_id, bool enabled);

/**
 * @brief Get button current state
 * @param button_id Button ID
 * @return Current button state
 */
button_state_t button_get_state(uint8_t button_id);

/**
 * @brief Check if button is currently pressed
 * @param button_id Button ID
 * @return true if pressed, false otherwise
 */
bool button_is_pressed(uint8_t button_id);

/**
 * @brief Get current hold time if button is being held
 * @param button_id Button ID
 * @return Hold time in milliseconds (0 if not holding)
 */
uint32_t button_get_hold_time(uint8_t button_id);

/**
 * @brief Get current click count if waiting for multi-click
 * @param button_id Button ID
 * @return Current click count (0 if not in multi-click state)
 */
uint8_t button_get_click_count(uint8_t button_id);

/**
 * @brief Process all registered buttons (call this in main loop)
 * This function should be called regularly (every few milliseconds)
 */
void button_process_all(void);

/**
 * @brief Process a specific button
 * @param button_id Button ID to process
 * @return true if successful, false otherwise
 */
bool button_process_single(uint8_t button_id);

/**
 * @brief Get button manager statistics
 * @param total_buttons Pointer to store total button count
 * @param active_buttons Pointer to store active button count
 */
void button_get_stats(uint8_t *total_buttons, uint8_t *active_buttons);

#ifdef __cplusplus
}
#endif

#endif // BUTTON_DRIVER_H
