/**
 * @file button_driver.c
 * @brief Implementation of flexible multi-button driver with multi-click support
 * @author Nghia Hoang
 * @date 2025
 */
#include "../../user_lib.h"

#include "button.h"
#include <string.h>
#include <stdio.h>

#define BUTTON_DRV_DEBUG			0
#if	U_APP_DEBUG
#define DB_LOG_BT(...)	//ULOGA(__VA_ARGS__)
#else
#define DB_LOG_BT(...)
#endif

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

static button_manager_t g_button_manager = {0};

// =============================================================================
// PRIVATE FUNCTION DECLARATIONS
// =============================================================================

static bool button_validate_id(uint8_t button_id);
static void button_process_state_machine(uint8_t button_id);
static void button_trigger_event(uint8_t button_id, button_event_t event, uint32_t data);
static void button_trigger_multiclick(uint8_t button_id);
static void button_sort_multiclick_levels(uint8_t button_id);

// **NEW**: Multi-button private functions
static void button_update_global_state(void);
static void button_process_combos(void);
static void button_process_patterns(void);
static bool button_validate_combo_id(uint8_t combo_id);
static bool button_validate_pattern_id(uint8_t pattern_id);
static void button_trigger_combo_event(uint8_t combo_id, button_event_t event, uint32_t data);
static void button_trigger_pattern_event(uint8_t pattern_id);
static bool button_is_combo_complete(uint8_t combo_id);
static bool button_check_pattern_step(uint8_t pattern_id, uint8_t button_id);

// =============================================================================
// PUBLIC API IMPLEMENTATION
// =============================================================================

bool button_manager_init(const button_hal_t *hal) {
    if (!hal || !hal->button_pin_init_all || !hal->button_pin_init || !hal->button_pin_read || !hal->get_system_time_ms) {
        return false;
    }

    // Clear manager structure
    memset(&g_button_manager, 0, sizeof(button_manager_t));

    // Copy HAL functions
    g_button_manager.hal = *hal;
    g_button_manager.initialized = true;
    g_button_manager.hal.button_pin_init_all();

    // **NEW**: Initialize global state for multi-button features
    g_button_manager.global_state.pressed_buttons_mask = 0;
    g_button_manager.global_state.global_state_change_time = 0;
    g_button_manager.global_state.last_pressed_button = 0xFF;
    g_button_manager.global_state.last_button_press_time = 0;

    DB_LOG_BT("[BTN_MGR] Initialized with HAL and multi-button support\n");
    return true;
}

void button_manager_deinit(void) {
    g_button_manager.initialized = false;
    g_button_manager.button_count = 0;
    g_button_manager.combo_count = 0;      // **NEW**
    g_button_manager.pattern_count = 0;    // **NEW**
    memset(&g_button_manager.buttons, 0, sizeof(g_button_manager.buttons));
    memset(&g_button_manager.combos, 0, sizeof(g_button_manager.combos));        // **NEW**
    memset(&g_button_manager.patterns, 0, sizeof(g_button_manager.patterns));    // **NEW**
    memset(&g_button_manager.global_state, 0, sizeof(g_button_manager.global_state)); // **NEW**
    DB_LOG_BT("[BTN_MGR] Deinitialized\n");
}

uint8_t button_add(uint8_t gpio_pin, bool active_low) {
    if (!g_button_manager.initialized) {
        DB_LOG_BT("[BTN] Error: Manager not initialized\n");
        return 0xFF;
    }

    if (g_button_manager.button_count >= MAX_BUTTONS) {
        DB_LOG_BT("[BTN] Error: Maximum buttons reached (%d)\n", MAX_BUTTONS);
        return 0xFF;
    }

    // Check for duplicate pin
    for (uint8_t i = 0; i < g_button_manager.button_count; i++) {
        if (g_button_manager.buttons[i].gpio_pin == gpio_pin) {
            DB_LOG_BT("[BTN] Error: GPIO pin %d already used\n", gpio_pin);
            return 0xFF;
        }
    }

    uint8_t button_id = g_button_manager.button_count;
    button_config_t *btn = &g_button_manager.buttons[button_id];

    // Initialize hardware
    if (!g_button_manager.hal.button_pin_init(gpio_pin, active_low)) {
        DB_LOG_BT("[BTN] Error: Failed to initialize GPIO pin %d\n", gpio_pin);
        return 0xFF;
    }

    // Configure button
    btn->gpio_pin = gpio_pin;
    btn->active_low = active_low;
    btn->debounce_time = DEFAULT_DEBOUNCE_TIME;
    btn->click_timeout = DEFAULT_CLICK_TIMEOUT;
    btn->multiclick_timeout = DEFAULT_MULTICLICK_TIMEOUT;
    btn->state = BUTTON_STATE_IDLE;
    btn->enabled = true;
    btn->initialized = true;
    btn->hold_level_count = 0;
    btn->multiclick_level_count = 0;
    btn->last_hold_level = 0;
    btn->click_count = 0;
    btn->in_combo = false;  // **NEW**: Initialize combo state

    // Clear all callbacks
    btn->on_press = NULL;
    btn->on_release = NULL;
    btn->on_click = NULL;
    btn->on_multiclick = NULL;
    btn->on_hold_start = NULL;
    btn->on_hold_end = NULL;

    // Clear hold levels and multi-click levels
    memset(btn->hold_levels, 0, sizeof(btn->hold_levels));
    memset(btn->multiclick_levels, 0, sizeof(btn->multiclick_levels));

    g_button_manager.button_count++;

    DB_LOG_BT("[BTN] Added button %d on GPIO %d (active_%s)\n",
           button_id, gpio_pin, active_low ? "low" : "high");

    return button_id;
}

bool button_remove(uint8_t button_id) {
    if (!button_validate_id(button_id)) {
        return false;
    }

    // **NEW**: Remove button from any combos that contain it
    for (uint8_t i = 0; i < g_button_manager.combo_count; i++) {
        button_combo_t *combo = &g_button_manager.combos[i];
        for (uint8_t j = 0; j < combo->button_count; j++) {
            if (combo->button_ids[j] == button_id) {
                // Found combo containing this button - remove it
                button_remove_combo(i);
                break;
            }
        }
    }

    // **NEW**: Remove button from any patterns that contain it
    for (uint8_t i = 0; i < g_button_manager.pattern_count; i++) {
        button_pattern_t *pattern = &g_button_manager.patterns[i];
        for (uint8_t j = 0; j < pattern->pattern_length; j++) {
            if (pattern->pattern[j] == button_id) {
                // Found pattern containing this button - remove it
                button_remove_pattern(i);
                break;
            }
        }
    }

    button_config_t *btn = &g_button_manager.buttons[button_id];

    // Disable and clear button
    btn->enabled = false;
    btn->initialized = false;
    memset(btn, 0, sizeof(button_config_t));

    DB_LOG_BT("[BTN] Removed button %d\n", button_id);
    return true;
}

