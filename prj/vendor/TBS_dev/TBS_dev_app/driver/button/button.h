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
 * - Simultaneous multi-button interactions (combo buttons)
 * - Chord detection (multiple buttons pressed together)
 * - Sequential button patterns
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
#define MAX_COMBO_BUTTONS   4        ///< Maximum buttons in a combo
#define MAX_COMBOS          8        ///< Maximum number of combo configurations
#define MAX_PATTERN_LENGTH  8        ///< Maximum buttons in a sequence pattern
#define MAX_PATTERNS        4        ///< Maximum number of patterns
#define DEFAULT_DEBOUNCE_TIME   20   ///< Default debounce time (ms)
#define DEFAULT_CLICK_TIMEOUT   500  ///< Default click timeout (ms)
#define DEFAULT_MULTICLICK_TIMEOUT  500  ///< Default multi-click timeout (ms)
#define DEFAULT_CLICK_SEPARATION 150   ///< Default time between clicks (ms)
#define DEFAULT_COMBO_TIMEOUT   1000 ///< Default combo detection timeout (ms)
#define DEFAULT_PATTERN_TIMEOUT 2000 ///< Default pattern timeout (ms)

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
    BUTTON_EVENT_HOLD_END,      ///< Hold ended
    // Multi-button events
    BUTTON_EVENT_COMBO_PRESS,   ///< Multiple buttons pressed together
    BUTTON_EVENT_COMBO_RELEASE, ///< Multiple buttons released together
    BUTTON_EVENT_COMBO_CLICK,   ///< Multiple buttons clicked together
    BUTTON_EVENT_COMBO_HOLD,    ///< Multiple buttons held together
    BUTTON_EVENT_PATTERN_MATCH  ///< Button sequence pattern matched
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
 * @brief Combo detection state
 */
typedef enum {
    COMBO_STATE_IDLE = 0,       ///< No combo in progress
    COMBO_STATE_DETECTING,      ///< Detecting button combination
    COMBO_STATE_ACTIVE,         ///< Combo is active (all buttons pressed)
    COMBO_STATE_HOLD           ///< Combo is being held
} combo_state_t;

/**
 * @brief Pattern detection state
 */
typedef enum {
    PATTERN_STATE_IDLE = 0,     ///< No pattern in progress
    PATTERN_STATE_DETECTING,    ///< Detecting pattern sequence
    PATTERN_STATE_TIMEOUT      ///< Pattern timed out
} pattern_state_t;

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
	BUTTON_PIN11 = 11,
	BUTTON_PIN12 = 12,
	BUTTON_PIN13 = 13,
	BUTTON_PIN14 = 14,
	BUTTON_PIN15 = 15,
	BUTTON_PIN16 = 16,
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
    uint64_t (*get_system_time_ms)(void);

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
 * @brief Button combination configuration
 */
typedef struct {
    uint8_t button_ids[MAX_COMBO_BUTTONS];  ///< Array of button IDs in combo
    uint8_t button_count;                   ///< Number of buttons in combo
    uint32_t detection_window;              ///< Time window for combo detection (ms)
    uint32_t hold_threshold;                ///< Hold threshold for combo hold event (ms)
    bool enabled;                           ///< Enable/disable this combo

    // Callbacks
    void (*on_combo_press)(uint8_t *button_ids, uint8_t count);    ///< Combo press callback
    void (*on_combo_release)(uint8_t *button_ids, uint8_t count);  ///< Combo release callback
    void (*on_combo_click)(uint8_t *button_ids, uint8_t count);    ///< Combo click callback
    void (*on_combo_hold)(uint8_t *button_ids, uint8_t count, uint32_t hold_time); ///< Combo hold callback

    // Internal state
    combo_state_t state;                    ///< Current combo state
    uint32_t first_press_time;              ///< Time of first button press in combo
    uint32_t all_pressed_time;              ///< Time when all buttons were pressed
    uint8_t pressed_mask;                   ///< Bitmask of currently pressed buttons in combo
} button_combo_t;

/**
 * @brief Button sequence pattern configuration
 */
typedef struct {
    uint8_t pattern[MAX_PATTERN_LENGTH];    ///< Sequence of button IDs
    uint8_t pattern_length;                 ///< Length of the pattern
    uint32_t timeout;                       ///< Timeout between button presses (ms)
    bool enabled;                           ///< Enable/disable this pattern

    // Callback
    void (*on_pattern_match)(uint8_t *pattern, uint8_t length); ///< Pattern match callback

    // Internal state
    pattern_state_t state;                  ///< Current pattern detection state
    uint8_t current_position;               ///< Current position in pattern
    uint32_t last_press_time;               ///< Time of last button press in pattern
} button_pattern_t;

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
    void (*on_release)(uint8_t button_id, uint32_t press_duration_ms);             ///< Release event callback
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
    bool in_combo;              ///< True if button is part of active combo

} button_config_t;

/**
 * @brief Global button state tracking for combos and patterns
 */
typedef struct {
    uint16_t pressed_buttons_mask;          ///< Bitmask of currently pressed buttons
    uint32_t global_state_change_time;      ///< Time of last global state change
    uint8_t last_pressed_button;            ///< Last button that was pressed
    uint32_t last_button_press_time;        ///< Time of last button press
} button_global_state_t;

/**
 * @brief Button manager handle
 */
