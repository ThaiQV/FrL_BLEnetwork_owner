/**
 * @file et6226m.c
 * @brief Driver source file for ET6226M 4-digit 7-segment LED display driver
 * @author Nghia Hoang
 * @date 2025
 */


#include "led_7_seg.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief 7-segment patterns lookup table
 */
static uint8_t digit_pos1[4] = {
		 ET6226M_DIGIT_0,
		 ET6226M_DIGIT_1,
		 ET6226M_DIGIT_2,
		 ET6226M_DIGIT_3,
};

static const uint8_t digit_patterns[10] = ET6226M_DIGIT_PATTERNS;

/**
 * @brief Character patterns lookup table
 */
static const uint8_t char_patterns[256] = {
    [' '] = ET6226M_CHAR_BLANK,
    ['-'] = ET6226M_CHAR_MINUS,
	[','] = ET6226M_CHAR_8_DOT,
    ['0'] = 0x3F, ['1'] = 0x06, ['2'] = 0x5B, ['3'] = 0x4F, ['4'] = 0x66,
    ['5'] = 0x6D, ['6'] = 0x7D, ['7'] = 0x07, ['8'] = 0x7F, ['9'] = 0x6F,
    ['A'] = 0x77, ['B'] = 0x7C, ['C'] = 0x39, ['D'] = 0x5E, ['E'] = 0x79 + ET6226M_REG_DP /* print "E."*/,
    ['F'] = 0x71, ['H'] = 0x76, ['L'] = 0x38, ['P'] = 0x73, ['U'] = 0x3E,
    ['a'] = 0x5F, ['b'] = 0x7C, ['c'] = 0x58, ['d'] = 0x5E, ['e'] = 0x7B,
    ['f'] = 0x71, ['h'] = 0x74, ['l'] = 0x38, ['n'] = 0x54, ['o'] = 0x5C,
    ['p'] = 0x73, ['r'] = 0x50, ['t'] = 0x78, ['u'] = 0x1C, ['y'] = 0x6E,
};

/**
 * @brief Send byte via SPI-like interface
 * @param handle Pointer to ET6226M handle
 * @param data Byte to send
 */
static void et6226m_send_byte(et6226m_handle_t *handle, uint8_t data)
{
    // data
    for (int i = 7; i >= 0; i--) {
        // Set data line
        handle->config.data_pin_set((data >> i) & 0x01);
        if (handle->config.delay_cycle) {
			handle->config.delay_cycle(1);
		}
        // Clock pulse
        handle->config.clk_pin_set(true);
        if (handle->config.delay_cycle) {
            handle->config.delay_cycle(1);
        }
        handle->config.clk_pin_set(false);

    }
    //ack
    handle->config.data_pin_set(true);
	if (handle->config.delay_cycle) {
		handle->config.delay_cycle(1);
	}
    handle->config.clk_pin_set(true);
	if (handle->config.delay_cycle) {
		handle->config.delay_cycle(1);
	}
	handle->config.clk_pin_set(false);


}

/**
 * @brief Send command to ET6226M
 * @param handle Pointer to ET6226M handle
 * @param reg Register address
 * @param data Data to send
 */
static void et6226m_send_command(et6226m_handle_t *handle, uint8_t cmd, uint8_t data)
{
    if (!handle || !handle->initialized) return;

    //start
    handle->config.data_pin_set(false);
    if (handle->config.delay_cycle) {
		handle->config.delay_cycle(1);
	}
    handle->config.clk_pin_set(false);
//    if (handle->config.delay_cycle) {
//		handle->config.delay_cycle(1);
//	}

    // Send register address
    et6226m_send_byte(handle, cmd);

    // Send data
    et6226m_send_byte(handle, data);

	//stop
	handle->config.clk_pin_set(true);
    if (handle->config.delay_cycle) {
		handle->config.delay_cycle(1);
	}
	handle->config.data_pin_set(true);
    if (handle->config.delay_cycle) {
		handle->config.delay_cycle(1);
	}
}

bool et6226m_init(et6226m_handle_t *handle, const et6226m_config_t *config)
{


    // Copy configuration
    memcpy(&handle->config, config, sizeof(et6226m_config_t));

    // Initialize pins
    handle->config.clk_pin_set(true);  // Clock high
    handle->config.data_pin_set(true); // Data low

    // Small delay for stabilization
    if (handle->config.delay_cycle) {
        handle->config.delay_cycle(10);
    }

    handle->config.pin_init();

    handle->initialized = true;

    handle->control.ET6226_CONTROL_b.Display_switch = ET6226M_DISPLAY_SWITCH_ON;
    handle->control.ET6226_CONTROL_b.Mode_select = ET6226M_MODE_SELECT_8SEGMENT;
    handle->control.ET6226_CONTROL_b.Sleep_command = ET6226M_SLEEP_COMMAND_OPERATION;
    handle->control.ET6226_CONTROL_b.Brightness_adjust = ET6226M_BRIGHTNESS_MAX;

    et6226m_send_command(handle, ET6226M_SEND_COMMAND, handle->control.ET6226_CONTROL);

    return true;
}


void et6226m_deinit(et6226m_handle_t *handle)
{
    if (!handle || !handle->initialized) {
        return;
    }

    // Clear display and shutdown
    et6226m_clear_display(handle);
    et6226m_send_command(handle, ET6226M_REG_SHUTDOWN, 0x00);

    handle->initialized = false;
}

