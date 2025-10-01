/**
 * @file tca9555.c
 * @brief Driver source file for TCA9555 16-bit I2C I/O expander
 * @author Nghia Hoang
 * @date 2025
 */

#ifndef MASTER_CORE

#include "stdio.h"
#include "tca9555.h"

/**
 * @brief Default register values after power-on reset
 */
#define TCA9555_DEFAULT_INPUT_PORT         0x00  // Don't care (depends on external signals)
#define TCA9555_DEFAULT_OUTPUT_PORT        0xFF  // All outputs high
#define TCA9555_DEFAULT_POLARITY_INV       0x00  // No polarity inversion
#define TCA9555_DEFAULT_CONFIG_PORT        0xFF  // All pins as inputs

/**
 * @brief Validate handle and initialization status
 */
static tca9555_error_t tca9555_validate_handle(tca9555_handle_t *handle)
{
    if (handle == NULL) {
        return TCA9555_ERROR_NULL_POINTER;
    }

    if (!handle->initialized) {
        return TCA9555_ERROR_NOT_INITIALIZED;
    }

    return TCA9555_OK;
}

/**
 * @brief Validate port number
 */
static tca9555_error_t tca9555_validate_port(tca9555_port_t port)
{
    if (port > TCA9555_PORT_1) {
        return TCA9555_ERROR_INVALID_PARAM;
    }
    return TCA9555_OK;
}

/**
 * @brief Validate pin number
 */
static tca9555_error_t tca9555_validate_pin(tca9555_pin_t pin)
{
    if (pin > TCA9555_PIN_7) {
        return TCA9555_ERROR_INVALID_PARAM;
    }
    return TCA9555_OK;
}

uint8_t tca9555_calculate_address(tca9555_address_t addr_select)
{
    return ((TCA9555_BASE_ADDR | (addr_select & 0x07))<<1) + 0x01;
}

tca9555_error_t tca9555_init(tca9555_handle_t *handle, const tca9555_config_t *config, tca9555_address_t addr_select)
{
    if (handle == NULL || config == NULL) {
        return TCA9555_ERROR_NULL_POINTER;
    }

    // Check required function pointers
    if (config->i2c_write == NULL || config->i2c_read == NULL) {
        return TCA9555_ERROR_NULL_POINTER;
    }

    // Copy configuration
    handle->config = *config;
    handle->config.device_address = tca9555_calculate_address(addr_select);

    // Initialize cache with default values
    handle->output_port0_cache = TCA9555_DEFAULT_OUTPUT_PORT;
    handle->output_port1_cache = TCA9555_DEFAULT_OUTPUT_PORT;
    handle->config_port0_cache = TCA9555_DEFAULT_CONFIG_PORT;
    handle->config_port1_cache = TCA9555_DEFAULT_CONFIG_PORT;

    // Small delay for device stabilization
    if (handle->config.tca9555_delay_ms != NULL) {
        handle->config.tca9555_delay_ms(10);
    }

    // Test communication by reading input port registers
    uint8_t test_data;
    tca9555_error_t result;
    handle->initialized = true;

    result = tca9555_read_register(handle, TCA9555_REG_INPUT_PORT_0, &test_data);
    if (result != TCA9555_OK) {
        return result;
    }

    result = tca9555_read_register(handle, TCA9555_REG_INPUT_PORT_1, &test_data);
    if (result != TCA9555_OK) {
        return result;
    }

    result = tca9555_read_register(handle, TCA9555_REG_OUTPUT_PORT_0, &test_data);
    if (result != TCA9555_OK) {
        return result;
    }

    result = tca9555_read_register(handle, TCA9555_REG_OUTPUT_PORT_1, &test_data);
    if (result != TCA9555_OK) {
        return result;
    }

    return TCA9555_OK;
}

tca9555_error_t tca9555_deinit(tca9555_handle_t *handle)
{
    tca9555_error_t result = tca9555_validate_handle(handle);
    if (result != TCA9555_OK) {
        return result;
    }

    handle->initialized = false;

    return TCA9555_OK;
}

