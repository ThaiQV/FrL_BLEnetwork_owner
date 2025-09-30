/**
 * @file app_manager.h
 * @brief 
 * @author Nghia Hoang
 * @date 2025
 */

#ifndef MASTER_CORE

#include "vendor/TBS_dev/TBS_dev_config.h"

#ifdef COUNTER_DEVICE

#include "include/app_manager.h"
#include "include/event_bus.h"
#include <stdio.h>

#include <stdint.h>

#define APP_MANAGER_DEBUG			0
#if	APP_MANAGER_DEBUG
#define AM_LOG_BT(...)	ULOGA(__VA_ARGS__)
#else
#define AM_LOG_BT(...)
#endif

// Internal SubApp registry
typedef struct {
    subapp_t* subapps[APP_MANAGER_MAX_SUBAPPS];
    uint32_t count;
    app_manager_state_t state;
    app_manager_stats_t stats;
    app_manager_config_t config;
    bool initialized;
    uint32_t start_time;
} app_manager_t;

// Event handler for App Manager
static void app_manager_event_handler(const event_t* event, void* user_data);

// Global app manager instance
static app_manager_t g_app_manager = {0};
static int g_app_manager_subscriber_id = EVENT_BUS_INVALID_ID;

// Private function declarations
static uint32_t get_system_time_ms(void);
static subapp_t* find_subapp_by_name(const char* name);
static void update_stats(void);
static void handle_subapp_error(subapp_t* subapp, subapp_result_t result);

// Public API implementation
app_manager_result_t app_manager_init(void) {
    if (g_app_manager.initialized) {
        AM_LOG_BT("[AppManager] Already initialized\n");
        return APP_MANAGER_OK;
    }
    
    // Initialize internal structures
    memset(&g_app_manager, 0, sizeof(app_manager_t));
    g_app_manager.state = APP_MANAGER_STATE_INITIALIZING;
    g_app_manager.start_time = get_system_time_ms();
    
    // Set default configuration
    g_app_manager.config.loop_delay_ms = 10;
    g_app_manager.config.auto_handle_events = true;
    g_app_manager.config.enable_watchdog = false;
    g_app_manager.config.max_loop_time_ms = 100;
    
    // Initialize Event Bus
    if (event_bus_init() != EVENT_BUS_OK) {
        AM_LOG_BT("[AppManager] Failed to initialize Event Bus\n");
        g_app_manager.state = APP_MANAGER_STATE_ERROR;
        return APP_MANAGER_ERROR;
    }
    
    // Subscribe to system events
    g_app_manager_subscriber_id = event_bus_subscribe(EVENT_SYSTEM_ERROR, 
                                                     app_manager_event_handler, 
                                                     &g_app_manager, 
                                                     "AppManager");
    if (g_app_manager_subscriber_id < 0) {
        AM_LOG_BT("[AppManager] Failed to subscribe to events\n");
        event_bus_deinit();
        g_app_manager.state = APP_MANAGER_STATE_ERROR;
        return APP_MANAGER_ERROR;
    }
    
    g_app_manager.initialized = true;
    g_app_manager.state = APP_MANAGER_STATE_IDLE;
    
    AM_LOG_BT("[AppManager] Initialized successfully (Version: %s)\n", APP_MANAGER_VERSION);
    return APP_MANAGER_OK;
}

void app_manager_deinit(void) {
    if (!g_app_manager.initialized) {
        return;
    }
    
    AM_LOG_BT("[AppManager] Deinitializing...\n");
    
    // Stop all SubApps
    app_manager_stop_all();
    
    // Unregister all SubApps
    app_manager_unregister_all();
    
    // Unsubscribe from events
    if (g_app_manager_subscriber_id != EVENT_BUS_INVALID_ID) {
        event_bus_unsubscribe(g_app_manager_subscriber_id);
        g_app_manager_subscriber_id = EVENT_BUS_INVALID_ID;
    }
    
    // Deinitialize Event Bus
    event_bus_deinit();
    
    g_app_manager.initialized = false;
    g_app_manager.state = APP_MANAGER_STATE_IDLE;
    
    AM_LOG_BT("[AppManager] Deinitialized\n");
}

