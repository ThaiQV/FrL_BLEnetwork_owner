/**
 * @file lcd_app.c
 * @brief LCD Application layer implementation for 16x2 LCD display
 * @author Nghia Hoang
 * @date 2025
 */

#ifndef MASTER_CORE
#include "tl_common.h"
#include "vendor/TBS_dev/TBS_dev_config.h"

#ifdef COUNTER_DEVICE

#include "lcd_app_drv.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/**
 * @brief Private function prototypes
 */
static lcd_app_error_t lcd_app_update_row_display(lcd_app_handle_t *handle, uint8_t row);
static lcd_app_error_t lcd_app_check_timeout(lcd_app_handle_t *handle, uint8_t row);
static void lcd_app_clear_row_buffer(char *buffer, uint8_t length);

/**
 * @brief Initialize LCD App
 */
lcd_app_error_t lcd_app_init_drv(lcd_app_handle_t *handle,
                             lcd16x2_handle_t *lcd_handle,
                             const lcd_app_config_t *config)
{
    // Parameter validation
    if (handle == NULL || lcd_handle == NULL || config == NULL) {
        return LCD_APP_ERROR_NULL_POINTER;
    }

    if (!lcd_handle->initialized) {
        return LCD_APP_ERROR_NOT_INITIALIZED;
    }

    if (config->get_tick == NULL) {
        return LCD_APP_ERROR_NULL_POINTER;
    }

    // Initialize handle
    memset(handle, 0, sizeof(lcd_app_handle_t));
    handle->lcd_handle = lcd_handle;
    handle->config = *config;

    // Initialize row configurations
    for (uint8_t i = 0; i < LCD_APP_MAX_ROWS; i++) {
        memset(handle->rows[i].message, 0, sizeof(handle->rows[i].message));
        handle->rows[i].message_length = 0;
        handle->rows[i].mode = LCD_APP_MODE_STATIC;
        handle->rows[i].scroll_position = 0;
        handle->rows[i].scroll_start_delay_ms = config->default_scroll_start_delay_ms;
        handle->rows[i].scroll_delay_ms = config->default_scroll_delay_ms;
        handle->rows[i].last_scroll_time = 0;
        handle->rows[i].needs_update = false;

        handle->timeout_ms[i] = config->default_timeout_ms;
        handle->start_time[i] = 0;
        handle->timeout_enabled[i] = false;
    }

    // Clear LCD display
    lcd16x2_clear(handle->lcd_handle);

    handle->initialized = true;
    return LCD_APP_OK;
}

/**
 * @brief Deinitialize LCD App
 */
lcd_app_error_t lcd_app_deinit_drv(lcd_app_handle_t *handle)
{
    if (handle == NULL) {
        return LCD_APP_ERROR_NULL_POINTER;
    }

    if (!handle->initialized) {
        return LCD_APP_ERROR_NOT_INITIALIZED;
    }

    // Clear display
    lcd16x2_clear(handle->lcd_handle);

    // Reset handle
    memset(handle, 0, sizeof(lcd_app_handle_t));

    return LCD_APP_OK;
}

/**
 * @brief Set message for a specific row
 */
lcd_app_error_t lcd_app_set_message(lcd_app_handle_t *handle,
                                   uint8_t row,
                                   char *message,
                                   uint32_t timeout_ms)
{
    return lcd_app_set_message_with_scroll_delay(handle, row, message, timeout_ms,
                                                handle->config.default_scroll_delay_ms);
}

/**
 * @brief Set message for a specific row with custom scroll delay
 */