bool et6226m_write_register(et6226m_handle_t *handle, uint8_t reg, uint8_t data)
{
    if (!handle || !handle->initialized) {
        return false;
    }

    et6226m_send_command(handle, reg, data);
    return true;
}

bool et6226m_set_brightness(et6226m_handle_t *handle, uint8_t brightness)
{
    if (!handle || !handle->initialized) {
        return false;
    }

    brightness = brightness > 15 ? 15 : brightness;
    handle->config.brightness = brightness;

    et6226m_send_command(handle, ET6226M_REG_INTENSITY, brightness);
    return true;
}

bool et6226m_enable_display(et6226m_handle_t *handle, bool enable)
{
    if (!handle || !handle->initialized) {
        return false;
    }

    et6226m_send_command(handle, ET6226M_REG_SHUTDOWN, enable ? 0x01 : 0x00);
    return true;
}


bool et6226m_display_digit(et6226m_handle_t *handle, uint8_t digit_pos, uint8_t segment_data)
{

    // Store in buffer
    handle->display_buffer[digit_pos] = segment_data;

    // Send to display
    et6226m_send_command(handle, digit_pos1[digit_pos], segment_data);
    return true;
}

bool et6226m_display_number(et6226m_handle_t *handle, uint16_t number, bool leading_zeros)
{
    if (!handle || !handle->initialized || number > 9999) {
        return false;
    }

    uint8_t digits[4];
    uint16_t temp = number;

    // Extract digits
    digits[0] = temp % 10; temp /= 10;
	digits[1] = temp % 10; temp /= 10;
	digits[2] = temp % 10; temp /= 10;
	digits[3] = temp % 10;
//    et6226m_send_command(handle, 0x48, 0x01);
    // Display digits (right to left)
    for (int i = 0; i < 4; i++) {
        uint8_t digit_data;

        if (!leading_zeros && i > 0) {
            // Check if this and all higher digits are zero
            bool all_zero = true;
            for (int j = i; j < 4; j++) {
                if (digits[j] != 0) {
                    all_zero = false;
                    break;
                }
            }
            if (all_zero) {
                digit_data = ET6226M_CHAR_BLANK;
            } else {
                digit_data = digit_patterns[digits[i]];
            }
        } else {
            digit_data = digit_patterns[digits[i]];
        }

        et6226m_display_digit(handle, 3-i, digit_data);
    }

    return true;
}

bool et6226m_display_float(et6226m_handle_t *handle, float number, uint8_t decimal_places)
{
    if (!handle || !handle->initialized || decimal_places > 3) {
        return false;
    }

    // Convert to integer by multiplying by power of 10
    int32_t multiplier = 1;
    for (uint8_t i = 0; i < decimal_places; i++) {
        multiplier *= 10;
    }

    int32_t int_number = (int32_t)(number * multiplier);
    bool negative = int_number < 0;
    if (negative) int_number = -int_number;

    if (int_number > 9999) return false;

    uint8_t digits[4] = {0};
    uint32_t temp = int_number;

    // Extract digits
    digits[0] = temp % 10; temp /= 10;
    digits[1] = temp % 10; temp /= 10;
    digits[2] = temp % 10; temp /= 10;
    digits[3] = temp % 10;

    // Display digits with decimal point
    for (int i = 0; i < 4; i++) {
        uint8_t digit_data = digit_patterns[digits[i]];

        // Add decimal point if needed
        if (decimal_places > 0 && i == (decimal_places - 1)) {
            digit_data |= ET6226M_CHAR_DOT;
        }

        et6226m_display_digit(handle,  3 - i, digit_data);
    }

    return true;
}

bool et6226m_display_string(et6226m_handle_t *handle, const char *str)
{
    if (!handle || !handle->initialized || !str) {
        return false;
    }

    int len = strlen(str);
    if (len > 4) len = 4;

    // Clear display first
    et6226m_clear_display(handle);

    // Display characters
    for (int i = 0; i < len; i++) {
        uint8_t char_data = char_patterns[(uint8_t)str[i]];
        et6226m_display_digit(handle, i, char_data);
    }

    return true;
}

bool et6226m_display_printf(et6226m_handle_t *handle, const char *format, ...)
{
	if (!handle || !handle->initialized) {
		return false;
	}

    char buffer[5];  // Adjust size as needed
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    return et6226m_display_string(handle, buffer);
}

bool et6226m_clear_display(et6226m_handle_t *handle)
{
    if (!handle || !handle->initialized) {
        return false;
    }

    // Clear all digits
    for (int i = 1; i < 4; i++) {
        et6226m_display_digit(handle, i, ET6226M_CHAR_BLANK);
    }

    // Clear buffer
    memset(handle->display_buffer, 0, sizeof(handle->display_buffer));

    return true;
}

bool et6226m_test_display(et6226m_handle_t *handle, bool enable)
{
    if (!handle || !handle->initialized) {
        return false;
    }

//    et6226m_send_command(handle, 0x48, 0x01);
//    et6226m_display_number(handle, 123, false);

    return true;
}

bool et6226m_update_display(et6226m_handle_t *handle)
{
    if (!handle || !handle->initialized) {
        return false;
    }

    // Send buffer data to display
    for (int i = 0; i < 4; i++) {
        et6226m_send_command(handle, i + 1, handle->display_buffer[i]);
    }

    return true;
}
