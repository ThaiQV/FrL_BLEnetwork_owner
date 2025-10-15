/**
 * @file event_bus.h
 * @brief Event-driven publish/subscribe system
 * @author Nghia Hoang
 * @date 2025
 */

#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#ifndef MASTER_CORE
#ifdef COUNTER_DEVICE

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== CONSTANTS ========== */
#define EVENT_BUS_MAX_SUBSCRIBERS       64
#define EVENT_BUS_MAX_EVENTS           16
#define EVENT_BUS_MAX_EVENT_DATA_SIZE   1
#define EVENT_BUS_INVALID_ID           -1

/* ========== TYPES ========== */

/**
 * @brief Event bus return codes
 */
typedef enum {
    EVENT_BUS_OK = 0,
    EVENT_BUS_ERROR = -1,
    EVENT_BUS_ERROR_MEMORY = -2,
    EVENT_BUS_ERROR_INVALID_PARAM = -3,
    EVENT_BUS_ERROR_QUEUE_FULL = -4,
    EVENT_BUS_ERROR_NOT_FOUND = -5,
    EVENT_BUS_ERROR_MAX_SUBSCRIBERS = -6
} event_bus_result_t;

/**
 * @brief Event types enumeration
 */
typedef enum {
    /* System events */
    EVENT_SYSTEM_INIT = 0x0000,
    EVENT_SYSTEM_READY,
    EVENT_SYSTEM_ERROR,
    EVENT_SYSTEM_SHUTDOWN,
    EVENT_SYSTEM_MODE_CHANGED,
    EVENT_SYSTEM_START,
    EVENT_SYSTEM_STOP,
    EVENT_SYSTEM_PAUSE,
    EVENT_SYSTEM_RESUME,

	/* User-defined subapp events */

	/********** Button events *********/
	//INPUT EVENT

	//OUTPUT EVENT
    EVENT_BUTTON_NONE = 0x0100,
	EVENT_BUTTON_RST_ONCLICK,
	EVENT_BUTTON_RST_HOLD_3S,
    EVENT_BUTTON_PPD_HOLD_3S,
	EVENT_BUTTON_CALL_ONCLICK,
	EVENT_BUTTON_ENDCALL_ONCLICK,
	EVENT_BUTTON_PPD_ONCLICK,
	EVENT_BUTTON_PPU_ONCLICK,
	EVENT_BUTTON_PED_ONCLICK,
	EVENT_BUTTON_PEU_ONCLICK,
	EVENT_BUTTON_RST_PED_HOLD_5S,
	EVENT_BUTTON_RST_PEU_HOLD_5S,
    EVENT_BUTTON_CALL_HOLD_3S,
    EVENT_BUTTON_ENDCALL_HOLD_5S,

	/***************************************/

	/********** Data storage event *********/
	//INPUT EVENT
	EVENT_DATA_STORAGE_START = 0x0200,
	EVENT_DATA_STORAGE_STOP,
	EVENT_DATA_STORAGE_READ,

	//OUTPUT EVENT
	/***************************************/

	/********** LCD event *********/
	//INPUT EVENT
	EVENT_LCD_PRINT_MESS = 0x0300,
    EVENT_LCD_PRINT_COUNT_PRODUCT,
    EVENT_LCD_PRINT_START_DONE,
    EVENT_LCD_PRINT_MAC,
    EVENT_LCD_PRINT_SELECT_MODE,
    EVENT_LCD_PRINT_FACTORY_RESET,
    EVENT_LCD_PRINT_PAIRING,
    EVENT_LCD_PRINT_CALL__,
    EVENT_LCD_PRINT_MESS_NEW,
    EVENT_LCD_PRINT_REMOVE_GW,

	//OUTPUT EVENT
	/***************************************/
    EVENT_LCD_PRINT_SELECT_MODE_TIMEOUT,

	/********** LED event *********/
	//INPUT EVENT
	EVENT_LED_NWK_ONLINE = 0x0400,
    EVENT_LED_NWK_OFFLINE,
    EVENT_LED_CALL_ON,
    EVENT_LED_NWK_CALL_OFF,
    EVENT_LED_CALL_BLINK3,

	//OUTPUT EVENT
	/***************************************/

	/********** LED7 event *********/
	//INPUT EVENT
	EVENT_LED7_PRINT_ERR = 0x0500,

	//OUTPUT EVENT
	/***************************************/

	/********** DATA handler event *********/
	//INPUT EVENT
	EVENT_DATA_HANDLER = 0x0600,

	//OUTPUT EVENT
    EVENT_DATA_CALL,
    EVENT_DATA_ENDCALL,
    EVENT_DATA_START_DONE,
    EVENT_DATA_RESET,
    EVENT_DATA_ERR_PRODUCT_CHANGE,
    EVENT_DATA_PASS_PRODUCT_UP,
    EVENT_DATA_PASS_PRODUCT_DOWN,
    EVENT_DATA_SWITCH_MODE,
    EVENT_DATA_CALL_FAIL_OFFLINE,
    EVENT_DATA_CALL_FAIL_NORSP,
    EVENT_DATA_ENDCALL_FAIL_NORSP,

	/***************************************/
    
    /* Maximum event value */
    EVENT_MAX = 0xFFFF
} event_type_t;
/**
 * ______________
 * |  SUBAPP    |
 * |            |<------- INPUT EVENT
 * |            |
 * |            |-------> OUTPUT EVENT
 * |____________|
 * 
 */

