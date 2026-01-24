/**
 * @file uart_driver.c
 * @brief UART Driver Implementation
 */

#include <stdlib.h>

#ifndef MASTER_CORE
#include "tl_common.h"
#include "vendor/TBS_dev/TBS_dev_config.h"
#ifdef POWER_METER_DEVICE

#include "uart_protocol.h"
// #include "../power_meter_app.h"

/*============================================================================
 *********************** UART DRIVER ******************************************
 *============================================================================*/
/*============================================================================
 * CIRCULAR BUFFER OPERATIONS
 *============================================================================*/
static void cb_init(uart_circular_buffer_t *cb)
{
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
}

static bool cb_push(uart_circular_buffer_t *cb, uint8_t data)
{
    if (cb->count >= UART_DRIVER_RX_BUFFER_SIZE)
    {
        return false;
    }
    cb->buffer[cb->head] = data;
    cb->head = (cb->head + 1) % UART_DRIVER_RX_BUFFER_SIZE;
    cb->count++;
    return true;
}

static bool cb_pop(uart_circular_buffer_t *cb, uint8_t *data)
{
    if (cb->count == 0)
    {
        return false;
    }
    *data = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) % UART_DRIVER_RX_BUFFER_SIZE;
    cb->count--;
    return true;
}

static uint16_t cb_available(const uart_circular_buffer_t *cb)
{
    return cb->count;
}

/*============================================================================
 * PUBLIC FUNCTIONS
 *============================================================================*/
uart_status_t uart_driver_init(
    uart_driver_t *driver,
    uart_hal_send_fn_t hal_send,
    uart_hal_get_tick_fn_t hal_get_tick)
{
    if (!driver || !hal_send || !hal_get_tick)
    {
        return UART_ERROR_INVALID_PARAM;
    }

    memset(driver, 0, sizeof(uart_driver_t));

    driver->hal_send = hal_send;
    driver->hal_get_tick = hal_get_tick;

    cb_init(&driver->rx_buffer);

    return UART_OK;
}

uart_status_t uart_driver_register_handler(
    uart_driver_t *driver,
    protocol_handler_t *handler)
{
    if (!driver || !handler || !handler->rx_callback)
    {
        return UART_ERROR_INVALID_PARAM;
    }

    if (driver->handler_count >= UART_DRIVER_MAX_PROTOCOLS)
    {
        return UART_ERROR_NO_SPACE;
    }

    // Check if already registered
    for (uint8_t i = 0; i < driver->handler_count; i++)
    {
        if (driver->handlers[i] == handler)
        {
            return UART_OK; // Already registered
        }
    }

    // Add handler
    driver->handlers[driver->handler_count++] = handler;

    return UART_OK;
}

uart_status_t uart_driver_unregister_handler(
    uart_driver_t *driver,
    protocol_handler_t *handler)
{
    if (!driver || !handler)
    {
        return UART_ERROR_INVALID_PARAM;
    }

    // Find and remove handler
    for (uint8_t i = 0; i < driver->handler_count; i++)
    {
        if (driver->handlers[i] == handler)
        {
            // Shift remaining handlers
            for (uint8_t j = i; j < driver->handler_count - 1; j++)
            {
                driver->handlers[j] = driver->handlers[j + 1];
            }
            driver->handler_count--;
            return UART_OK;
        }
    }

    return UART_ERROR_NOT_FOUND;
}

uart_status_t uart_driver_send(
    uart_driver_t *driver,
    const uint8_t *data,
    uint16_t len)
{
    if (!driver || !data || len == 0)
    {
        return UART_ERROR_INVALID_PARAM;
    }

    driver->hal_send(data, len);
    driver->total_tx_bytes += len;

    return UART_OK;
}

