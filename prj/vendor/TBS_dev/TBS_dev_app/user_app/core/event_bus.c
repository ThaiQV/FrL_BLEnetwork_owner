/**
 * @file event_bus.c
 * @brief Event-driven publish/subscribe system implementation
 * @author Your Name
 * @date 2025
 */

#include "include/event_bus.h"
#include <string.h>
#include <stdio.h>

/* ========== PRIVATE CONSTANTS ========== */
#define EVENT_QUEUE_SIZE            32      ///< Maximum events in queue
#define EVENT_SUBSCRIBER_NAME_MAX   15

/* ========== PRIVATE TYPES ========== */

/**
 * @brief Event queue structure
 */
typedef struct {
    event_t events[EVENT_QUEUE_SIZE];
    uint16_t head;              ///< Queue head index
    uint16_t tail;              ///< Queue tail index
    uint16_t count;             ///< Current number of events
    uint16_t max_count;         ///< Maximum count reached
} event_queue_t;

/* ========== PRIVATE VARIABLES ========== */
static event_queue_t g_event_queue = {0};
static event_subscriber_t g_subscribers[EVENT_BUS_MAX_SUBSCRIBERS];
static uint8_t g_subscriber_count = 0;
static event_bus_stats_t g_stats = {0};
static bool g_initialized = false;

/* ========== PRIVATE FUNCTION PROTOTYPES ========== */
static bool event_queue_is_empty(void);
static bool event_queue_is_full(void);
static event_bus_result_t event_queue_enqueue(const event_t* event);
static event_bus_result_t event_queue_dequeue(event_t* event);
static uint32_t event_get_system_time_ms(void);
static int event_find_free_subscriber_slot(void);
static event_subscriber_t* event_get_subscriber_by_id(int subscriber_id);
static void event_process_single_event(const event_t* event);
static void event_deliver_to_subscriber(const event_t* event, event_subscriber_t* subscriber);
static int event_compare_priority(const event_t* a, const event_t* b);
static void event_sort_queue_by_priority(void);

/* ========== PUBLIC FUNCTION IMPLEMENTATIONS ========== */

event_bus_result_t event_bus_init(void)
{
    // Initialize queue
    memset(&g_event_queue, 0, sizeof(event_queue_t));
    
    // Initialize subscribers
    memset(g_subscribers, 0, sizeof(g_subscribers));
    g_subscriber_count = 0;
    
    // Initialize statistics
    memset(&g_stats, 0, sizeof(event_bus_stats_t));
    
    g_initialized = true;
    
    return EVENT_BUS_OK;
}

event_bus_result_t event_bus_deinit(void)
{
    if (!g_initialized) {
        return EVENT_BUS_ERROR;
    }
    
    // Clear all data
    memset(&g_event_queue, 0, sizeof(event_queue_t));
    memset(g_subscribers, 0, sizeof(g_subscribers));
    memset(&g_stats, 0, sizeof(event_bus_stats_t));
    
    g_subscriber_count = 0;
    g_initialized = false;
    
    return EVENT_BUS_OK;
}

int event_bus_process(void)
{
    if (!g_initialized) {
        return EVENT_BUS_ERROR;
    }
    
    int events_processed = 0;
    event_t event;
    
    // Process all events in queue
    while (!event_queue_is_empty()) {
        if (event_queue_dequeue(&event) == EVENT_BUS_OK) {
            event_process_single_event(&event);
            events_processed++;
            g_stats.events_processed++;
        } else {
            break;
        }
    }
    
    return events_processed;
}

event_bus_result_t event_bus_publish(event_type_t type, const void* data, 
                                    uint16_t data_size, event_priority_t priority,
                                    void* sender)
{
    if (!g_initialized) {
        return EVENT_BUS_ERROR;
    }
    
    if (data_size > EVENT_BUS_MAX_EVENT_DATA_SIZE) {
        return EVENT_BUS_ERROR_INVALID_PARAM;
    }
    
    // Create event
    event_t event = {0};
    event.type = type;
    event.priority = priority;
    event.timestamp = event_get_system_time_ms();
    event.data_size = data_size;
    event.sender = sender;
    
    // Copy data if provided
    if (data != NULL && data_size > 0) {
        memcpy(event.data, data, data_size);
    }
    
    // Enqueue event
    event_bus_result_t result = event_queue_enqueue(&event);
    if (result == EVENT_BUS_OK) {
        g_stats.events_published++;
        
        // Sort queue by priority for better performance
        event_sort_queue_by_priority();
    } else if (result == EVENT_BUS_ERROR_QUEUE_FULL) {
        g_stats.events_dropped++;
    }
    
    return result;
}

