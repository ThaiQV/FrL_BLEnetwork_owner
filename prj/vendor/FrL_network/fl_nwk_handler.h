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
#include "../TBS_dev/TBS_dev_config.h"
/**
 * @brief	callback function for rsp
 */
typedef void (*fl_rsp_callback_fnc)(void*,void*);
#define QUEUE_RSP_SLOT_MAX		10
#define QUEUQ_REQcRSP_INTERVAL  20*1000 //ms

#define RAND(min, max)				((rand() % ((max) - (min) + 1)) + (min))
#define RAND_INT(min, max)  		((rand() % ((min) + (max) + 1)) - (min))
#define SIZEU8(x)					(sizeof(x)/sizeof(u8))

#define MAC_ZERO_CLEAR(mac,x)		(memset(mac,x,sizeof(mac)/sizeof(mac[0])))
#define IS_MAC_INVALID(mac,x) 		(((mac[0] == x) && (mac[1] == x) && (mac[2] == x) && (mac[3] == x) && (mac[4] == x) && (mac[5] == x)))
#define MAC_MATCH(mac1, mac2) 		(memcmp(mac1, mac2, 6) == 0)
#define MAC_COPY(mac,data)			(memcpy(mac,data,6))

typedef enum {
	NWK_HDR_NONE = 0,
	// slave -> req -> master -> rsp
	NWK_HDR_RECONNECT = 0x11,
	/*Frl protocols*/
	NWK_HDR_55 = 0x55, //
	// master -> req -> slave -> rsp
	NWK_HDR_F5_INFO = 0xF5,
	NWK_HDR_ASSIGN = 0xFC,	//Use to assign SlaveID to slave
	NWK_HDR_HEARTBEAT = 0xFD,
	NWK_HDR_COLLECT = 0xFE, //Use to collect slave (master and slaves)
}__attribute__((packed)) fl_hdr_nwk_type_e;

typedef union {
	u8 id_u8;
	struct {
		u8 memID :3; //3 bits : 0-8 => total 8*32 = 256 slaves
		u8 grpID :5; //5bits : 0-32
	};
} fl_slaveID_u;

typedef struct {
	u8 hdr;
	u8 timetamp[4];
	u8 milltamp;
	fl_slaveID_u slaveID;
	u8 payload[22]; //modify to special parameter in the packet
	u8 crc8;
	union {
		u8 ep_u8; //
		struct {
			u8 repeat_cnt :2;   // 2 bits
			u8 master :2;  		// 2 bits
			u8 dbg :1;			// 1 bit
			u8 rep_mode :2;		//for setting slave
		};
	} endpoint;
// LSB: don't change location byte
	s8 rssi;
}__attribute__((packed)) fl_dataframe_format_t; //Must less than 31 bytes

typedef union {
	fl_dataframe_format_t frame;
	u8 bytes[SIZEU8(fl_dataframe_format_t)];
}__attribute__((packed)) fl_data_frame_u;

typedef union {
	struct {
		u8 mac[6];
		u32 timetamp;
		u8 type;
		u8 bt_call;
		u8 bt_endcall;
		u8 bt_rst;
		u32 pass_product;
		u32 err_product;
	//reserve
	//u8 rsv[11];
	} data;
	u8 bytes[22];
}__attribute__((packed)) fl_device_counter_t;


typedef struct {
	u8 mac[6];
	fl_slaveID_u slaveID;
	u32 timelife;
	bool active;
	tbs_dev_type_e dev_type;
#ifndef MASTER_CORE
//todo: parameters
	fl_slave_profiles_t profile;
#endif
	//data of dev
	u8 data[22];
}__attribute__((packed)) fl_nodeinnetwork_t;

typedef struct {
	fl_rsp_callback_fnc rsp_cb;
	s32 timeout;
	struct {
		u8 hdr_cmdid;
		u8 slaveID;
	} rsp_check;
}__attribute__((packed)) fl_rsp_container_t;

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
	}nwk;
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

#define DEBUG_TURN(x) do { \
							if (x) { PLOG_Start(ALL); } \
							else   { PLOG_Stop(ALL); } \
						} while(0)

#ifdef MASTER_CORE
void fl_nwk_master_init(void);
void fl_nwk_master_run(fl_pack_t *_pack_handle);
void fl_nwk_master_process(void);
fl_pack_t fl_master_packet_GetInfo_build(u8 *_slave_mac_arr, u8 _slave_num);
void fl_master_nodelist_AddRefesh(fl_nodeinnetwork_t _node);
s16 fl_master_SlaveID_find(u8 _id);
void fl_nwk_master_nodelist_load(void);
void fl_queue_REQcRSP_ScanRec(fl_pack_t _pack);
#else
extern volatile u8 NWK_DEBUG_STT; // it will be assigned into end-point byte (dbg :1bit);
bool IsJoinedNetwork(void);
bool IsOnline(void);
void fl_nwk_slave_init(void);
void fl_nwk_slave_run(fl_pack_t *_pack_handle);
void fl_nwk_slave_process(void);
bool fl_nwk_slave_checkHDR(u8 _hdr);
u32 fl_adv_timetampInPack(fl_pack_t _pack);
fl_timetamp_withstep_t fl_adv_timetampStepInPack(fl_pack_t _pack);
bool fl_req_slave_packet_createNsend(u8 _cmdid, u8* _data, u8 _len);
void fl_queue_REQcRSP_ScanRec(fl_pack_t _pack,void *_id);
int fl_nwk_slave_reconnect(void);
#endif
s8 fl_queueREQcRSP_add(u8 cmdid, u8 slaveid, fl_rsp_callback_fnc *_cb, u32 _timeout_ms);

int fl_queue_REQnRSP_TimeoutStart(void);
void fl_adv_setting_update(void);
int fl_adv_sendFIFO_add(fl_pack_t _pack);

#endif /* VENDOR_FRL_NETWORK_FL_NWK_HANDLER_H_ */
