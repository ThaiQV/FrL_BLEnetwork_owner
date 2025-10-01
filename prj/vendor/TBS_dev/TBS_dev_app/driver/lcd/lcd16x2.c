/**
 * @file lcd16x2.c
 * @brief Driver header file for 16x2 LCD display
 * @author Nghia Hoang
 * @date 2025
 */


#ifndef MASTER_CORE

#include "lcd16x2.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Default timing values (in microseconds)
 */
#define LCD16X2_ENABLE_PULSE_WIDTH_US          1
#define LCD16X2_COMMAND_DELAY_US               50
#define LCD16X2_DATA_DELAY_US                  50
#define LCD16X2_CLEAR_DELAY_MS                 2
#define LCD16X2_HOME_DELAY_MS                  2
#define LCD16X2_INIT_DELAY_MS                  50
#define LCD16X2_POWER_ON_DELAY_MS              15
#define LCD16X2_BUSY_TIMEOUT_MS                10

/**
 * @brief Validate handle and initialization status
 */
static lcd16x2_error_t lcd16x2_validate_handle(lcd16x2_handle_t *handle)
{
    if (handle == NULL) {
        return LCD16X2_ERROR_NULL_POINTER;
    }

    if (!handle->initialized) {
        return LCD16X2_ERROR_NOT_INITIALIZED;
    }

    return LCD16X2_OK;
}

/**
 * @brief Generate enable pulse
 */
static void lcd16x2_enable_pulse(lcd16x2_handle_t *handle)
{
    handle->config.lcd_pin_set( LCD16X2_PIN_E,true);
    if (handle->config.delay_us) {
        handle->config.delay_us(LCD16X2_ENABLE_PULSE_WIDTH_US);
    }
    handle->config.lcd_pin_set( LCD16X2_PIN_E,false);
}

/**
 * @brief Set data pins for 4-bit mode
 */
static void lcd16x2_set_data_4bit(lcd16x2_handle_t *handle, uint8_t data)
{
    handle->config.lcd_pin_set(LCD16X2_PIN_D4, (data >> 0) & 0x01);
    handle->config.lcd_pin_set(LCD16X2_PIN_D5, (data >> 1) & 0x01);
    handle->config.lcd_pin_set(LCD16X2_PIN_D6, (data >> 2) & 0x01);
    handle->config.lcd_pin_set(LCD16X2_PIN_D7, (data >> 3) & 0x01);
}

/**
 * @brief Set data pins for 8-bit mode
 */
static void lcd16x2_set_data_8bit(lcd16x2_handle_t *handle, uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++) {
        handle->config.lcd_pin_set(i, (data >> i) & 0x01);
    }
}

/**
 * @brief Read data pins for 4-bit mode
 */
static uint8_t lcd16x2_read_data_4bit(lcd16x2_handle_t *handle)
{
    uint8_t data = 0;

    if (handle->config.lcd_pin_read) {
        data |= (handle->config.lcd_pin_read(LCD16X2_PIN_D4) << 0);
        data |= (handle->config.lcd_pin_read(LCD16X2_PIN_D5) << 1);
        data |= (handle->config.lcd_pin_read(LCD16X2_PIN_D6) << 2);
        data |= (handle->config.lcd_pin_read(LCD16X2_PIN_D7) << 3);
    }

    return data;
}

/**
 * @brief Read data pins for 8-bit mode
 */
static uint8_t lcd16x2_read_data_8bit(lcd16x2_handle_t *handle)
{
    uint8_t data = 0;

    if (handle->config.lcd_pin_read) {
        for (uint8_t i = 0; i < 8; i++) {
            data |= (handle->config.lcd_pin_read(i) << i);
        }
    }

    return data;
}

/**
 * @brief Write 4 bits to LCD
 */
static void lcd16x2_write_4bits(lcd16x2_handle_t *handle, uint8_t value)
{
    lcd16x2_set_data_4bit(handle, value);
    lcd16x2_enable_pulse(handle);
}

/**
 * @brief Write 8 bits to LCD
 */
static void lcd16x2_write_8bits(lcd16x2_handle_t *handle, uint8_t value)
{
    if (handle->config.use_4bit_mode) {
        // Send upper 4 bits first
        lcd16x2_write_4bits(handle, value >> 4);
        // Send lower 4 bits
        lcd16x2_write_4bits(handle, value & 0x0F);
    } else {
        // 8-bit mode
        lcd16x2_set_data_8bit(handle, value);
        lcd16x2_enable_pulse(handle);
    }
}