tca9555_error_t tca9555_write_register(tca9555_handle_t *handle, uint8_t reg, uint8_t data)
{
    if (handle == NULL) {
        return TCA9555_ERROR_NULL_POINTER;
    }

    if (!handle->initialized) {
        return TCA9555_ERROR_NOT_INITIALIZED;
    }

    uint8_t result = handle->config.i2c_write(handle->config.device_address, reg, &data, 1);

    if (result != 0) {
        return TCA9555_ERROR_I2C_WRITE;
    }

    // Update cache for output and configuration registers
    switch (reg) {
        case TCA9555_REG_OUTPUT_PORT_0:
            handle->output_port0_cache = data;
            break;
        case TCA9555_REG_OUTPUT_PORT_1:
            handle->output_port1_cache = data;
            break;
        case TCA9555_REG_CONFIG_PORT_0:
            handle->config_port0_cache = data;
            break;
        case TCA9555_REG_CONFIG_PORT_1:
            handle->config_port1_cache = data;
            break;
        default:
            break;
    }

    return TCA9555_OK;
}

tca9555_error_t tca9555_read_register(tca9555_handle_t *handle, uint8_t reg, uint8_t *data)
{
    if (handle == NULL || data == NULL) {
        return TCA9555_ERROR_NULL_POINTER;
    }

    if (!handle->initialized) {
        return TCA9555_ERROR_NOT_INITIALIZED;
    }

    uint8_t result = handle->config.i2c_read(handle->config.device_address, reg, data, 1);

    if (result != 0) {
        return TCA9555_ERROR_I2C_READ;
    }

    return TCA9555_OK;
}

tca9555_error_t tca9555_set_pin_direction(tca9555_handle_t *handle, tca9555_port_t port, tca9555_pin_t pin, tca9555_pin_dir_t direction)
{
    tca9555_error_t result;

    result = tca9555_validate_handle(handle);
    if (result != TCA9555_OK) return result;

    result = tca9555_validate_port(port);
    if (result != TCA9555_OK) return result;

    result = tca9555_validate_pin(pin);
    if (result != TCA9555_OK) return result;

    uint8_t reg = (port == TCA9555_PORT_0) ? TCA9555_REG_CONFIG_PORT_0 : TCA9555_REG_CONFIG_PORT_1;
    uint8_t *cache = (port == TCA9555_PORT_0) ? &handle->config_port0_cache : &handle->config_port1_cache;

    // Modify the cached value
    if (direction == TCA9555_PIN_INPUT) {
        *cache |= (1 << pin);   // Set bit for input
    } else {
        *cache &= ~(1 << pin);  // Clear bit for output
    }

    // Write to register
    return tca9555_write_register(handle, reg, *cache);
}

tca9555_error_t tca9555_set_port_direction(tca9555_handle_t *handle, tca9555_port_t port, uint8_t direction_mask)
{
    tca9555_error_t result;

    result = tca9555_validate_handle(handle);
    if (result != TCA9555_OK) return result;

    result = tca9555_validate_port(port);
    if (result != TCA9555_OK) return result;

    uint8_t reg = (port == TCA9555_PORT_0) ? TCA9555_REG_CONFIG_PORT_0 : TCA9555_REG_CONFIG_PORT_1;
    uint8_t *cache = (port == TCA9555_PORT_0) ? &handle->config_port0_cache : &handle->config_port1_cache;

    *cache = direction_mask;

    return tca9555_write_register(handle, reg, direction_mask);
}

tca9555_error_t tca9555_set_pin_output(tca9555_handle_t *handle, tca9555_port_t port, tca9555_pin_t pin, tca9555_pin_state_t state)
{
    tca9555_error_t result;

    result = tca9555_validate_handle(handle);
    if (result != TCA9555_OK) return result;

    result = tca9555_validate_port(port);
    if (result != TCA9555_OK) return result;

    result = tca9555_validate_pin(pin);
    if (result != TCA9555_OK) return result;

    uint8_t reg = (port == TCA9555_PORT_0) ? TCA9555_REG_OUTPUT_PORT_0 : TCA9555_REG_OUTPUT_PORT_1;
    uint8_t *cache = (port == TCA9555_PORT_0) ? &handle->output_port0_cache : &handle->output_port1_cache;

    // Modify the cached value
    if (state == TCA9555_PIN_HIGH) {
        *cache |= (1 << pin);   // Set bit for high
    } else {
        *cache &= ~(1 << pin);  // Clear bit for low
    }

    // Write to register
    return tca9555_write_register(handle, reg, *cache);
}

tca9555_error_t tca9555_set_port_output(tca9555_handle_t *handle, tca9555_port_t port, uint8_t output_mask)
{
    tca9555_error_t result;

    result = tca9555_validate_handle(handle);
    if (result != TCA9555_OK) return result;

    result = tca9555_validate_port(port);
    if (result != TCA9555_OK) return result;

    uint8_t reg = (port == TCA9555_PORT_0) ? TCA9555_REG_OUTPUT_PORT_0 : TCA9555_REG_OUTPUT_PORT_1;
    uint8_t *cache = (port == TCA9555_PORT_0) ? &handle->output_port0_cache : &handle->output_port1_cache;

    *cache = output_mask;

    return tca9555_write_register(handle, reg, output_mask);
}

