/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_nwk_protocol.h
 *Created on		: Jul 23, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#ifndef VENDOR_FRL_NETWORK_FL_NWK_PROTOCOL_H_
#define VENDOR_FRL_NETWORK_FL_NWK_PROTOCOL_H_

typedef struct {
	u16 adv_interval_min;
	u16 adv_interval_max;
	u16 adv_duration;
	u16 scan_window;
	u16 scan_interval;
	struct {
		u8 chn1;
		u8 chn2;
		u8 chn3;
	} nwk_chn;
}__attribute__((packed)) fl_adv_settings_t;

//typedef struct {
//	u8 threshold_rssi;
//	u8 level;
//}__attribute__((packed)) fl_repeat_settings_t;
//struct array use to check rsp when get info
typedef struct {
	u8 num_sla;
	u8 id[20];
	u32 timetamp;
}__attribute__((packed)) fl_slave_getinfo_t;

void _Passing_CmdLine(type_debug_t _type, u8 *_data);
#endif /* VENDOR_FRL_NETWORK_FL_NWK_PROTOCOL_H_ */