int event_bus_subscribe(event_type_t event_type, event_handler_t handler,
                       void* user_data, const char* name)
{
    return event_bus_subscribe_filtered(event_type, handler, NULL, user_data, name);
}

int event_bus_subscribe_filtered(event_type_t event_type, event_handler_t handler,
                                event_filter_t filter, void* user_data, 
                                const char* name)
{
    if (!g_initialized || handler == NULL) {
        return EVENT_BUS_ERROR_INVALID_PARAM;
    }
    
    int slot = event_find_free_subscriber_slot();
    if (slot < 0) {
        return EVENT_BUS_ERROR_MAX_SUBSCRIBERS;
    }
    
    event_subscriber_t* subscriber = &g_subscribers[slot];
    
    subscriber->event_type = event_type;
    subscriber->handler = handler;
    subscriber->filter = filter;
    subscriber->user_data = user_data;
    subscriber->enabled = true;
    
    // Copy subscriber name safely
    if (name != NULL) {
        strncpy(subscriber->name, name, EVENT_SUBSCRIBER_NAME_MAX);
        subscriber->name[EVENT_SUBSCRIBER_NAME_MAX] = '\0';
    } else {
        snprintf(subscriber->name, sizeof(subscriber->name), "Sub_%d", slot);
    }
    
    g_subscriber_count++;
    g_stats.subscribers_count = g_subscriber_count;
    
    return slot;
}

event_bus_result_t event_bus_unsubscribe(int subscriber_id)
{
    if (!g_initialized) {
        return EVENT_BUS_ERROR;
    }
    
    event_subscriber_t* subscriber = event_get_subscriber_by_id(subscriber_id);
    if (subscriber == NULL) {
        return EVENT_BUS_ERROR_INVALID_PARAM;
    }
    
    // Clear subscriber data
    memset(subscriber, 0, sizeof(event_subscriber_t));
    g_subscriber_count--;
    g_stats.subscribers_count = g_subscriber_count;
    
    return EVENT_BUS_OK;
}

event_bus_result_t event_bus_set_subscription_enabled(int subscriber_id, bool enabled)
{
    if (!g_initialized) {
        return EVENT_BUS_ERROR;
    }
    
    event_subscriber_t* subscriber = event_get_subscriber_by_id(subscriber_id);
    if (subscriber == NULL) {
        return EVENT_BUS_ERROR_INVALID_PARAM;
    }
    
    subscriber->enabled = enabled;
    return EVENT_BUS_OK;
}

event_bus_result_t event_bus_get_stats(event_bus_stats_t* stats)
{
    if (!g_initialized || stats == NULL) {
        return EVENT_BUS_ERROR_INVALID_PARAM;
    }
    
    g_stats.queue_size = g_event_queue.count;
    memcpy(stats, &g_stats, sizeof(event_bus_stats_t));
    
    return EVENT_BUS_OK;
}

event_bus_result_t event_bus_clear_stats(void)
{
    if (!g_initialized) {
        return EVENT_BUS_ERROR;
    }
    
    uint32_t subscribers_count = g_stats.subscribers_count;
    uint32_t queue_max_size = g_stats.queue_max_size;
    
    memset(&g_stats, 0, sizeof(event_bus_stats_t));
    
    // Preserve some stats that shouldn't be cleared
    g_stats.subscribers_count = subscribers_count;
    g_stats.queue_max_size = queue_max_size;
    g_stats.queue_size = g_event_queue.count;
    
    return EVENT_BUS_OK;
}

bool event_bus_is_queue_full(void)
{
    if (!g_initialized) {
        return false;
    }
    
    return event_queue_is_full();
}

uint32_t event_bus_get_queue_size(void)
{
    if (!g_initialized) {
        return 0;
    }
    
    return g_event_queue.count;
}

