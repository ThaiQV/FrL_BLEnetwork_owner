/**
 * @file tca9555.h
 * @brief Driver header file for TCA9555 16-bit I2C I/O expander
 * @author Nghia Hoang
 * @date 2025
 */

#ifndef TCA9555_H
#define TCA9555_H

#include <stdint.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TCA9555 I2C Configuration Structure
 */
typedef struct {
    uint8_t (*i2c_write)(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint16_t length);
    uint8_t (*i2c_read)(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint16_t length);
    void (*tca9555_delay_ms)(uint32_t ms);               // Delay function pointer
    uint8_t device_address;                      // I2C device address (0x20-0x27)
} tca9555_config_t;

/**
 * @brief TCA9555 Handle Structure
 */
typedef struct {
    tca9555_config_t config;
    bool initialized;
    uint8_t output_port0_cache;                  // Cache for Output Port 0
    uint8_t output_port1_cache;                  // Cache for Output Port 1
    uint8_t config_port0_cache;                  // Cache for Config Port 0
    uint8_t config_port1_cache;                  // Cache for Config Port 1
} tca9555_handle_t;

/**
 * @brief TCA9555 Register Addresses
 */
#define TCA9555_REG_INPUT_PORT_0           0x00  // Input Port 0 register
#define TCA9555_REG_INPUT_PORT_1           0x01  // Input Port 1 register
#define TCA9555_REG_OUTPUT_PORT_0          0x02  // Output Port 0 register
#define TCA9555_REG_OUTPUT_PORT_1          0x03  // Output Port 1 register
#define TCA9555_REG_POLARITY_INV_PORT_0    0x04  // Polarity Inversion Port 0 register
#define TCA9555_REG_POLARITY_INV_PORT_1    0x05  // Polarity Inversion Port 1 register
#define TCA9555_REG_CONFIG_PORT_0          0x06  // Configuration Port 0 register
#define TCA9555_REG_CONFIG_PORT_1          0x07  // Configuration Port 1 register


/**
 * @brief TCA9555 Base I2C Address
 */
#define TCA9555_BASE_ADDR                  0x20  // Base address (0100 000x)

/**
 * @brief TCA9555 Address Selection Values
 */
typedef enum {
    TCA9555_ADDR_A2_L_A1_L_A0_L = 0x00,        // A2=0, A1=0, A0=0 -> 0x20
    TCA9555_ADDR_A2_L_A1_L_A0_H = 0x01,        // A2=0, A1=0, A0=1 -> 0x21
    TCA9555_ADDR_A2_L_A1_H_A0_L = 0x02,        // A2=0, A1=1, A0=0 -> 0x22
    TCA9555_ADDR_A2_L_A1_H_A0_H = 0x03,        // A2=0, A1=1, A0=1 -> 0x23
    TCA9555_ADDR_A2_H_A1_L_A0_L = 0x04,        // A2=1, A1=0, A0=0 -> 0x24
    TCA9555_ADDR_A2_H_A1_L_A0_H = 0x05,        // A2=1, A1=0, A0=1 -> 0x25
    TCA9555_ADDR_A2_H_A1_H_A0_L = 0x06,        // A2=1, A1=1, A0=0 -> 0x26
    TCA9555_ADDR_A2_H_A1_H_A0_H = 0x07         // A2=1, A1=1, A0=1 -> 0x27
} tca9555_address_t;

/**
 * @brief TCA9555 Port Selection
 */
typedef enum {
    TCA9555_PORT_0 = 0,
    TCA9555_PORT_1 = 1
} tca9555_port_t;

/**
 * @brief TCA9555 Pin Numbers
 */
typedef enum {
    TCA9555_PIN_0 = 0,
    TCA9555_PIN_1 = 1,
    TCA9555_PIN_2 = 2,
    TCA9555_PIN_3 = 3,
    TCA9555_PIN_4 = 4,
    TCA9555_PIN_5 = 5,
    TCA9555_PIN_6 = 6,
    TCA9555_PIN_7 = 7
} tca9555_pin_t;

