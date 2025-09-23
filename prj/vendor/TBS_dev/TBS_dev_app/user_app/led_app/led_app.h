/**
 * @file led_app.h
 * @author Nghia Hoang
 * @date 2025
 */
#ifndef LED_APP_H
#define LED_APP_H

#ifndef MASTER_CORE
#ifdef COUNTER_DEVICE

//#include <stdint.h>
//#include <stdbool.h>
#define LED_NUMBER  2

typedef enum {
    LEDID_MODE_OFF,
    LEDID_MODE_ON,
    LEDID_MODE_BLINK3,
} ledid_mode_t;

// SubApp context structure
typedef struct {
	ledid_mode_t leds[LED_NUMBER];
    bool is_call;
    bool start;
} led_context_t;

typedef struct {
	bool led_nwk_on;
	bool led_call_on;
	bool led_call_blink_3;
} led_shared_data_t;

#define get_led_event() {\
	EVENT_LED_NWK_ONLINE ,\
    EVENT_LED_NWK_OFFLINE,\
    EVENT_LED_CALL_ON,\
    EVENT_LED_NWK_CALL_OFF,\
    EVENT_DATA_RESET,\
    EVENT_DATA_CALL,\
    EVENT_DATA_ENDCALL,\
    EVENT_DATA_START_DONE,\
}

#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/
#endif /* LED_7_SEG_APP_H*/
