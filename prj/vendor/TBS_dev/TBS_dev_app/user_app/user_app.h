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

typedef struct {
	lcd_shared_data_t lcd_data;
	led7seg_shared_data_t led7seg_data;
} app_share_data_t;

void user_app_init(void);
void user_app_loop(void);
void user_app_run(void);

#endif
