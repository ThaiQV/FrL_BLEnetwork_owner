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
typedef void (*fl_rsp_callback_fnc)(void*, void*);
#define QUEUE_RSP_SLOT_MAX		16
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
	NWK_HDR_55 = 0x55, // REQ from slave
	// master -> req -> slave -> rsp
	NWK_HDR_F5_INFO = 0xF5, //get data information real-time
	NWK_HDR_F6_SENDMESS = 0xF6, //send mess to slave
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
			u8 rep_settings :2; //for setting slave
			u8 repeat_mode:1;   // mode repeat : level or non-level
		};
	} endpoint;
// LSB: don't change location byte
	s8 rssi;
}__attribute__((packed)) fl_dataframe_format_t; //Must less than 31 bytes

typedef union {
	fl_dataframe_format_t frame;
	u8 bytes[SIZEU8(fl_dataframe_format_t)];
}__attribute__((packed)) fl_data_frame_u;

/* COUNTER DEVICE
 * |Call butt|End call butt|Reset button|Pass product|Error product|Reserve| (sum 22 bytes)
 * |   1B	 |      1B     |     1B     |    4Bs     |     4Bs     |  11Bs |
 * */
typedef union {
	struct {
		u8 mac[6];
		u32 timetamp;
		u8 type;
		u8 bt_call;
		u8 bt_endcall;
		u8 bt_rst;
		u16 pass_product;
		u16 err_product;
		//add new mode and indx
		u8 mode;
		//previous_status
//		u16 pre_pass_product;
//		u16 pre_err_product;
//		u8 pre_mode;
		//add new index of packet
//		u16 index;
		//reverse
//		u8 reverse[7];
	} data;
	u8 bytes[22];
}__attribute__((packed)) tbs_device_counter_t;

//typedef struct {
//	u8 len;
//	u8 message[22];
//}__attribute__((packed)) tbs_counter_lcd_t;
//For POWER-METER DEVICEs
/*
 * | Frequency | Voltage | Current 1 | Current 2 | Current 3 | Power 1 | Power 2 | Power 3 | Energy 1 | Energy 2 | Energy 3 | Reserve | (sum 176 bits)
 * |   7 bits  |  9 bits |  10 bits  |  10 bits  |  10 bits  | 14 bits | 14 bits | 14 bits | 24 bits  | 24 bits  | 24 bits  | 16 bits |
 */
typedef struct {
	u8 mac[6];         // MAC address (48 bits)
	u32 timetamp;     // timetamp (32 bits)
	u8 type;			//device type
	// Measurement fields (bit-level precision noted)
	struct {
		u8 frequency;      // 7 bits
		u16 voltage;       // 9 bits
		u16 current1;      // 10 bits
		u16 current2;      // 10 bits
		u16 current3;      // 10 bits
		u16 power1;        // 14 bits
		u16 power2;        // 14 bits
		u16 power3;        // 14 bits
		u32 energy1;       // 24 bits
		u32 energy2;       // 24 bits
		u32 energy3;       // 24 bits
		u16 reserve;       // 16 bits
	} data;
}__attribute__((packed)) tbs_device_powermeter_t;

#define POWER_METER_STRUCT_BYTESIZE			(SIZEU8(tbs_device_powermeter_t))
#define POWER_METER_BITSIZE					34
static inline void tbs_pack_powermeter_data(const tbs_device_powermeter_t *src, u8 *dst) {
    u32 bitpos = 0;
    u32 byte_idx = 0;
    memset(dst, 0, POWER_METER_BITSIZE);

    // Copy MAC (6 bytes)
    memcpy(&dst[byte_idx], src->mac, sizeof(src->mac));
    byte_idx += sizeof(src->mac);

    // Copy timetamp (4 bytes)
    memcpy(&dst[byte_idx], &src->timetamp, sizeof(src->timetamp));
    byte_idx += sizeof(src->timetamp);

    // Copy type (1 byte)
    dst[byte_idx++] = src->type;

    // Start bit-level packing after 11 bytes
    bitpos = byte_idx * 8;

    #define WRITE_BITS(val, bits) do { \
        for (int i = 0; i < (bits); ++i) { \
            u32 byte_index = (bitpos + i) / 8; \
            u32 bit_index  = (bitpos + i) % 8; \
            dst[byte_index] |= ((val >> i) & 1) << bit_index; \
        } \
        bitpos += (bits); \
    } while (0)

    WRITE_BITS(src->data.frequency, 7);
    WRITE_BITS(src->data.voltage, 9);
    WRITE_BITS(src->data.current1, 10);
    WRITE_BITS(src->data.current2, 10);
    WRITE_BITS(src->data.current3, 10);
    WRITE_BITS(src->data.power1, 14);
    WRITE_BITS(src->data.power2, 14);
    WRITE_BITS(src->data.power3, 14);
    WRITE_BITS(src->data.energy1, 24);
    WRITE_BITS(src->data.energy2, 24);
    WRITE_BITS(src->data.energy3, 24);

    #undef WRITE_BITS
}
static inline void tbs_unpack_powermeter_data(tbs_device_powermeter_t *dst, const u8 *src) {
    u32 bitpos = 0;
    u32 byte_idx = 0;

    // Read MAC (6 bytes)
    memcpy(dst->mac, &src[byte_idx], sizeof(dst->mac));
    byte_idx += sizeof(dst->mac);

    // Read timetamp (4 bytes)
    memcpy(&dst->timetamp, &src[byte_idx], sizeof(dst->timetamp));
    byte_idx += sizeof(dst->timetamp);

    // Read type (1 byte)
    dst->type = src[byte_idx++];

    // Start bit-level unpacking after 11 bytes
    bitpos = byte_idx * 8;

    #define READ_BITS(var, bits) do { \
        var = 0; \
        for (int i = 0; i < (bits); ++i) { \
            u32 byte_index = (bitpos + i) / 8; \
            u32 bit_index  = (bitpos + i) % 8; \
            var |= ((src[byte_index] >> bit_index) & 1) << i; \
        } \
        bitpos += (bits); \
    } while (0)

    READ_BITS(dst->data.frequency, 7);
    READ_BITS(dst->data.voltage, 9);
    READ_BITS(dst->data.current1, 10);
    READ_BITS(dst->data.current2, 10);
    READ_BITS(dst->data.current3, 10);
    READ_BITS(dst->data.power1, 14);
    READ_BITS(dst->data.power2, 14);
    READ_BITS(dst->data.power3, 14);
    READ_BITS(dst->data.energy1, 24);
    READ_BITS(dst->data.energy2, 24);
    READ_BITS(dst->data.energy3, 24);

    #undef READ_BITS
}