/**
 * @brief Read 8 bits from LCD
 */
static uint8_t lcd16x2_read_8bits(lcd16x2_handle_t *handle)
{
    uint8_t value = 0;

    if (handle->config.use_4bit_mode) {
        // Read upper 4 bits first
        handle->config.lcd_pin_set( LCD16X2_PIN_E,true);
        if (handle->config.delay_us) {
            handle->config.delay_us(LCD16X2_ENABLE_PULSE_WIDTH_US);
        }
        value = lcd16x2_read_data_4bit(handle) << 4;
        handle->config.lcd_pin_set( LCD16X2_PIN_E,false);

        // Read lower 4 bits
        handle->config.lcd_pin_set( LCD16X2_PIN_E,true);
        if (handle->config.delay_us) {
            handle->config.delay_us(LCD16X2_ENABLE_PULSE_WIDTH_US);
        }
        value |= lcd16x2_read_data_4bit(handle);
        handle->config.lcd_pin_set( LCD16X2_PIN_E,false);
    } else {
        // 8-bit mode
        handle->config.lcd_pin_set( LCD16X2_PIN_E,true);
        if (handle->config.delay_us) {
            handle->config.delay_us(LCD16X2_ENABLE_PULSE_WIDTH_US);
        }
        value = lcd16x2_read_data_8bit(handle);
        handle->config.lcd_pin_set( LCD16X2_PIN_E,false);
    }

    return value;
}

/**
 * @brief Wait for LCD to be ready
 */
static lcd16x2_error_t lcd16x2_wait_ready(lcd16x2_handle_t *handle)
{
    if (handle->config.use_busy_flag  && handle->config.lcd_pin_read) {
        // Use busy flag checking
        uint32_t timeout = 0;

        handle->config.lcd_pin_set( LCD16X2_PIN_RS,false);  // Command mode
        handle->config.lcd_pin_set( LCD16X2_PIN_RW,true);   // Read mode

        while (timeout < LCD16X2_BUSY_TIMEOUT_MS) {
            uint8_t status = lcd16x2_read_8bits(handle);

            if (!(status & LCD16X2_BUSY_FLAG)) {
                handle->config.lcd_pin_set( LCD16X2_PIN_RW,false);  // Back to write mode
                return LCD16X2_OK;
            }

            if (handle->config.delay_ms) {
                handle->config.delay_ms(1);
            }
            timeout++;
        }

        handle->config.lcd_pin_set( LCD16X2_PIN_RW,false);  // Back to write mode
        return LCD16X2_ERROR_TIMEOUT;
    } else {
        // Use fixed delay
        if (handle->config.delay_us) {
            handle->config.delay_us(LCD16X2_COMMAND_DELAY_US);
        }
        return LCD16X2_OK;
    }
}

lcd16x2_error_t lcd16x2_send_command(lcd16x2_handle_t *handle, uint8_t cmd)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    result = lcd16x2_wait_ready(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    handle->config.lcd_pin_set( LCD16X2_PIN_RS,false);  // Command mode
    if (handle->config.lcd_pin_set) {
        handle->config.lcd_pin_set( LCD16X2_PIN_RW,false);  // Write mode
    }

    lcd16x2_write_8bits(handle, cmd);

    // Special delays for certain commands
    if (cmd == LCD16X2_CMD_CLEAR_DISPLAY || cmd == LCD16X2_CMD_RETURN_HOME) {
        if (handle->config.delay_ms) {
            handle->config.delay_ms(LCD16X2_CLEAR_DELAY_MS);
        }
    }

    return LCD16X2_OK;
}

lcd16x2_error_t lcd16x2_send_data(lcd16x2_handle_t *handle, uint8_t data)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    result = lcd16x2_wait_ready(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    handle->config.lcd_pin_set( LCD16X2_PIN_RS,true);   // Data mode
    if (handle->config.lcd_pin_set) {
        handle->config.lcd_pin_set( LCD16X2_PIN_RW,false);  // Write mode
    }

    lcd16x2_write_8bits(handle, data);

    // Update cursor position
    handle->current_col++;
    if (handle->current_col >= handle->config.cols) {
        handle->current_col = 0;
        handle->current_row = (handle->current_row + 1) % handle->config.rows;
    }

    return LCD16X2_OK;
}