int event_bus_flush_queue(void)
{
    if (!g_initialized) {
        return EVENT_BUS_ERROR;
    }
    
    int flushed_count = g_event_queue.count;
    
    g_event_queue.head = 0;
    g_event_queue.tail = 0;
    g_event_queue.count = 0;
    
    return flushed_count;
}

/* ========== PRIVATE FUNCTION IMPLEMENTATIONS ========== */

static bool event_queue_is_empty(void)
{
    return (g_event_queue.count == 0);
}

static bool event_queue_is_full(void)
{
    return (g_event_queue.count >= EVENT_QUEUE_SIZE);
}

static event_bus_result_t event_queue_enqueue(const event_t* event)
{
    if (event_queue_is_full()) {
        return EVENT_BUS_ERROR_QUEUE_FULL;
    }
    
    // Add event to queue
    memcpy(&g_event_queue.events[g_event_queue.tail], event, sizeof(event_t));
    
    g_event_queue.tail = (g_event_queue.tail + 1) % EVENT_QUEUE_SIZE;
    g_event_queue.count++;
    
    // Update max count statistics
    if (g_event_queue.count > g_event_queue.max_count) {
        g_event_queue.max_count = g_event_queue.count;
        g_stats.queue_max_size = g_event_queue.max_count;
    }
    
    return EVENT_BUS_OK;
}

static event_bus_result_t event_queue_dequeue(event_t* event)
{
    if (event_queue_is_empty()) {
        return EVENT_BUS_ERROR;
    }
    
    // Get event from queue
    memcpy(event, &g_event_queue.events[g_event_queue.head], sizeof(event_t));
    
    g_event_queue.head = (g_event_queue.head + 1) % EVENT_QUEUE_SIZE;
    g_event_queue.count--;
    
    return EVENT_BUS_OK;
}

static uint32_t event_get_system_time_ms(void)
{
    // Platform-specific implementation to get system time in milliseconds
    // This should match the implementation in app_main.c
    
    // Placeholder implementation - replace with actual platform code
    static uint32_t mock_time = 0;
    mock_time += 10; // Simulate 10ms increment
    return mock_time;
}

static int event_find_free_subscriber_slot(void)
{
    for (int i = 0; i < EVENT_BUS_MAX_SUBSCRIBERS; i++) {
        if (g_subscribers[i].handler == NULL) {
            return i;
        }
    }
    return -1; // No free slot found
}

static event_subscriber_t* event_get_subscriber_by_id(int subscriber_id)
{
    if (subscriber_id < 0 || subscriber_id >= EVENT_BUS_MAX_SUBSCRIBERS) {
        return NULL;
    }
    
    if (g_subscribers[subscriber_id].handler == NULL) {
        return NULL; // Subscriber slot is empty
    }
    
    return &g_subscribers[subscriber_id];
}

static void event_process_single_event(const event_t* event)
{
    // Find all subscribers for this event type
    for (int i = 0; i < EVENT_BUS_MAX_SUBSCRIBERS; i++) {
        event_subscriber_t* subscriber = &g_subscribers[i];
        
        if (subscriber->handler != NULL && subscriber->enabled &&
            subscriber->event_type == event->type) {
            
            event_deliver_to_subscriber(event, subscriber);
        }
    }
}

static void event_deliver_to_subscriber(const event_t* event, event_subscriber_t* subscriber)
{
    // Apply filter if present
    if (subscriber->filter != NULL) {
        if (!subscriber->filter(event, subscriber->user_data)) {
            return; // Event filtered out
        }
    }
    
    // Call the event handler
    subscriber->handler(event, subscriber->user_data);
}

static int event_compare_priority(const event_t* a, const event_t* b)
{
    // Lower priority value = higher priority
    if (a->priority < b->priority) return -1;
    if (a->priority > b->priority) return 1;
    
    // If same priority, older events have priority
    if (a->timestamp < b->timestamp) return -1;
    if (a->timestamp > b->timestamp) return 1;
    
    return 0;
}