lcd_app_error_t lcd_app_set_message_with_scroll_delay(lcd_app_handle_t *handle,
                                                     uint8_t row,
                                                     const char *message,
                                                     uint32_t timeout_ms,
                                                     uint32_t scroll_delay_ms)
{
    if (handle == NULL || message == NULL) {
        return LCD_APP_ERROR_NULL_POINTER;
    }

    if (!handle->initialized) {
        return LCD_APP_ERROR_NOT_INITIALIZED;
    }

    if (row >= LCD_APP_MAX_ROWS) {
        return LCD_APP_ERROR_INVALID_PARAM;
    }

    uint8_t msg_len = strlen(message);
    if (msg_len >= LCD_APP_MAX_MESSAGE_LENGTH) {
        return LCD_APP_ERROR_MESSAGE_TOO_LONG;
    }

    // Copy message to row buffer
    strcpy(handle->rows[row].message, message);
    handle->rows[row].message_length = msg_len;
    handle->rows[row].scroll_delay_ms = scroll_delay_ms;
    handle->rows[row].scroll_position = 0;
    handle->rows[row].needs_update = true;

    // Determine display mode
    if (msg_len > LCD_APP_MAX_DISPLAY_WIDTH) {
        handle->rows[row].mode = LCD_APP_MODE_SCROLL;
    } else {
        handle->rows[row].mode = LCD_APP_MODE_STATIC;
    }

    // Setup timeout
    if (timeout_ms > 0) {
        handle->timeout_ms[row] = timeout_ms;
        handle->start_time[row] = handle->config.get_tick();
        handle->timeout_enabled[row] = true;
    } else {
        handle->timeout_enabled[row] = false;
    }

    // Immediate update
    return lcd_app_force_update_row(handle, row);
}

/**
 * @brief Clear a specific row
 */
lcd_app_error_t lcd_app_clear_row(lcd_app_handle_t *handle, uint8_t row)
{
    if (handle == NULL) {
        return LCD_APP_ERROR_NULL_POINTER;
    }

    if (!handle->initialized) {
        return LCD_APP_ERROR_NOT_INITIALIZED;
    }

    if (row >= LCD_APP_MAX_ROWS) {
        return LCD_APP_ERROR_INVALID_PARAM;
    }

    // Clear row configuration
    memset(handle->rows[row].message, 0, sizeof(handle->rows[row].message));
    handle->rows[row].message_length = 0;
    handle->rows[row].mode = LCD_APP_MODE_STATIC;
    handle->rows[row].scroll_position = 0;
    handle->rows[row].needs_update = true;
    handle->timeout_enabled[row] = false;

    // Clear LCD row
    lcd16x2_error_t result = lcd16x2_set_cursor(handle->lcd_handle, row, 0);
    if (result != LCD16X2_OK) {
        return LCD_APP_ERROR_LCD_DRIVER;
    }

    // Print spaces to clear the row
    for (uint8_t i = 0; i < LCD_APP_MAX_DISPLAY_WIDTH; i++) {
        result = lcd16x2_print_char(handle->lcd_handle, ' ');
        if (result != LCD16X2_OK) {
            return LCD_APP_ERROR_LCD_DRIVER;
        }
    }

    return LCD_APP_OK;
}

/**
 * @brief Clear all rows
 */
lcd_app_error_t lcd_app_clear_all(lcd_app_handle_t *handle)
{
    if (handle == NULL) {
        return LCD_APP_ERROR_NULL_POINTER;
    }

    if (!handle->initialized) {
        return LCD_APP_ERROR_NOT_INITIALIZED;
    }

    lcd_app_error_t result;
    for (uint8_t i = 0; i < LCD_APP_MAX_ROWS; i++) {
        result = lcd_app_clear_row(handle, i);
        if (result != LCD_APP_OK) {
            return result;
        }
    }

    return LCD_APP_OK;
}

/**
 * @brief Update display (call this periodically in main loop)
 */
lcd_app_error_t lcd_app_update(lcd_app_handle_t *handle)
{
    if (handle == NULL) {
        return LCD_APP_ERROR_NULL_POINTER;
    }

    if (!handle->initialized) {
        return LCD_APP_ERROR_NOT_INITIALIZED;
    }

    lcd_app_error_t result;
    uint32_t current_time = handle->config.get_tick();

    for (uint8_t row = 0; row < LCD_APP_MAX_ROWS; row++) {
        // Check timeout first
        result = lcd_app_check_timeout(handle, row);
        if (result != LCD_APP_OK) {
            return result;
        }

        // Update scrolling display
        if (handle->rows[row].mode == LCD_APP_MODE_SCROLL &&
            handle->rows[row].message_length > 0) {
            
            if (current_time - handle->start_time[row] > handle->rows[row].scroll_start_delay_ms) {

                if ((current_time - handle->rows[row].last_scroll_time) >= handle->rows[row].scroll_delay_ms) {
                    handle->rows[row].needs_update = true;
                    handle->rows[row].last_scroll_time = current_time;

                    // Update scroll position
                    handle->rows[row].scroll_position++;
                    if (handle->rows[row].scroll_position > handle->rows[row].message_length) {
                        handle->rows[row].scroll_position = 0;
                    }
                }
            }


            
        }

        // Update display if needed
        if (handle->rows[row].needs_update) {
            result = lcd_app_update_row_display(handle, row);
            if (result != LCD_APP_OK) {
                return result;
            }
            handle->rows[row].needs_update = false;
        }
    }

    return LCD_APP_OK;
}