app_manager_result_t app_manager_register(subapp_t* subapp) {
    if (!g_app_manager.initialized || !subapp) {
        return APP_MANAGER_INVALID_PARAM;
    }
    
    if (g_app_manager.count >= APP_MANAGER_MAX_SUBAPPS) {
        AM_LOG_BT("[AppManager] Cannot register SubApp '%s': registry is full\n", 
               subapp->name ? subapp->name : "unknown");
        return APP_MANAGER_FULL;
    }
    
    // Check if SubApp already exists
    if (find_subapp_by_name(subapp->name)) {
        AM_LOG_BT("[AppManager] SubApp '%s' already registered\n", subapp->name);
        return APP_MANAGER_ALREADY_EXISTS;
    }
    
    // Validate SubApp structure
    if (!subapp->init || !subapp->loop || !subapp->deinit) {
        AM_LOG_BT("[AppManager] Invalid SubApp '%s': missing required callbacks\n", 
               subapp->name ? subapp->name : "unknown");
        return APP_MANAGER_INVALID_PARAM;
    }
    
    // Register SubApp
    g_app_manager.subapps[g_app_manager.count] = subapp;
    subapp->is_registered = true;
    subapp->state = SUBAPP_STATE_IDLE;
    g_app_manager.count++;
    g_app_manager.stats.total_registered++;
    
    AM_LOG_BT("[AppManager] Registered SubApp '%s' (%lu/%d)\n",
           subapp->name, g_app_manager.count, APP_MANAGER_MAX_SUBAPPS);
    
    return APP_MANAGER_OK;
}

app_manager_result_t app_manager_unregister(const char* name) {
    if (!g_app_manager.initialized || !name) {
        return APP_MANAGER_INVALID_PARAM;
    }
    
    for (uint32_t i = 0; i < g_app_manager.count; i++) {
        subapp_t* subapp = g_app_manager.subapps[i];
        
        if (subapp && strcmp(subapp->name, name) == 0) {
            // Stop SubApp if running
            if (subapp->state == SUBAPP_STATE_RUNNING) {
                if (subapp->deinit) {
                    subapp->deinit(subapp);
                }
            }
            
            // Remove from registry by shifting
            for (uint32_t j = i; j < g_app_manager.count - 1; j++) {
                g_app_manager.subapps[j] = g_app_manager.subapps[j + 1];
            }
            
            subapp->is_registered = false;
            subapp->state = SUBAPP_STATE_IDLE;
            g_app_manager.count--;
            
            AM_LOG_BT("[AppManager] Unregistered SubApp '%s'\n", name);
            return APP_MANAGER_OK;
        }
    }
    
    return APP_MANAGER_NOT_FOUND;
}

app_manager_result_t app_manager_unregister_all(void) {
    AM_LOG_BT("[AppManager] Unregistering all SubApps...\n");
    
    while (g_app_manager.count > 0) {
        subapp_t* subapp = g_app_manager.subapps[0];
        app_manager_unregister(subapp->name);
    }
    
    return APP_MANAGER_OK;
}

app_manager_result_t app_manager_start_all(void) {
    if (!g_app_manager.initialized) {
        return APP_MANAGER_INVALID_PARAM;
    }
    
    AM_LOG_BT("[AppManager] Starting all SubApps...\n");
    g_app_manager.state = APP_MANAGER_STATE_INITIALIZING;
    
    uint32_t started = 0;
    
    for (uint32_t i = 0; i < g_app_manager.count; i++) {
        subapp_t* subapp = g_app_manager.subapps[i];
        
        if (subapp && subapp->init) {
            AM_LOG_BT("[AppManager] Initializing SubApp '%s'...\n", subapp->name);
            
            subapp_result_t result = subapp->init(subapp);
            g_app_manager.stats.total_init_calls++;
            
            if (result == SUBAPP_OK) {
                subapp->state = SUBAPP_STATE_RUNNING;
                started++;
                AM_LOG_BT("[AppManager] SubApp '%s' started successfully\n", subapp->name);
            } else {
                subapp->state = SUBAPP_STATE_ERROR;
                g_app_manager.stats.total_errors++;
                AM_LOG_BT("[AppManager] Failed to start SubApp '%s' (error: %d)\n", 
                       subapp->name, result);
            }
        }
    }
    
    g_app_manager.stats.active_subapps = started;
    g_app_manager.state = APP_MANAGER_STATE_RUNNING;
    
    // Send system start event
    EVENT_PUBLISH_SIMPLE(EVENT_SYSTEM_START, EVENT_PRIORITY_NORMAL);
    
    AM_LOG_BT("[AppManager] Started %lu/%lu SubApps successfully\n", started, g_app_manager.count);
    return APP_MANAGER_OK;
}

