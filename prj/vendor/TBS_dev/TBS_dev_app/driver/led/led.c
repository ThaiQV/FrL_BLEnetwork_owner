/**
 * @file led_driver.c
 * @brief Flexible multi-LED driver implementation
 * @author Nghia Hoang
 * @date 2025
 */

#include "led.h"
#include <string.h>

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

static led_manager_t g_led_manager = {0};

// =============================================================================
// PRIVATE FUNCTION DECLARATIONS
// =============================================================================

static bool is_valid_led_id(uint8_t led_id);
static void led_update_physical_state(uint8_t led_id, bool state);
static void led_set_state(uint8_t led_id, led_state_t new_state);
static void led_process_blink(uint8_t led_id);
static void led_process_pattern(uint8_t led_id);
static uint32_t calculate_on_time(uint32_t period_ms, uint8_t duty_cycle);
static uint32_t calculate_off_time(uint32_t period_ms, uint8_t duty_cycle);

// =============================================================================
// PUBLIC FUNCTION IMPLEMENTATIONS
// =============================================================================

bool led_manager_init(const led_hal_t *hal)
{
    if (!hal || !hal->led_pin_init || !hal->led_pin_set ||
        !hal->led_pin_get || !hal->get_system_time_ms) {
        return false;
    }

    // Initialize manager
    memset(&g_led_manager, 0, sizeof(led_manager_t));
    g_led_manager.hal = *hal;
    g_led_manager.led_count = 0;
    g_led_manager.initialized = true;

    // Initialize hardware
    if (g_led_manager.hal.led_pin_init_all) {
        g_led_manager.hal.led_pin_init_all();
    }

    return true;
}

void led_manager_deinit(void)
{
    if (!g_led_manager.initialized) {
        return;
    }

    // Turn off all LEDs
    led_all_off();

    // Clear manager
    memset(&g_led_manager, 0, sizeof(led_manager_t));
}

uint8_t led_add(uint8_t gpio_pin, bool active_high)
{
    if (!g_led_manager.initialized || g_led_manager.led_count >= MAX_LEDS) {
        return 0xFF;
    }

    // Check if pin already used
    for (uint8_t i = 0; i < g_led_manager.led_count; i++) {
        if (g_led_manager.leds[i].initialized && g_led_manager.leds[i].gpio_pin == gpio_pin) {
            return 0xFF; // Pin already in use
        }
    }

    uint8_t led_id = g_led_manager.led_count;
    led_config_t *led = &g_led_manager.leds[led_id];

    // Initialize LED configuration
    memset(led, 0, sizeof(led_config_t));
    led->gpio_pin = gpio_pin;
    led->active_high = active_high;
    led->mode = LED_MODE_OFF;
    led->state = LED_STATE_IDLE;
    led->enabled = true;
    led->physical_state = false;
    led->pattern_count = 0;
    led->active_pattern = 0;

    // Initialize hardware pin
    if (g_led_manager.hal.led_pin_init) {
        g_led_manager.hal.led_pin_init((led_pin_t)gpio_pin, active_high);
    }

    led->initialized = true;
    led_update_physical_state(led_id, false); // Start with LED off

    g_led_manager.led_count++;
    return led_id;
}

bool led_remove(uint8_t led_id)
{
    if (!is_valid_led_id(led_id)) {
        return false;
    }

    led_config_t *led = &g_led_manager.leds[led_id];

    // Turn off LED before removing
    led_off(led_id);

    // Clear configuration
    memset(led, 0, sizeof(led_config_t));

    // If this was the last LED, decrease count
    if (led_id == g_led_manager.led_count - 1) {
        g_led_manager.led_count--;
    }

    return true;
}

bool led_on(uint8_t led_id)
{
    if (!is_valid_led_id(led_id)) {
        return false;
    }

    led_config_t *led = &g_led_manager.leds[led_id];
    if (!led->enabled) {
        return false;
    }

    led->mode = LED_MODE_ON;
    led_set_state(led_id, LED_STATE_ON);
    led_update_physical_state(led_id, true);

    return true;
}

bool led_off(uint8_t led_id)
{
    if (!is_valid_led_id(led_id)) {
        return false;
    }

    led_config_t *led = &g_led_manager.leds[led_id];

    led->mode = LED_MODE_OFF;
    led_set_state(led_id, LED_STATE_IDLE);
    led_update_physical_state(led_id, false);

    return true;
}

bool led_toggle(uint8_t led_id)
{
    if (!is_valid_led_id(led_id)) {
        return false;
    }

    led_config_t *led = &g_led_manager.leds[led_id];
    if (!led->enabled) {
        return false;
    }

    if (led->mode == LED_MODE_ON) {
        return led_off(led_id);
    } else {
        return led_on(led_id);
    }
}