/**
 * @brief Force immediate update of a specific row
 */
lcd_app_error_t lcd_app_force_update_row(lcd_app_handle_t *handle, uint8_t row)
{
    if (handle == NULL) {
        return LCD_APP_ERROR_NULL_POINTER;
    }

    if (!handle->initialized) {
        return LCD_APP_ERROR_NOT_INITIALIZED;
    }

    if (row >= LCD_APP_MAX_ROWS) {
        return LCD_APP_ERROR_INVALID_PARAM;
    }

    handle->rows[row].needs_update = true;
    return lcd_app_update_row_display(handle, row);
}

/**
 * @brief Set scroll delay for a specific row
 */
lcd_app_error_t lcd_app_set_scroll_delay(lcd_app_handle_t *handle,
                                        uint8_t row,
                                        uint32_t scroll_delay_ms)
{
    if (handle == NULL) {
        return LCD_APP_ERROR_NULL_POINTER;
    }

    if (!handle->initialized) {
        return LCD_APP_ERROR_NOT_INITIALIZED;
    }

    if (row >= LCD_APP_MAX_ROWS) {
        return LCD_APP_ERROR_INVALID_PARAM;
    }

    handle->rows[row].scroll_delay_ms = scroll_delay_ms;
    return LCD_APP_OK;
}

/**
 * @brief Reset timeout for a specific row
 */
lcd_app_error_t lcd_app_reset_timeout(lcd_app_handle_t *handle, uint8_t row)
{
    if (handle == NULL) {
        return LCD_APP_ERROR_NULL_POINTER;
    }

    if (!handle->initialized) {
        return LCD_APP_ERROR_NOT_INITIALIZED;
    }

    if (row >= LCD_APP_MAX_ROWS) {
        return LCD_APP_ERROR_INVALID_PARAM;
    }

    handle->start_time[row] = handle->config.get_tick();
    return LCD_APP_OK;
}

/**
 * @brief Enable/disable timeout for a specific row
 */
lcd_app_error_t lcd_app_enable_timeout(lcd_app_handle_t *handle,
                                      uint8_t row,
                                      bool enable)
{
    if (handle == NULL) {
        return LCD_APP_ERROR_NULL_POINTER;
    }

    if (!handle->initialized) {
        return LCD_APP_ERROR_NOT_INITIALIZED;
    }

    if (row >= LCD_APP_MAX_ROWS) {
        return LCD_APP_ERROR_INVALID_PARAM;
    }

    handle->timeout_enabled[row] = enable;
    if (enable) {
        handle->start_time[row] = handle->config.get_tick();
    }

    return LCD_APP_OK;
}

/**
 * @brief Get current message for a specific row
 */
lcd_app_error_t lcd_app_get_message(lcd_app_handle_t *handle,
                                   uint8_t row,
                                   char *message,
                                   uint8_t buffer_size)
{
    if (handle == NULL || message == NULL) {
        return LCD_APP_ERROR_NULL_POINTER;
    }

    if (!handle->initialized) {
        return LCD_APP_ERROR_NOT_INITIALIZED;
    }

    if (row >= LCD_APP_MAX_ROWS) {
        return LCD_APP_ERROR_INVALID_PARAM;
    }

    if (buffer_size <= handle->rows[row].message_length) {
        return LCD_APP_ERROR_INVALID_PARAM;
    }

    strcpy(message, handle->rows[row].message);
    return LCD_APP_OK;
}

/**
 * @brief Check if a row has timed out
 */
