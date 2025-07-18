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
#include "vendor/FrL_Network/fl_nwk_database.h"


/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/

#define RTC_SYNC_SPREAD		2 // 2s : diff real-timetamp with curr-timetamp
static const uint8_t days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

_attribute_data_retention_ u32 RTC_OFFSET_TIME = 1752473460; // 14/07/2025-11:31:00

/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
datetime_t fl_parse_datetime(uint8_t *buf) {

	datetime_t dt;

    if (!buf ) return dt;

    char temp[5];  //

    // Year: buf[0]–[3]
    memcpy(temp, &buf[0], 2); temp[2] = '\0';
    dt.year = (uint16_t)atoi(temp);

    // Month: buf[5]–[6]
    memcpy(temp, &buf[2], 2); temp[2] = '\0';
    dt.month = (uint8_t)atoi(temp);

    // Day: buf[8]–[9]
    memcpy(temp, &buf[4], 2); temp[2] = '\0';
    dt.day = (uint8_t)atoi(temp);

    // Hour: buf[11]–[12]
    memcpy(temp, &buf[6], 2); temp[2] = '\0';
    dt.hour = (uint8_t)atoi(temp);

    // Minute: buf[14]–[15]
    memcpy(temp, &buf[8], 2); temp[2] = '\0';
    dt.minute = (uint8_t)atoi(temp);

    // Second: buf[17]–[18]
    memcpy(temp, &buf[10], 2); temp[2] = '\0';
    dt.second = (uint8_t)atoi(temp);

    return dt;  //
}
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
	clock_32k_init(CLK_32K_RC);  //
	clock_cal_32k_rc();
	fl_db_rtc_init();
	RTC_OFFSET_TIME = fl_db_rtc_load();
	datetime_t cur_dt;
	cur_dt.year = 0;
	cur_dt.month = 0;
	cur_dt.day = 0;
	cur_dt.hour = 0;
	cur_dt.minute = 0;
	cur_dt.second = 00;
	fl_rtc_timestamp_to_datetime(RTC_OFFSET_TIME,&cur_dt);
//	LOGA(APP,"SYSTIME:%d\r\n",RTC_OFFSET_TIME);
	LOGA(APP,"SYSTIME:%02d/%02d/%02d - %02d:%02d:%02d\r\n",cur_dt.year,cur_dt.month,cur_dt.day,cur_dt.hour,cur_dt.minute,cur_dt.second);
}

void fl_rtc_set(uint32_t timestamp_tick) {
	if(timestamp_tick == 0){
		timestamp_tick = fl_rtc_get();
	}
	fl_db_rtc_save(timestamp_tick);
}

uint32_t fl_rtc_get(void) {
	uint32_t tick = RTC_OFFSET_TIME + clock_get_32k_tick()/32000;
	return tick;
}

void fl_rtc_sync(u32 timetamp_sync){
	int time_spread = timetamp_sync - fl_rtc_get();
	if (abs(time_spread) > (int)RTC_SYNC_SPREAD ) {
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
