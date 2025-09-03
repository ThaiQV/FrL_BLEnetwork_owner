/*
 * mstick.h
 *
 *  Created on: Nov 30, 2023
 *      Author: hoang
 */

#ifndef APPS_USER_MSTICK_H_
#define APPS_USER_MSTICK_H_

/*  System tick
enum{
	SYSTEM_TIMER_TICK_1US 		= 16,
	SYSTEM_TIMER_TICK_1MS 		= 16000,
	SYSTEM_TIMER_TICK_1S 		= 16000000,

	SYSTEM_TIMER_TICK_625US  	= 10000,  //625*16
	SYSTEM_TIMER_TICK_1250US 	= 20000,  //1250*16
};
*/

uint64_t get_system_time_us(void);
uint64_t get_system_time_ms(void);

#endif /* APPS_USER_MSTICK_H_ */