app_manager_result_t app_manager_loop(void) {
    if (!g_app_manager.initialized || g_app_manager.state != APP_MANAGER_STATE_RUNNING) {
        return APP_MANAGER_INVALID_PARAM;
    }
    
    // Handle events first if enabled
    if (g_app_manager.config.auto_handle_events) {
        event_bus_process();
    }
    
    // Run loop for each active SubApp
    for (uint32_t i = 0; i < g_app_manager.count; i++) {
        subapp_t* subapp = g_app_manager.subapps[i];
        
        if (subapp && subapp->state == SUBAPP_STATE_RUNNING && subapp->loop) {
            uint32_t start_time = get_system_time_ms();
            
            subapp_result_t result = subapp->loop(subapp);
            g_app_manager.stats.total_loop_calls++;
            
            uint32_t loop_time = get_system_time_ms() - start_time;
            
            // Check for long-running loops
            if (loop_time > g_app_manager.config.max_loop_time_ms) {
                AM_LOG_BT("[AppManager] Warning: SubApp '%s' loop took %lu ms (max: %lu ms)\n",
                       subapp->name, loop_time, g_app_manager.config.max_loop_time_ms);
            }
            
            // Handle SubApp result
            if (result == SUBAPP_FINISHED) {
                AM_LOG_BT("[AppManager] SubApp '%s' finished\n", subapp->name);
                subapp->state = SUBAPP_STATE_FINISHED;
                g_app_manager.stats.active_subapps--;
            } else if (result != SUBAPP_OK) {
                handle_subapp_error(subapp, result);
            }
        }
    }
    
    update_stats();
    return APP_MANAGER_OK;
}

app_manager_result_t app_manager_pause_all(void) {
    if (!g_app_manager.initialized) {
        return APP_MANAGER_INVALID_PARAM;
    }
    
    AM_LOG_BT("[AppManager] Pausing all SubApps...\n");
    g_app_manager.state = APP_MANAGER_STATE_PAUSING;
    
    for (uint32_t i = 0; i < g_app_manager.count; i++) {
        subapp_t* subapp = g_app_manager.subapps[i];
        
        if (subapp && subapp->state == SUBAPP_STATE_RUNNING) {
            if (subapp->pause) {
                subapp->pause(subapp);
            }
            subapp->state = SUBAPP_STATE_PAUSED;
        }
    }
    
    // Send system pause event
    EVENT_PUBLISH_SIMPLE(EVENT_SYSTEM_PAUSE, EVENT_PRIORITY_HIGH);
    
    return APP_MANAGER_OK;
}

app_manager_result_t app_manager_resume_all(void) {
    if (!g_app_manager.initialized) {
        return APP_MANAGER_INVALID_PARAM;
    }
    
    AM_LOG_BT("[AppManager] Resuming all SubApps...\n");
    
    for (uint32_t i = 0; i < g_app_manager.count; i++) {
        subapp_t* subapp = g_app_manager.subapps[i];
        
        if (subapp && subapp->state == SUBAPP_STATE_PAUSED) {
            if (subapp->resume) {
                subapp->resume(subapp);
            }
            subapp->state = SUBAPP_STATE_RUNNING;
        }
    }
    
    g_app_manager.state = APP_MANAGER_STATE_RUNNING;
    
    // Send system resume event
    EVENT_PUBLISH_SIMPLE(EVENT_SYSTEM_RESUME, EVENT_PRIORITY_HIGH);
    
    return APP_MANAGER_OK;
}

app_manager_result_t app_manager_stop_all(void) {
    if (!g_app_manager.initialized) {
        return APP_MANAGER_INVALID_PARAM;
    }
    
    AM_LOG_BT("[AppManager] Stopping all SubApps...\n");
    g_app_manager.state = APP_MANAGER_STATE_STOPPING;
    
    // Send system stop event
    EVENT_PUBLISH_SIMPLE(EVENT_SYSTEM_STOP, EVENT_PRIORITY_HIGH);
    
    for (uint32_t i = 0; i < g_app_manager.count; i++) {
        subapp_t* subapp = g_app_manager.subapps[i];
        
        if (subapp && (subapp->state == SUBAPP_STATE_RUNNING || 
                      subapp->state == SUBAPP_STATE_PAUSED)) {
            AM_LOG_BT("[AppManager] Stopping SubApp '%s'...\n", subapp->name);
            
            if (subapp->deinit) {
                subapp_result_t result = subapp->deinit(subapp);
                if (result != SUBAPP_OK) {
                    AM_LOG_BT("[AppManager] Warning: SubApp '%s' deinit returned error: %d\n",
                           subapp->name, result);
                }
            }
            
            subapp->state = SUBAPP_STATE_IDLE;
        }
    }
    
    g_app_manager.stats.active_subapps = 0;
    g_app_manager.state = APP_MANAGER_STATE_IDLE;
    
    AM_LOG_BT("[AppManager] All SubApps stopped\n");
    return APP_MANAGER_OK;
}

