/**
 * @file et6226m.h
 * @brief Driver header file for ET6226M 4-digit 7-segment LED display driver
 * @author Nghia Hoang
 * @date 2025
 */

#ifndef ET6226M_H
#define ET6226M_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif



/**
 * @brief ET6226M Configuration Structure
 */
typedef union {
	uint8_t ET6226_CONTROL;
	struct {
		uint8_t Display_switch 		:1;
		uint8_t 					:1;
		uint8_t Sleep_command		:1;
		uint8_t Mode_select			:1;
		uint8_t Brightness_adjust	:3;
		uint8_t						:1;
	} ET6226_CONTROL_b;
} ET6226_CONTROL;
typedef struct {
	void (*pin_init)(void);					// inin pin
    void (*delay_cycle)(uint32_t num_cycle);          // Delay function pointer
    void (*cs_pin_set)(bool state);         // Chip Select pin control
    void (*clk_pin_set)(bool state);        // Clock pin control
    void (*data_pin_set)(bool state);       // Data pin control
    uint8_t brightness;                      // Brightness level (0-7)
    bool scan_mode;                          // Scan mode enable
    bool decode_mode;                        // BCD decode mode
} et6226m_config_t;

/**
 * @brief ET6226M Handle Structure
 */
typedef struct {
    et6226m_config_t config;
    ET6226_CONTROL control;
    bool initialized;
    uint8_t display_buffer[4];               // Display data buffer
} et6226m_handle_t;

/**
 * @brief ET6226M Register Addresses
 */
#define ET6226M_REG_DECODE_MODE    0x09
#define ET6226M_REG_INTENSITY      0x0A
#define ET6226M_REG_SCAN_LIMIT     0x0B
#define ET6226M_REG_SHUTDOWN       0x0C
#define ET6226M_REG_DISPLAY_TEST   0x0F
#define ET6226M_REG_DP   		   0x80

/**
 * @brief Digit positions
 */
#define ET6226M_DIGIT_0            0x68
#define ET6226M_DIGIT_1            0x6A
#define ET6226M_DIGIT_2            0x6C
#define ET6226M_DIGIT_3            0x6E

/**
 * @brief 7-segment patterns for digits 0-9
 */
#define ET6226M_DIGIT_PATTERNS     { \
    0x3F, /* 0 */ \
    0x06, /* 1 */ \
    0x5B, /* 2 */ \
    0x4F, /* 3 */ \
    0x66, /* 4 */ \
    0x6D, /* 5 */ \
    0x7D, /* 6 */ \
    0x07, /* 7 */ \
    0x7F, /* 8 */ \
    0x6F  /* 9 */ \
}

/**
 * @brief Special characters
 */
#define ET6226M_CHAR_BLANK         0x00
#define ET6226M_CHAR_MINUS         0x40
#define ET6226M_CHAR_DOT           0x80
#define ET6226M_CHAR_H             0x76
#define ET6226M_CHAR_E             0x79
#define ET6226M_CHAR_L             0x38
#define ET6226M_CHAR_P             0x73
#define ET6226M_CHAR_8_DOT         0xFF

/**
 * @brief Brightness levels
 */
#define ET6226M_BRIGHTNESS_MAX     0x00
#define ET6226M_BRIGHTNESS_MIN     0x01
#define ET6226M_BRIGHTNESS_2       0x02
#define ET6226M_BRIGHTNESS_3       0x03
#define ET6226M_BRIGHTNESS_4       0x04
#define ET6226M_BRIGHTNESS_5       0x05
#define ET6226M_BRIGHTNESS_6       0x06
#define ET6226M_BRIGHTNESS_7       0x07

/**
 * @brief command
 */
#define ET6226M_SEND_COMMAND			0X48
#define ET6226M_DISPLAY_SWITCH_ON		0X01
#define ET6226M_DISPLAY_SWITCH_OFF		0X00
#define ET6226M_SLEEP_COMMAND_SLEEP		0X01
#define ET6226M_SLEEP_COMMAND_OPERATION	0X00
#define ET6226M_MODE_SELECT_7SEGMENT	0X01
#define ET6226M_MODE_SELECT_8SEGMENT	0X00
/**
 * @brief Function prototypes
 */

/**
 * @brief Initialize ET6226M driver
 * @param handle Pointer to ET6226M handle
 * @param config Pointer to configuration structure
 * @return true if successful, false otherwise
 */
bool et6226m_init(et6226m_handle_t *handle, const et6226m_config_t *config);

/**
 * @brief Deinitialize ET6226M driver
 * @param handle Pointer to ET6226M handle
 */
void et6226m_deinit(et6226m_handle_t *handle);

/**
 * @brief Write data to register
 * @param handle Pointer to ET6226M handle
 * @param reg Register address
 * @param data Data to write
 * @return true if successful, false otherwise
 */
bool et6226m_write_register(et6226m_handle_t *handle, uint8_t reg, uint8_t data);

/**
 * @brief Set brightness level
 * @param handle Pointer to ET6226M handle
 * @param brightness Brightness level (0-15)
 * @return true if successful, false otherwise
 */
bool et6226m_set_brightness(et6226m_handle_t *handle, uint8_t brightness);

/**
 * @brief Enable/disable display
 * @param handle Pointer to ET6226M handle
 * @param enable true to enable, false to disable
 * @return true if successful, false otherwise
 */
bool et6226m_enable_display(et6226m_handle_t *handle, bool enable);

/**
 * @brief Display digit at specific position
 * @param handle Pointer to ET6226M handle
 * @param digit_pos Digit position (1-4)
 * @param segment_data 7-segment data
 * @return true if successful, false otherwise
 */
bool et6226m_display_digit(et6226m_handle_t *handle, uint8_t digit_pos, uint8_t segment_data);

/**
 * @brief Display number (0-9999)
 * @param handle Pointer to ET6226M handle
 * @param number Number to display
 * @param leading_zeros Show leading zeros
 * @return true if successful, false otherwise
 */
bool et6226m_display_number(et6226m_handle_t *handle, uint16_t number, bool leading_zeros);

/**
 * @brief Display floating point number
 * @param handle Pointer to ET6226M handle
 * @param number Number to display
 * @param decimal_places Number of decimal places
 * @return true if successful, false otherwise
 */
bool et6226m_display_float(et6226m_handle_t *handle, float number, uint8_t decimal_places);

/**
 * @brief Display string (up to 4 characters)
 * @param handle Pointer to ET6226M handle
 * @param str String to display
 * @return true if successful, false otherwise
 */
bool et6226m_display_string(et6226m_handle_t *handle, const char *str);

/**
 * @brief Display string (up to 4 characters)
 * @param handle Pointer to ET6226M handle
 * @param format Format string
 * @param ... Arguments
 * @return true if successful, false otherwise
 */
bool et6226m_display_printf(et6226m_handle_t *handle, const char *format, ...);

/**
 * @brief Clear display
 * @param handle Pointer to ET6226M handle
 * @return true if successful, false otherwise
 */
bool et6226m_clear_display(et6226m_handle_t *handle);

/**
 * @brief Test display (all segments on)
 * @param handle Pointer to ET6226M handle
 * @param enable true to enable test, false to disable
 * @return true if successful, false otherwise
 */
bool et6226m_test_display(et6226m_handle_t *handle, bool enable);

/**
 * @brief Update display buffer
 * @param handle Pointer to ET6226M handle
 * @return true if successful, false otherwise
 */
bool et6226m_update_display(et6226m_handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif /* ET6226M_H */