// ... [Keep all existing callback functions as they were in original] ...

bool button_set_press_callback(uint8_t button_id, void (*callback)(uint8_t)) {
    if (!button_validate_id(button_id)) {
        return false;
    }

    g_button_manager.buttons[button_id].on_press = callback;
    return true;
}

bool button_set_release_callback(uint8_t button_id,void (*callback)(uint8_t button_id, uint32_t press_duration_ms)) {
    if (!button_validate_id(button_id)) {
        return false;
    }

    g_button_manager.buttons[button_id].on_release = callback;
    return true;
}

bool button_set_click_callback(uint8_t button_id, void (*callback)(uint8_t)) {
    if (!button_validate_id(button_id)) {
        return false;
    }

    g_button_manager.buttons[button_id].on_click = callback;
    return true;
}

bool button_set_multiclick_callback(uint8_t button_id,
                                   void (*callback)(uint8_t button_id, uint8_t click_count)) {
    if (!button_validate_id(button_id)) {
        return false;
    }
    
    g_button_manager.buttons[button_id].on_multiclick = callback;
    return true;
}

bool button_set_hold_callbacks(uint8_t button_id,
                               void (*on_start)(uint8_t),
                               void (*on_end)(uint8_t)) {
    if (!button_validate_id(button_id)) {
        return false;
    }

    button_config_t *btn = &g_button_manager.buttons[button_id];
    btn->on_hold_start = on_start;
    btn->on_hold_end = on_end;
    return true;
}

// ... [Keep all existing level management functions as they were] ...

bool button_add_hold_level(uint8_t button_id,
                          uint32_t hold_time_ms,
                          void (*callback)(uint8_t, uint32_t)) {
    if (!button_validate_id(button_id)) {
        return false;
    }

    button_config_t *btn = &g_button_manager.buttons[button_id];

    if (btn->hold_level_count >= MAX_HOLD_LEVELS) {
        DB_LOG_BT("[BTN] Error: Maximum hold levels reached for button %d\n", button_id);
        return false;
    }

    uint8_t level = btn->hold_level_count;
    btn->hold_levels[level].hold_time = hold_time_ms;
    btn->hold_levels[level].callback = callback;
    btn->hold_levels[level].enabled = true;
    btn->hold_level_count++;

    DB_LOG_BT("[BTN] Added hold level %d for button %d: %d ms\n",
           level, button_id, hold_time_ms);

    return true;
}

bool button_remove_hold_level(uint8_t button_id, uint8_t level_index) {
    if (!button_validate_id(button_id)) {
        return false;
    }

    button_config_t *btn = &g_button_manager.buttons[button_id];

    if (level_index >= btn->hold_level_count) {
        return false;
    }

    // Shift remaining levels down
    for (uint8_t i = level_index; i < btn->hold_level_count - 1; i++) {
        btn->hold_levels[i] = btn->hold_levels[i + 1];
    }

    btn->hold_level_count--;
    memset(&btn->hold_levels[btn->hold_level_count], 0, sizeof(button_hold_level_t));

    DB_LOG_BT("[BTN] Removed hold level %d from button %d\n", level_index, button_id);
    return true;
}

bool button_set_hold_level_enabled(uint8_t button_id, uint8_t level_index, bool enabled) {
    if (!button_validate_id(button_id)) {
        return false;
    }

    button_config_t *btn = &g_button_manager.buttons[button_id];

    if (level_index >= btn->hold_level_count) {
        return false;
    }

    btn->hold_levels[level_index].enabled = enabled;
    return true;
}

bool button_add_multiclick_level(uint8_t button_id,
                                uint8_t click_count,
                                void (*callback)(uint8_t, uint8_t)) {
    if (!button_validate_id(button_id)) {
        return false;
    }

    if (click_count == 0 || click_count > MAX_MULTI_CLICK) {
        DB_LOG_BT("[BTN] Error: Invalid click count %d for button %d\n", click_count, button_id);
        return false;
    }

    button_config_t *btn = &g_button_manager.buttons[button_id];

    if (btn->multiclick_level_count >= MAX_MULTI_CLICK) {
        DB_LOG_BT("[BTN] Error: Maximum multi-click levels reached for button %d\n", button_id);
        return false;
    }

    // Check for duplicate click count
    for (uint8_t i = 0; i < btn->multiclick_level_count; i++) {
        if (btn->multiclick_levels[i].click_count == click_count) {
            DB_LOG_BT("[BTN] Error: Click count %d already configured for button %d\n",
                   click_count, button_id);
            return false;
        }
    }

    uint8_t level = btn->multiclick_level_count;
    btn->multiclick_levels[level].click_count = click_count;
    btn->multiclick_levels[level].callback = callback;
    btn->multiclick_levels[level].enabled = true;
    btn->multiclick_level_count++;

    // Sort levels by click count for efficient processing
    button_sort_multiclick_levels(button_id);

    DB_LOG_BT("[BTN] Added multi-click level for button %d: %d clicks\n",
           button_id, click_count);

    return true;
}

bool button_remove_multiclick_level(uint8_t button_id, uint8_t level_index) {
    if (!button_validate_id(button_id)) {
        return false;
    }

    button_config_t *btn = &g_button_manager.buttons[button_id];

    if (level_index >= btn->multiclick_level_count) {
        return false;
    }

//    uint8_t removed_clicks = btn->multiclick_levels[level_index].click_count;

    // Shift remaining levels down
    for (uint8_t i = level_index; i < btn->multiclick_level_count - 1; i++) {
        btn->multiclick_levels[i] = btn->multiclick_levels[i + 1];
    }

    btn->multiclick_level_count--;
    memset(&btn->multiclick_levels[btn->multiclick_level_count], 0, sizeof(button_multiclick_t));

    DB_LOG_BT("[BTN] Removed multi-click level (%d clicks) from button %d\n",
           removed_clicks, button_id);
    return true;
}