tca9555_error_t tca9555_read_pin_input(tca9555_handle_t *handle, tca9555_port_t port, tca9555_pin_t pin, tca9555_pin_state_t *state)
{
    tca9555_error_t result;

    result = tca9555_validate_handle(handle);
    if (result != TCA9555_OK) return result;

    result = tca9555_validate_port(port);
    if (result != TCA9555_OK) return result;

    result = tca9555_validate_pin(pin);
    if (result != TCA9555_OK) return result;

    if (state == NULL) {
        return TCA9555_ERROR_NULL_POINTER;
    }

    uint8_t reg = (port == TCA9555_PORT_0) ? TCA9555_REG_INPUT_PORT_0 : TCA9555_REG_INPUT_PORT_1;
    uint8_t port_data;

    result = tca9555_read_register(handle, reg, &port_data);
    if (result != TCA9555_OK) {
        return result;
    }

    *state = (port_data & (1 << pin)) ? TCA9555_PIN_HIGH : TCA9555_PIN_LOW;

    return TCA9555_OK;
}

tca9555_error_t tca9555_read_port_input(tca9555_handle_t *handle, tca9555_port_t port, uint8_t *input_data)
{
    tca9555_error_t result;

    result = tca9555_validate_handle(handle);
    if (result != TCA9555_OK) return result;

    result = tca9555_validate_port(port);
    if (result != TCA9555_OK) return result;

    if (input_data == NULL) {
        return TCA9555_ERROR_NULL_POINTER;
    }

    uint8_t reg = (port == TCA9555_PORT_0) ? TCA9555_REG_INPUT_PORT_0 : TCA9555_REG_INPUT_PORT_1;

    return tca9555_read_register(handle, reg, input_data);
}

tca9555_error_t tca9555_set_pin_polarity(tca9555_handle_t *handle, tca9555_port_t port, tca9555_pin_t pin, bool invert)
{
    tca9555_error_t result;

    result = tca9555_validate_handle(handle);
    if (result != TCA9555_OK) return result;

    result = tca9555_validate_port(port);
    if (result != TCA9555_OK) return result;

    result = tca9555_validate_pin(pin);
    if (result != TCA9555_OK) return result;

    uint8_t reg = (port == TCA9555_PORT_0) ? TCA9555_REG_POLARITY_INV_PORT_0 : TCA9555_REG_POLARITY_INV_PORT_1;
    uint8_t polarity_data;

    // Read current polarity setting
    result = tca9555_read_register(handle, reg, &polarity_data);
    if (result != TCA9555_OK) {
        return result;
    }

    // Modify the polarity bit
    if (invert) {
        polarity_data |= (1 << pin);   // Set bit for inversion
    } else {
        polarity_data &= ~(1 << pin);  // Clear bit for normal polarity
    }

    // Write back to register
    return tca9555_write_register(handle, reg, polarity_data);
}

tca9555_error_t tca9555_set_port_polarity(tca9555_handle_t *handle, tca9555_port_t port, uint8_t polarity_mask)
{
    tca9555_error_t result;

    result = tca9555_validate_handle(handle);
    if (result != TCA9555_OK) return result;

    result = tca9555_validate_port(port);
    if (result != TCA9555_OK) return result;

    uint8_t reg = (port == TCA9555_PORT_0) ? TCA9555_REG_POLARITY_INV_PORT_0 : TCA9555_REG_POLARITY_INV_PORT_1;

    return tca9555_write_register(handle, reg, polarity_mask);
}

tca9555_error_t tca9555_toggle_pin(tca9555_handle_t *handle, tca9555_port_t port, tca9555_pin_t pin)
{
    tca9555_error_t result;

    result = tca9555_validate_handle(handle);
    if (result != TCA9555_OK) return result;

    result = tca9555_validate_port(port);
    if (result != TCA9555_OK) return result;

    result = tca9555_validate_pin(pin);
    if (result != TCA9555_OK) return result;

    uint8_t *cache = (port == TCA9555_PORT_0) ? &handle->output_port0_cache : &handle->output_port1_cache;
    uint8_t reg = (port == TCA9555_PORT_0) ? TCA9555_REG_OUTPUT_PORT_0 : TCA9555_REG_OUTPUT_PORT_1;

    // Toggle the bit in cache
    *cache ^= (1 << pin);

    // Write to register
    return tca9555_write_register(handle, reg, *cache);
}

