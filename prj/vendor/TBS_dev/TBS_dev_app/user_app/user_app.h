/*
 * user_app.h
 *
 *      Author: Nghia Hoang
 */

#ifndef LCD_H
#define LCD_H

#include <stdint.h>
//#include "button_app/button_app.h"
//#include "lcd_app/lcd_app.h"
//#include "led_7_seg_app/led_7_seg_app.h"

/*  System tick
enum{
	SYSTEM_TIMER_TICK_1US 		= 16,
	SYSTEM_TIMER_TICK_1MS 		= 16000,
	SYSTEM_TIMER_TICK_1S 		= 16000000,

	SYSTEM_TIMER_TICK_625US  	= 10000,  //625*16
	SYSTEM_TIMER_TICK_1250US 	= 20000,  //1250*16
};
*/

#define TIME_LED7SEG_TASK_MS 			(100 * SYSTEM_TIMER_TICK_1MS)  // ms
#define TIME_LCD_TASK_MS 				(100 * SYSTEM_TIMER_TICK_1MS)
#define TIME_BUTTON_TASK_MS				(10 * SYSTEM_TIMER_TICK_1MS)

//typedef struct {
//	led7seg_shared_data_t 	*led7seg;
//	lcd_shared_data_t		*lcd;
//} app_share_data_t;

typedef struct {
	uint32_t timetamp;
	uint8_t bt_call;
	uint8_t bt_endcall;
	uint8_t bt_rst;
	int32_t pass_product;
	int32_t err_product;
	bool is_call;
}app_share_data_t;


void user_app_init(void);
void user_app_loop(void);
void user_app_run(void);

#endif