bool button_set_multiclick_level_enabled(uint8_t button_id, uint8_t level_index, bool enabled) {
    if (!button_validate_id(button_id)) {
        return false;
    }

    button_config_t *btn = &g_button_manager.buttons[button_id];

    if (level_index >= btn->multiclick_level_count) {
        return false;
    }

    btn->multiclick_levels[level_index].enabled = enabled;
    return true;
}

// ... [Keep all existing configuration functions] ...

bool button_set_debounce_time(uint8_t button_id, uint32_t debounce_ms) {
    if (!button_validate_id(button_id)) {
        return false;
    }

    g_button_manager.buttons[button_id].debounce_time = debounce_ms;
    return true;
}

bool button_set_click_timeout(uint8_t button_id, uint32_t timeout_ms) {
    if (!button_validate_id(button_id)) {
        return false;
    }

    g_button_manager.buttons[button_id].click_timeout = timeout_ms;
    return true;
}

bool button_set_multiclick_timeout(uint8_t button_id, uint32_t timeout_ms) {
    if (!button_validate_id(button_id)) {
        return false;
    }

    g_button_manager.buttons[button_id].multiclick_timeout = timeout_ms;
    return true;
}

bool button_set_enabled(uint8_t button_id, bool enabled) {
    if (!button_validate_id(button_id)) {
        return false;
    }

    g_button_manager.buttons[button_id].enabled = enabled;
    return true;
}

// ... [Keep all existing getter functions] ...

button_state_t button_get_state(uint8_t button_id) {
    if (!button_validate_id(button_id)) {
        return BUTTON_STATE_IDLE;
    }

    return g_button_manager.buttons[button_id].state;
}

bool button_is_pressed(uint8_t button_id) {
    if (!button_validate_id(button_id)) {
        return false;
    }

    button_config_t *btn = &g_button_manager.buttons[button_id];
    return btn->stable_state;
}

uint32_t button_get_hold_time(uint8_t button_id) {
    if (!button_validate_id(button_id)) {
        return 0;
    }

    button_config_t *btn = &g_button_manager.buttons[button_id];

    if (btn->state == BUTTON_STATE_PRESSED || btn->state == BUTTON_STATE_HOLD) {
        uint32_t current_time = g_button_manager.hal.get_system_time_ms();
        return current_time - btn->press_start_time;
    }

    return 0;
}

uint8_t button_get_click_count(uint8_t button_id) {
    if (!button_validate_id(button_id)) {
        return 0;
    }

    button_config_t *btn = &g_button_manager.buttons[button_id];

    if (btn->state == BUTTON_STATE_WAIT_MULTICLICK) {
        return btn->click_count;
    }

    return 0;
}

void button_process_all(void) {
    if (!g_button_manager.initialized) {
        return;
    }

    // Process individual buttons
    for (uint8_t i = 0; i < g_button_manager.button_count; i++) {
        if (g_button_manager.buttons[i].initialized) {
            button_process_state_machine(i);
        }
    }

    // **NEW**: Process multi-button features after individual button processing
    button_update_global_state();
    button_process_combos();
    button_process_patterns();
}

bool button_process_single(uint8_t button_id) {
    if (!button_validate_id(button_id)) {
        return false;
    }

    button_process_state_machine(button_id);

    // **NEW**: Also update global state when processing single button
    button_update_global_state();
    button_process_combos();
    button_process_patterns();

    return true;
}

void button_get_stats(uint8_t *total_buttons, uint8_t *active_buttons) {
    if (total_buttons) {
        *total_buttons = g_button_manager.button_count;
    }

    if (active_buttons) {
        uint8_t active = 0;
        for (uint8_t i = 0; i < g_button_manager.button_count; i++) {
            if (g_button_manager.buttons[i].enabled && g_button_manager.buttons[i].initialized) {
                active++;
            }
        }
        *active_buttons = active;
    }
}

// =============================================================================
// **NEW**: MULTI-BUTTON COMBO API IMPLEMENTATION
// =============================================================================

uint8_t button_add_combo(uint8_t *button_ids, uint8_t button_count, uint32_t detection_window) {
    if (!g_button_manager.initialized) {
        DB_LOG_BT("[BTN_COMBO] Error: Manager not initialized\n");
        return 0xFF;
    }

    if (button_count < 2 || button_count > MAX_COMBO_BUTTONS) {
        DB_LOG_BT("[BTN_COMBO] Error: Invalid button count %d (must be 2-%d)\n",
               button_count, MAX_COMBO_BUTTONS);
        return 0xFF;
    }

    if (g_button_manager.combo_count >= MAX_COMBOS) {
        DB_LOG_BT("[BTN_COMBO] Error: Maximum combos reached (%d)\n", MAX_COMBOS);
        return 0xFF;
    }

    // Validate all button IDs
    for (uint8_t i = 0; i < button_count; i++) {
        if (!button_validate_id(button_ids[i])) {
            DB_LOG_BT("[BTN_COMBO] Error: Invalid button ID %d in combo\n", button_ids[i]);
            return 0xFF;
        }
    }

    uint8_t combo_id = g_button_manager.combo_count;
    button_combo_t *combo = &g_button_manager.combos[combo_id];

    // Configure combo
    memcpy(combo->button_ids, button_ids, button_count * sizeof(uint8_t));
    combo->button_count = button_count;
    combo->detection_window = detection_window;
    combo->hold_threshold = DEFAULT_COMBO_TIMEOUT;
    combo->enabled = true;
    combo->state = COMBO_STATE_IDLE;
    combo->pressed_mask = 0;
    combo->first_press_time = 0;
    combo->all_pressed_time = 0;

    // Clear callbacks
    combo->on_combo_press = NULL;
    combo->on_combo_release = NULL;
    combo->on_combo_click = NULL;
    combo->on_combo_hold = NULL;

    g_button_manager.combo_count++;

    DB_LOG_BT("[BTN_COMBO] Added combo %d with %d buttons (window: %dms)\n",
           combo_id, button_count, detection_window);

    return combo_id;
}