typedef struct {
	u8 mac[6];
	fl_slaveID_u slaveID;
	u32 timelife;
	bool active;
	tbs_dev_type_e dev_type;
#ifndef MASTER_CORE
//todo: parameters
	fl_slave_profiles_t profile;
	//pointer data of device
	u8 *data;
#ifdef COUNTER_DEVICE
	u8 *lcd_mess[COUNTER_LCD_MESS_MAX];
#endif
#else
	//data of dev
	u8 data[POWER_METER_STRUCT_BYTESIZE+1];
#endif
}__attribute__((packed)) fl_nodeinnetwork_t;

typedef struct {
	fl_rsp_callback_fnc rsp_cb;
	u32 timeout; //use to count-down
	u32 timeout_set; //use to retry
	u8 retry;
	struct {
		u32 seqTimetamp;
		u8 hdr_cmdid;
		u8 slaveID;
	} rsp_check;
	struct {
		u8 payload[22];
		u8 len;
	} req_payload;
	fl_pack_t *p_REQ[QUEUE_RSP_SLOT_MAX];
}fl_rsp_container_t;

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
		u8 private_key[16];
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

#define DEBUG_TURN(x) do { \
							if (x) { PLOG_Start(ALL); } \
							else   { PLOG_Stop(ALL);  }\
						} while(0)

static inline uint32_t swap_endian32(uint32_t val) {
    return ((val >> 24) & 0x000000FF) |
           ((val >> 8)  & 0x0000FF00) |
           ((val << 8)  & 0x00FF0000) |
           ((val << 24) & 0xFF000000);
}

#ifdef MASTER_CORE
fl_pack_t fl_master_packet_RSP_55Com_build(u8* _slaveID,u8 _numslave,u8* _seqtimetamp,u16 _deltaTT);
void fl_master_SYNC_ORIGINAL_TIMETAMP(fl_timetamp_withstep_t _new_origin);
void fl_nwk_master_init(void);
void fl_nwk_master_run(fl_pack_t *_pack_handle);
void fl_nwk_master_process(void);
int fl_send_heartbeat(void);
void fl_nwk_master_heartbeat_run(void);
fl_pack_t fl_master_packet_GetInfo_build(u8 *_slave_mac_arr, u8 _slave_num);
s8 fl_master_packet_F5_CreateNSend(u8 *_slave_mac_arr, u8 _slave_num);
void fl_master_nodelist_AddRefesh(fl_nodeinnetwork_t _node);
s16 fl_master_SlaveID_find(u8 _id);
u8 fl_master_SlaveID_get(u8* _mac);
s8 fl_master_SlaveMAC_get(u8 _slaveid,u8* mac);
void fl_nwk_master_nodelist_load(void);
s8 fl_queue_REQcRSP_ScanRec(fl_pack_t _pack);
s8 fl_api_master_req(u8* _mac_slave,u8 _cmdid, u8* _data, u8 _len, fl_rsp_callback_fnc _cb, u32 _timeout_ms,u8 _retry);
#else
extern volatile u8 NWK_DEBUG_STT; // it will be assigned into end-point byte (dbg :1bit);
void fl_nwk_slave_init(void);
void fl_nwk_slave_run(fl_pack_t *_pack_handle);
void fl_nwk_slave_process(void);
bool fl_nwk_slave_checkHDR(u8 _hdr);
u32 fl_req_slave_packet_createNsend(u8 _cmdid, u8* _data, u8 _len);
s8 fl_queue_REQcRSP_ScanRec(fl_pack_t _pack,void *_id);
int fl_nwk_slave_reconnect(void);
s8 fl_api_slave_req(u8 _cmdid, u8* _data, u8 _len, fl_rsp_callback_fnc _cb, u32 _timeout_ms,u8 _retry);
#endif
u32 fl_adv_timetampInPack(fl_pack_t _pack);
fl_timetamp_withstep_t fl_adv_timetampStepInPack(fl_pack_t _pack);
s8 fl_queueREQcRSP_add(u8 slaveid,u8 cmdid,u32 _SeqTimetamp,u8* _payloadreq,u8 _len,fl_rsp_callback_fnc *_cb, u32 _timeout_ms,u8 _retry);
void fl_queue_REQnRSP_TimeoutInit(void);
void fl_adv_setting_update(void);
int fl_adv_sendFIFO_add(fl_pack_t _pack);

#endif /* VENDOR_FRL_NETWORK_FL_NWK_HANDLER_H_ */