lcd_app_error_t lcd_app_is_timeout(lcd_app_handle_t *handle,
                                  uint8_t row,
                                  bool *is_timeout)
{
    if (handle == NULL || is_timeout == NULL) {
        return LCD_APP_ERROR_NULL_POINTER;
    }

    if (!handle->initialized) {
        return LCD_APP_ERROR_NOT_INITIALIZED;
    }

    if (row >= LCD_APP_MAX_ROWS) {
        return LCD_APP_ERROR_INVALID_PARAM;
    }

    *is_timeout = false;

    if (handle->timeout_enabled[row]) {
        uint32_t current_time = handle->config.get_tick();
        uint32_t elapsed_time = current_time - handle->start_time[row];

        if (elapsed_time >= handle->timeout_ms[row]) {
            *is_timeout = true;
        }
    }

    return LCD_APP_OK;
}

/**
 * @brief Private function: Update row display
 */
static lcd_app_error_t lcd_app_update_row_display(lcd_app_handle_t *handle, uint8_t row)
{
    char display_buffer[LCD_APP_MAX_DISPLAY_WIDTH + 1] = {0};
    lcd16x2_error_t result;

    // Set cursor to row start
    result = lcd16x2_set_cursor(handle->lcd_handle, row, 0);
    if (result != LCD16X2_OK) {
        return LCD_APP_ERROR_LCD_DRIVER;
    }

    if (handle->rows[row].message_length == 0) {
        // Empty message, clear the row
        lcd_app_clear_row_buffer(display_buffer, LCD_APP_MAX_DISPLAY_WIDTH);
    } else if (handle->rows[row].mode == LCD_APP_MODE_STATIC) {
        // Static display - show from beginning
        strncpy(display_buffer, handle->rows[row].message, LCD_APP_MAX_DISPLAY_WIDTH);
        display_buffer[LCD_APP_MAX_DISPLAY_WIDTH] = '\0';

        // Pad with spaces if message is shorter than display width
        for (uint8_t i = strlen(display_buffer); i < LCD_APP_MAX_DISPLAY_WIDTH; i++) {
            display_buffer[i] = ' ';
        }
    } else {
        // Scrolling display
        uint8_t start_pos = handle->rows[row].scroll_position;
        uint8_t chars_to_copy = 0;

        if (start_pos < handle->rows[row].message_length) {
            chars_to_copy = handle->rows[row].message_length - start_pos;
            if (chars_to_copy > LCD_APP_MAX_DISPLAY_WIDTH) {
                chars_to_copy = LCD_APP_MAX_DISPLAY_WIDTH;
            }

            strncpy(display_buffer, &handle->rows[row].message[start_pos], chars_to_copy);
        }

        // Fill remaining space with spaces or wrap-around text
        for (uint8_t i = chars_to_copy; i < LCD_APP_MAX_DISPLAY_WIDTH; i++) {
            if (start_pos == handle->rows[row].message_length && i < 3) {
                display_buffer[i] = ' '; // Add some space before wrap-around
            } else if (start_pos == handle->rows[row].message_length && i >= 3) {
                uint8_t wrap_idx = i - 3;
                if (wrap_idx < handle->rows[row].message_length) {
                    display_buffer[i] = handle->rows[row].message[wrap_idx];
                } else {
                    display_buffer[i] = ' ';
                }
            } else {
                display_buffer[i] = ' ';
            }
        }
    }

    display_buffer[LCD_APP_MAX_DISPLAY_WIDTH] = '\0';

    // Display the buffer
    result = lcd16x2_print_string(handle->lcd_handle, display_buffer);
    if (result != LCD16X2_OK) {
        return LCD_APP_ERROR_LCD_DRIVER;
    }

    return LCD_APP_OK;
}

/**
 * @brief Private function: Check timeout
 */
static lcd_app_error_t lcd_app_check_timeout(lcd_app_handle_t *handle, uint8_t row)
{
    bool is_timeout;
    lcd_app_error_t result = lcd_app_is_timeout(handle, row, &is_timeout);

    if (result == LCD_APP_OK && is_timeout) {
        // Disable timeout to prevent multiple callbacks
        handle->timeout_enabled[row] = false;

        // Call timeout callback if available
        if (handle->config.timeout_callback != NULL) {
            handle->config.timeout_callback(row);
        }
    }

    return result;
}

/**
 * @brief Private function: Clear row buffer
 */
static void lcd_app_clear_row_buffer(char *buffer, uint8_t length)
{
    for (uint8_t i = 0; i < length; i++) {
        buffer[i] = ' ';
    }
    buffer[length] = '\0';
}

#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/