lcd16x2_error_t lcd16x2_init(lcd16x2_handle_t *handle, const lcd16x2_config_t *config)
{
    if (handle == NULL || config == NULL) {
        return LCD16X2_ERROR_NULL_POINTER;
    }

    // Check required function pointers
    if (config->lcd_pin_init == NULL)
    {
        return LCD16X2_ERROR_NULL_POINTER;
    }

    if (config->lcd_pin_set == NULL)
    {
        return LCD16X2_ERROR_NULL_POINTER;
    }


    // Copy configuration
    handle->config = *config;

    handle->config.lcd_pin_init();
    handle->initialized = true;

    // Set default values if not specified
    if (handle->config.rows == 0) {
        handle->config.rows = 2;
    }
    if (handle->config.cols == 0) {
        handle->config.cols = 16;
    }

    // Initialize state
    handle->current_row = 0;
    handle->current_col = 0;
    handle->display_on = true;
    handle->cursor_on = false;
    handle->blink_on = false;

    // Power-on delay
    if (handle->config.delay_ms) {
        handle->config.delay_ms(LCD16X2_POWER_ON_DELAY_MS);
    }

    // Initialize pins
    handle->config.lcd_pin_set( LCD16X2_PIN_RS,false);
    handle->config.lcd_pin_set( LCD16X2_PIN_E,false);
    handle->config.lcd_pin_set( LCD16X2_PIN_RW,false);


    // Initialization sequence
    if (handle->config.delay_ms) {
        handle->config.delay_ms(LCD16X2_INIT_DELAY_MS);
    }

    if (handle->config.use_4bit_mode) {
        // 4-bit initialization sequence
        lcd16x2_write_4bits(handle, 0x03);
        if (handle->config.delay_ms) {
            handle->config.delay_ms(5);
        }

        lcd16x2_write_4bits(handle, 0x03);
        if (handle->config.delay_us) {
            handle->config.delay_us(150);
        }

        lcd16x2_write_4bits(handle, 0x03);
        if (handle->config.delay_us) {
            handle->config.delay_us(150);
        }

        lcd16x2_write_4bits(handle, 0x02);  // Set to 4-bit mode
        if (handle->config.delay_us) {
            handle->config.delay_us(150);
        }
    } else {
        // 8-bit initialization sequence
        lcd16x2_write_8bits(handle, 0x30);
        if (handle->config.delay_ms) {
            handle->config.delay_ms(5);
        }

        lcd16x2_write_8bits(handle, 0x30);
        if (handle->config.delay_us) {
            handle->config.delay_us(150);
        }

        lcd16x2_write_8bits(handle, 0x30);
        if (handle->config.delay_us) {
            handle->config.delay_us(150);
        }
    }

    // Function Set
    uint8_t function_set = LCD16X2_CMD_FUNCTION_SET;
    if (!handle->config.use_4bit_mode) {
        function_set |= LCD16X2_8BIT_MODE;
    }
    if (handle->config.rows > 1) {
        function_set |= LCD16X2_2LINE;
    }
    function_set |= LCD16X2_5x8_DOTS;

    lcd16x2_send_command(handle, function_set);

    // Display Control
    lcd16x2_send_command(handle, LCD16X2_CMD_DISPLAY_CONTROL | LCD16X2_DISPLAY_ON);

    // Entry Mode Set
    lcd16x2_send_command(handle, LCD16X2_CMD_ENTRY_MODE_SET | LCD16X2_ENTRY_LEFT);

    // Clear Display
    lcd16x2_clear(handle);

    return LCD16X2_OK;
}

lcd16x2_error_t lcd16x2_deinit(lcd16x2_handle_t *handle)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    // Clear display and turn off
    lcd16x2_clear(handle);
    lcd16x2_send_command(handle, LCD16X2_CMD_DISPLAY_CONTROL);

    handle->initialized = false;

    return LCD16X2_OK;
}

lcd16x2_error_t lcd16x2_clear(lcd16x2_handle_t *handle)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    result = lcd16x2_send_command(handle, LCD16X2_CMD_CLEAR_DISPLAY);
    if (result == LCD16X2_OK) {
        handle->current_row = 0;
        handle->current_col = 0;
    }

    return result;
}

lcd16x2_error_t lcd16x2_home(lcd16x2_handle_t *handle)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    result = lcd16x2_send_command(handle, LCD16X2_CMD_RETURN_HOME);
    if (result == LCD16X2_OK) {
        handle->current_row = 0;
        handle->current_col = 0;
    }

    return result;
}