bool button_remove_combo(uint8_t combo_id) {
    if (!button_validate_combo_id(combo_id)) {
        return false;
    }

    button_combo_t *combo = &g_button_manager.combos[combo_id];

    // Reset any buttons that were marked as in_combo
    for (uint8_t i = 0; i < combo->button_count; i++) {
        uint8_t btn_id = combo->button_ids[i];
        if (button_validate_id(btn_id)) {
            g_button_manager.buttons[btn_id].in_combo = false;
        }
    }

    // Shift remaining combos down
    for (uint8_t i = combo_id; i < g_button_manager.combo_count - 1; i++) {
        g_button_manager.combos[i] = g_button_manager.combos[i + 1];
    }

    g_button_manager.combo_count--;
    memset(&g_button_manager.combos[g_button_manager.combo_count], 0, sizeof(button_combo_t));

    DB_LOG_BT("[BTN_COMBO] Removed combo %d\n", combo_id);
    return true;
}

bool button_set_combo_press_callback(uint8_t combo_id,
                                     void (*callback)(uint8_t *button_ids, uint8_t count)) {
    if (!button_validate_combo_id(combo_id)) {
        return false;
    }

    g_button_manager.combos[combo_id].on_combo_press = callback;
    return true;
}

bool button_set_combo_release_callback(uint8_t combo_id,
                                       void (*callback)(uint8_t *button_ids, uint8_t count)) {
    if (!button_validate_combo_id(combo_id)) {
        return false;
    }

    g_button_manager.combos[combo_id].on_combo_release = callback;
    return true;
}

bool button_set_combo_click_callback(uint8_t combo_id,
                                     void (*callback)(uint8_t *button_ids, uint8_t count)) {
    if (!button_validate_combo_id(combo_id)) {
        return false;
    }

    g_button_manager.combos[combo_id].on_combo_click = callback;
    return true;
}

bool button_set_combo_hold_callback(uint8_t combo_id, uint32_t hold_threshold,
                                    void (*callback)(uint8_t *button_ids, uint8_t count, uint32_t hold_time)) {
    if (!button_validate_combo_id(combo_id)) {
        return false;
    }

    button_combo_t *combo = &g_button_manager.combos[combo_id];
    combo->hold_threshold = hold_threshold;
    combo->on_combo_hold = callback;
    return true;
}

bool button_set_combo_enabled(uint8_t combo_id, bool enabled) {
    if (!button_validate_combo_id(combo_id)) {
        return false;
    }

    g_button_manager.combos[combo_id].enabled = enabled;
    return true;
}

// =============================================================================
// **NEW**: BUTTON SEQUENCE PATTERN API IMPLEMENTATION
// =============================================================================

uint8_t button_add_pattern(uint8_t *pattern, uint8_t pattern_length, uint32_t timeout) {
    if (!g_button_manager.initialized) {
        DB_LOG_BT("[BTN_PATTERN] Error: Manager not initialized\n");
        return 0xFF;
    }

    if (pattern_length < 2 || pattern_length > MAX_PATTERN_LENGTH) {
        DB_LOG_BT("[BTN_PATTERN] Error: Invalid pattern length %d (must be 2-%d)\n",
               pattern_length, MAX_PATTERN_LENGTH);
        return 0xFF;
    }

    if (g_button_manager.pattern_count >= MAX_PATTERNS) {
        DB_LOG_BT("[BTN_PATTERN] Error: Maximum patterns reached (%d)\n", MAX_PATTERNS);
        return 0xFF;
    }

    // Validate all button IDs in pattern
    for (uint8_t i = 0; i < pattern_length; i++) {
        if (!button_validate_id(pattern[i])) {
            DB_LOG_BT("[BTN_PATTERN] Error: Invalid button ID %d in pattern\n", pattern[i]);
            return 0xFF;
        }
    }

    uint8_t pattern_id = g_button_manager.pattern_count;
    button_pattern_t *pat = &g_button_manager.patterns[pattern_id];

    // Configure pattern
    memcpy(pat->pattern, pattern, pattern_length * sizeof(uint8_t));
    pat->pattern_length = pattern_length;
    pat->timeout = timeout;
    pat->enabled = true;
    pat->state = PATTERN_STATE_IDLE;
    pat->current_position = 0;
    pat->last_press_time = 0;
    pat->on_pattern_match = NULL;

    g_button_manager.pattern_count++;

    DB_LOG_BT("[BTN_PATTERN] Added pattern %d with %d buttons (timeout: %dms)\n",
           pattern_id, pattern_length, timeout);

    return pattern_id;
}

bool button_remove_pattern(uint8_t pattern_id) {
    if (!button_validate_pattern_id(pattern_id)) {
        return false;
    }

    // Shift remaining patterns down
    for (uint8_t i = pattern_id; i < g_button_manager.pattern_count - 1; i++) {
        g_button_manager.patterns[i] = g_button_manager.patterns[i + 1];
    }

    g_button_manager.pattern_count--;
    memset(&g_button_manager.patterns[g_button_manager.pattern_count], 0, sizeof(button_pattern_t));

    DB_LOG_BT("[BTN_PATTERN] Removed pattern %d\n", pattern_id);
    return true;
}

bool button_set_pattern_callback(uint8_t pattern_id,
                                void (*callback)(uint8_t *pattern, uint8_t length)) {
    if (!button_validate_pattern_id(pattern_id)) {
        return false;
    }

    g_button_manager.patterns[pattern_id].on_pattern_match = callback;
    return true;
}

bool button_set_pattern_enabled(uint8_t pattern_id, bool enabled) {
    if (!button_validate_pattern_id(pattern_id)) {
        return false;
    }

    g_button_manager.patterns[pattern_id].enabled = enabled;
    return true;
}

bool button_reset_pattern(uint8_t pattern_id) {
    if (pattern_id == 0xFF) {
        // Reset all patterns
        for (uint8_t i = 0; i < g_button_manager.pattern_count; i++) {
            button_pattern_t *pat = &g_button_manager.patterns[i];
            pat->state = PATTERN_STATE_IDLE;
            pat->current_position = 0;
            pat->last_press_time = 0;
        }
        DB_LOG_BT("[BTN_PATTERN] Reset all patterns\n");
        return true;
    }

    if (!button_validate_pattern_id(pattern_id)) {
        return false;
    }

    button_pattern_t *pat = &g_button_manager.patterns[pattern_id];
    pat->state = PATTERN_STATE_IDLE;
    pat->current_position = 0;
    pat->last_press_time = 0;

    DB_LOG_BT("[BTN_PATTERN] Reset pattern %d\n", pattern_id);
    return true;
}

