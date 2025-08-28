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

#define DB_LOG_BT(...)	ULOGA(__VA_ARGS__)
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
    DB_LOG_BT("[BTN_MGR] Initialized with HAL\n");
    return true;
}

void button_manager_deinit(void) {
    g_button_manager.initialized = false;
    g_button_manager.button_count = 0;
    memset(&g_button_manager.buttons, 0, sizeof(g_button_manager.buttons));
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

    button_config_t *btn = &g_button_manager.buttons[button_id];

    // Disable and clear button
    btn->enabled = false;
    btn->initialized = false;
    memset(btn, 0, sizeof(button_config_t));

    DB_LOG_BT("[BTN] Removed button %d\n", button_id);
    return true;
}

bool button_set_press_callback(uint8_t button_id, void (*callback)(uint8_t)) {
    if (!button_validate_id(button_id)) {
        return false;
    }

    g_button_manager.buttons[button_id].on_press = callback;
    return true;
}

bool button_set_release_callback(uint8_t button_id, void (*callback)(uint8_t)) {
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
    g_button_manager.buttons[button_id].multiclick_level_count = MAX_MULTI_CLICK;
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

    uint8_t removed_clicks = btn->multiclick_levels[level_index].click_count;

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

    for (uint8_t i = 0; i < g_button_manager.button_count; i++) {
        if (g_button_manager.buttons[i].initialized) {
            button_process_state_machine(i);
        }
    }
}

bool button_process_single(uint8_t button_id) {
    if (!button_validate_id(button_id)) {
        return false;
    }

    button_process_state_machine(button_id);
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

static void button_process_state_machine(uint8_t button_id) {
    button_config_t *btn = &g_button_manager.buttons[button_id];

    if (!btn->enabled) {
        return;
    }

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
                    if (btn->multiclick_level_count > 0) {
                        btn->state = BUTTON_STATE_WAIT_MULTICLICK;
                        // Don't reset to IDLE yet, wait for potential additional clicks
                    } else {
                        // No multi-click configured, trigger single click and reset
                        if (btn->on_multiclick) {
                            btn->on_multiclick(button_id, btn->click_count);
                        }
                        button_trigger_event(button_id, BUTTON_EVENT_MULTI_CLICK, btn->click_count);
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

    switch (event) {
        case BUTTON_EVENT_PRESS:
            if (btn->on_press) {
                btn->on_press(button_id);
            }
            DB_LOG_BT("[BTN%d] PRESS\n", button_id);
            break;

        case BUTTON_EVENT_RELEASE:
            if (btn->on_release) {
                btn->on_release(button_id);
            }
            DB_LOG_BT("[BTN%d] RELEASE (%dms)\n", button_id, data);
            break;

        case BUTTON_EVENT_CLICK:
            if (btn->on_click) {
                btn->on_click(button_id);
            }
            DB_LOG_BT("[BTN%d] CLICK (%dms)\n", button_id, data);
            break;

        case BUTTON_EVENT_MULTI_CLICK:
            if (btn->on_multiclick) {
                btn->on_multiclick(button_id, btn->click_count);
            }
            DB_LOG_BT("[BTN%d] MULTI_CLICK (%d clicks)\n", button_id, data);
            break;

        case BUTTON_EVENT_HOLD_START:
            if (btn->on_hold_start) {
                btn->on_hold_start(button_id);
            }
            DB_LOG_BT("[BTN%d] HOLD_START\n", button_id);
            break;

        case BUTTON_EVENT_HOLD_END:
            if (btn->on_hold_end) {
                btn->on_hold_end(button_id);
            }
            DB_LOG_BT("[BTN%d] HOLD_END (%dms)\n", button_id, data);
            break;

        case BUTTON_EVENT_HOLD_LEVEL:
            DB_LOG_BT("[BTN%d] HOLD_LEVEL (%dms)\n", button_id, data);
            break;

        default:
            break;
    }
}

static void button_trigger_multiclick(uint8_t button_id) {
    button_config_t *btn = &g_button_manager.buttons[button_id];

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

    // Always trigger multi-click event
    button_trigger_event(button_id, BUTTON_EVENT_MULTI_CLICK, btn->click_count);

    if (level_found) {
        DB_LOG_BT("[BTN%d] Multi-click level triggered: %d clicks\n", button_id, btn->click_count);
    } else {
        DB_LOG_BT("[BTN%d] Multi-click detected (no specific level): %d clicks\n", button_id, btn->click_count);
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
