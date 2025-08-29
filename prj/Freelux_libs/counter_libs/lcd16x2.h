/**
 * @file lcd16x2.h
 * @brief Driver header file for 16x2 LCD display
 * @author Nghia Hoang
 * @date 2025
 */

#ifndef LCD16X2_H
#define LCD16X2_H

#include <stdint.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LCD16x2 PIN
 */
typedef enum {
    LCD16X2_PIN_RS = 0,
    LCD16X2_PIN_RW,
    LCD16X2_PIN_E,
    LCD16X2_PIN_D0,
    LCD16X2_PIN_D1,
    LCD16X2_PIN_D2,
    LCD16X2_PIN_D3,
    LCD16X2_PIN_D4,
    LCD16X2_PIN_D5,
    LCD16X2_PIN_D6,
    LCD16X2_PIN_D7,

    LCD16X2_PIN_MAX   //
} LCD16X2_PinId_t;

/**
 * @brief LCD16x2 Configuration Structure
 */
typedef struct {
    // GPIO control functions
	void (*lcd_pin_init)(void);					// Config and init pin
//    void (*rs_pin_set)(bool state);             // Register Select pin control
//    void (*enable_pin_set)(bool state);         // Enable pin control
//    void (*rw_pin_set)(bool state);             // Read/Write pin control (optional, can be NULL for write-only)
    void (*lcd_pin_set)(LCD16X2_PinId_t pin, bool state); // Data pin control (D4-D7 for 4-bit mode)
    bool (*lcd_pin_read)(LCD16X2_PinId_t pin);         // Data pin read (for busy flag check, optional)

    // Delay functions
    void (*delay_us)(uint32_t us);              // Microsecond delay function
    void (*delay_ms)(uint32_t ms);              // Millisecond delay function

    // Configuration options
    bool use_4bit_mode;                         // true for 4-bit mode, false for 8-bit mode
    bool use_busy_flag;                         // true to check busy flag, false to use fixed delays
    uint8_t rows;                               // Number of rows (typically 2)
    uint8_t cols;                               // Number of columns (typically 16)
} lcd16x2_config_t;

/**
 * @brief LCD16x2 Handle Structure
 */
typedef struct {
    lcd16x2_config_t config;
    bool initialized;
    uint8_t current_row;                        // Current cursor row (0-based)
    uint8_t current_col;                        // Current cursor column (0-based)
    bool display_on;                            // Display on/off state
    bool cursor_on;                             // Cursor on/off state
    bool blink_on;                              // Cursor blink on/off state
} lcd16x2_handle_t;

/**
 * @brief LCD16x2 Commands
 */
#define LCD16X2_CMD_CLEAR_DISPLAY              0x01
#define LCD16X2_CMD_RETURN_HOME                0x02
#define LCD16X2_CMD_ENTRY_MODE_SET             0x04
#define LCD16X2_CMD_DISPLAY_CONTROL            0x08
#define LCD16X2_CMD_CURSOR_SHIFT               0x10
#define LCD16X2_CMD_FUNCTION_SET               0x20
#define LCD16X2_CMD_SET_CGRAM_ADDR             0x40
#define LCD16X2_CMD_SET_DDRAM_ADDR             0x80

/**
 * @brief Entry Mode Set flags
 */
#define LCD16X2_ENTRY_RIGHT                    0x00
#define LCD16X2_ENTRY_LEFT                     0x02
#define LCD16X2_ENTRY_SHIFT_INCREMENT          0x01
#define LCD16X2_ENTRY_SHIFT_DECREMENT          0x00

/**
 * @brief Display Control flags
 */
#define LCD16X2_DISPLAY_ON                     0x04
#define LCD16X2_DISPLAY_OFF                    0x00
#define LCD16X2_CURSOR_ON                      0x02
#define LCD16X2_CURSOR_OFF                     0x00
#define LCD16X2_BLINK_ON                       0x01
#define LCD16X2_BLINK_OFF                      0x00

/**
 * @brief Cursor/Display Shift flags
 */
#define LCD16X2_DISPLAY_MOVE                   0x08
#define LCD16X2_CURSOR_MOVE                    0x00
#define LCD16X2_MOVE_RIGHT                     0x04
#define LCD16X2_MOVE_LEFT                      0x00

/**
 * @brief Function Set flags
 */
#define LCD16X2_8BIT_MODE                      0x10
#define LCD16X2_4BIT_MODE                      0x00
#define LCD16X2_2LINE                          0x08
#define LCD16X2_1LINE                          0x00
#define LCD16X2_5x10_DOTS                      0x04
#define LCD16X2_5x8_DOTS                       0x00

/**
 * @brief Data pins for 4-bit mode
 */


/**
 * @brief Busy flag mask
 */
#define LCD16X2_BUSY_FLAG                      0x80

/**
 * @brief DDRAM addresses for each row
 */
#define LCD16X2_ROW_0_ADDR                     0x00
#define LCD16X2_ROW_1_ADDR                     0x40

/**
 * @brief Error codes
 */
typedef enum {
    LCD16X2_OK = 0,                            // No error
    LCD16X2_ERROR_NULL_POINTER = 1,            // Null pointer error
    LCD16X2_ERROR_INVALID_PARAM = 2,           // Invalid parameter
    LCD16X2_ERROR_NOT_INITIALIZED = 3,         // Not initialized
    LCD16X2_ERROR_TIMEOUT = 4                  // Timeout error
} lcd16x2_error_t;