typedef struct {
    button_config_t buttons[MAX_BUTTONS];   ///< Array of button configurations
    uint8_t button_count;                   ///< Number of registered buttons
    button_hal_t hal;                       ///< Hardware abstraction layer
    bool initialized;                       ///< Manager initialization flag

    // Multi-button features
    button_combo_t combos[MAX_COMBOS];      ///< Button combination configurations
    uint8_t combo_count;                    ///< Number of registered combos
    button_pattern_t patterns[MAX_PATTERNS]; ///< Button sequence patterns
    uint8_t pattern_count;                  ///< Number of registered patterns
    button_global_state_t global_state;     ///< Global button state tracking
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
bool button_set_release_callback(uint8_t button_id, void (*callback)(uint8_t button_id, uint32_t press_duration_ms));

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

// =============================================================================
// MULTI-BUTTON COMBO API
// =============================================================================

/**
 * @brief Add a button combination
 * @param button_ids Array of button IDs in the combo
 * @param button_count Number of buttons in combo (2-4)
 * @param detection_window Time window for detecting combo (ms)
 * @return Combo ID on success, 0xFF on failure
 */
uint8_t button_add_combo(uint8_t *button_ids, uint8_t button_count, uint32_t detection_window);

/**
 * @brief Remove a button combination
 * @param combo_id Combo ID to remove
 * @return true if successful, false otherwise
 */
bool button_remove_combo(uint8_t combo_id);

/**
 * @brief Set combo press callback
 * @param combo_id Combo ID
 * @param callback Callback function
 * @return true if successful, false otherwise
 */
bool button_set_combo_press_callback(uint8_t combo_id,
                                     void (*callback)(uint8_t *button_ids, uint8_t count));

/**
 * @brief Set combo release callback
 * @param combo_id Combo ID
 * @param callback Callback function
 * @return true if successful, false otherwise
 */
bool button_set_combo_release_callback(uint8_t combo_id,
                                       void (*callback)(uint8_t *button_ids, uint8_t count));

/**
 * @brief Set combo click callback
 * @param combo_id Combo ID
 * @param callback Callback function
 * @return true if successful, false otherwise
 */
bool button_set_combo_click_callback(uint8_t combo_id,
                                     void (*callback)(uint8_t *button_ids, uint8_t count));

/**
 * @brief Set combo hold callback
 * @param combo_id Combo ID
 * @param hold_threshold Hold time threshold (ms)
 * @param callback Callback function
 * @return true if successful, false otherwise
 */
bool button_set_combo_hold_callback(uint8_t combo_id, uint32_t hold_threshold,
                                    void (*callback)(uint8_t *button_ids, uint8_t count, uint32_t hold_time));

/**
 * @brief Enable/disable a combo
 * @param combo_id Combo ID
 * @param enabled true to enable, false to disable
 * @return true if successful, false otherwise
 */
bool button_set_combo_enabled(uint8_t combo_id, bool enabled);

// =============================================================================
// BUTTON SEQUENCE PATTERN API
// =============================================================================

/**
 * @brief Add a button sequence pattern
 * @param pattern Array of button IDs in sequence
 * @param pattern_length Length of the pattern (2-8)
 * @param timeout Timeout between button presses (ms)
 * @return Pattern ID on success, 0xFF on failure
 */
uint8_t button_add_pattern(uint8_t *pattern, uint8_t pattern_length, uint32_t timeout);

/**
 * @brief Remove a button sequence pattern
 * @param pattern_id Pattern ID to remove
 * @return true if successful, false otherwise
 */
bool button_remove_pattern(uint8_t pattern_id);

/**
 * @brief Set pattern match callback
 * @param pattern_id Pattern ID
 * @param callback Callback function
 * @return true if successful, false otherwise
 */
bool button_set_pattern_callback(uint8_t pattern_id,
                                void (*callback)(uint8_t *pattern, uint8_t length));

/**
 * @brief Enable/disable a pattern
 * @param pattern_id Pattern ID
 * @param enabled true to enable, false to disable
 * @return true if successful, false otherwise
 */
bool button_set_pattern_enabled(uint8_t pattern_id, bool enabled);

/**
 * @brief Reset pattern detection (useful for clearing partial matches)
 * @param pattern_id Pattern ID (0xFF for all patterns)
 * @return true if successful, false otherwise
 */
bool button_reset_pattern(uint8_t pattern_id);

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Get currently pressed buttons
 * @param pressed_buttons Array to store pressed button IDs
 * @param max_buttons Maximum number of buttons to store
 * @return Number of currently pressed buttons
 */
uint8_t button_get_pressed_buttons(uint8_t *pressed_buttons, uint8_t max_buttons);

/**
 * @brief Check if specific buttons are currently pressed
 * @param button_ids Array of button IDs to check
 * @param button_count Number of buttons to check
 * @return true if ALL specified buttons are pressed, false otherwise
 */
bool button_are_buttons_pressed(uint8_t *button_ids, uint8_t button_count);

/**
 * @brief Get combo manager statistics
 * @param total_combos Pointer to store total combo count
 * @param active_combos Pointer to store active combo count
 */
void button_get_combo_stats(uint8_t *total_combos, uint8_t *active_combos);

/**
 * @brief Get pattern manager statistics
 * @param total_patterns Pointer to store total pattern count
 * @param active_patterns Pointer to store active pattern count
 */
void button_get_pattern_stats(uint8_t *total_patterns, uint8_t *active_patterns);

#ifdef __cplusplus
}
#endif

#endif // BUTTON_DRIVER_H