uart_status_t uart_driver_receive_irq(
    uart_driver_t *driver,
    const uint8_t *data,
    uint16_t len)
{
    if (!driver || !data)
    {
        return UART_ERROR_INVALID_PARAM;
    }

    // Push data into circular buffer
    for (uint16_t i = 0; i < len; i++)
    {
        if (!cb_push(&driver->rx_buffer, data[i]))
        {
            driver->rx_overflow_count++;
            return UART_ERROR_BUFFER_FULL;
        }
    }

    driver->total_rx_bytes += len;

    return UART_OK;
}

void uart_driver_process(uart_driver_t *driver)
{
    if (!driver)
        return;

    // Process all available data in buffer
    while (cb_available(&driver->rx_buffer) > 0)
    {
        uint8_t byte;
        cb_pop(&driver->rx_buffer, &byte);

        // Distribute this byte to ALL protocol handlers
        // Each protocol parses itself and decides if data belongs to it
        for (uint8_t i = 0; i < driver->handler_count; i++)
        {
            protocol_handler_t *handler = driver->handlers[i];
            if (handler && handler->enabled && handler->rx_callback)
            {
                handler->rx_callback(handler, &byte, 1);
            }
        }
    }
}

void uart_driver_get_stats(
    uart_driver_t *driver,
    uint32_t *total_rx_bytes,
    uint32_t *total_tx_bytes,
    uint32_t *rx_overflow_count)
{
    if (!driver)
        return;

    if (total_rx_bytes)
        *total_rx_bytes = driver->total_rx_bytes;
    if (total_tx_bytes)
        *total_tx_bytes = driver->total_tx_bytes;
    if (rx_overflow_count)
        *rx_overflow_count = driver->rx_overflow_count;
}

/*============================================================================
 * ************************** PROTOCOL ****************************************
 *============================================================================*/

/*============================================================================
 * CRC16 FUNCTIONS
 *============================================================================*/
static uint16_t crc16_update(uint16_t crc, uint8_t data)
{
    crc ^= (uint16_t)data << 8;
    for (uint8_t i = 0; i < 8; i++)
    {
        if (crc & 0x8000)
        {
            crc = (crc << 1) ^ 0x1021;
        }
        else
        {
            crc <<= 1;
        }
    }
    return crc;
}

static uint16_t crc16_calculate(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++)
    {
        crc = crc16_update(crc, data[i]);
    }
    return crc;
}

/*============================================================================
 * FRAME BUILDING
 *============================================================================*/
static uint16_t build_frame(
    uint8_t *frame,
    uint8_t protocol_id,
    const uint8_t *data,
    uint16_t len)
{
    uint16_t idx = 0;
    frame[idx++] = PROTOCOL_HEADER1;
    frame[idx++] = PROTOCOL_HEADER2;
    frame[idx++] = PROTOCOL_STX;
    frame[idx++] = protocol_id;
    frame[idx++] = len & 0xFF;
    frame[idx++] = (len >> 8) & 0xFF;

    if (len > 0 && data != NULL)
    {
        memcpy(&frame[idx], data, len);
        idx += len;
    }

    uint16_t crc = crc16_calculate(&frame[3], idx - 3);
    frame[idx++] = crc & 0xFF;
    frame[idx++] = (crc >> 8) & 0xFF;
    frame[idx++] = PROTOCOL_ETX;

    return idx;
}
//
//static void send_ack(protocol_context_t *protocol)
//{
//    uint8_t ack = PROTOCOL_ACK;
//    uart_driver_send(protocol->uart_driver, &ack, 1);
//}
//
//static void send_nack(protocol_context_t *protocol)
//{
//    uint8_t nack = PROTOCOL_NACK;
//    uart_driver_send(protocol->uart_driver, &nack, 1);
//}

/*============================================================================
 * FRAME PROCESSING
 *============================================================================*/
