/*
 * user_lib.h
 *
 *      Author: hoang
 */

#ifndef APPS_USER_USER_LIB_H_
#define APPS_USER_USER_LIB_H_

#ifndef MASTER_CORE

#include "tl_common.h"

#define U_APP_DEBUG			0
#if	U_APP_DEBUG
#define ULOGA(...)	printf(__VA_ARGS__)
#else
#define ULOGA(...)
#endif

#include <vendor/TBS_dev/TBS_Counter_app/driver/common/msTick.h>
#include <vendor/TBS_dev/TBS_Counter_app/driver/lcd/lcd16x2.h>
#include <vendor/TBS_dev/TBS_Counter_app/driver/led_7_seg/led_7_seg.h>
#include <vendor/TBS_dev/TBS_Counter_app/driver/tca9555/tca9555.h>
#include <vendor/TBS_dev/TBS_Counter_app/driver/button/button.h>
#include <vendor/TBS_dev/TBS_Counter_app/driver/led/led.h>
#include <vendor/TBS_dev/TBS_Counter_app/user_app/user_app.h>
#include <vendor/TBS_dev/TBS_Counter_app/user_app/core/include/event_bus.h>
#include <vendor/TBS_dev/TBS_Counter_app/user_app/core/include/sub_app.h>
#include <vendor/TBS_dev/TBS_Counter_app/user_app/core/include/app_manager.h>
#include <vendor/TBS_dev/TBS_Counter_app/user_app/tca9555_app/tca9555_app.h>
#include <vendor/TBS_dev/TBS_Counter_app/user_app/lcd_app/lcd_app_drv.h>
#include <vendor/TBS_dev/TBS_Counter_app/user_app/lcd_app/lcd_app.h>
#include <vendor/TBS_dev/TBS_Counter_app/user_app/led_7_seg_app/led_7_seg_app.h>
#include <vendor/TBS_dev/TBS_Counter_app/user_app/button_app/button_app.h>
#include <vendor/TBS_dev/TBS_Counter_app/user_app/led_app/led_app.h>

#endif /* MASTER_CORE*/
#endif /* APPS_USER_USER_LIB_H_ */

