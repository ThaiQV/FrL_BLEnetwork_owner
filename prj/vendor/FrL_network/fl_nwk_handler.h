/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_nwk_handler.h
 *Created on		: Jul 11, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#ifndef VENDOR_FRL_NETWORK_FL_NWK_HANDLER_H_
#define VENDOR_FRL_NETWORK_FL_NWK_HANDLER_H_

#include "fl_nwk_database.h"

#define RAND(min, max)				((rand() % ((max) - (min) + 1)) + (min))
#define RAND_INT(min, max)  		((rand() % ((min) + (max) + 1)) - (min))
#define SIZEU8(x)					(sizeof(x)/sizeof(u8))

typedef enum {
	NWK_HDR_NONE = 0,
	NWK_HDR_55 = 0x55,
	//
	NWK_HDR_F5_INFO = 0xF5,
	NWK_HDR_ASSIGN = 0xFC,//Use to assign SlaveID to slave
	NWK_HDR_HEARTBEAT = 0xFD,
	NWK_HDR_COLLECT = 0xFE, //Use to collect slave (master and slaves)
}__attribute__((packed)) fl_hdr_nwk_type_e;

typedef union {
	u8 id_u8;
	struct {
		u8 memID :3; //3 bits : 0-8 => total 8*32 = 256 slaves
		u8 grpID :5; //5bits : 0-32
	};
}fl_slaveID_u;

typedef struct {
	u8 hdr;
	u8 timetamp[4];
	u8 milltamp;
	fl_slaveID_u slaveID;
	u8 payload[20]; //modify to special parameter in the packet
	u8 crc8;
	union {
		u8 ep_u8; //
		struct {
			u8 repeat_cnt :2;   // 2 bits
			u8 master :2;  		// 2 bits
			u8 dbg:1;			// 1 bit
			u8 rep_mode:1;
			//u8 millis_step :2;  // 1000 millisecond / 4 = 250
		};
	} endpoint;
// LSB: don't change location byte
	s8 rssi;
}__attribute__((packed)) fl_dataframe_format_t; //Must less than 30 bytes

typedef union {
	fl_dataframe_format_t frame;
	u8 bytes[SIZEU8(fl_dataframe_format_t)];
}__attribute__((packed)) fl_data_frame_u;

typedef struct {
	union {
		u32 mac_u32;
		u8 byte[4];
	} mac_short;
	fl_slaveID_u slaveID;
	u32 timelife;
	bool active;
#ifndef MASTER_CORE
//todo: parameters
	fl_slave_profiles_t profile;
#endif
}__attribute__((packed)) fl_nodeinnetwork_t;

#ifdef MASTER_CORE

#define MAX_NODES 	200
typedef struct {
	u8 slot_inused;
	fl_nodeinnetwork_t sla_info[MAX_NODES];
}__attribute__((packed)) fl_slaves_list_t;

typedef struct {
	struct {
		u8 collect_chn[3];
		u8 chn[3];

	} nwk;
	u32 my_mac;
}__attribute__((packed)) fl_master_config_t;

#endif

/*******************************************************************************
 * @brief 		Crc8
 *
 * @param[in] 	data
 *
 * @return	  	crc output.
 ********************************************************************************/
inline u8 fl_crc8(u8* _pdata, u8 _len) {
	u8 crc = 0;
	for (u8 i = 0; i < _len; i++) {
		crc += _pdata[i];
	}
	return (u8) (crc % 256);
}

#ifdef MASTER_CORE
void fl_nwk_master_init(void);
void fl_nwk_master_run(fl_pack_t *_pack_handle);
void fl_nwk_master_process(void);
fl_pack_t fl_master_packet_GetInfo_build(u8 *_slave_mac_arr, u8 _slave_num);
void fl_master_nodelist_AddRefesh(fl_nodeinnetwork_t _node);
s16 fl_master_SlaveID_find(u8 _id) ;
void fl_nwk_master_nodelist_load(void);
#else
void fl_nwk_slave_init(void);
void fl_nwk_slave_run(fl_pack_t *_pack_handle);
void fl_nwk_slave_process(void);
bool fl_nwk_slave_checkHDR(u8 _hdr);
u32 fl_adv_timetampInPack(fl_pack_t _pack);
fl_timetamp_withstep_t fl_adv_timetampStepInPack(fl_pack_t _pack);
#endif
void fl_adv_setting_update(void);
int fl_adv_sendFIFO_add(fl_pack_t _pack);

#endif /* VENDOR_FRL_NETWORK_FL_NWK_HANDLER_H_ */