bool led_blink(uint8_t led_id, uint32_t period_ms)
{
    return led_blink_duty(led_id, period_ms, DEFAULT_DUTY_CYCLE);
}

bool led_blink_duty(uint8_t led_id, uint32_t period_ms, uint8_t duty_cycle)
{
    return led_blink_count(led_id, period_ms, duty_cycle, 0); // 0 = infinite
}

bool led_blink_count(uint8_t led_id, uint32_t period_ms, uint8_t duty_cycle, uint16_t count)
{
    if (!is_valid_led_id(led_id) || duty_cycle > 100 || period_ms == 0) {
        return false;
    }

    led_config_t *led = &g_led_manager.leds[led_id];
    if (!led->enabled) {
        return false;
    }

    // Set mode
    led->mode = (count == 0) ? LED_MODE_BLINK_CONTINUOUS : LED_MODE_BLINK_COUNTED;

    // Setup pattern 0 as default blink pattern
    led->patterns[0].period_ms = period_ms;
    led->patterns[0].duty_cycle = duty_cycle;
    led->patterns[0].blink_count = count;
    led->patterns[0].pause_after_ms = 0;
    led->patterns[0].repeat = (count == 0);
    led->patterns[0].enabled = true;

    if (led->pattern_count == 0) {
        led->pattern_count = 1;
    }
    led->active_pattern = 0;

    // Initialize timing
    led->cycle_start_time = g_led_manager.hal.get_system_time_ms();
    led->current_blink_count = 0;

    // Start with LED on
    led_set_state(led_id, LED_STATE_BLINK_ON);
    led_update_physical_state(led_id, true);

    return true;
}

bool led_stop(uint8_t led_id)
{
    return led_off(led_id);
}

uint8_t led_add_pattern(uint8_t led_id, uint32_t period_ms, uint8_t duty_cycle,
                       uint16_t blink_count, uint32_t pause_after_ms, bool repeat)
{
    if (!is_valid_led_id(led_id) || duty_cycle > 100 || period_ms == 0) {
        return 0xFF;
    }

    led_config_t *led = &g_led_manager.leds[led_id];
    if (led->pattern_count >= MAX_BLINK_PATTERNS) {
        return 0xFF;
    }

    uint8_t pattern_id = led->pattern_count;
    led_blink_pattern_t *pattern = &led->patterns[pattern_id];

    pattern->period_ms = period_ms;
    pattern->duty_cycle = duty_cycle;
    pattern->blink_count = blink_count;
    pattern->pause_after_ms = pause_after_ms;
    pattern->repeat = repeat;
    pattern->enabled = true;

    led->pattern_count++;
    return pattern_id;
}

bool led_remove_pattern(uint8_t led_id, uint8_t pattern_id)
{
    if (!is_valid_led_id(led_id)) {
        return false;
    }

    led_config_t *led = &g_led_manager.leds[led_id];
    if (pattern_id >= led->pattern_count) {
        return false;
    }

    // Stop pattern if it's currently active
    if (led->active_pattern == pattern_id && led->mode == LED_MODE_PATTERN) {
        led_stop(led_id);
    }

    // Shift patterns down to fill gap
    for (uint8_t i = pattern_id; i < led->pattern_count - 1; i++) {
        led->patterns[i] = led->patterns[i + 1];
    }

    led->pattern_count--;

    // Adjust active pattern index if necessary
    if (led->active_pattern > pattern_id) {
        led->active_pattern--;
    } else if (led->active_pattern == pattern_id) {
        led->active_pattern = 0;
    }

    return true;
}

bool led_start_pattern(uint8_t led_id, uint8_t pattern_id)
{
    if (!is_valid_led_id(led_id)) {
        return false;
    }

    led_config_t *led = &g_led_manager.leds[led_id];
    if (pattern_id >= led->pattern_count || !led->patterns[pattern_id].enabled || !led->enabled) {
        return false;
    }

    led->mode = LED_MODE_PATTERN;
    led->active_pattern = pattern_id;
    led->current_blink_count = 0;
    led->cycle_start_time = g_led_manager.hal.get_system_time_ms();

    led_set_state(led_id, LED_STATE_BLINK_ON);
    led_update_physical_state(led_id, true);

    return true;
}

bool led_set_pattern_enabled(uint8_t led_id, uint8_t pattern_id, bool enabled)
{
    if (!is_valid_led_id(led_id)) {
        return false;
    }

    led_config_t *led = &g_led_manager.leds[led_id];
    if (pattern_id >= led->pattern_count) {
        return false;
    }

    led->patterns[pattern_id].enabled = enabled;

    // Stop pattern if it's currently active and being disabled
    if (!enabled && led->active_pattern == pattern_id && led->mode == LED_MODE_PATTERN) {
        led_stop(led_id);
    }

    return true;
}

