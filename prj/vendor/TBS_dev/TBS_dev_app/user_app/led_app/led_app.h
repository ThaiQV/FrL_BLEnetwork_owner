/**
 * @file led_app.h
 * @author Nghia Hoang
 * @date 2025
 */
#ifndef LED_APP_H
#define LED_APP_H
//
//#include <stdint.h>
//#include <stdbool.h>


typedef struct {
	bool led_nwk_on;
	bool led_call_on;
	bool led_call_blink_3;
} led_shared_data_t;

void user_led_app_init(void);
void user_led_app_task(void);

#endif /* LED_7_SEG_APP_H*/
