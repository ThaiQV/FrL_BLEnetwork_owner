/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_sys_datetime.h
 *Created on		: Jul 14, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#ifndef FREELUX_LIBS_FL_SYS_DATETIME_H_
#define FREELUX_LIBS_FL_SYS_DATETIME_H_

#pragma once

typedef struct {
	uint16_t year;   // eg. 2025
	uint8_t year_u8;
	uint8_t month;  // 1-12
	uint8_t day;    // 1-31
	uint8_t hour;   // 0-23
	uint8_t minute; // 0-59
	uint8_t second; // 0-59
} datetime_t;

typedef struct {
	u32 timetamp;
	uint8_t milstep;
}__attribute__((packed))  fl_timetamp_withstep_t;

void fl_rtc_init(void);
void fl_rtc_set(uint32_t timestamp_seconds);
uint32_t fl_rtc_get(void);
fl_timetamp_withstep_t fl_rtc_getWithMilliStep(void);
fl_timetamp_withstep_t fl_rtc_milltampStep2timetamp(u64 _millstamp) ;
u64 fl_rtc_timetamp2milltampStep(fl_timetamp_withstep_t _timetamp_step);
u64 fl_rtc_timetampmillstep_convert(u8 *_array_timetampmill);
void fl_rtc_sync(u32 timetamp_sync);
datetime_t fl_parse_datetime(uint8_t *buf);
uint32_t fl_rtc_datetime_to_timestamp(datetime_t* dt);
void fl_rtc_timestamp_to_datetime(uint32_t timestamp, datetime_t* dt);

#endif /* FREELUX_LIBS_FL_SYS_DATETIME_H_ */