tca9555_error_t tca9555_reset(tca9555_handle_t *handle)
{
    tca9555_error_t result;

    result = tca9555_validate_handle(handle);
    if (result != TCA9555_OK) return result;

    // Reset all registers to default values
    // Set all pins as inputs
    result = tca9555_write_register(handle, TCA9555_REG_CONFIG_PORT_0, TCA9555_DEFAULT_CONFIG_PORT);
    if (result != TCA9555_OK) return result;

    result = tca9555_write_register(handle, TCA9555_REG_CONFIG_PORT_1, TCA9555_DEFAULT_CONFIG_PORT);
    if (result != TCA9555_OK) return result;

    // Set all outputs high
    result = tca9555_write_register(handle, TCA9555_REG_OUTPUT_PORT_0, TCA9555_DEFAULT_OUTPUT_PORT);
    if (result != TCA9555_OK) return result;

    result = tca9555_write_register(handle, TCA9555_REG_OUTPUT_PORT_1, TCA9555_DEFAULT_OUTPUT_PORT);
    if (result != TCA9555_OK) return result;

    // Clear polarity inversion
    result = tca9555_write_register(handle, TCA9555_REG_POLARITY_INV_PORT_0, TCA9555_DEFAULT_POLARITY_INV);
    if (result != TCA9555_OK) return result;

    result = tca9555_write_register(handle, TCA9555_REG_POLARITY_INV_PORT_1, TCA9555_DEFAULT_POLARITY_INV);
    if (result != TCA9555_OK) return result;

    // Update cache
    handle->output_port0_cache = TCA9555_DEFAULT_OUTPUT_PORT;
    handle->output_port1_cache = TCA9555_DEFAULT_OUTPUT_PORT;
    handle->config_port0_cache = TCA9555_DEFAULT_CONFIG_PORT;
    handle->config_port1_cache = TCA9555_DEFAULT_CONFIG_PORT;

    return TCA9555_OK;
}

tca9555_error_t tca9555_read_all_inputs(tca9555_handle_t *handle, uint16_t *input_data)
{
    tca9555_error_t result;

    result = tca9555_validate_handle(handle);
    if (result != TCA9555_OK) return result;

    if (input_data == NULL) {
        return TCA9555_ERROR_NULL_POINTER;
    }

    uint8_t port0_data, port1_data;

    // Read Port 0
    result = tca9555_read_register(handle, TCA9555_REG_INPUT_PORT_0, &port0_data);
    if (result != TCA9555_OK) return result;

    // Read Port 1
    result = tca9555_read_register(handle, TCA9555_REG_INPUT_PORT_1, &port1_data);
    if (result != TCA9555_OK) return result;

    // Combine into 16-bit value (Port 1 in upper byte, Port 0 in lower byte)
    *input_data = ((uint16_t)port1_data << 8) | port0_data;

    return TCA9555_OK;
}

tca9555_error_t tca9555_write_all_outputs(tca9555_handle_t *handle, uint16_t output_data)
{
    tca9555_error_t result;

    result = tca9555_validate_handle(handle);
    if (result != TCA9555_OK) return result;

    uint8_t port0_data = output_data & 0xFF;         // Lower 8 bits
    uint8_t port1_data = (output_data >> 8) & 0xFF; // Upper 8 bits

    // Write Port 0
    result = tca9555_write_register(handle, TCA9555_REG_OUTPUT_PORT_0, port0_data);
    if (result != TCA9555_OK) return result;

    // Write Port 1
    result = tca9555_write_register(handle, TCA9555_REG_OUTPUT_PORT_1, port1_data);
    if (result != TCA9555_OK) return result;

    return TCA9555_OK;
}

tca9555_error_t tca9555_get_port_config(tca9555_handle_t *handle, tca9555_port_t port, uint8_t *config_data)
{
    tca9555_error_t result;

    result = tca9555_validate_handle(handle);
    if (result != TCA9555_OK) return result;

    result = tca9555_validate_port(port);
    if (result != TCA9555_OK) return result;

    if (config_data == NULL) {
        return TCA9555_ERROR_NULL_POINTER;
    }

    uint8_t reg = (port == TCA9555_PORT_0) ? TCA9555_REG_CONFIG_PORT_0 : TCA9555_REG_CONFIG_PORT_1;

    return tca9555_read_register(handle, reg, config_data);
}

#endif /* MASTER_CORE*/