/*
 * user_lib.h
 *
 *      Author: hoang
 */

#ifndef APPS_USER_USER_LIB_H_
#define APPS_USER_USER_LIB_H_

#include "tl_common.h"
#include "driver/lcd/lcd16x2.h"
#include "driver/led_7_seg/led_7_seg.h"
#include "driver/tca9555/tca9555.h"
#include "driver/button/button.h"
#include "user_app/tca9555_app/tca9555_app.h"
#include "user_app/lcd_app/lcd_app.h"
#include "user_app/led_7_seg_app/led_7_seg_app.h"
#include "user_app/button_app/button_app.h"
#include "user_app/user_app.h"

#endif /* APPS_USER_USER_LIB_H_ */

inline uint32_t get_system_time_ms(void)
{
	return reg_system_tick/SYSTEM_TIMER_TICK_1MS;
}