static void process_rx_frame(protocol_context_t *protocol)
{
    // Extract protocol ID
    uint8_t frame_protocol_id = protocol->rx_frame[1];

    // Check if this frame belongs to this protocol
    if (frame_protocol_id != protocol->protocol_id)
    {
        LOG_P(PERI,"err_id\n");
        return; // Not for this protocol, ignore
    }
    // // Extract length
    // uint16_t payload_len = protocol->rx_frame[2] | (protocol->rx_frame[3] << 8);

    // // Verify length
    // uint16_t expected_len = PROTOCOL_FRAME_OVERHEAD + payload_len;
    // if (protocol->rx_frame_idx != expected_len) {
    //     protocol->rx_frame_error_count++;
    //     send_nack(protocol);
    //     PROTOCOL_DEBUG("Frame length mismatch");
    //     return;
    // }

    // // Extract CRC
    // uint16_t frame_crc = protocol->rx_frame[protocol->rx_frame_idx - 3] |
    //                      (protocol->rx_frame[protocol->rx_frame_idx - 2] << 8);

    // // Calculate CRC
    // uint16_t calc_crc = crc16_calculate(&protocol->rx_frame[1], payload_len + 3);

    // if (frame_crc != calc_crc) {
    //     protocol->rx_crc_error_count++;
    //     send_nack(protocol);
    //     PROTOCOL_DEBUG("CRC error %d: %d %d",payload_len + 3, frame_crc, calc_crc );
    //     return;
    // }

    // CRC OK - send ACK
    // send_ack(protocol);
    protocol->rx_success_count++;
    // Call user callback
    if (protocol->data_callback)
    {
        const uint8_t *payload = &protocol->rx_frame[4];
        protocol->data_callback(protocol->protocol_id, payload, protocol->rx_frame_idx - 5);
    }

    // PROTOCOL_DEBUG("Frame processed");
}

/*============================================================================
 * UART RX CALLBACK (called by UART driver)
 *============================================================================*/
static void protocol_uart_rx_callback(protocol_handler_t *handler, const uint8_t *data, uint16_t len)
{
    if (!handler || !handler->context)
        return;

    protocol_context_t *protocol = (protocol_context_t *)handler->context;

    for (uint16_t i = 0; i < len; i++)
    {
        uint8_t byte = data[i];

        if (byte == PROTOCOL_ACK && protocol->tx_state == PROTOCOL_STATE_WAIT_ACK)
        {
            protocol->tx_state = PROTOCOL_STATE_IDLE;
            protocol->tx_success_count++;
            PROTOCOL_DEBUG("ACK received");
            continue;
        }
        else if (byte == PROTOCOL_NACK && protocol->tx_state == PROTOCOL_STATE_WAIT_ACK)
        {
            uart_driver_send(protocol->uart_driver, protocol->tx_frame, protocol->tx_frame_len);
            protocol->tx_retry_count++;
            protocol->tx_timeout_tick = protocol->uart_driver->hal_get_tick() + PROTOCOL_TIMEOUT_MS;
            PROTOCOL_DEBUG("NACK received - retry");
            continue;
        }

        switch (protocol->rx_state)
        {
        case RX_WAIT_PRE1:
            if (byte == PROTOCOL_HEADER1)
            {
                protocol->rx_state = RX_WAIT_PRE2;
            }
            break;

        case RX_WAIT_PRE2:
            if (byte == PROTOCOL_HEADER2)
            {
                protocol->rx_state = RX_WAIT_STX;
            }
            else
            {
                protocol->rx_state = RX_WAIT_PRE1;
            }
            break;

        case RX_WAIT_STX:
            if (byte == PROTOCOL_STX)
            {
                protocol->rx_state = RX_WAIT_ID;
                protocol->rx_frame_idx = 0;
                protocol->rx_frame[protocol->rx_frame_idx++] = byte;
            }
            else
            {
                protocol->rx_state = RX_WAIT_PRE1;
            }
            break;

        case RX_WAIT_ID:
            protocol->rx_frame[protocol->rx_frame_idx++] = byte;
            protocol->rx_state = 10;
            break;

        case RX_WAIT_LEN1:
            protocol->rx_frame[protocol->rx_frame_idx++] = byte;
            protocol->rx_expected_payload = byte;
            protocol->rx_state = RX_WAIT_LEN2;
            break;

        case RX_WAIT_LEN2:
            protocol->rx_frame[protocol->rx_frame_idx++] = byte;
            protocol->rx_expected_payload |= (byte << 8);
            if (protocol->rx_expected_payload > PROTOCOL_MAX_PAYLOAD_SIZE)
            {
                protocol->rx_state = RX_WAIT_PRE1;
                protocol->rx_frame_error_count++;
            }
            else
            {
                protocol->rx_bytes_needed = protocol->rx_expected_payload;
                protocol->rx_state = (protocol->rx_bytes_needed == 0) ? RX_WAIT_CRC1 : RX_WAIT_DATA;
            }
            break;

        case RX_WAIT_DATA:
            protocol->rx_frame[protocol->rx_frame_idx++] = byte;
            if (--protocol->rx_bytes_needed == 0)
            {
                protocol->rx_state = RX_WAIT_CRC1;
            }
            break;

        case RX_WAIT_CRC1:
            protocol->rx_frame[protocol->rx_frame_idx++] = byte;
            protocol->rx_state = RX_WAIT_CRC2;
            break;

        case RX_WAIT_CRC2:
            protocol->rx_frame[protocol->rx_frame_idx++] = byte;
            protocol->rx_state = RX_WAIT_ETX;
            break;

        case RX_WAIT_ETX:
            if (byte == PROTOCOL_ETX)
            {
                protocol->rx_frame[protocol->rx_frame_idx++] = byte;
                process_rx_frame(protocol);
            }
            else
            {
                protocol->rx_frame_error_count++;
            }
            protocol->rx_state = RX_WAIT_PRE1;
            break;

        default:
            protocol->rx_frame[protocol->rx_frame_idx++] = byte;
            if (byte == PROTOCOL_ETX)
            {
                process_rx_frame(protocol);
                protocol->rx_state = RX_WAIT_PRE1;
            }

            break;
        }
    }
}