/**
 * @brief
 */
typedef struct _PortPin_Map {
	tca9555_port_t tca_port;
	tca9555_pin_t tca_pin;
} PortPin_Map;

/**
 * @brief TCA9555 Pin Direction
 */
typedef enum {
    TCA9555_PIN_OUTPUT = 0,                     // Pin configured as output
    TCA9555_PIN_INPUT = 1                       // Pin configured as input
} tca9555_pin_dir_t;

/**
 * @brief TCA9555 Pin State
 */
typedef enum {
    TCA9555_PIN_LOW = 0,                        // Pin state low
    TCA9555_PIN_HIGH = 1                        // Pin state high
} tca9555_pin_state_t;

/**
 * @brief TCA9555 Error Codes
 */
typedef enum {
    TCA9555_OK = 0,                             // No error
    TCA9555_ERROR_NULL_POINTER = 1,             // Null pointer error
    TCA9555_ERROR_I2C_WRITE = 2,                // I2C write error
    TCA9555_ERROR_I2C_READ = 3,                 // I2C read error
    TCA9555_ERROR_INVALID_PARAM = 4,            // Invalid parameter
    TCA9555_ERROR_NOT_INITIALIZED = 5           // Device not initialized
} tca9555_error_t;

/**
 * @brief Function prototypes
 */

/**
 * @brief Initialize TCA9555 driver
 * @param handle Pointer to TCA9555 handle
 * @param config Pointer to configuration structure
 * @param addr_select Address selection (A2, A1, A0 pins)
 * @return TCA9555_OK if successful, error code otherwise
 */
tca9555_error_t tca9555_init(tca9555_handle_t *handle, const tca9555_config_t *config, tca9555_address_t addr_select);

/**
 * @brief Deinitialize TCA9555 driver
 * @param handle Pointer to TCA9555 handle
 * @return TCA9555_OK if successful, error code otherwise
 */
tca9555_error_t tca9555_deinit(tca9555_handle_t *handle);

/**
 * @brief Write data to register
 * @param handle Pointer to TCA9555 handle
 * @param reg Register address
 * @param data Data to write
 * @return TCA9555_OK if successful, error code otherwise
 */
tca9555_error_t tca9555_write_register(tca9555_handle_t *handle, uint8_t reg, uint8_t data);

/**
 * @brief Read data from register
 * @param handle Pointer to TCA9555 handle
 * @param reg Register address
 * @param data Pointer to store read data
 * @return TCA9555_OK if successful, error code otherwise
 */
tca9555_error_t tca9555_read_register(tca9555_handle_t *handle, uint8_t reg, uint8_t *data);

/**
 * @brief Configure pin direction
 * @param handle Pointer to TCA9555 handle
 * @param port Port number (0 or 1)
 * @param pin Pin number (0-7)
 * @param direction Pin direction (input/output)
 * @return TCA9555_OK if successful, error code otherwise
 */
tca9555_error_t tca9555_set_pin_direction(tca9555_handle_t *handle, tca9555_port_t port, tca9555_pin_t pin, tca9555_pin_dir_t direction);

/**
 * @brief Configure port direction
 * @param handle Pointer to TCA9555 handle
 * @param port Port number (0 or 1)
 * @param direction_mask Direction mask (bit per pin, 1=input, 0=output)
 * @return TCA9555_OK if successful, error code otherwise
 */
tca9555_error_t tca9555_set_port_direction(tca9555_handle_t *handle, tca9555_port_t port, uint8_t direction_mask);

/**
 * @brief Set pin output state
 * @param handle Pointer to TCA9555 handle
 * @param port Port number (0 or 1)
 * @param pin Pin number (0-7)
 * @param state Pin state (high/low)
 * @return TCA9555_OK if successful, error code otherwise
 */
tca9555_error_t tca9555_set_pin_output(tca9555_handle_t *handle, tca9555_port_t port, tca9555_pin_t pin, tca9555_pin_state_t state);

