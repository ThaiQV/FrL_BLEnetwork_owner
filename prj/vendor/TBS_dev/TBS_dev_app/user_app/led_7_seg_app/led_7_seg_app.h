/**
 * @file led_7_seg_app.h
 * @author Nghia Hoang
 * @date 2025
 */
#ifndef LED_7_SEG_APP_H
#define LED_7_SEG_APP_H
//
//#include <stdint.h>
//#include <stdbool.h>

#define FREQUENCY_CLK		1000000
#define LED_CLK_PIN			GPIO_PD2
#define LED_DATA_PIN		GPIO_PD3


/**
 * @brief 7-Segment LED sub-app shared data
 */
typedef struct {
	uint32_t value;         ///< Value to display (0-9999)
	uint32_t value_err;
	bool print_err;
	bool enabled;           ///< Display enabled
} led7seg_shared_data_t;

void user_led_7_seg_app_init(void);
void user_led_7_seg_app_task(void);

#endif /* LED_7_SEG_APP_H*/