bool led_set_state_callback(uint8_t led_id, void (*callback)(uint8_t, bool))
{
    if (!is_valid_led_id(led_id)) {
        return false;
    }

    g_led_manager.leds[led_id].on_state_change = callback;
    return true;
}

bool led_set_blink_complete_callback(uint8_t led_id, void (*callback)(uint8_t))
{
    if (!is_valid_led_id(led_id)) {
        return false;
    }

    g_led_manager.leds[led_id].on_blink_complete = callback;
    return true;
}

bool led_set_pattern_complete_callback(uint8_t led_id, void (*callback)(uint8_t, uint8_t))
{
    if (!is_valid_led_id(led_id)) {
        return false;
    }

    g_led_manager.leds[led_id].on_pattern_complete = callback;
    return true;
}

bool led_set_enabled(uint8_t led_id, bool enabled)
{
    if (!is_valid_led_id(led_id)) {
        return false;
    }

    led_config_t *led = &g_led_manager.leds[led_id];
    led->enabled = enabled;

    if (!enabled) {
        led_off(led_id);
    }

    return true;
}

led_mode_t led_get_mode(uint8_t led_id)
{
    if (!is_valid_led_id(led_id)) {
        return LED_MODE_OFF;
    }

    return g_led_manager.leds[led_id].mode;
}

led_state_t led_get_state(uint8_t led_id)
{
    if (!is_valid_led_id(led_id)) {
        return LED_STATE_IDLE;
    }

    return g_led_manager.leds[led_id].state;
}

bool led_is_on(uint8_t led_id)
{
    if (!is_valid_led_id(led_id)) {
        return false;
    }

    return g_led_manager.leds[led_id].physical_state;
}

uint16_t led_get_blink_count(uint8_t led_id)
{
    if (!is_valid_led_id(led_id)) {
        return 0;
    }

    return g_led_manager.leds[led_id].current_blink_count;
}

void led_process_all(void)
{
    if (!g_led_manager.initialized) {
        return;
    }

    for (uint8_t i = 0; i < g_led_manager.led_count; i++) {
        if (g_led_manager.leds[i].initialized && g_led_manager.leds[i].enabled) {
            led_process_single(i);
        }
    }
}

bool led_process_single(uint8_t led_id)
{
    if (!is_valid_led_id(led_id)) {
        return false;
    }

    led_config_t *led = &g_led_manager.leds[led_id];
    if (!led->enabled || !led->initialized) {
        return false;
    }

    switch (led->mode) {
        case LED_MODE_OFF:
        case LED_MODE_ON:
            // Static modes - no processing needed
            break;

        case LED_MODE_BLINK_CONTINUOUS:
        case LED_MODE_BLINK_COUNTED:
            led_process_blink(led_id);
            break;

        case LED_MODE_PATTERN:
            led_process_pattern(led_id);
            break;

        default:
            break;
    }

    return true;
}

void led_get_stats(uint8_t *total_leds, uint8_t *active_leds)
{
    if (total_leds) {
        *total_leds = g_led_manager.led_count;
    }

    if (active_leds) {
        uint8_t active = 0;
        for (uint8_t i = 0; i < g_led_manager.led_count; i++) {
            if (g_led_manager.leds[i].enabled &&
                g_led_manager.leds[i].mode != LED_MODE_OFF) {
                active++;
            }
        }
        *active_leds = active;
    }
}

void led_all_off(void)
{
    for (uint8_t i = 0; i < g_led_manager.led_count; i++) {
        led_off(i);
    }
}

void led_all_on(void)
{
    for (uint8_t i = 0; i < g_led_manager.led_count; i++) {
        led_on(i);
    }
}

void led_all_stop(void)
{
    for (uint8_t i = 0; i < g_led_manager.led_count; i++) {
        led_stop(i);
    }
}

// =============================================================================
// PRIVATE FUNCTION IMPLEMENTATIONS
// =============================================================================

static bool is_valid_led_id(uint8_t led_id)
{
    return (g_led_manager.initialized &&
            led_id < g_led_manager.led_count &&
            g_led_manager.leds[led_id].initialized);
}

static void led_update_physical_state(uint8_t led_id, bool state)
{
    if (!is_valid_led_id(led_id)) {
        return;
    }

    led_config_t *led = &g_led_manager.leds[led_id];

    // Apply active_high logic
    bool gpio_state = led->active_high ? state : !state;

    // Update physical LED
    g_led_manager.hal.led_pin_set((led_pin_t)led->gpio_pin, gpio_state);
    led->physical_state = state; // Store logical state, not GPIO state

    // Call state change callback if set
    if (led->on_state_change) {
        led->on_state_change(led_id, state); // Callback with logical state
    }
}

