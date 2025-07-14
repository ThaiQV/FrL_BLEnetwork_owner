/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_sys_datetime.c
 *Created on		: Jul 14, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#include "tl_common.h"
#include "fl_sys_datetime.h"

/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
#define RTC_STORAGE_ADDR  	0xFF100
#define RTC_SYNC_SPREAD		2 // 10s : diff real-timetamp with curr-timetamp
static const uint8_t days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

_attribute_data_retention_ u32 RTC_OFFSET_TIME = 1752473460; // 14/07/2025-11:31:00

/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/

/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/
static uint8_t is_leap_year(uint16_t year) {
	return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/

/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/
void fl_rtc_init(void) {
	u32 current_timetamp = 0;
	clock_32k_init(CLK_32K_RC);  //
	clock_cal_32k_rc();

	flash_read_page(RTC_STORAGE_ADDR,4,(u8*) &current_timetamp);

	datetime_t cur_dt;
	cur_dt.year = 0;
	cur_dt.month = 0;
	cur_dt.day = 0;
	cur_dt.hour = 0;
	cur_dt.minute = 0;
	cur_dt.second = 00;

	fl_rtc_timestamp_to_datetime(current_timetamp,&cur_dt);

	if (cur_dt.year < 2025) {
		current_timetamp = RTC_OFFSET_TIME;
		fl_rtc_set(current_timetamp); // store origin time
		fl_rtc_timestamp_to_datetime(current_timetamp,&cur_dt);
	}

	RTC_OFFSET_TIME = current_timetamp;
	LOGA(APP,"SYSTIME:%d\r\n",RTC_OFFSET_TIME);
	LOGA(APP,"SYSTIME:%02d/%02d/%02d - %02d:%02d:%02d\r\n",cur_dt.year,cur_dt.month,cur_dt.day,cur_dt.hour,cur_dt.minute,cur_dt.second);
}

void fl_rtc_set(uint32_t timestamp_tick) {
	u8 u32_arr[4] = { U32_BYTE0(timestamp_tick), U32_BYTE1(timestamp_tick), U32_BYTE2(timestamp_tick), U32_BYTE3(timestamp_tick) };
	flash_write_page(RTC_STORAGE_ADDR,4,u32_arr);
//	flash_read_page(RTC_STORAGE_ADDR,4,(u8*) &RTC_OFFSET_TIME);
}

uint32_t fl_rtc_get(void) {
	uint32_t tick = RTC_OFFSET_TIME + clock_get_32k_tick()/32000;
	return tick;
}

void fl_rtc_sync(u32 timetamp_sync){

	int32_t time_spread =  (fl_rtc_get() - timetamp_sync)/32000;
	if ((u32)abs(time_spread) > RTC_SYNC_SPREAD) {
		datetime_t cur_dt;
		fl_rtc_timestamp_to_datetime(timetamp_sync,&cur_dt);
		LOGA(APP,"MASTERTIME:%02d/%02d/%02d - %02d:%02d:%02d\r\n",cur_dt.year,cur_dt.month,cur_dt.day,cur_dt.hour,cur_dt.minute,cur_dt.second);
		ERR(FLA,"Synchronize system time (spread:%d)!!!\r\n",time_spread);
		RTC_OFFSET_TIME = timetamp_sync - clock_get_32k_tick()/32000;
		fl_rtc_set(RTC_OFFSET_TIME);
	}
}

uint32_t fl_rtc_datetime_to_timestamp(datetime_t* dt) {
	uint32_t timestamp = 0;

	for (uint16_t y = 1970; y < dt->year; y++) {
		timestamp += is_leap_year(y) ? 366 * 86400 : 365 * 86400;
	}

	for (uint8_t m = 1; m < dt->month; m++) {
		timestamp += days_in_month[m - 1] * 86400;
		if (m == 2 && is_leap_year(dt->year))
			timestamp += 86400;
	}

	timestamp += (dt->day - 1) * 86400;
	timestamp += dt->hour * 3600 + dt->minute * 60 + dt->second;

	return timestamp;
}

void fl_rtc_timestamp_to_datetime(uint32_t timestamp, datetime_t* dt) {

//	u32 timestamp = 1752473460 + current_timetamp;

	uint32_t days = timestamp / 86400;
	uint32_t secs = timestamp % 86400;

	dt->hour = secs / 3600;
	dt->minute = (secs % 3600) / 60;
	dt->second = secs % 60;

	dt->year = 1970;
	while (1) {
		uint16_t year_days = is_leap_year(dt->year) ? 366 : 365;
		if (days >= year_days)
			days -= year_days, dt->year++;
		else
			break;
	}

	dt->month = 1;
	while (1) {
		uint8_t mdays = days_in_month[dt->month - 1];
		if (dt->month == 2 && is_leap_year(dt->year))
			mdays++;
		if (days >= mdays)
			days -= mdays, dt->month++;
		else
			break;
	}

	dt->day = days + 1;
}