/**
 * @brief Custom character structure (5x8 dots)
 */
typedef struct {
    uint8_t pattern[8];                        // 8 bytes for 5x8 character pattern
} lcd16x2_custom_char_t;

/**
 * @brief Function prototypes
 */

/**
 * @brief Initialize LCD16x2 driver
 * @param handle Pointer to LCD16x2 handle
 * @param config Pointer to configuration structure
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_init(lcd16x2_handle_t *handle, const lcd16x2_config_t *config);

/**
 * @brief Deinitialize LCD16x2 driver
 * @param handle Pointer to LCD16x2 handle
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_deinit(lcd16x2_handle_t *handle);

/**
 * @brief Send command to LCD
 * @param handle Pointer to LCD16x2 handle
 * @param cmd Command byte
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_send_command(lcd16x2_handle_t *handle, uint8_t cmd);

/**
 * @brief Send data to LCD
 * @param handle Pointer to LCD16x2 handle
 * @param data Data byte
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_send_data(lcd16x2_handle_t *handle, uint8_t data);

/**
 * @brief Clear display
 * @param handle Pointer to LCD16x2 handle
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_clear(lcd16x2_handle_t *handle);

/**
 * @brief Return cursor to home position
 * @param handle Pointer to LCD16x2 handle
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_home(lcd16x2_handle_t *handle);

/**
 * @brief Set cursor position
 * @param handle Pointer to LCD16x2 handle
 * @param row Row number (0-based)
 * @param col Column number (0-based)
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_set_cursor(lcd16x2_handle_t *handle, uint8_t row, uint8_t col);

/**
 * @brief Print character at current cursor position
 * @param handle Pointer to LCD16x2 handle
 * @param ch Character to print
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_print_char(lcd16x2_handle_t *handle, char ch);

/**
 * @brief Print string at current cursor position
 * @param handle Pointer to LCD16x2 handle
 * @param str String to print
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_print_string(lcd16x2_handle_t *handle, const char *str);

/**
 * @brief Print string at specified position
 * @param handle Pointer to LCD16x2 handle
 * @param row Row number (0-based)
 * @param col Column number (0-based)
 * @param str String to print
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_print_at(lcd16x2_handle_t *handle, uint8_t row, uint8_t col, const char *str);

/**
 * @brief Print formatted string (simple printf-like)
 * @param handle Pointer to LCD16x2 handle
 * @param format Format string
 * @param ... Arguments
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_printf(lcd16x2_handle_t *handle, const char *format, ...);

/**
 * @brief Control display on/off
 * @param handle Pointer to LCD16x2 handle
 * @param display_on true to turn on display, false to turn off
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_display_control(lcd16x2_handle_t *handle, bool display_on);

/**
 * @brief Control cursor visibility
 * @param handle Pointer to LCD16x2 handle
 * @param cursor_on true to show cursor, false to hide cursor
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_cursor_control(lcd16x2_handle_t *handle, bool cursor_on);

/**
 * @brief Control cursor blinking
 * @param handle Pointer to LCD16x2 handle
 * @param blink_on true to enable blinking, false to disable blinking
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_blink_control(lcd16x2_handle_t *handle, bool blink_on);

/**
 * @brief Shift display left
 * @param handle Pointer to LCD16x2 handle
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_shift_display_left(lcd16x2_handle_t *handle);

/**
 * @brief Shift display right
 * @param handle Pointer to LCD16x2 handle
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_shift_display_right(lcd16x2_handle_t *handle);

/**
 * @brief Shift cursor left
 * @param handle Pointer to LCD16x2 handle
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_shift_cursor_left(lcd16x2_handle_t *handle);

/**
 * @brief Shift cursor right
 * @param handle Pointer to LCD16x2 handle
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_shift_cursor_right(lcd16x2_handle_t *handle);

/**
 * @brief Create custom character
 * @param handle Pointer to LCD16x2 handle
 * @param location Character location (0-7)
 * @param custom_char Pointer to custom character pattern
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_create_custom_char(lcd16x2_handle_t *handle, uint8_t location, const lcd16x2_custom_char_t *custom_char);

/**
 * @brief Print custom character
 * @param handle Pointer to LCD16x2 handle
 * @param location Character location (0-7)
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_print_custom_char(lcd16x2_handle_t *handle, uint8_t location);

/**
 * @brief Check if LCD is busy (if busy flag checking is enabled)
 * @param handle Pointer to LCD16x2 handle
 * @param is_busy Pointer to store busy status
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_is_busy(lcd16x2_handle_t *handle, bool *is_busy);

/**
 * @brief Set entry mode (text direction and auto-shift)
 * @param handle Pointer to LCD16x2 handle
 * @param left_to_right true for left-to-right, false for right-to-left
 * @param auto_shift true to enable auto-shift, false to disable
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_set_entry_mode(lcd16x2_handle_t *handle, bool left_to_right, bool auto_shift);

/**
 * @brief Get current cursor position
 * @param handle Pointer to LCD16x2 handle
 * @param row Pointer to store current row
 * @param col Pointer to store current column
 * @return LCD16X2_OK if successful, error code otherwise
 */
lcd16x2_error_t lcd16x2_get_cursor_position(lcd16x2_handle_t *handle, uint8_t *row, uint8_t *col);

#ifdef __cplusplus
}
#endif

#endif /* LCD16X2_H */