static void led_set_state(uint8_t led_id, led_state_t new_state)
{
    if (!is_valid_led_id(led_id)) {
        return;
    }

    led_config_t *led = &g_led_manager.leds[led_id];
    led->state = new_state;
    led->last_change_time = g_led_manager.hal.get_system_time_ms();
}

static void led_process_blink(uint8_t led_id)
{
    led_config_t *led = &g_led_manager.leds[led_id];
    led_blink_pattern_t *pattern = &led->patterns[led->active_pattern];
    uint32_t current_time = g_led_manager.hal.get_system_time_ms();
    uint32_t elapsed = current_time - led->last_change_time;

    switch (led->state) {
        case LED_STATE_BLINK_ON: {
            uint32_t on_time = calculate_on_time(pattern->period_ms, pattern->duty_cycle);
            if (elapsed >= on_time) {
                led_set_state(led_id, LED_STATE_BLINK_OFF);
                led_update_physical_state(led_id, false);
            }
            break;
        }

        case LED_STATE_BLINK_OFF: {
            uint32_t off_time = calculate_off_time(pattern->period_ms, pattern->duty_cycle);
            if (elapsed >= off_time) {
                led->current_blink_count++;

                // Check if we've completed the required number of blinks
                if (led->mode == LED_MODE_BLINK_COUNTED &&
                    pattern->blink_count > 0 &&
                    led->current_blink_count >= pattern->blink_count) {

                    // Blink sequence complete
                    led_off(led_id);
                    if (led->on_blink_complete) {
                        led->on_blink_complete(led_id);
                    }
                } else {
                    // Continue blinking
                    led_set_state(led_id, LED_STATE_BLINK_ON);
                    led_update_physical_state(led_id, true);
                }
            }
            break;
        }

        default:
            break;
    }
}

static void led_process_pattern(uint8_t led_id)
{
    led_config_t *led = &g_led_manager.leds[led_id];
    led_blink_pattern_t *pattern = &led->patterns[led->active_pattern];
    uint32_t current_time = g_led_manager.hal.get_system_time_ms();
    uint32_t elapsed = current_time - led->last_change_time;

    switch (led->state) {
        case LED_STATE_BLINK_ON: {
            uint32_t on_time = calculate_on_time(pattern->period_ms, pattern->duty_cycle);
            if (elapsed >= on_time) {
                led_set_state(led_id, LED_STATE_BLINK_OFF);
                led_update_physical_state(led_id, false);
            }
            break;
        }

        case LED_STATE_BLINK_OFF: {
            uint32_t off_time = calculate_off_time(pattern->period_ms, pattern->duty_cycle);
            if (elapsed >= off_time) {
                led->current_blink_count++;

                // Check if pattern is complete
                if (pattern->blink_count > 0 &&
                    led->current_blink_count >= pattern->blink_count) {

                    // Pattern complete
                    if (pattern->pause_after_ms > 0) {
                        led_set_state(led_id, LED_STATE_IDLE);
                        led_update_physical_state(led_id, false);
                        // TODO: Implement pause state handling
                    } else if (pattern->repeat) {
                        // Restart pattern
                        led->current_blink_count = 0;
                        led_set_state(led_id, LED_STATE_BLINK_ON);
                        led_update_physical_state(led_id, true);
                    } else {
                        // Pattern finished
                        led_off(led_id);
                    }

                    if (led->on_pattern_complete) {
                        led->on_pattern_complete(led_id, led->active_pattern);
                    }
                } else {
                    // Continue pattern
                    led_set_state(led_id, LED_STATE_BLINK_ON);
                    led_update_physical_state(led_id, true);
                }
            }
            break;
        }

        case LED_STATE_IDLE:
            // Handle pause state
            if (pattern->pause_after_ms > 0) {
                if (elapsed >= pattern->pause_after_ms && pattern->repeat) {
                    led->current_blink_count = 0;
                    led_set_state(led_id, LED_STATE_BLINK_ON);
                    led_update_physical_state(led_id, true);
                }
            }
            break;

        default:
            break;
    }
}

static uint32_t calculate_on_time(uint32_t period_ms, uint8_t duty_cycle)
{
    return (period_ms * duty_cycle) / 100;
}

static uint32_t calculate_off_time(uint32_t period_ms, uint8_t duty_cycle)
{
    return period_ms - calculate_on_time(period_ms, duty_cycle);
}