lcd16x2_error_t lcd16x2_set_cursor(lcd16x2_handle_t *handle, uint8_t row, uint8_t col)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    if (row >= handle->config.rows || col >= handle->config.cols) {
        return LCD16X2_ERROR_INVALID_PARAM;
    }

    uint8_t address;
    switch (row) {
        case 0:
            address = LCD16X2_ROW_0_ADDR + col;
            break;
        case 1:
            address = LCD16X2_ROW_1_ADDR + col;
            break;
        default:
            return LCD16X2_ERROR_INVALID_PARAM;
    }

    result = lcd16x2_send_command(handle, LCD16X2_CMD_SET_DDRAM_ADDR | address);
    if (result == LCD16X2_OK) {
        handle->current_row = row;
        handle->current_col = col;
    }

    return result;
}

lcd16x2_error_t lcd16x2_print_char(lcd16x2_handle_t *handle, char ch)
{
    return lcd16x2_send_data(handle, (uint8_t)ch);
}

//lcd16x2_error_t lcd16x2_print_string(lcd16x2_handle_t *handle, const char *str)
//{
//    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
//    if (result != LCD16X2_OK) {
//        return result;
//    }
//
//    if (str == NULL) {
//        return LCD16X2_ERROR_NULL_POINTER;
//    }
//
//    while (*str) {
//        result = lcd16x2_print_char(handle, *str);
//        if (result != LCD16X2_OK) {
//            return result;
//        }
//        str++;
//    }
//
//    return LCD16X2_OK;
//}

lcd16x2_error_t lcd16x2_print_string(lcd16x2_handle_t *handle, const char *str)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }
    if (str == NULL) {
        return LCD16X2_ERROR_NULL_POINTER;
    }

    uint8_t char_count = 0;
    const uint8_t MAX_CHARS = handle->config.cols;

    while (*str && char_count < MAX_CHARS) {
        result = lcd16x2_print_char(handle, *str);
        if (result != LCD16X2_OK) {
            return result;
        }
        str++;
        char_count++;
    }

    while (char_count < MAX_CHARS) {
        result = lcd16x2_print_char(handle, ' ');
        if (result != LCD16X2_OK) {
            return result;
        }
        char_count++;
    }

    return LCD16X2_OK;
}

lcd16x2_error_t lcd16x2_print_at(lcd16x2_handle_t *handle, uint8_t row, uint8_t col, const char *str)
{
    lcd16x2_error_t result = lcd16x2_set_cursor(handle, row, col);
    if (result != LCD16X2_OK) {
        return result;
    }

    return lcd16x2_print_string(handle, str);
}

lcd16x2_error_t lcd16x2_printf(lcd16x2_handle_t *handle, const char *format, ...)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    if (format == NULL) {
        return LCD16X2_ERROR_NULL_POINTER;
    }

    char buffer[64];  // Adjust size as needed
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    return lcd16x2_print_string(handle, buffer);
}

lcd16x2_error_t lcd16x2_display_control(lcd16x2_handle_t *handle, bool display_on)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    uint8_t cmd = LCD16X2_CMD_DISPLAY_CONTROL;

    if (display_on) {
        cmd |= LCD16X2_DISPLAY_ON;
        handle->display_on = true;
    } else {
        handle->display_on = false;
    }

    if (handle->cursor_on) {
        cmd |= LCD16X2_CURSOR_ON;
    }

    if (handle->blink_on) {
        cmd |= LCD16X2_BLINK_ON;
    }

    return lcd16x2_send_command(handle, cmd);
}

lcd16x2_error_t lcd16x2_cursor_control(lcd16x2_handle_t *handle, bool cursor_on)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    uint8_t cmd = LCD16X2_CMD_DISPLAY_CONTROL;

    if (handle->display_on) {
        cmd |= LCD16X2_DISPLAY_ON;
    }

    if (cursor_on) {
        cmd |= LCD16X2_CURSOR_ON;
        handle->cursor_on = true;
    } else {
        handle->cursor_on = false;
    }

    if (handle->blink_on) {
        cmd |= LCD16X2_BLINK_ON;
    }

    return lcd16x2_send_command(handle, cmd);
}

lcd16x2_error_t lcd16x2_blink_control(lcd16x2_handle_t *handle, bool blink_on)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    uint8_t cmd = LCD16X2_CMD_DISPLAY_CONTROL;

    if (handle->display_on) {
        cmd |= LCD16X2_DISPLAY_ON;
    }

    if (handle->cursor_on) {
        cmd |= LCD16X2_CURSOR_ON;
    }

    if (blink_on) {
        cmd |= LCD16X2_BLINK_ON;
        handle->blink_on = true;
    } else {
        handle->blink_on = false;
    }

    return lcd16x2_send_command(handle, cmd);
}