// =============================================================================
// **NEW**: UTILITY FUNCTIONS IMPLEMENTATION
// =============================================================================

uint8_t button_get_pressed_buttons(uint8_t *pressed_buttons, uint8_t max_buttons) {
    if (!pressed_buttons || max_buttons == 0) {
        return 0;
    }

    uint8_t count = 0;
    for (uint8_t i = 0; i < g_button_manager.button_count && count < max_buttons; i++) {
        if (g_button_manager.buttons[i].enabled &&
            g_button_manager.buttons[i].initialized &&
            g_button_manager.buttons[i].stable_state) {
            pressed_buttons[count++] = i;
        }
    }

    return count;
}

bool button_are_buttons_pressed(uint8_t *button_ids, uint8_t button_count) {
    if (!button_ids || button_count == 0) {
        return false;
    }

    for (uint8_t i = 0; i < button_count; i++) {
        if (!button_is_pressed(button_ids[i])) {
            return false;
        }
    }

    return true;
}

void button_get_combo_stats(uint8_t *total_combos, uint8_t *active_combos) {
    if (total_combos) {
        *total_combos = g_button_manager.combo_count;
    }

    if (active_combos) {
        uint8_t active = 0;
        for (uint8_t i = 0; i < g_button_manager.combo_count; i++) {
            if (g_button_manager.combos[i].enabled) {
                active++;
            }
        }
        *active_combos = active;
    }
}

void button_get_pattern_stats(uint8_t *total_patterns, uint8_t *active_patterns) {
    if (total_patterns) {
        *total_patterns = g_button_manager.pattern_count;
    }

    if (active_patterns) {
        uint8_t active = 0;
        for (uint8_t i = 0; i < g_button_manager.pattern_count; i++) {
            if (g_button_manager.patterns[i].enabled) {
                active++;
            }
        }
        *active_patterns = active;
    }
}

// =============================================================================
// PRIVATE FUNCTION IMPLEMENTATIONS
// =============================================================================

static bool button_validate_id(uint8_t button_id) {
    if (!g_button_manager.initialized) {
        DB_LOG_BT("[BTN] Error: Manager not initialized\n");
        return false;
    }

    if (button_id >= g_button_manager.button_count) {
        DB_LOG_BT("[BTN] Error: Invalid button ID %d\n", button_id);
        return false;
    }

    if (!g_button_manager.buttons[button_id].initialized) {
        DB_LOG_BT("[BTN] Error: Button %d not initialized\n", button_id);
        return false;
    }

    return true;
}

// **NEW**: Validate combo ID
static bool button_validate_combo_id(uint8_t combo_id) {
    if (!g_button_manager.initialized) {
        DB_LOG_BT("[BTN_COMBO] Error: Manager not initialized\n");
        return false;
    }

    if (combo_id >= g_button_manager.combo_count) {
        DB_LOG_BT("[BTN_COMBO] Error: Invalid combo ID %d\n", combo_id);
        return false;
    }

    return true;
}

// **NEW**: Validate pattern ID
static bool button_validate_pattern_id(uint8_t pattern_id) {
    if (!g_button_manager.initialized) {
        DB_LOG_BT("[BTN_PATTERN] Error: Manager not initialized\n");
        return false;
    }

    if (pattern_id >= g_button_manager.pattern_count) {
        DB_LOG_BT("[BTN_PATTERN] Error: Invalid pattern ID %d\n", pattern_id);
        return false;
    }

    return true;
}

static void button_process_state_machine(uint8_t button_id) {
    button_config_t *btn = &g_button_manager.buttons[button_id];

    if (!btn->enabled) {
        return;
    }

    // **FIXED**: Always process state machine, but limit callbacks when in active combo
    // This allows individual features to work alongside combo/pattern features

    uint32_t current_time = g_button_manager.hal.get_system_time_ms();
    bool raw_state = g_button_manager.hal.button_pin_read(btn->gpio_pin);

    // Apply active_low logic
    if (btn->active_low) {
        raw_state = !raw_state;
    }

    btn->raw_state = raw_state;

    // State machine
    switch (btn->state) {
        case BUTTON_STATE_IDLE:
            if (raw_state) {
                // Button pressed - start debounce
                btn->state = BUTTON_STATE_DEBOUNCE;
                btn->last_change_time = current_time;
            }
            break;

        case BUTTON_STATE_DEBOUNCE:
            if (current_time - btn->last_change_time >= btn->debounce_time) {
                if (raw_state) {
                    // Confirmed press
                    btn->state = BUTTON_STATE_PRESSED;
                    btn->stable_state = true;
                    btn->press_start_time = current_time;
                    btn->last_hold_level = 0;

                    button_trigger_event(button_id, BUTTON_EVENT_PRESS, 0);
                } else {
                    // False positive - back to idle
                    btn->state = BUTTON_STATE_IDLE;
                }
            } else if (!raw_state) {
                // Button released during debounce
                btn->state = BUTTON_STATE_IDLE;
            }
            break;

        case BUTTON_STATE_PRESSED:
            if (!raw_state) {
                // Button released
                btn->stable_state = false;
                btn->release_time = current_time;

                uint32_t press_duration = current_time - btn->press_start_time;

                button_trigger_event(button_id, BUTTON_EVENT_RELEASE, press_duration);

                // Check for click (short press)
                if (press_duration < btn->click_timeout) {
                    btn->click_count++;

                    // Legacy click callback for single click
                    if (btn->click_count == 1 && btn->on_click) {
                        btn->on_click(button_id);
                    }

                    // Check if we have multi-click levels configured
                    if (btn->multiclick_level_count > 0 || btn->on_multiclick) {
                        btn->state = BUTTON_STATE_WAIT_MULTICLICK;
                        // Don't reset to IDLE yet, wait for potential additional clicks
                    } else {
                        // No multi-click configured, reset and go to idle
                        btn->click_count = 0;
                        btn->state = BUTTON_STATE_IDLE;
                    }
                } else {
                    // Long press, reset and go to idle
                    btn->click_count = 0;
                    btn->state = BUTTON_STATE_IDLE;
                }
            } else {
                // Check hold levels
                uint32_t hold_time = current_time - btn->press_start_time;

                for (uint8_t i = btn->last_hold_level; i < btn->hold_level_count; i++) {
                    if (btn->hold_levels[i].enabled && hold_time >= btn->hold_levels[i].hold_time) {
                        btn->last_hold_level = i + 1;

                        // Transition to hold state on first level
                        if (btn->state == BUTTON_STATE_PRESSED && i == 0) {
                            btn->state = BUTTON_STATE_HOLD;
                            button_trigger_event(button_id, BUTTON_EVENT_HOLD_START, hold_time);
                        }

                        // Trigger hold level callback
                        if (btn->hold_levels[i].callback) {
                            btn->hold_levels[i].callback(button_id, hold_time);
                        }

                        button_trigger_event(button_id, BUTTON_EVENT_HOLD_LEVEL, hold_time);
                    }
                }
            }
            break;

        case BUTTON_STATE_HOLD:
            if (!raw_state) {
                // Hold ended
                btn->state = BUTTON_STATE_IDLE;
                btn->stable_state = false;
                btn->click_count = 0;  // Reset click count on hold

                uint32_t hold_duration = current_time - btn->press_start_time;

                button_trigger_event(button_id, BUTTON_EVENT_HOLD_END, hold_duration);
                button_trigger_event(button_id, BUTTON_EVENT_RELEASE, hold_duration);
            } else {
                // Continue checking higher hold levels
                uint32_t hold_time = current_time - btn->press_start_time;

                for (uint8_t i = btn->last_hold_level; i < btn->hold_level_count; i++) {
                    if (btn->hold_levels[i].enabled && hold_time >= btn->hold_levels[i].hold_time) {
                        btn->last_hold_level = i + 1;

                        if (btn->hold_levels[i].callback) {
                            btn->hold_levels[i].callback(button_id, hold_time);
                        }

                        button_trigger_event(button_id, BUTTON_EVENT_HOLD_LEVEL, hold_time);
                    }
                }
            }
            break;

        case BUTTON_STATE_WAIT_MULTICLICK:
            if (raw_state) {
                // Another press detected
                btn->state = BUTTON_STATE_DEBOUNCE;
                btn->last_change_time = current_time;
            } else {
                // Check timeout
                if (current_time - btn->release_time >= btn->multiclick_timeout) {
                    // Timeout reached, trigger multi-click event
                    button_trigger_multiclick(button_id);
                    btn->click_count = 0;
                    btn->state = BUTTON_STATE_IDLE;
                }
            }
            break;
    }
}

