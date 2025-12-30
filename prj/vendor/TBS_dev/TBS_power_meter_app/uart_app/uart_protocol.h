/**
 * @file uart_protocol.h
 * @brief UART Driver Layer - Platform independent
 * @version 2.0
 *
 * Architecture:
 * ┌─────────────────────────────────────────────────┐
 * │  Protocol Layer (can have multiple instances)   │
 * │  - Protocol A, Protocol B, Protocol C...        │
 * └─────────────────────────────────────────────────┘
 *                      ↕ (callback)
 * ┌─────────────────────────────────────────────────┐
 * │  UART Driver Layer (shared, singleton)          │
 * │  - Receive data from IRQ                        │
 * │  - Distribute to correct protocol handler       │
 * └─────────────────────────────────────────────────┘
 *                      ↕
 * ┌─────────────────────────────────────────────────┐
 * │  HAL Layer (platform specific)                  │
 * │  - UART hardware, IRQ                           │
 * └─────────────────────────────────────────────────┘
 */

#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../power_meter_app.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================
 * UART DRIVER
 *============================================================================*/

/*============================================================================
 * UART DRIVER CONFIGURATION
 *============================================================================*/
#define UART_DRIVER_MAX_PROTOCOLS 8    // Max number of protocol instances
#define UART_DRIVER_RX_BUFFER_SIZE 512 // Shared RX buffer

    /*============================================================================
     * TYPES
     *============================================================================*/
    typedef enum
    {
        UART_OK = 0,
        UART_ERROR_INVALID_PARAM,
        UART_ERROR_BUFFER_FULL,
        UART_ERROR_NO_SPACE,
        UART_ERROR_NOT_FOUND
    } uart_status_t;

    // Forward declaration
    typedef struct uart_driver_t uart_driver_t;
    typedef struct protocol_handler_t protocol_handler_t;

    /**
     * Callback when UART driver receives data
     * Protocol handler will implement this function
     * @param handler Protocol handler instance
     * @param data Received data
     * @param len Data length
     */
    typedef void (*uart_rx_callback_t)(protocol_handler_t *handler, const uint8_t *data, uint16_t len);

    /**
     * HAL function to send data via UART hardware
     * User implements this function for each platform
     */
    typedef void (*uart_hal_send_fn_t)(const uint8_t *data, uint16_t len);

    /**
     * HAL function to get system tick (milliseconds)
     */
    typedef uint32_t (*uart_hal_get_tick_fn_t)(void);

    /*============================================================================
     * PROTOCOL HANDLER STRUCTURE
     *============================================================================*/
    struct protocol_handler_t
    {
        void *context;                  // Protocol context
        uart_rx_callback_t rx_callback; // Rx Callback
        bool enabled;                   // Enable/disable handler
        const char *name;               // Protocol name (for debug)
    };

    /*============================================================================
     * UART DRIVER STRUCTURE
     *============================================================================*/
    typedef struct
    {
        uint8_t buffer[UART_DRIVER_RX_BUFFER_SIZE];
        uint16_t head;
        uint16_t tail;
        uint16_t count;
    } uart_circular_buffer_t;

    struct uart_driver_t
    {
        // HAL functions
        uart_hal_send_fn_t hal_send;
        uart_hal_get_tick_fn_t hal_get_tick;

        // Protocol handlers
        protocol_handler_t *handlers[UART_DRIVER_MAX_PROTOCOLS];
        uint8_t handler_count;

        // RX buffer
        uart_circular_buffer_t rx_buffer;

        // Statistics
        uint32_t total_rx_bytes;
        uint32_t total_tx_bytes;
        uint32_t rx_overflow_count;
    };

    /*============================================================================
     * UART DRIVER API
     *============================================================================*/

    /**
     * @brief Initialize UART driver
     * @param driver UART driver instance
     * @param hal_send HAL function to send data
     * @param hal_get_tick HAL function to get tick
     * @return uart_status_t
     */
    uart_status_t uart_driver_init(
        uart_driver_t *driver,
        uart_hal_send_fn_t hal_send,
        uart_hal_get_tick_fn_t hal_get_tick);

    /**
     * @brief Register protocol handler to UART driver
     * @param driver UART driver instance
     * @param handler Protocol handler
     * @return uart_status_t
     */
    uart_status_t uart_driver_register_handler(
        uart_driver_t *driver,
        protocol_handler_t *handler);

    uart_status_t uart_driver_unregister_handler(
        uart_driver_t *driver,
        protocol_handler_t *handler);

    /**
     * @brief Send data via UART
     * This function is called by protocol layer to send data
     * @param driver UART driver instance
     * @param data Data to send
     * @param len Data length
     * @return uart_status_t
     */
    uart_status_t uart_driver_send(
        uart_driver_t *driver,
        const uint8_t *data,
        uint16_t len);

    /**
     * @brief Receive data from UART IRQ
     * CALL THIS FUNCTION IN UART RX IRQ HANDLER
     * @param driver UART driver instance
     * @param data Data received from UART hardware
     * @param len Data length
     * @return uart_status_t
     */
    uart_status_t uart_driver_receive_irq(
        uart_driver_t *driver,
        const uint8_t *data,
        uint16_t len);

    /**
     * @brief Process UART driver - distribute data to protocols
     * CALL THIS FUNCTION IN MAIN LOOP
     * @param driver UART driver instance
     */
    void uart_driver_process(uart_driver_t *driver);

    /**
     * @brief Get statistics
     */
    void uart_driver_get_stats(
        uart_driver_t *driver,
        uint32_t *total_rx_bytes,
        uint32_t *total_tx_bytes,
        uint32_t *rx_overflow_count);

