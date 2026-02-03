/**
 * @file led_7_seg_app.h
 * @author Nghia Hoang
 * @date 2025
 */
#ifndef LED_7_SEG_APP_H
#define LED_7_SEG_APP_H

#ifndef MASTER_CORE
#ifdef COUNTER_DEVICE

//#include <stdint.h>
//#include <stdbool.h>

#define FREQUENCY_CLK		1000000
#define LED_CLK_PIN			GPIO_PD2
#define LED_DATA_PIN		GPIO_PD3


/**
 * @brief 7-Segment LED sub-app shared data
 */

#define get_led7_event() {\
	EVENT_DATA_START_DONE,\
    EVENT_LED7_PRINT_ERR,\
	EVENT_DATA_ERR_PRODUCT_CHANGE,\
    EVENT_DATA_PASS_PRODUCT_UP,\
    EVENT_DATA_PASS_PRODUCT_DOWN,\
	EVENT_DATA_RESET,\
}

#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/
#endif /* LED_7_SEG_APP_H*/