static void button_trigger_event(uint8_t button_id, button_event_t event, uint32_t data) {
    button_config_t *btn = &g_button_manager.buttons[button_id];

    // **FIXED**: Only suppress individual callbacks when button is in active combo
    // This allows individual features to work when combo is not active
    bool suppress_individual = btn->in_combo;

    switch (event) {
        case BUTTON_EVENT_PRESS:
            if (!suppress_individual && btn->on_press) {
                btn->on_press(button_id);
            }
            if (!suppress_individual) {
                DB_LOG_BT("[BTN%d] PRESS\n", button_id);
            }
            break;

        case BUTTON_EVENT_RELEASE:
            if (!suppress_individual && btn->on_release) {
                btn->on_release(button_id, btn->release_time - btn->press_start_time);
            }
            if (!suppress_individual) {
                DB_LOG_BT("[BTN%d] RELEASE (%dms)\n", button_id, data);
            }
            break;

        case BUTTON_EVENT_CLICK:
            if (!suppress_individual && btn->on_click) {
                btn->on_click(button_id);
            }
            if (!suppress_individual) {
                DB_LOG_BT("[BTN%d] CLICK (%dms)\n", button_id, data);
            }
            break;

        case BUTTON_EVENT_MULTI_CLICK:
            if (!suppress_individual && btn->on_multiclick) {
                btn->on_multiclick(button_id, btn->click_count);
            }
            if (!suppress_individual) {
                DB_LOG_BT("[BTN%d] MULTI_CLICK (%d clicks)\n", button_id, data);
            }
            break;

        case BUTTON_EVENT_HOLD_START:
            if (!suppress_individual && btn->on_hold_start) {
                btn->on_hold_start(button_id);
            }
            if (!suppress_individual) {
                DB_LOG_BT("[BTN%d] HOLD_START\n", button_id);
            }
            break;

        case BUTTON_EVENT_HOLD_END:
            if (!suppress_individual && btn->on_hold_end) {
                btn->on_hold_end(button_id);
            }
            if (!suppress_individual) {
                DB_LOG_BT("[BTN%d] HOLD_END (%dms)\n", button_id, data);
            }
            break;

        case BUTTON_EVENT_HOLD_LEVEL:
            if (!suppress_individual) {
                DB_LOG_BT("[BTN%d] HOLD_LEVEL (%dms)\n", button_id, data);
            }
            break;

        default:
            break;
    }
}

static void button_trigger_multiclick(uint8_t button_id) {
    button_config_t *btn = &g_button_manager.buttons[button_id];

    // **FIXED**: Only suppress individual callbacks when button is in active combo
    bool suppress_individual = btn->in_combo;

    if (!suppress_individual) {
        // Find matching multi-click level (levels are sorted by click count)
        bool level_found = false;
        for (uint8_t i = 0; i < btn->multiclick_level_count; i++) {
            if (btn->multiclick_levels[i].enabled &&
                btn->multiclick_levels[i].click_count == btn->click_count) {

                // Found exact match, trigger callback
                if (btn->multiclick_levels[i].callback) {
                    btn->multiclick_levels[i].callback(button_id, btn->click_count);
                }
                level_found = true;
                break;
            }
        }

        // Always trigger multi-click event when not in combo
        button_trigger_event(button_id, BUTTON_EVENT_MULTI_CLICK, btn->click_count);

        if (level_found) {
            DB_LOG_BT("[BTN%d] Multi-click level triggered: %d clicks\n", button_id, btn->click_count);
        } else {
            DB_LOG_BT("[BTN%d] Multi-click detected (no specific level): %d clicks\n", button_id, btn->click_count);
        }
    }
}

static void button_sort_multiclick_levels(uint8_t button_id) {
    button_config_t *btn = &g_button_manager.buttons[button_id];

    // Simple bubble sort by click count (ascending)
    for (uint8_t i = 0; i < btn->multiclick_level_count - 1; i++) {
        for (uint8_t j = 0; j < btn->multiclick_level_count - 1 - i; j++) {
            if (btn->multiclick_levels[j].click_count > btn->multiclick_levels[j + 1].click_count) {
                // Swap
                button_multiclick_t temp = btn->multiclick_levels[j];
                btn->multiclick_levels[j] = btn->multiclick_levels[j + 1];
                btn->multiclick_levels[j + 1] = temp;
            }
        }
    }
}