/*============================================================================
 * PUBLIC FUNCTIONS
 *============================================================================*/
protocol_status_t protocol_init(
    protocol_context_t *protocol,
    uart_driver_t *uart_driver,
    uint8_t protocol_id,
    protocol_data_callback_t data_callback)
{
    if (!protocol || !uart_driver || !data_callback)
    {
        return PROTOCOL_ERROR_INVALID_PARAM;
    }

    memset(protocol, 0, sizeof(protocol_context_t));

    protocol->uart_driver = uart_driver;
    protocol->protocol_id = protocol_id;
    protocol->data_callback = data_callback;
    protocol->tx_state = PROTOCOL_STATE_IDLE;

    protocol->rx_state = RX_WAIT_PRE1;
    protocol->rx_frame_idx = 0;
    protocol->rx_expected_payload = 0;
    protocol->rx_bytes_needed = 0;

    // Setup UART handler
    protocol->uart_handler.context = protocol;
    protocol->uart_handler.rx_callback = protocol_uart_rx_callback;
    protocol->uart_handler.enabled = true;
    protocol->uart_handler.name = "Protocol";

    // Register with UART driver
    uart_status_t status = uart_driver_register_handler(uart_driver, &protocol->uart_handler);
    if (status != UART_OK)
    {
        return PROTOCOL_ERROR_INVALID_PARAM;
    }

    PROTOCOL_DEBUG("Protocol initialized");

    return PROTOCOL_OK;
}

protocol_status_t protocol_deinit(protocol_context_t *protocol)
{
    if (!protocol || !protocol->uart_driver)
    {
        return PROTOCOL_ERROR_INVALID_PARAM;
    }

    uart_driver_unregister_handler(protocol->uart_driver, &protocol->uart_handler);
    memset(protocol, 0, sizeof(protocol_context_t));

    return PROTOCOL_OK;
}

