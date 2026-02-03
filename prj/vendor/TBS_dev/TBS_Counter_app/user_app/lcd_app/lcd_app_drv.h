/**
 * @file lcd_app.h
 * @brief LCD Application layer header file for 16x2 LCD display
 * @author Nghia Hoang
 * @date 2025
 */

#ifndef LCD_APP_DRV_H
#define LCD_APP_DRV_H

#ifndef MASTER_CORE
#ifdef COUNTER_DEVICE

#include <vendor/TBS_dev/TBS_Counter_app/driver/lcd/lcd16x2.h>
#include <stdint.h>
#include <stdbool.h>


/**
 * @brief Maximum message length
 */
#define LCD_APP_MAX_MESSAGE_LENGTH      64
#define LCD_APP_MAX_DISPLAY_WIDTH       16
#define LCD_APP_MAX_ROWS               2
#define LCD_APP_SCROLL_START_DELAY_MS  1000
#define LCD_APP_SCROLL_DELAY_MS        500     // Default scroll delay in milliseconds

/**
 * @brief LCD App Error codes
 */
typedef enum {
    LCD_APP_OK = 0,                    // No error
    LCD_APP_ERROR_NULL_POINTER,        // Null pointer error
    LCD_APP_ERROR_INVALID_PARAM,       // Invalid parameter
    LCD_APP_ERROR_NOT_INITIALIZED,     // Not initialized
    LCD_APP_ERROR_MESSAGE_TOO_LONG,    // Message too long
    LCD_APP_ERROR_LCD_DRIVER           // LCD driver error
} lcd_app_error_t;

/**
 * @brief Display mode enumeration
 */
typedef enum {
    LCD_APP_MODE_STATIC = 0,           // Static display (no scrolling)
    LCD_APP_MODE_SCROLL                // Scrolling display
} lcd_app_display_mode_t;

/**
 * @brief Row configuration structure
 */
typedef struct {
    char message[LCD_APP_MAX_MESSAGE_LENGTH];   // Message to display
    uint8_t message_length;                     // Actual message length
    lcd_app_display_mode_t mode;                // Display mode for this row
    uint32_t scroll_position;                   // Current scroll position
    uint32_t scroll_start_delay_ms;             // delay befor start Scroll in milliseconds
    uint32_t scroll_delay_ms;                   // Scroll delay in milliseconds
    uint32_t last_scroll_time;                  // Last scroll update time
    bool needs_update;                          // Flag to indicate if row needs update
} lcd_app_row_config_t;

/**
 * @brief LCD App timeout callback function type
 */
typedef void (*lcd_app_timeout_callback_t)(uint8_t row);

/**
 * @brief LCD App get tick function type (returns current time in milliseconds)
 */
typedef uint32_t (*lcd_app_get_tick_t)(void);

/**
 * @brief LCD App configuration structure
 */
typedef struct {
    lcd_app_timeout_callback_t timeout_callback;   // Callback function when timeout occurs
    lcd_app_get_tick_t get_tick;                    // Function to get current tick/time in ms
    uint32_t default_timeout_ms;                    // Default timeout in milliseconds
    uint32_t default_scroll_delay_ms;               // Default scroll delay in milliseconds
    uint32_t default_scroll_start_delay_ms; 
} lcd_app_config_t;

/**
 * @brief LCD App handle structure
 */
typedef struct {
    lcd16x2_handle_t *lcd_handle;               // Pointer to LCD driver handle
    lcd_app_config_t config;                    // App configuration
    lcd_app_row_config_t rows[LCD_APP_MAX_ROWS]; // Row configurations
    uint32_t timeout_ms[LCD_APP_MAX_ROWS];      // Timeout for each row
    uint32_t start_time[LCD_APP_MAX_ROWS];      // Start time for timeout calculation
    bool timeout_enabled[LCD_APP_MAX_ROWS];     // Timeout enable flag for each row
    bool initialized;                           // Initialization flag
} lcd_app_handle_t;

/**
 * @brief Function prototypes
 */

/**
 * @brief Initialize LCD App
 * @param handle Pointer to LCD App handle
 * @param lcd_handle Pointer to LCD driver handle (must be initialized)
 * @param config Pointer to app configuration
 * @return LCD_APP_OK if successful, error code otherwise
 */
lcd_app_error_t lcd_app_init_drv(lcd_app_handle_t *handle,
                             lcd16x2_handle_t *lcd_handle,
                             const lcd_app_config_t *config);