/*============================================================================
 * ************************** PROTOCOL ****************************************
 *============================================================================*/
/*============================================================================
 * PROTOCOL CONFIGURATION
 *============================================================================*/
#define PROTOCOL_MAX_PAYLOAD_SIZE 256
#define PROTOCOL_TX_RETRY_COUNT 1
#define PROTOCOL_TIMEOUT_MS 1000
#define PROTOCOL_ENABLE_DEBUG 1

/*============================================================================
 * FRAME STRUCTURE
 *============================================================================*/
#define PROTOCOL_HEADER1 'S'
#define PROTOCOL_HEADER2 'T'
#define PROTOCOL_STX 'A'
#define PROTOCOL_ETX '#'
#define PROTOCOL_ACK 'R'
#define PROTOCOL_NACK 'r'

#define PROTOCOL_FRAME_OVERHEAD 7
#define PROTOCOL_MAX_FRAME_SIZE (PROTOCOL_FRAME_OVERHEAD + PROTOCOL_MAX_PAYLOAD_SIZE)

    /*============================================================================
     * TYPES
     *============================================================================*/
    typedef enum
    {
        PROTOCOL_OK = 0,
        PROTOCOL_ERROR_INVALID_PARAM,
        PROTOCOL_ERROR_TIMEOUT,
        PROTOCOL_ERROR_CRC_FAILED,
        PROTOCOL_ERROR_TX_BUSY,
        PROTOCOL_ERROR_FRAME_TOO_LARGE
    } protocol_status_t;

    typedef enum
    {
        PROTOCOL_STATE_IDLE = 0,
        PROTOCOL_STATE_WAIT_ACK,
        PROTOCOL_STATE_RECEIVING
    } protocol_state_t;

    /* RX state machine */
    enum
    {
        RX_WAIT_PRE1 = 0,
        RX_WAIT_PRE2,
        RX_WAIT_STX,
        RX_WAIT_ID,
        RX_WAIT_LEN1,
        RX_WAIT_LEN2,
        RX_WAIT_DATA,
        RX_WAIT_CRC1,
        RX_WAIT_CRC2,
        RX_WAIT_ETX
    };

    /**
     * Callback when protocol receives valid data
     */
    typedef void (*protocol_data_callback_t)(uint8_t protocol_id, const uint8_t *data, uint16_t len);

    /**
     * Debug callback
     */
    typedef void (*protocol_debug_callback_t)(const char *msg);

    /*============================================================================
     * PROTOCOL CONTEXT
     *============================================================================*/
    typedef struct
    {
        // Link with UART driver
        uart_driver_t *uart_driver;
        protocol_handler_t uart_handler; // Handler registered with UART driver

        // Protocol identity
        uint8_t protocol_id; // ID of this protocol (0-255)

        // User callback
        protocol_data_callback_t data_callback;
        protocol_debug_callback_t debug_callback;

        // TX State
        protocol_state_t tx_state;
        uint8_t tx_frame[PROTOCOL_MAX_FRAME_SIZE];
        uint16_t tx_frame_len;
        uint8_t tx_retry_count;
        uint32_t tx_timeout_tick;

        // RX State
        uint8_t rx_frame[PROTOCOL_MAX_FRAME_SIZE];
        uint16_t rx_frame_idx;
        uint16_t rx_expected_payload;
        uint16_t rx_bytes_needed;
        uint8_t rx_state;

        // Statistics
        uint32_t tx_success_count;
        uint32_t tx_fail_count;
        uint32_t rx_success_count;
        uint32_t rx_crc_error_count;
        uint32_t rx_frame_error_count;
    } protocol_context_t;

    /*============================================================================
     * PROTOCOL API
     *============================================================================*/

    /**
     * @brief Initialize protocol instance
     * @param protocol Protocol context
     * @param uart_driver UART driver already initialized
     * @param protocol_id ID of this protocol (used to distinguish between protocols)
     * @param data_callback Callback when data is received
     * @return protocol_status_t
     */
    protocol_status_t protocol_init(
        protocol_context_t *protocol,
        uart_driver_t *uart_driver,
        uint8_t protocol_id,
        protocol_data_callback_t data_callback);

    /**
     * @brief Deinitialize protocol and unregister from UART driver
     * @param protocol Protocol context
     * @return protocol_status_t
     */
    protocol_status_t protocol_deinit(protocol_context_t *protocol);

    /**
     * @brief Send data with ACK and retry
     * @param protocol Protocol context
     * @param data Data to send
     * @param len Data length
     * @return protocol_status_t
     */
    protocol_status_t protocol_send(
        protocol_context_t *protocol,
        const uint8_t *data,
        uint16_t len);

    /**
     * @brief Send data without ACK
     * @param protocol Protocol context
     * @param data Data to send
     * @param len Data length
     * @return protocol_status_t
     */
    protocol_status_t protocol_send_no_ack(
        protocol_context_t *protocol,
        const uint8_t *data,
        uint16_t len);

    /**
     * @brief Process protocol - handle timeout, retry, etc.
     * CALL THIS FUNCTION IN MAIN LOOP
     * @param protocol Protocol context
     */
    void protocol_process(protocol_context_t *protocol);

    /**
     * @brief Set debug callback
     */
    void protocol_set_debug_callback(
        protocol_context_t *protocol,
        protocol_debug_callback_t callback);

    /**
     * @brief Get statistics
     */
    void protocol_get_stats(
        protocol_context_t *protocol,
        uint32_t *tx_success,
        uint32_t *tx_fail,
        uint32_t *rx_success,
        uint32_t *rx_crc_error,
        uint32_t *rx_frame_error);

    /**
     * @brief Reset statistics
     */
    void protocol_reset_stats(protocol_context_t *protocol);

/*============================================================================
 * HELPER MACROS
 *============================================================================*/
#ifdef PROTOCOL_DEBUG_ENABLE
#define PROTOCOL_DEBUG(...)             \
    do                                  \
    {                                   \
        print_uart("PROTOCOL_DEBUG: "); \
        print_uart(__VA_ARGS__);        \
        print_uart("\n");               \
    } while (0)
#else
#define PROTOCOL_DEBUG(...)           \
    do                                \
    {                                 \
        PMT_LOGA("PROTOCOL_DEBUG: "); \
        PMT_LOGA(__VA_ARGS__);        \
        PMT_LOGA("\n");               \
    } while (0)
#endif
/**
 * Macro to create protocol handler
 * Protocol only needs to define context and rx_callback
 */
#define PROTOCOL_HANDLER_CREATE(name, ctx, callback) \
    {                                                \
        .context = (ctx),                            \
        .rx_callback = (callback),                   \
        .enabled = true,                             \
        .name = (name)}

#ifdef __cplusplus
}
#endif

#endif // UART_DRIVER_H