protocol_status_t protocol_send(
    protocol_context_t *protocol,
    const uint8_t *data,
    uint16_t len)
{
    if (!protocol || !protocol->uart_driver)
    {
        return PROTOCOL_ERROR_INVALID_PARAM;
    }

    if (len > PROTOCOL_MAX_PAYLOAD_SIZE)
    {
        return PROTOCOL_ERROR_FRAME_TOO_LARGE;
    }

    if (protocol->tx_state != PROTOCOL_STATE_IDLE)
    {
        return PROTOCOL_ERROR_TX_BUSY;
    }

    // Build frame
    protocol->tx_frame_len = build_frame(protocol->tx_frame, protocol->protocol_id, data, len);

    // Send frame
    uart_driver_send(protocol->uart_driver, protocol->tx_frame, protocol->tx_frame_len);

    // Setup for ACK waiting
    protocol->tx_state = PROTOCOL_STATE_WAIT_ACK;
    protocol->tx_retry_count = 0;
    protocol->tx_timeout_tick = protocol->uart_driver->hal_get_tick() + PROTOCOL_TIMEOUT_MS;

    PROTOCOL_DEBUG("Frame sent, waiting ACK");

    return PROTOCOL_OK;
}

protocol_status_t protocol_send_no_ack(
    protocol_context_t *protocol,
    const uint8_t *data,
    uint16_t len)
{
    if (!protocol || !protocol->uart_driver)
    {
        return PROTOCOL_ERROR_INVALID_PARAM;
    }

    if (len > PROTOCOL_MAX_PAYLOAD_SIZE)
    {
        return PROTOCOL_ERROR_FRAME_TOO_LARGE;
    }

    uint8_t frame[PROTOCOL_MAX_FRAME_SIZE];
    uint16_t frame_len = build_frame(frame, protocol->protocol_id, data, len);

    uart_driver_send(protocol->uart_driver, frame, frame_len);

    return PROTOCOL_OK;
}

void protocol_process(protocol_context_t *protocol)
{
    if (!protocol)
        return;

    // Handle TX timeout and retry
    if (protocol->tx_state == PROTOCOL_STATE_WAIT_ACK)
    {
        uint32_t current_tick = protocol->uart_driver->hal_get_tick();

        if (current_tick >= protocol->tx_timeout_tick)
        {
            protocol->tx_retry_count++;

            if (protocol->tx_retry_count >= PROTOCOL_TX_RETRY_COUNT)
            {
                protocol->tx_state = PROTOCOL_STATE_IDLE;
                protocol->tx_fail_count++;
                PROTOCOL_DEBUG("TX failed - max retry");
            }
            else
            {
                uart_driver_send(protocol->uart_driver, protocol->tx_frame, protocol->tx_frame_len);
                protocol->tx_timeout_tick = current_tick + PROTOCOL_TIMEOUT_MS;
                PROTOCOL_DEBUG("TX retry");
            }
        }
    }
}

void protocol_set_debug_callback(
    protocol_context_t *protocol,
    protocol_debug_callback_t callback)
{
    if (protocol)
    {
        protocol->debug_callback = callback;
    }
}

void protocol_get_stats(
    protocol_context_t *protocol,
    uint32_t *tx_success,
    uint32_t *tx_fail,
    uint32_t *rx_success,
    uint32_t *rx_crc_error,
    uint32_t *rx_frame_error)
{
    if (!protocol)
        return;

    if (tx_success)
        *tx_success = protocol->tx_success_count;
    if (tx_fail)
        *tx_fail = protocol->tx_fail_count;
    if (rx_success)
        *rx_success = protocol->rx_success_count;
    if (rx_crc_error)
        *rx_crc_error = protocol->rx_crc_error_count;
    if (rx_frame_error)
        *rx_frame_error = protocol->rx_frame_error_count;
}

void protocol_reset_stats(protocol_context_t *protocol)
{
    if (!protocol)
        return;

    protocol->tx_success_count = 0;
    protocol->tx_fail_count = 0;
    protocol->rx_success_count = 0;
    protocol->rx_crc_error_count = 0;
    protocol->rx_frame_error_count = 0;
}

#endif /* POWER_METER_DEVICE*/
#endif /* MASTER_CORE*/