app_manager_result_t app_manager_start_subapp(const char* name) {
    subapp_t* subapp = find_subapp_by_name(name);
    if (!subapp) {
        return APP_MANAGER_NOT_FOUND;
    }
    
    if (subapp->state != SUBAPP_STATE_IDLE) {
        AM_LOG_BT("[AppManager] SubApp '%s' is not in idle state\n", name);
        return APP_MANAGER_ERROR;
    }
    
    if (subapp->init) {
        subapp_result_t result = subapp->init(subapp);
        g_app_manager.stats.total_init_calls++;
        
        if (result == SUBAPP_OK) {
            subapp->state = SUBAPP_STATE_RUNNING;
            g_app_manager.stats.active_subapps++;
            AM_LOG_BT("[AppManager] Started SubApp '%s'\n", name);
            return APP_MANAGER_OK;
        } else {
            subapp->state = SUBAPP_STATE_ERROR;
            g_app_manager.stats.total_errors++;
            AM_LOG_BT("[AppManager] Failed to start SubApp '%s'\n", name);
            return APP_MANAGER_ERROR;
        }
    }
    
    return APP_MANAGER_ERROR;
}

app_manager_result_t app_manager_pause_subapp(const char* name) {
    subapp_t* subapp = find_subapp_by_name(name);
    if (!subapp) {
        return APP_MANAGER_NOT_FOUND;
    }
    
    if (subapp->state != SUBAPP_STATE_RUNNING) {
        return APP_MANAGER_ERROR;
    }
    
    if (subapp->pause) {
        subapp->pause(subapp);
    }
    subapp->state = SUBAPP_STATE_PAUSED;
    
    AM_LOG_BT("[AppManager] Paused SubApp '%s'\n", name);
    return APP_MANAGER_OK;
}

app_manager_result_t app_manager_resume_subapp(const char* name) {
    subapp_t* subapp = find_subapp_by_name(name);
    if (!subapp) {
        return APP_MANAGER_NOT_FOUND;
    }
    
    if (subapp->state != SUBAPP_STATE_PAUSED) {
        return APP_MANAGER_ERROR;
    }
    
    if (subapp->resume) {
        subapp->resume(subapp);
    }
    subapp->state = SUBAPP_STATE_RUNNING;
    
    AM_LOG_BT("[AppManager] Resumed SubApp '%s'\n", name);
    return APP_MANAGER_OK;
}

app_manager_result_t app_manager_stop_subapp(const char* name) {
    subapp_t* subapp = find_subapp_by_name(name);
    if (!subapp) {
        return APP_MANAGER_NOT_FOUND;
    }
    
    if (subapp->state == SUBAPP_STATE_RUNNING || subapp->state == SUBAPP_STATE_PAUSED) {
        if (subapp->deinit) {
            subapp->deinit(subapp);
        }
        subapp->state = SUBAPP_STATE_IDLE;
        g_app_manager.stats.active_subapps--;
        
        AM_LOG_BT("[AppManager] Stopped SubApp '%s'\n", name);
    }
    
    return APP_MANAGER_OK;
}

subapp_t* app_manager_find_subapp(const char* name) {
    return find_subapp_by_name(name);
}

uint32_t app_manager_get_subapp_count(void) {
    return g_app_manager.count;
}

app_manager_state_t app_manager_get_state(void) {
    return g_app_manager.state;
}

bool app_manager_is_running(void) {
    return g_app_manager.state == APP_MANAGER_STATE_RUNNING;
}

void app_manager_get_stats(app_manager_stats_t* stats) {
    if (stats) {
        memcpy(stats, &g_app_manager.stats, sizeof(app_manager_stats_t));
    }
}

void app_manager_reset_stats(void) {
    memset(&g_app_manager.stats, 0, sizeof(app_manager_stats_t));
    g_app_manager.start_time = get_system_time_ms();
}