// =============================================================================
// **NEW**: MULTI-BUTTON PRIVATE FUNCTION IMPLEMENTATIONS
// =============================================================================

static void button_update_global_state(void) {
    uint16_t old_mask = g_button_manager.global_state.pressed_buttons_mask;
    uint16_t new_mask = 0;
    uint32_t current_time = g_button_manager.hal.get_system_time_ms();

    // Build bitmask of currently pressed buttons
    for (uint8_t i = 0; i < g_button_manager.button_count; i++) {
        if (g_button_manager.buttons[i].enabled && 
            g_button_manager.buttons[i].initialized &&
            g_button_manager.buttons[i].stable_state) {
            new_mask |= (1 << i);
        }
    }

    // Check for state changes
    if (new_mask != old_mask) {
        g_button_manager.global_state.pressed_buttons_mask = new_mask;
        g_button_manager.global_state.global_state_change_time = current_time;

        // Find the newly pressed button(s)
        uint16_t new_presses = new_mask & ~old_mask;
        if (new_presses) {
            // Find the highest bit set (most recent button press)
            for (int8_t i = g_button_manager.button_count - 1; i >= 0; i--) {
                if (new_presses & (1 << i)) {
                    g_button_manager.global_state.last_pressed_button = i;
                    g_button_manager.global_state.last_button_press_time = current_time;
                    break;
                }
            }
        }
    }
}

static void button_process_combos(void) {
    uint32_t current_time = g_button_manager.hal.get_system_time_ms();

    for (uint8_t combo_id = 0; combo_id < g_button_manager.combo_count; combo_id++) {
        button_combo_t *combo = &g_button_manager.combos[combo_id];

        if (!combo->enabled) {
            continue;
        }

        // Update pressed mask for this combo
        uint8_t pressed_count = 0;
        uint8_t new_pressed_mask = 0;

        for (uint8_t i = 0; i < combo->button_count; i++) {
            uint8_t btn_id = combo->button_ids[i];
            if (button_is_pressed(btn_id)) {
                new_pressed_mask |= (1 << i);
                pressed_count++;
            }
        }

        // Combo state machine
        switch (combo->state) {
            case COMBO_STATE_IDLE:
                if (pressed_count > 0) {
                    combo->state = COMBO_STATE_DETECTING;
                    combo->first_press_time = current_time;
                    combo->pressed_mask = new_pressed_mask;
                    DB_LOG_BT("[BTN_COMBO] Combo %d detecting started\n", combo_id);
                }
                break;

            case COMBO_STATE_DETECTING:
                combo->pressed_mask = new_pressed_mask;
                
                if (button_is_combo_complete(combo_id)) {
                    // All buttons pressed - combo complete
                    combo->state = COMBO_STATE_ACTIVE;
                    combo->all_pressed_time = current_time;
                    
                    // Mark buttons as in combo to prevent individual processing
                    for (uint8_t i = 0; i < combo->button_count; i++) {
                        uint8_t btn_id = combo->button_ids[i];
                        g_button_manager.buttons[btn_id].in_combo = true;
                    }
                    
                    button_trigger_combo_event(combo_id, BUTTON_EVENT_COMBO_PRESS, 0);
                    
                } else if (pressed_count == 0) {
                    // All buttons released before combo complete
                    combo->state = COMBO_STATE_IDLE;
                    combo->pressed_mask = 0;
                    DB_LOG_BT("[BTN_COMBO] Combo %d detection cancelled - all released\n", combo_id);
                    
                } else if (current_time - combo->first_press_time > combo->detection_window) {
                    // Detection window timeout
                    combo->state = COMBO_STATE_IDLE;
                    combo->pressed_mask = 0;
                    DB_LOG_BT("[BTN_COMBO] Combo %d detection timeout\n", combo_id);
                }
                break;

            case COMBO_STATE_ACTIVE:
                combo->pressed_mask = new_pressed_mask;
                
                if (pressed_count == 0) {
                    // All buttons released - combo click or release
                    uint32_t hold_time = current_time - combo->all_pressed_time;
                    
                    // Unmark buttons from combo
                    for (uint8_t i = 0; i < combo->button_count; i++) {
                        uint8_t btn_id = combo->button_ids[i];
                        g_button_manager.buttons[btn_id].in_combo = false;
                    }
                    
                    if (hold_time < combo->hold_threshold) {
                        button_trigger_combo_event(combo_id, BUTTON_EVENT_COMBO_CLICK, 0);
                    }
                    
                    button_trigger_combo_event(combo_id, BUTTON_EVENT_COMBO_RELEASE, 0);
                    combo->state = COMBO_STATE_IDLE;
                    combo->pressed_mask = 0;
                    
                } else if (button_is_combo_complete(combo_id)) {
                    // Check for hold threshold
                    uint32_t hold_time = current_time - combo->all_pressed_time;
                    if (hold_time >= combo->hold_threshold && combo->state != COMBO_STATE_HOLD) {
                        combo->state = COMBO_STATE_HOLD;
                        button_trigger_combo_event(combo_id, BUTTON_EVENT_COMBO_HOLD, hold_time);
                    }
                } else {
                    // Some buttons released but not all - treat as release
                    for (uint8_t i = 0; i < combo->button_count; i++) {
                        uint8_t btn_id = combo->button_ids[i];
                        g_button_manager.buttons[btn_id].in_combo = false;
                    }
                    
                    button_trigger_combo_event(combo_id, BUTTON_EVENT_COMBO_RELEASE, 0);
                    combo->state = COMBO_STATE_IDLE;
                    combo->pressed_mask = 0;
                }
                break;

            case COMBO_STATE_HOLD:
                combo->pressed_mask = new_pressed_mask;
                
                if (pressed_count == 0) {
                    // Released from hold
                    for (uint8_t i = 0; i < combo->button_count; i++) {
                        uint8_t btn_id = combo->button_ids[i];
                        g_button_manager.buttons[btn_id].in_combo = false;
                    }
                    
                    button_trigger_combo_event(combo_id, BUTTON_EVENT_COMBO_RELEASE, 0);
                    combo->state = COMBO_STATE_IDLE;
                    combo->pressed_mask = 0;
                } else if (!button_is_combo_complete(combo_id)) {
                    // Some buttons released during hold
                    for (uint8_t i = 0; i < combo->button_count; i++) {
                        uint8_t btn_id = combo->button_ids[i];
                        g_button_manager.buttons[btn_id].in_combo = false;
                    }
                    
                    button_trigger_combo_event(combo_id, BUTTON_EVENT_COMBO_RELEASE, 0);
                    combo->state = COMBO_STATE_IDLE;
                    combo->pressed_mask = 0;
                }
                break;

            default:
                combo->state = COMBO_STATE_IDLE;
                combo->pressed_mask = 0;
                break;
        }
    }
}