/**
 * @brief Event priority levels
 */
typedef enum {
    EVENT_PRIORITY_CRITICAL = 0,
    EVENT_PRIORITY_HIGH = 1,
    EVENT_PRIORITY_NORMAL = 2,
    EVENT_PRIORITY_LOW = 3
} event_priority_t;

/**
 * @brief Event structure
 */
typedef struct {
	event_type_t type;                          ///< Event type
    event_priority_t priority;                  ///< Event priority
    uint32_t timestamp;                         ///< Event timestamp
    uint16_t data_size;                         ///< Size of event data
    uint8_t data[EVENT_BUS_MAX_EVENT_DATA_SIZE]; ///< Event data payload
    void* sender;                               ///< Sender context (optional)
} event_t;

/**
 * @brief Event handler function pointer
 * @param event Pointer to event structure
 * @param user_data User data passed during subscription
 */
typedef void (*event_handler_t)(const event_t* event, void* user_data);

/**
 * @brief Event filter function pointer
 * @param event Pointer to event structure
 * @param user_data User data passed during subscription
 * @return true if event should be processed, false to filter out
 */
typedef bool (*event_filter_t)(const event_t* event, void* user_data);

/**
 * @brief Subscriber structure (internal use)
 */
typedef struct {
    event_type_t event_type;        ///< Subscribed event type
    event_handler_t handler;        ///< Event handler function
    event_filter_t filter;          ///< Optional event filter (NULL if none)
    void* user_data;                ///< User data for handler/filter
    bool enabled;                   ///< Subscription enabled flag
    char name[16];                  ///< Subscriber name for debugging
} event_subscriber_t;

/**
 * @brief Event bus statistics
 */
typedef struct {
    uint32_t events_published;      ///< Total events published
    uint32_t events_processed;      ///< Total events processed
    uint32_t events_dropped;        ///< Events dropped (queue full)
    uint32_t subscribers_count;     ///< Current number of subscribers
    uint32_t queue_size;            ///< Current queue size
    uint32_t queue_max_size;        ///< Maximum queue size reached
} event_bus_stats_t;

/* ========== PUBLIC FUNCTIONS ========== */

/**
 * @brief Initialize the event bus system
 * @return EVENT_BUS_OK on success, error code on failure
 */
event_bus_result_t event_bus_init(void);

/**
 * @brief Deinitialize the event bus system
 * @return EVENT_BUS_OK on success
 */
event_bus_result_t event_bus_deinit(void);

/**
 * @brief Process pending events in the queue
 * @note This should be called regularly from the main loop
 * @return Number of events processed, or negative error code
 */
int event_bus_process(void);

