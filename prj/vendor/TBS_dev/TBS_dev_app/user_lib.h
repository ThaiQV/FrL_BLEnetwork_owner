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
#define ULOGA(...)	//LOGA(DEFAULT,##__VA_ARGS__)
#else
#define ULOGA(...)
#endif

#include "driver/common/msTick.h"
#include "driver/lcd/lcd16x2.h"
#include "driver/led_7_seg/led_7_seg.h"
#include "driver/tca9555/tca9555.h"
#include "driver/button/button.h"
#include "driver/led/led.h"
#include "user_app/tca9555_app/tca9555_app.h"
#include "user_app/lcd_app/lcd_app.h"
#include "user_app/led_7_seg_app/led_7_seg_app.h"
#include "user_app/button_app/button_app.h"
#include "user_app/led_app/led_app.h"
#include "user_app/data_storage/data_storage.h"
#include "user_app/user_app.h"

#endif /* MASTER_CORE*/
#endif /* APPS_USER_USER_LIB_H_ */