/**
 * @brief Deinitialize LCD App
 * @param handle Pointer to LCD App handle
 * @return LCD_APP_OK if successful, error code otherwise
 */
lcd_app_error_t lcd_app_deinit_drv(lcd_app_handle_t *handle);

/**
 * @brief Set message for a specific row
 * @param handle Pointer to LCD App handle
 * @param row Row number (0 or 1)
 * @param message Message string to display
 * @param timeout_ms Timeout in milliseconds (0 = no timeout)
 * @return LCD_APP_OK if successful, error code otherwise
 */
lcd_app_error_t lcd_app_set_message(lcd_app_handle_t *handle,
                                   uint8_t row,
                                   char *message,
                                   uint32_t timeout_ms);

/**
 * @brief Set message for a specific row with custom scroll delay
 * @param handle Pointer to LCD App handle
 * @param row Row number (0 or 1)
 * @param message Message string to display
 * @param timeout_ms Timeout in milliseconds (0 = no timeout)
 * @param scroll_delay_ms Scroll delay in milliseconds
 * @return LCD_APP_OK if successful, error code otherwise
 */
lcd_app_error_t lcd_app_set_message_with_scroll_delay(lcd_app_handle_t *handle,
                                                     uint8_t row,
                                                     const char *message,
                                                     uint32_t timeout_ms,
                                                     uint32_t scroll_delay_ms);

/**
 * @brief Clear a specific row
 * @param handle Pointer to LCD App handle
 * @param row Row number (0 or 1)
 * @return LCD_APP_OK if successful, error code otherwise
 */
lcd_app_error_t lcd_app_clear_row(lcd_app_handle_t *handle, uint8_t row);

/**
 * @brief Clear all rows
 * @param handle Pointer to LCD App handle
 * @return LCD_APP_OK if successful, error code otherwise
 */
lcd_app_error_t lcd_app_clear_all(lcd_app_handle_t *handle);

/**
 * @brief Update display (call this periodically in main loop)
 * @param handle Pointer to LCD App handle
 * @return LCD_APP_OK if successful, error code otherwise
 */
lcd_app_error_t lcd_app_update(lcd_app_handle_t *handle);

/**
 * @brief Force immediate update of a specific row
 * @param handle Pointer to LCD App handle
 * @param row Row number (0 or 1)
 * @return LCD_APP_OK if successful, error code otherwise
 */
lcd_app_error_t lcd_app_force_update_row(lcd_app_handle_t *handle, uint8_t row);

/**
 * @brief Set scroll delay for a specific row
 * @param handle Pointer to LCD App handle
 * @param row Row number (0 or 1)
 * @param scroll_delay_ms Scroll delay in milliseconds
 * @return LCD_APP_OK if successful, error code otherwise
 */
lcd_app_error_t lcd_app_set_scroll_delay(lcd_app_handle_t *handle,
                                        uint8_t row,
                                        uint32_t scroll_delay_ms);

/**
 * @brief Reset timeout for a specific row
 * @param handle Pointer to LCD App handle
 * @param row Row number (0 or 1)
 * @return LCD_APP_OK if successful, error code otherwise
 */
lcd_app_error_t lcd_app_reset_timeout(lcd_app_handle_t *handle, uint8_t row);

/**
 * @brief Enable/disable timeout for a specific row
 * @param handle Pointer to LCD App handle
 * @param row Row number (0 or 1)
 * @param enable true to enable timeout, false to disable
 * @return LCD_APP_OK if successful, error code otherwise
 */
lcd_app_error_t lcd_app_enable_timeout(lcd_app_handle_t *handle,
                                      uint8_t row,
                                      bool enable);

/**
 * @brief Get current message for a specific row
 * @param handle Pointer to LCD App handle
 * @param row Row number (0 or 1)
 * @param message Buffer to store the message
 * @param buffer_size Size of the message buffer
 * @return LCD_APP_OK if successful, error code otherwise
 */
lcd_app_error_t lcd_app_get_message(lcd_app_handle_t *handle,
                                   uint8_t row,
                                   char *message,
                                   uint8_t buffer_size);

/**
 * @brief Check if a row has timed out
 * @param handle Pointer to LCD App handle
 * @param row Row number (0 or 1)
 * @param is_timeout Pointer to store timeout status
 * @return LCD_APP_OK if successful, error code otherwise
 */
lcd_app_error_t lcd_app_is_timeout(lcd_app_handle_t *handle,
                                  uint8_t row,
                                  bool *is_timeout);


#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/
#endif /* LCD_APP_DRV_H */