void app_manager_print_subapps(void) {
#if APP_MANAGER_DEBUG
    AM_LOG_BT("[AppManager] Registered SubApps (%lu/%d):\n",
           g_app_manager.count, APP_MANAGER_MAX_SUBAPPS);
    
    for (uint32_t i = 0; i < g_app_manager.count; i++) {
        subapp_t* subapp = g_app_manager.subapps[i];

        const char* state_str = "Unknown";
        
        switch (subapp->state) {
            case SUBAPP_STATE_IDLE: state_str = "Idle"; break;
            case SUBAPP_STATE_RUNNING: state_str = "Running"; break;
            case SUBAPP_STATE_PAUSED: state_str = "Paused"; break;
            case SUBAPP_STATE_ERROR: state_str = "Error"; break;
            case SUBAPP_STATE_FINISHED: state_str = "Finished"; break;
        }
        
        AM_LOG_BT("  [%lu] %s - State: %s, Events: 0x%08lx\n",
               i, subapp->name, state_str, subapp->event_mask);
    }
#endif
}

const char* app_manager_get_version(void) {
    return APP_MANAGER_VERSION;
}

app_manager_result_t app_manager_configure(const app_manager_config_t* config) {
    if (!config) {
        return APP_MANAGER_INVALID_PARAM;
    }
    
    memcpy(&g_app_manager.config, config, sizeof(app_manager_config_t));
    
    AM_LOG_BT("[AppManager] Configuration updated:\n");
    AM_LOG_BT("  Loop delay: %lu ms\n", config->loop_delay_ms);
    AM_LOG_BT("  Auto handle events: %s\n", config->auto_handle_events ? "Yes" : "No");
    AM_LOG_BT("  Enable watchdog: %s\n", config->enable_watchdog ? "Yes" : "No");
    AM_LOG_BT("  Max loop time: %lu ms\n", config->max_loop_time_ms);
    
    return APP_MANAGER_OK;
}

app_manager_result_t app_manager_run(void) {
    if (!g_app_manager.initialized) {
        return APP_MANAGER_INVALID_PARAM;
    }
    
    AM_LOG_BT("[AppManager] Starting main run loop...\n");
    
    // Start all SubApps
    app_manager_result_t result = app_manager_start_all();
    if (result != APP_MANAGER_OK) {
        AM_LOG_BT("[AppManager] Failed to start SubApps\n");
        return result;
    }
    
    // Main loop
    while (g_app_manager.state == APP_MANAGER_STATE_RUNNING && 
           g_app_manager.stats.active_subapps > 0) {
        
        result = app_manager_loop();
        if (result != APP_MANAGER_OK) {
            AM_LOG_BT("[AppManager] Loop error: %d\n", result);
            break;
        }
        
        // Delay between iterations
        if (g_app_manager.config.loop_delay_ms > 0) {
            delay_ms(g_app_manager.config.loop_delay_ms);
        }
    }
    
    AM_LOG_BT("[AppManager] Main run loop finished\n");
    app_manager_stop_all();
    
    return APP_MANAGER_OK;
}

// Private functions implementation
static uint32_t get_system_time_ms(void) {
    // Simplified timestamp - in real system would use actual timer
    static uint32_t counter = 0;
    return counter += 10;  // Simulate 10ms increments
}

static subapp_t* find_subapp_by_name(const char* name) {
    if (!name) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < g_app_manager.count; i++) {
        subapp_t* subapp = g_app_manager.subapps[i];
        if (subapp && strcmp(subapp->name, name) == 0) {
            return subapp;
        }
    }
    
    return NULL;
}

static void update_stats(void) {
    g_app_manager.stats.uptime_ms = get_system_time_ms() - g_app_manager.start_time;
}

static void handle_subapp_error(subapp_t* subapp, subapp_result_t result) {
    AM_LOG_BT("[AppManager] SubApp '%s' error: %d\n", subapp->name, result);
    subapp->state = SUBAPP_STATE_ERROR;
    g_app_manager.stats.total_errors++;
    g_app_manager.stats.active_subapps--;
}


// Event handler for App Manager
static void app_manager_event_handler(const event_t* event, void* user_data) {
    app_manager_t* app_mgr = (app_manager_t*)user_data;
    
    AM_LOG_BT("[AppManager] Received event: 0x%04lx\n", (uint32_t)event->type);
    
    switch (event->type) {
        case EVENT_SYSTEM_ERROR:
            AM_LOG_BT("[AppManager] System error detected!\n");
            app_mgr->stats.total_errors++;
            
            // Handle system error
            if (event->data_size > 0 && event->data_size <= sizeof(app_mgr->stats)) {
                AM_LOG_BT("[AppManager] Error data received\n");
            }
            break;
            
        default:
            AM_LOG_BT("[AppManager] Unhandled event: 0x%04lx\n", (uint32_t)event->type);
            break;
    }
}

#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/