/**
 * @brief Publish an event to the bus
 * @param type Event type
 * @param data Event data (can be NULL)
 * @param data_size Size of event data (0 if data is NULL)
 * @param priority Event priority
 * @param sender Sender context (optional, can be NULL)
 * @return EVENT_BUS_OK on success, error code on failure
 */
event_bus_result_t event_bus_publish(event_type_t type, const void* data, 
                                    uint16_t data_size, event_priority_t priority,
                                    void* sender);

/**
 * @brief Subscribe to an event type
 * @param event_type Event type to subscribe to
 * @param handler Event handler function
 * @param user_data User data to pass to handler (optional)
 * @param name Subscriber name for debugging
 * @return Subscriber ID on success, negative error code on failure
 */
int event_bus_subscribe(event_type_t event_type, event_handler_t handler,
                       void* user_data, const char* name);

/**
 * @brief Subscribe to an event type with filter
 * @param event_type Event type to subscribe to
 * @param handler Event handler function
 * @param filter Event filter function (optional)
 * @param user_data User data to pass to handler/filter (optional)
 * @param name Subscriber name for debugging
 * @return Subscriber ID on success, negative error code on failure
 */
int event_bus_subscribe_filtered(event_type_t event_type, event_handler_t handler,
                                event_filter_t filter, void* user_data, 
                                const char* name);

/**
 * @brief Unsubscribe from events
 * @param subscriber_id Subscriber ID returned from subscribe function
 * @return EVENT_BUS_OK on success, error code on failure
 */
event_bus_result_t event_bus_unsubscribe(int subscriber_id);

/**
 * @brief Enable/disable a subscription
 * @param subscriber_id Subscriber ID
 * @param enabled true to enable, false to disable
 * @return EVENT_BUS_OK on success, error code on failure
 */
event_bus_result_t event_bus_set_subscription_enabled(int subscriber_id, bool enabled);

/**
 * @brief Get event bus statistics
 * @param stats Pointer to statistics structure to fill
 * @return EVENT_BUS_OK on success
 */
event_bus_result_t event_bus_get_stats(event_bus_stats_t* stats);

/**
 * @brief Clear event bus statistics
 * @return EVENT_BUS_OK on success
 */
event_bus_result_t event_bus_clear_stats(void);

/**
 * @brief Check if event queue is full
 * @return true if queue is full, false otherwise
 */
bool event_bus_is_queue_full(void);

/**
 * @brief Get current queue size
 * @return Current number of events in queue
 */
uint32_t event_bus_get_queue_size(void);

/**
 * @brief Flush all pending events in queue
 * @return Number of events flushed
 */
int event_bus_flush_queue(void);

/* ========== CONVENIENCE MACROS ========== */

/**
 * @brief Publish a simple event without data
 */
#define EVENT_PUBLISH_SIMPLE(type, priority) \
    event_bus_publish((type), NULL, 0, (priority), NULL)

/**
 * @brief Publish an event with data
 */
#define EVENT_PUBLISH_DATA(type, data_ptr, size, priority) \
    event_bus_publish((type), (data_ptr), (size), (priority), NULL)

/**
 * @brief Subscribe to event with simple handler
 */
#define EVENT_SUBSCRIBE(type, handler, name) \
    event_bus_subscribe((type), (handler), NULL, NULL)

/**
 * @brief Get event data as specific type
 */
#define EVENT_GET_DATA(event, type) \
    ((event)->data_size >= sizeof(type) ? (const type*)(event)->data : NULL)

/**
 * @brief Check if event has data
 */
#define EVENT_HAS_DATA(event) \
    ((event)->data_size > 0)

/* ========== DEBUG/LOGGING FUNCTIONS ========== */
#ifdef DEBUG
/**
 * @brief Print event bus debug information
 */
void event_bus_debug_print(void);

/**
 * @brief Convert event type to string (for debugging)
 * @param type Event type
 * @return String representation of event type
 */
const char* event_type_to_string(event_type_t type);
#endif

#ifdef __cplusplus
}
#endif

#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/
#endif /* EVENT_BUS_H */