/**
 * @brief Set port output state
 * @param handle Pointer to TCA9555 handle
 * @param port Port number (0 or 1)
 * @param output_mask Output mask (bit per pin)
 * @return TCA9555_OK if successful, error code otherwise
 */
tca9555_error_t tca9555_set_port_output(tca9555_handle_t *handle, tca9555_port_t port, uint8_t output_mask);

/**
 * @brief Read pin input state
 * @param handle Pointer to TCA9555 handle
 * @param port Port number (0 or 1)
 * @param pin Pin number (0-7)
 * @param state Pointer to store pin state
 * @return TCA9555_OK if successful, error code otherwise
 */
tca9555_error_t tca9555_read_pin_input(tca9555_handle_t *handle, tca9555_port_t port, tca9555_pin_t pin, tca9555_pin_state_t *state);

/**
 * @brief Read port input state
 * @param handle Pointer to TCA9555 handle
 * @param port Port number (0 or 1)
 * @param input_data Pointer to store input data
 * @return TCA9555_OK if successful, error code otherwise
 */
tca9555_error_t tca9555_read_port_input(tca9555_handle_t *handle, tca9555_port_t port, uint8_t *input_data);

/**
 * @brief Set pin polarity inversion
 * @param handle Pointer to TCA9555 handle
 * @param port Port number (0 or 1)
 * @param pin Pin number (0-7)
 * @param invert true to invert polarity, false for normal
 * @return TCA9555_OK if successful, error code otherwise
 */
tca9555_error_t tca9555_set_pin_polarity(tca9555_handle_t *handle, tca9555_port_t port, tca9555_pin_t pin, bool invert);

/**
 * @brief Set port polarity inversion
 * @param handle Pointer to TCA9555 handle
 * @param port Port number (0 or 1)
 * @param polarity_mask Polarity mask (bit per pin, 1=invert, 0=normal)
 * @return TCA9555_OK if successful, error code otherwise
 */
tca9555_error_t tca9555_set_port_polarity(tca9555_handle_t *handle, tca9555_port_t port, uint8_t polarity_mask);

/**
 * @brief Toggle pin output
 * @param handle Pointer to TCA9555 handle
 * @param port Port number (0 or 1)
 * @param pin Pin number (0-7)
 * @return TCA9555_OK if successful, error code otherwise
 */
tca9555_error_t tca9555_toggle_pin(tca9555_handle_t *handle, tca9555_port_t port, tca9555_pin_t pin);

/**
 * @brief Reset TCA9555 to default configuration
 * @param handle Pointer to TCA9555 handle
 * @return TCA9555_OK if successful, error code otherwise
 */
tca9555_error_t tca9555_reset(tca9555_handle_t *handle);

/**
 * @brief Read all input ports (16-bit)
 * @param handle Pointer to TCA9555 handle
 * @param input_data Pointer to store 16-bit input data
 * @return TCA9555_OK if successful, error code otherwise
 */
tca9555_error_t tca9555_read_all_inputs(tca9555_handle_t *handle, uint16_t *input_data);

/**
 * @brief Write all output ports (16-bit)
 * @param handle Pointer to TCA9555 handle
 * @param output_data 16-bit output data
 * @return TCA9555_OK if successful, error code otherwise
 */
tca9555_error_t tca9555_write_all_outputs(tca9555_handle_t *handle, uint16_t output_data);

/**
 * @brief Get current configuration of a port
 * @param handle Pointer to TCA9555 handle
 * @param port Port number (0 or 1)
 * @param config_data Pointer to store configuration data
 * @return TCA9555_OK if successful, error code otherwise
 */
tca9555_error_t tca9555_get_port_config(tca9555_handle_t *handle, tca9555_port_t port, uint8_t *config_data);

/**
 * @brief Calculate I2C address based on address pins
 * @param addr_select Address selection value
 * @return Calculated I2C address
 */
uint8_t tca9555_calculate_address(tca9555_address_t addr_select);

#ifdef __cplusplus
}
#endif

#endif /* TCA9555_H */