static void event_sort_queue_by_priority(void)
{
    // Simple bubble sort for small queue sizes
    // For larger queues, consider using a priority queue data structure
    
    if (g_event_queue.count <= 1) {
        return; // Nothing to sort
    }
    
    // Create temporary array for sorting
    event_t temp_events[EVENT_QUEUE_SIZE];
    int temp_count = 0;
    
    // Copy events to temporary array
    uint16_t index = g_event_queue.head;
    for (int i = 0; i < g_event_queue.count; i++) {
        temp_events[temp_count++] = g_event_queue.events[index];
        index = (index + 1) % EVENT_QUEUE_SIZE;
    }
    
    // Sort temporary array (bubble sort)
    for (int i = 0; i < temp_count - 1; i++) {
        for (int j = 0; j < temp_count - i - 1; j++) {
            if (event_compare_priority(&temp_events[j], &temp_events[j + 1]) > 0) {
                // Swap events
                event_t temp = temp_events[j];
                temp_events[j] = temp_events[j + 1];
                temp_events[j + 1] = temp;
            }
        }
    }
    
    // Copy sorted events back to queue
    g_event_queue.head = 0;
    g_event_queue.tail = temp_count % EVENT_QUEUE_SIZE;
    
    for (int i = 0; i < temp_count; i++) {
        g_event_queue.events[i] = temp_events[i];
    }
}

/* ========== DEBUG FUNCTIONS ========== */

#ifdef DEBUG

void event_bus_debug_print(void)
{
    printf("=== EVENT BUS DEBUG INFO ===\n");
    printf("Initialized: %s\n", g_initialized ? "Yes" : "No");
    printf("Queue: %d/%d events\n", g_event_queue.count, EVENT_QUEUE_SIZE);
    printf("Subscribers: %d/%d\n", g_subscriber_count, EVENT_BUS_MAX_SUBSCRIBERS);
    printf("Stats:\n");
    printf("  Published: %lu\n", g_stats.events_published);
    printf("  Processed: %lu\n", g_stats.events_processed);
    printf("  Dropped: %lu\n", g_stats.events_dropped);
    printf("  Max Queue Size: %lu\n", g_stats.queue_max_size);
    
    printf("Active Subscribers:\n");
    for (int i = 0; i < EVENT_BUS_MAX_SUBSCRIBERS; i++) {
        event_subscriber_t* sub = &g_subscribers[i];
        if (sub->handler != NULL) {
            printf("  [%d] %s: Event 0x%04X, %s\n", 
                   i, sub->name, sub->event_type, 
                   sub->enabled ? "Enabled" : "Disabled");
        }
    }
    printf("============================\n");
}

const char* event_type_to_string(event_type_t type)
{
    switch (type) {
        case EVENT_SYSTEM_INIT: return "SYSTEM_INIT";
        case EVENT_SYSTEM_READY: return "SYSTEM_READY";
        case EVENT_SYSTEM_ERROR: return "SYSTEM_ERROR";
        case EVENT_SYSTEM_SHUTDOWN: return "SYSTEM_SHUTDOWN";
        case EVENT_SYSTEM_MODE_CHANGED: return "SYSTEM_MODE_CHANGED";
        
        case EVENT_BUTTON_PRESSED: return "BUTTON_PRESSED";
        case EVENT_BUTTON_RELEASED: return "BUTTON_RELEASED";
        case EVENT_BUTTON_LONG_PRESS: return "BUTTON_LONG_PRESS";
        case EVENT_BUTTON_DOUBLE_CLICK: return "BUTTON_DOUBLE_CLICK";
        
        case EVENT_UART_DATA_RECEIVED: return "UART_DATA_RECEIVED";
        case EVENT_UART_TX_COMPLETE: return "UART_TX_COMPLETE";
        case EVENT_UART_ERROR: return "UART_ERROR";
        
        case EVENT_LCD_REFRESH: return "LCD_REFRESH";
        case EVENT_LCD_BACKLIGHT_CHANGE: return "LCD_BACKLIGHT_CHANGE";
        case EVENT_LCD_ERROR: return "LCD_ERROR";
        
        case EVENT_TIMER_TICK: return "TIMER_TICK";
        case EVENT_TIMER_EXPIRED: return "TIMER_EXPIRED";
        
        default:
            if (type >= EVENT_USER_DEFINED) {
                return "USER_DEFINED";
            }
            return "UNKNOWN";
    }
}

#endif /* DEBUG */
