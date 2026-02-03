#ifndef SUBAPP_H
#define SUBAPP_H

#ifndef MASTER_CORE
#ifdef COUNTER_DEVICE

#include <stdint.h>
#include <stdbool.h>
#include <vendor/TBS_dev/TBS_Counter_app/user_app/core/include/event_bus.h>

// Forward declaration

// SubApp return codes
typedef enum {
    SUBAPP_OK = 0,
    SUBAPP_ERROR = -1,
    SUBAPP_FINISHED = 1
} subapp_result_t;

// SubApp states
typedef enum {
    SUBAPP_STATE_IDLE = 0,
    SUBAPP_STATE_RUNNING,
    SUBAPP_STATE_PAUSED,
    SUBAPP_STATE_ERROR,
    SUBAPP_STATE_FINISHED
} subapp_state_t;

// SubApp structure
typedef struct subapp_t {
    const char* name;              // SubApp name
    void* context;                 // Private context for this SubApp
    subapp_state_t state;          // Current state
    
    // Lifecycle callbacks - mandatory
    subapp_result_t (*init)(struct subapp_t* self);
    subapp_result_t (*loop)(struct subapp_t* self);
    subapp_result_t (*deinit)(struct subapp_t* self);
    
    // Event handling callback - optional
    subapp_result_t (*on_event)(struct subapp_t* self, const event_t* event);
    
    // Utility callbacks - optional
    subapp_result_t (*pause)(struct subapp_t* self);
    subapp_result_t (*resume)(struct subapp_t* self);
    
    // Internal data
    bool is_registered;
    uint32_t event_mask;          // Bitmask for events this SubApp is interested in
} subapp_t;


// Utility functions for SubApp developers
bool subapp_is_running(const subapp_t* subapp);
bool subapp_is_paused(const subapp_t* subapp);
bool subapp_is_finished(const subapp_t* subapp);
void subapp_set_context(subapp_t* subapp, void* context);
void* subapp_get_context(const subapp_t* subapp);

#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/
#endif // SUBAPP_H