lcd16x2_error_t lcd16x2_shift_display_left(lcd16x2_handle_t *handle)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    return lcd16x2_send_command(handle, LCD16X2_CMD_CURSOR_SHIFT | LCD16X2_DISPLAY_MOVE | LCD16X2_MOVE_LEFT);
}

lcd16x2_error_t lcd16x2_shift_display_right(lcd16x2_handle_t *handle)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    return lcd16x2_send_command(handle, LCD16X2_CMD_CURSOR_SHIFT | LCD16X2_DISPLAY_MOVE | LCD16X2_MOVE_RIGHT);
}

lcd16x2_error_t lcd16x2_shift_cursor_left(lcd16x2_handle_t *handle)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    result = lcd16x2_send_command(handle, LCD16X2_CMD_CURSOR_SHIFT | LCD16X2_CURSOR_MOVE | LCD16X2_MOVE_LEFT);
    if (result == LCD16X2_OK) {
        if (handle->current_col > 0) {
            handle->current_col--;
        }
    }

    return result;
}

lcd16x2_error_t lcd16x2_shift_cursor_right(lcd16x2_handle_t *handle)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    result = lcd16x2_send_command(handle, LCD16X2_CMD_CURSOR_SHIFT | LCD16X2_CURSOR_MOVE | LCD16X2_MOVE_RIGHT);
    if (result == LCD16X2_OK) {
        if (handle->current_col < handle->config.cols - 1) {
            handle->current_col++;
        }
    }

    return result;
}

lcd16x2_error_t lcd16x2_create_custom_char(lcd16x2_handle_t *handle, uint8_t location, const lcd16x2_custom_char_t *custom_char)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    if (location > 7 || custom_char == NULL) {
        return LCD16X2_ERROR_INVALID_PARAM;
    }

    // Set CGRAM address
    result = lcd16x2_send_command(handle, LCD16X2_CMD_SET_CGRAM_ADDR | (location << 3));
    if (result != LCD16X2_OK) {
        return result;
    }

    // Write character pattern
    for (uint8_t i = 0; i < 8; i++) {
        result = lcd16x2_send_data(handle, custom_char->pattern[i]);
        if (result != LCD16X2_OK) {
            return result;
        }
    }

    // Return to DDRAM mode
    return lcd16x2_set_cursor(handle, handle->current_row, handle->current_col);
}

lcd16x2_error_t lcd16x2_print_custom_char(lcd16x2_handle_t *handle, uint8_t location)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    if (location > 7) {
        return LCD16X2_ERROR_INVALID_PARAM;
    }

    return lcd16x2_send_data(handle, location);
}

lcd16x2_error_t lcd16x2_is_busy(lcd16x2_handle_t *handle, bool *is_busy)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    if (is_busy == NULL) {
        return LCD16X2_ERROR_NULL_POINTER;
    }

    if (!handle->config.use_busy_flag || !handle->config.lcd_pin_read) {
        *is_busy = false;  // Can't check busy flag
        return LCD16X2_OK;
    }

    handle->config.lcd_pin_set( LCD16X2_PIN_RS,false);  // Command mode
    handle->config.lcd_pin_set( LCD16X2_PIN_RW,true);   // Read mode

    uint8_t status = lcd16x2_read_8bits(handle);

    handle->config.lcd_pin_set( LCD16X2_PIN_RW,false);  // Back to write mode

    *is_busy = (status & LCD16X2_BUSY_FLAG) != 0;

    return LCD16X2_OK;
}

lcd16x2_error_t lcd16x2_set_entry_mode(lcd16x2_handle_t *handle, bool left_to_right, bool auto_shift)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    uint8_t cmd = LCD16X2_CMD_ENTRY_MODE_SET;

    if (left_to_right) {
        cmd |= LCD16X2_ENTRY_LEFT;
    } else {
        cmd |= LCD16X2_ENTRY_RIGHT;
    }

    if (auto_shift) {
        cmd |= LCD16X2_ENTRY_SHIFT_INCREMENT;
    }

    return lcd16x2_send_command(handle, cmd);
}

lcd16x2_error_t lcd16x2_get_cursor_position(lcd16x2_handle_t *handle, uint8_t *row, uint8_t *col)
{
    lcd16x2_error_t result = lcd16x2_validate_handle(handle);
    if (result != LCD16X2_OK) {
        return result;
    }

    if (row == NULL || col == NULL) {
        return LCD16X2_ERROR_NULL_POINTER;
    }

    *row = handle->current_row;
    *col = handle->current_col;

    return LCD16X2_OK;
}

#endif /* MASTER_CORE*/