static void button_process_patterns(void) {
    uint32_t current_time = g_button_manager.hal.get_system_time_ms();

    // Check for new button presses to process patterns
    uint8_t last_pressed = g_button_manager.global_state.last_pressed_button;
    uint32_t last_press_time = g_button_manager.global_state.last_button_press_time;

    for (uint8_t pattern_id = 0; pattern_id < g_button_manager.pattern_count; pattern_id++) {
        button_pattern_t *pattern = &g_button_manager.patterns[pattern_id];

        if (!pattern->enabled) {
            continue;
        }

        switch (pattern->state) {
            case PATTERN_STATE_IDLE:
                // Check if any button was pressed to potentially start pattern
                if (last_pressed != 0xFF && last_press_time > pattern->last_press_time) {
                    if (button_check_pattern_step(pattern_id, last_pressed)) {
                        pattern->state = PATTERN_STATE_DETECTING;
                        pattern->current_position = 1;
                        pattern->last_press_time = last_press_time;
                        DB_LOG_BT("[BTN_PATTERN] Pattern %d started with button %d\n", 
                               pattern_id, last_pressed);
                    }
                }
                break;

            case PATTERN_STATE_DETECTING:
                // Check for timeout
                if (current_time - pattern->last_press_time > pattern->timeout) {
                    pattern->state = PATTERN_STATE_TIMEOUT;
                    DB_LOG_BT("[BTN_PATTERN] Pattern %d timed out at position %d\n", 
                           pattern_id, pattern->current_position);
                    break;
                }

                // Check for next button in sequence
                if (last_pressed != 0xFF && last_press_time > pattern->last_press_time) {
                    
                    if (button_check_pattern_step(pattern_id, last_pressed)) {
                        pattern->current_position++;
                        pattern->last_press_time = last_press_time;
                        
                        if (pattern->current_position >= pattern->pattern_length) {
                            // Pattern complete!
                            button_trigger_pattern_event(pattern_id);
                            pattern->state = PATTERN_STATE_IDLE;
                            pattern->current_position = 0;
                            DB_LOG_BT("[BTN_PATTERN] Pattern %d completed!\n", pattern_id);
                        } else {
                            DB_LOG_BT("[BTN_PATTERN] Pattern %d step %d/%d\n", 
                                   pattern_id, pattern->current_position, pattern->pattern_length);
                        }
                    } else {
                        // Wrong button - check if it could start a new pattern
                        if (button_check_pattern_step(pattern_id, last_pressed)) {
                            // Reset and start over
                            pattern->current_position = 1;
                            pattern->last_press_time = last_press_time;
                            DB_LOG_BT("[BTN_PATTERN] Pattern %d restarted with button %d\n", 
                                   pattern_id, last_pressed);
                        } else {
                            // Reset pattern completely
                            pattern->state = PATTERN_STATE_IDLE;
                            pattern->current_position = 0;
                            DB_LOG_BT("[BTN_PATTERN] Pattern %d reset - wrong button\n", pattern_id);
                        }
                    }
                }
                break;

            case PATTERN_STATE_TIMEOUT:
                // Reset to idle after timeout
                pattern->state = PATTERN_STATE_IDLE;
                pattern->current_position = 0;
                break;

            default:
                pattern->state = PATTERN_STATE_IDLE;
                pattern->current_position = 0;
                break;
        }
    }
}

static void button_trigger_combo_event(uint8_t combo_id, button_event_t event, uint32_t data) {
    button_combo_t *combo = &g_button_manager.combos[combo_id];

    switch (event) {
        case BUTTON_EVENT_COMBO_PRESS:
            if (combo->on_combo_press) {
                combo->on_combo_press(combo->button_ids, combo->button_count);
            }
            DB_LOG_BT("[BTN_COMBO] Combo %d pressed\n", combo_id);
            break;

        case BUTTON_EVENT_COMBO_RELEASE:
            if (combo->on_combo_release) {
                combo->on_combo_release(combo->button_ids, combo->button_count);
            }
            DB_LOG_BT("[BTN_COMBO] Combo %d released\n", combo_id);
            break;

        case BUTTON_EVENT_COMBO_CLICK:
            if (combo->on_combo_click) {
                combo->on_combo_click(combo->button_ids, combo->button_count);
            }
            DB_LOG_BT("[BTN_COMBO] Combo %d clicked\n", combo_id);
            break;

        case BUTTON_EVENT_COMBO_HOLD:
            if (combo->on_combo_hold) {
                combo->on_combo_hold(combo->button_ids, combo->button_count, data);
            }
            DB_LOG_BT("[BTN_COMBO] Combo %d held for %dms\n", combo_id, data);
            break;

        default:
            break;
    }
}

static void button_trigger_pattern_event(uint8_t pattern_id) {
    button_pattern_t *pattern = &g_button_manager.patterns[pattern_id];

    if (pattern->on_pattern_match) {
        pattern->on_pattern_match(pattern->pattern, pattern->pattern_length);
    }

    DB_LOG_BT("[BTN_PATTERN] Pattern %d matched!\n", pattern_id);
}

static bool button_is_combo_complete(uint8_t combo_id) {
    button_combo_t *combo = &g_button_manager.combos[combo_id];
    
    // Check if all buttons in combo are pressed
    for (uint8_t i = 0; i < combo->button_count; i++) {
        uint8_t btn_id = combo->button_ids[i];
        if (!button_is_pressed(btn_id)) {
            return false;
        }
    }
    
    return true;
}

static bool button_check_pattern_step(uint8_t pattern_id, uint8_t button_id) {
    button_pattern_t *pattern = &g_button_manager.patterns[pattern_id];
    
    if (pattern->current_position >= pattern->pattern_length) {
        return false;
    }
    
    return (pattern->pattern[pattern->current_position] == button_id);
}
