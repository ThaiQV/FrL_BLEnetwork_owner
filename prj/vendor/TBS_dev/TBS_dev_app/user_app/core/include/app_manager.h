/**
 * @file app_manager.h
 * @brief 
 * @author Nghia Hoang
 * @date 2025
 */

#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#ifndef MASTER_CORE
#ifdef COUNTER_DEVICE

#include "sub_app.h"
#include "event_bus.h"
#include <stdint.h>
#include <stdbool.h>

// App Manager configuration
#define APP_MANAGER_MAX_SUBAPPS 8
#define APP_MANAGER_VERSION "1.0.0"

// App Manager states
typedef enum {
    APP_MANAGER_STATE_IDLE = 0,
    APP_MANAGER_STATE_INITIALIZING,
    APP_MANAGER_STATE_RUNNING,
    APP_MANAGER_STATE_PAUSING,
    APP_MANAGER_STATE_STOPPING,
    APP_MANAGER_STATE_ERROR
} app_manager_state_t;

// App Manager result codes
typedef enum {
    APP_MANAGER_OK = 0,
    APP_MANAGER_ERROR = -1,
    APP_MANAGER_FULL = -2,
    APP_MANAGER_NOT_FOUND = -3,
    APP_MANAGER_ALREADY_EXISTS = -4,
    APP_MANAGER_INVALID_PARAM = -5
} app_manager_result_t;

// App Manager statistics
typedef struct {
    uint32_t total_registered;
    uint32_t active_subapps;
    uint32_t total_init_calls;
    uint32_t total_loop_calls;
    uint32_t total_errors;
    uint32_t uptime_ms;
} app_manager_stats_t;

// Main App Manager API
app_manager_result_t app_manager_init(void);
void app_manager_deinit(void);

// SubApp management
app_manager_result_t app_manager_register(subapp_t* subapp);
app_manager_result_t app_manager_unregister(const char* name);
app_manager_result_t app_manager_unregister_all(void);

// Lifecycle control
app_manager_result_t app_manager_start_all(void);
app_manager_result_t app_manager_loop(void);
app_manager_result_t app_manager_pause_all(void);
app_manager_result_t app_manager_resume_all(void);
app_manager_result_t app_manager_stop_all(void);

// Individual SubApp control
app_manager_result_t app_manager_start_subapp(const char* name);
app_manager_result_t app_manager_pause_subapp(const char* name);
app_manager_result_t app_manager_resume_subapp(const char* name);
app_manager_result_t app_manager_stop_subapp(const char* name);

// Query functions
subapp_t* app_manager_find_subapp(const char* name);
uint32_t app_manager_get_subapp_count(void);
app_manager_state_t app_manager_get_state(void);
bool app_manager_is_running(void);

// Statistics and monitoring
void app_manager_get_stats(app_manager_stats_t* stats);
void app_manager_reset_stats(void);

// Utility functions
void app_manager_print_subapps(void);
const char* app_manager_get_version(void);

// Main application loop helper
typedef struct {
    uint32_t loop_delay_ms;        // Delay between loop iterations
    bool auto_handle_events;       // Automatically handle EventBus events
    bool enable_watchdog;          // Enable SubApp watchdog (future feature)
    uint32_t max_loop_time_ms;     // Maximum allowed loop time per SubApp
} app_manager_config_t;

app_manager_result_t app_manager_configure(const app_manager_config_t* config);
app_manager_result_t app_manager_run(void);  // Main run loop

#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/
#endif // APP_MANAGER_H
