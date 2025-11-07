/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_nwk_master_handler.c
 *Created on		: Jul 11, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "app.h"
#include "stdio.h"
#include "utility.h"

#include "fl_nwk_handler.h"
#include "fl_adv_proc.h"
#include "fl_input_ext.h"
#include "fl_adv_repeat.h"
#include "fl_nwk_database.h"
#include "fl_nwk_protocol.h"
#include "fl_ble_wifi.h"

#ifdef MASTER_CORE

extern _attribute_data_retention_ volatile fl_timetamp_withstep_t ORIGINAL_MASTER_TIME;
#define SYNC_ORIGIN_MASTER(x,y) 			do{	\
												ORIGINAL_MASTER_TIME.timetamp = x;\
												ORIGINAL_MASTER_TIME.milstep = y;\
											}while(0) //Sync original time-master req
/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
#define TIME_COLLECT_NODE				5*1000 //

#define PACK_HANDLE_MASTER_SIZE 		32
fl_pack_t g_handle_master_array[PACK_HANDLE_MASTER_SIZE];
fl_data_container_t G_HANDLE_MASTER_CONTAINER = {.data = g_handle_master_array, .head_index = 0, .tail_index = 0, .mask = PACK_HANDLE_MASTER_SIZE - 1, .count = 0 };

fl_slaves_list_t G_NODE_LIST = { .slot_inused = 0xFF };
//fl_slaves_list_t G_OFFLINE_LIST = { .slot_inused = 0xFF };
fl_master_config_t G_MASTER_INFO = { .nwk = { .chn = { 37, 39, 39 }, .collect_chn = { 37, 38, 39 } } };

volatile u8 MASTER_INSTALL_STATE = 0;
//Period of the heartbeat
u16 PERIOD_HEARTBEAT = (16+1)*101; // 16 slots sending and 100ms interval adv
//flag debug of the network
volatile u8 NWK_DEBUG_STT = 0; // it will be assigned into endpoint byte (dbg :1bit)
volatile u8 NWK_REPEAT_MODE = 0; // 1: level | 0 : non-level
volatile u8 NWK_REPEAT_LEVEL = 3;

fl_hdr_nwk_type_e G_NWK_HDR_REQLIST[] = {NWK_HDR_FOTA,NWK_HDR_A5_HIS, NWK_HDR_F6_SENDMESS,NWK_HDR_F7_RSTPWMETER,NWK_HDR_F8_PWMETER_SET,NWK_HDR_22_PING}; // register cmdid REQ

#define NWK_HDR_REQ_SIZE (sizeof(G_NWK_HDR_REQLIST)/sizeof(G_NWK_HDR_REQLIST[0]))

static inline u8 IsREQHDR(fl_hdr_nwk_type_e cmdid) {
	for (u8 i = 0; i < NWK_HDR_REQ_SIZE; i++) {
		if (cmdid == G_NWK_HDR_REQLIST[i]) {
			return 1;
		}
	}
	return 0;
}

/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
s16 fl_master_Node_find(u8 *_mac);
u8 fl_master_SlaveID_get(u8* _mac);
s16 fl_master_SlaveID_find(u8 _id);
void fl_nwk_master_nodelist_init(void);
void fl_nwk_master_nodelist_load(void);
void fl_nwk_master_nodelist_store(void);
void fl_nwk_master_heartbeat_init(void);

void _master_nodelist_printf(fl_slaves_list_t *_node, u8 _size) {
	if (_size < 0xFF && _size > 0) {
		P_INFO("******** NODELIST ********\r\n");
		for (u8 var = 0; var < _size; ++var) {
			P_INFO("[%3d]0x%02X%02X%02X%02X%02X%02X(%d):%d\r\n",_node->sla_info[var].slaveID,_node->sla_info[var].mac[0],
					_node->sla_info[var].mac[1],_node->sla_info[var].mac[2],_node->sla_info[var].mac[3],_node->sla_info[var].mac[4],
					_node->sla_info[var].mac[5],_node->sla_info[var].dev_type,
					_node->sla_info[var].timelife);
		}
		P_INFO("******** END *************\r\n");
	}
}

void fl_master_nodelist_AddRefesh(fl_nodeinnetwork_t _node) {
	for (u8 var = 0; var < MAX_NODES; ++var) {
		if (IS_MAC_INVALID(G_NODE_LIST.sla_info[var].mac,0) != 1) {
			G_NODE_LIST.slot_inused = var + 1;
		} else {
			//debug
			u8 inused = 0xFF; //Empty
			u8 grp_inused = 0xFF;
			u8 mem_inused = 0xFF;
			if (var != 0) {
				inused = G_NODE_LIST.slot_inused;
				grp_inused = FL_SLAVEID_GRPID(G_NODE_LIST.sla_info[var - 1].slaveID);
				mem_inused = FL_SLAVEID_MEMID(G_NODE_LIST.sla_info[var - 1].slaveID);
			}
			LOGA(FLA,"NODE LIST: %d(grpID:%d,memID:%d)\r\n",inused,grp_inused,mem_inused);
			break;
		}
	}
	if (IS_MAC_INVALID(_node.mac,0) != 1) {
		//update
		u8 slot_ins = G_NODE_LIST.slot_inused++;
		G_NODE_LIST.sla_info[slot_ins] = _node;
		G_NODE_LIST.sla_info[slot_ins].slaveID = slot_ins;
		//Clear data in the first joinning
		memset(G_NODE_LIST.sla_info[slot_ins].data,0,SIZEU8(G_NODE_LIST.sla_info[slot_ins].data));
		LOGA(FLA,"Update Node [%d]slaveID:%d\r\n",slot_ins,G_NODE_LIST.sla_info[slot_ins].slaveID);
		fl_nwk_master_nodelist_store();
	}
}

/***************************************************
 * @brief 		: build packet heartbeat
 *
 * @param[in] 	: none
 *
 * @return	  	: fl_pack_t
 *
 ***************************************************/
fl_pack_t fl_master_packet_heartbeat_build(void) {
	extern volatile fl_timetamp_withstep_t WIFI_ORIGINAL_GETALL;
	extern u8 GETINFO_FLAG_EVENTTEST;
	fl_pack_t packet_built;
	fl_data_frame_u packet;
	memset(packet.bytes,0,SIZEU8(packet.bytes));
	packet.frame.hdr = NWK_HDR_HEARTBEAT;

	fl_timetamp_withstep_t timetampStep = fl_rtc_getWithMilliStep();
//	Recheck with ORIGINAL_TIMETAMP
//	u32 inpack = fl_rtc_timetamp2milltampStep(timetampStep);
//	u32 origin_pack = fl_rtc_timetamp2milltampStep(ORIGINAL_MASTER_TIME);
//	if(inpack <= origin_pack){
//		LOGA(APP,"Last:%d-%d\r\n",timetampStep.timetamp,timetampStep.milstep);
//		delay_ms(5);
//		timetampStep = fl_rtc_getWithMilliStep();
//		LOGA(APP,"New:%d-%d\r\n",timetampStep.timetamp,timetampStep.milstep);
//	}
//	LOGA(APP,"HB New:%d-%d\r\n",timetampStep.timetamp,timetampStep.milstep);

	packet.frame.timetamp[0] = U32_BYTE0(timetampStep.timetamp);
	packet.frame.timetamp[1] = U32_BYTE1(timetampStep.timetamp);
	packet.frame.timetamp[2] = U32_BYTE2(timetampStep.timetamp);
	packet.frame.timetamp[3] = U32_BYTE3(timetampStep.timetamp);

	//Add new mill-step
	packet.frame.milltamp = timetampStep.milstep;
	packet.frame.slaveID = 0xFF; // all grps + all members

	memset(packet.frame.payload,0xFF,SIZEU8(packet.frame.payload));
	packet.frame.payload[0]=GETINFO_FLAG_EVENTTEST;
	packet.frame.payload[1] = U32_BYTE0(WIFI_ORIGINAL_GETALL.timetamp);
	packet.frame.payload[2] = U32_BYTE1(WIFI_ORIGINAL_GETALL.timetamp);
	packet.frame.payload[3] = U32_BYTE2(WIFI_ORIGINAL_GETALL.timetamp);
	packet.frame.payload[4] = U32_BYTE3(WIFI_ORIGINAL_GETALL.timetamp);
	packet.frame.payload[5]=WIFI_ORIGINAL_GETALL.milstep;

	//crc
	packet.frame.crc8 = fl_crc8(packet.frame.payload,SIZEU8(packet.frame.payload));

	packet.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;
	packet.frame.endpoint.master = FL_FROM_MASTER; //non-ack
	packet.frame.endpoint.dbg = NWK_DEBUG_STT;
	packet.frame.endpoint.rep_settings = NWK_REPEAT_LEVEL;
	packet.frame.endpoint.repeat_mode = NWK_REPEAT_MODE;

	packet_built.length = SIZEU8(packet.bytes) - 1; //skip rssi
	memcpy(packet_built.data_arr,packet.bytes,packet_built.length);
	return packet_built;
}
/***************************************************
 * @brief 		: build packet collection
 *
 * @param[in] 	: none
 *
 * @return	  	: fl_pack_t
 *
 ***************************************************/
fl_pack_t fl_master_packet_collect_build(void) {
	fl_pack_t packet_built;
	fl_data_frame_u packet;
	memset(packet.bytes,0,SIZEU8(packet.bytes));
	packet.frame.hdr = NWK_HDR_COLLECT;

	fl_timetamp_withstep_t timetampStep = fl_rtc_getWithMilliStep();
//	u32 timetamp = fl_rtc_get();
	packet.frame.timetamp[0] = U32_BYTE0(timetampStep.timetamp);
	packet.frame.timetamp[1] = U32_BYTE1(timetampStep.timetamp);
	packet.frame.timetamp[2] = U32_BYTE2(timetampStep.timetamp);
	packet.frame.timetamp[3] = U32_BYTE3(timetampStep.timetamp);

	//Add new mill-step
	packet.frame.milltamp = timetampStep.milstep;
	/*****************************************************************/
	/* | HDR | Timetamp | Mill_time | SlaveID | payload | crc8 | Ep | */
	/* | 1B  |   4Bs    |    1B     |    1B   |   22Bs  |  1B  | 1B | -> .master = FL_FROM_SLAVE_ACK / FL_FROM_SLAVE */
	/*****************************************************************/

	/*****************************************************************/
	/* |      PAYLOAD 		 |*/
	/* | 6 bytes  Master MAC |*/

	packet.frame.slaveID = 0xFF;
	memset(packet.frame.payload,0xFF,SIZEU8(packet.frame.payload));
	//Add master's mac
	memcpy(&packet.frame.payload[0],blc_ll_get_macAddrPublic(),6);

	//crc
	packet.frame.crc8 = fl_crc8(packet.frame.payload,SIZEU8(packet.frame.payload));

	packet.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;
	packet.frame.endpoint.master = FL_FROM_MASTER_ACK; //ack
	packet.frame.endpoint.dbg = NWK_DEBUG_STT;
	packet.frame.endpoint.rep_settings = NWK_REPEAT_LEVEL;
	packet.frame.endpoint.repeat_mode = NWK_REPEAT_MODE;

	packet_built.length = SIZEU8(packet.bytes) - 1; //skip rssi
	memcpy(packet_built.data_arr,packet.bytes,packet_built.length);
	return packet_built;
}
/***************************************************
 * @brief 		: build packet assign SlaveID
 *
 * @param[in] 	: none
 *
 * @return	  	: fl_pack_t
 *
 ***************************************************/
fl_pack_t fl_master_packet_assignSlaveID_build(u8* _mac) {
	extern fl_adv_settings_t G_ADV_SETTINGS;
	fl_pack_t packet_built;

	fl_data_frame_u packet;
	memset(packet.bytes,0,SIZEU8(packet.bytes));
	packet.frame.hdr = NWK_HDR_ASSIGN;

	fl_timetamp_withstep_t timetampStep = fl_rtc_getWithMilliStep();
//	u32 timetamp = fl_rtc_get();
	packet.frame.timetamp[0] = U32_BYTE0(timetampStep.timetamp);
	packet.frame.timetamp[1] = U32_BYTE1(timetampStep.timetamp);
	packet.frame.timetamp[2] = U32_BYTE2(timetampStep.timetamp);
	packet.frame.timetamp[3] = U32_BYTE3(timetampStep.timetamp);

	//Add new mill-step
	packet.frame.milltamp = timetampStep.milstep;
	/*****************************************************************/
	/* | HDR | Timetamp | Mill_time | SlaveID | payload | crc8 | Ep | */
	/* | 1B  |   4Bs    |    1B     |    1B   |   22Bs  |  1B  | 1B | -> .master = FL_FROM_SLAVE */
	/*****************************************************************/

	/*****************************************************************/
	/* |       			  			PAYLOAD   		   			      |*/
	/* | 6 bytes  Slave's MAC |3 bytes Channel | 13 bytes Private_key | */
	//Add slave's mac
	memset(packet.frame.payload,0xFF,SIZEU8(packet.frame.payload));
	memcpy(&packet.frame.payload[0],_mac,6);
	//Add channel communication
	packet.frame.payload[6] = *G_ADV_SETTINGS.nwk_chn.chn1;
	packet.frame.payload[7] = *G_ADV_SETTINGS.nwk_chn.chn2;
	packet.frame.payload[8] = *G_ADV_SETTINGS.nwk_chn.chn3;
	//Add private key => use to decrypte packet data
	memcpy(&packet.frame.payload[9],G_MASTER_INFO.nwk.private_key,NWK_PRIVATE_KEY_SIZE);
	//Calculate crc
	packet.frame.crc8 = fl_crc8(packet.frame.payload,SIZEU8(packet.frame.payload));
	//Create payload assignment
	packet.frame.slaveID = fl_master_SlaveID_get(_mac);

	packet.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;
	packet.frame.endpoint.master = FL_FROM_MASTER; //non-ack
	packet.frame.endpoint.dbg = NWK_DEBUG_STT;
	packet.frame.endpoint.rep_settings = NWK_REPEAT_LEVEL;
	packet.frame.endpoint.repeat_mode = NWK_REPEAT_MODE;

	packet_built.length = SIZEU8(packet.bytes) - 1; //skip rssi
	memcpy(packet_built.data_arr,packet.bytes,packet_built.length);
	return packet_built;
}
/***************************************************
 * @brief 		: build packet RSP 55 to rsp all of slave
 *
 * @param[in] 	: none
 *
 * @return	  	: fl_pack_t
 *
 ***************************************************/
fl_pack_t fl_master_packet_RSP_1155Com_build(fl_hdr_nwk_type_e _cmdid,u8* _slaveID,u8 _numslave,u8* _seqtimetamp,u16 _deltaTT) {
	fl_pack_t packet_built;

	fl_data_frame_u packet;
	memset(packet.bytes,0,SIZEU8(packet.bytes));
	packet.frame.hdr = _cmdid;

	fl_timetamp_withstep_t timetampStep = fl_rtc_getWithMilliStep();
//	u32 timetamp = fl_rtc_get();
	packet.frame.timetamp[0] = U32_BYTE0(timetampStep.timetamp);
	packet.frame.timetamp[1] = U32_BYTE1(timetampStep.timetamp);
	packet.frame.timetamp[2] = U32_BYTE2(timetampStep.timetamp);
	packet.frame.timetamp[3] = U32_BYTE3(timetampStep.timetamp);

	//Add new mill-step
	packet.frame.milltamp = timetampStep.milstep;

	//Create payload
	memset(packet.frame.payload,0xFF,SIZEU8(packet.frame.payload));

	memcpy(packet.frame.payload,(u8*)_seqtimetamp,SIZEU8(fl_timetamp_withstep_t));
	packet.frame.payload[SIZEU8(fl_timetamp_withstep_t)] = U16_HI(_deltaTT);
	packet.frame.payload[SIZEU8(fl_timetamp_withstep_t)+1] = U16_LO(_deltaTT);
	memcpy(&packet.frame.payload[SIZEU8(fl_timetamp_withstep_t)+SIZEU8(_deltaTT)],_slaveID,_numslave);

	//crc
	packet.frame.crc8 = fl_crc8(packet.frame.payload,SIZEU8(packet.frame.payload));
	//assign slaveID
	packet.frame.slaveID = 0xFF;

	packet.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;
	packet.frame.endpoint.master = FL_FROM_MASTER; //non-ack
	packet.frame.endpoint.dbg = NWK_DEBUG_STT;
	packet.frame.endpoint.rep_settings = NWK_REPEAT_LEVEL;
	packet.frame.endpoint.repeat_mode = NWK_REPEAT_MODE;

	packet_built.length = SIZEU8(packet.bytes) - 1; //skip rssi
	memcpy(packet_built.data_arr,packet.bytes,packet_built.length);
	return packet_built;
}

fl_pack_t fl_master_packet_RSP_TimetampREQ_build(u8 cmdid, u8 slaveID, u8* _seqtimetamp) {
	extern fl_adv_settings_t G_ADV_SETTINGS;
	fl_pack_t packet_built;

	fl_data_frame_u packet;
	memset(packet.bytes,0,SIZEU8(packet.bytes));
	packet.frame.hdr = cmdid;

	fl_timetamp_withstep_t timetampStep = fl_rtc_getWithMilliStep();
	//	u32 timetamp = fl_rtc_get();
	packet.frame.timetamp[0] = U32_BYTE0(timetampStep.timetamp);
	packet.frame.timetamp[1] = U32_BYTE1(timetampStep.timetamp);
	packet.frame.timetamp[2] = U32_BYTE2(timetampStep.timetamp);
	packet.frame.timetamp[3] = U32_BYTE3(timetampStep.timetamp);

	//Add new mill-step
	packet.frame.milltamp = timetampStep.milstep;

	//Create payload
	memset(packet.frame.payload,0xFF,SIZEU8(packet.frame.payload));

	//u8 ack[2] = { 'O', 'K' };
	//fl_timetamp_withstep_t *ack_seqtimetamp = (fl_timetamp_withstep_t *)_seqtimetamp;
	memcpy(packet.frame.payload,(u8*) _seqtimetamp,SIZEU8(fl_timetamp_withstep_t));
	//crc
	packet.frame.crc8 = fl_crc8(packet.frame.payload,SIZEU8(packet.frame.payload));
	//assign slaveID
	packet.frame.slaveID = slaveID;

	packet.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;
	packet.frame.endpoint.master = FL_FROM_MASTER; //non-ack
	packet.frame.endpoint.dbg = NWK_DEBUG_STT;
	packet.frame.endpoint.rep_settings = NWK_REPEAT_LEVEL;
	packet.frame.endpoint.repeat_mode = NWK_REPEAT_MODE;

	packet_built.length = SIZEU8(packet.bytes) - 1; //skip rssi
	memcpy(packet_built.data_arr,packet.bytes,packet_built.length);
	return packet_built;
}

/***************************************************
 * @brief 		: build packet RSP 55 to rsp SlaveID
 *
 * @param[in] 	: none
 *
 * @return	  	: fl_pack_t
 *
 ***************************************************/
fl_pack_t fl_master_packet_RSP_55_build(u8 slaveID,u8* _seqtimetamp) {
	return fl_master_packet_RSP_TimetampREQ_build(NWK_HDR_55,slaveID,_seqtimetamp);
}
/***************************************************
 * @brief 		: build packet get information slave
 *
 * @param[in] 	: mac slave,count slave
 *
 * @return	  	: 1 if success, 0 otherwise
 *
 ***************************************************/
fl_pack_t fl_master_packet_GetInfo_build(u8 *_slave_mac_arr, u8 _slave_num) {
	fl_pack_t packet_built;
	fl_data_frame_u packet;

	/* F5|timetamp|slaveID 1|slaveID 2.......|CRC|TTL */
	/* 1B|  4Bs   |  1Bs  	|     18Bs  	 |1B |1B  */
	memset(packet.bytes,0,SIZEU8(packet.bytes));
	packet.frame.hdr = NWK_HDR_F5_INFO;

	fl_timetamp_withstep_t timetampStep = fl_rtc_getWithMilliStep();
//	u32 timetamp = fl_rtc_get();
	packet.frame.timetamp[0] = U32_BYTE0(timetampStep.timetamp);
	packet.frame.timetamp[1] = U32_BYTE1(timetampStep.timetamp);
	packet.frame.timetamp[2] = U32_BYTE2(timetampStep.timetamp);
	packet.frame.timetamp[3] = U32_BYTE3(timetampStep.timetamp);

	//Add new mill-step
	packet.frame.milltamp = timetampStep.milstep;

	packet.frame.slaveID = 0xFF;

	memset(packet.frame.payload,0xFF,SIZEU8(packet.frame.payload));
	//create payload req
	memcpy(packet.frame.payload,_slave_mac_arr,_slave_num);
	//crc
	packet.frame.crc8 = fl_crc8(packet.frame.payload,SIZEU8(packet.frame.payload));

	packet.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;
	packet.frame.endpoint.master = FL_FROM_MASTER_ACK;
	packet.frame.endpoint.dbg = NWK_DEBUG_STT;
	packet.frame.endpoint.rep_settings= NWK_REPEAT_LEVEL;
	packet.frame.endpoint.repeat_mode = NWK_REPEAT_MODE;

	packet_built.length = SIZEU8(packet.bytes) - 1; //skip rssi
	memcpy(packet_built.data_arr,packet.bytes,packet_built.length);
	return packet_built;
}

s8 fl_master_packet_F5_CreateNSend(u8 *_slave_mac_arr, u8 _slave_num) {
	fl_send_heartbeat();
	fl_pack_t info_pack = fl_master_packet_GetInfo_build(_slave_mac_arr,_slave_num);
	P_PRINTFHEX_A(INF,info_pack.data_arr,info_pack.length,"%s(%d):","Info Pack",info_pack.length);
	//Send ADV
	s8 add_rslt = fl_adv_sendFIFO_add(info_pack);
	if (add_rslt != -1) {
		fl_nwk_master_heartbeat_run();
	}
	return add_rslt;
}
/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/
void fl_master_SYNC_ORIGINAL_TIMETAMP(fl_timetamp_withstep_t _new_origin) {
	u64 origin = fl_rtc_timetamp2milltampStep(ORIGINAL_MASTER_TIME);
	u64 new_origin = fl_rtc_timetamp2milltampStep(_new_origin);
	if (origin < new_origin) {
		SYNC_ORIGIN_MASTER(_new_origin.timetamp,_new_origin.milstep);
		LOGA(INF,"Master Synchronzation Timetamp:%d(%d)\r\n",ORIGINAL_MASTER_TIME.timetamp,ORIGINAL_MASTER_TIME.milstep);
	}
}
//FOR TESTIING
void fl_master_StatusNodePrintf(void) {
	u8 online_cnt = 0;
	u32 max_time = 0;
	u8 var = 0;
	for (var = 0; var < G_NODE_LIST.slot_inused; ++var) {
		if (G_NODE_LIST.sla_info[var].active) {
			online_cnt++;
			if (max_time < G_NODE_LIST.sla_info[var].timelife) {
				max_time = G_NODE_LIST.sla_info[var].timelife;
			}
		}
	}
	if (online_cnt == G_NODE_LIST.slot_inused) {
//		datetime_t cur_dt;
//		u32 cur_timetamp = fl_rtc_get();
//		fl_rtc_timestamp_to_datetime(cur_timetamp,&cur_dt);
//		LOGA(APP,"CollectDONE:%02d/%02d/%02d - %02d:%02d:%02d\r\n",cur_dt.year,cur_dt.month,cur_dt.day,cur_dt.hour,cur_dt.minute,cur_dt.second);
//		LOGA(APP,"Online :%d/%d\r\n",online_cnt,var);
//		LOGA(APP,"RspTime:%d ms\r\n",max_time);
		P_INFO("Online :%d/%d\r\n",online_cnt,var);
		P_INFO("RspTime:%d ms\r\n",max_time);
	}
}

s16 fl_master_Node_find(u8 *_mac) {
	for (u8 var = 0; var < G_NODE_LIST.slot_inused && G_NODE_LIST.slot_inused != 0xFF; ++var) {
		if (MAC_MATCH(G_NODE_LIST.sla_info[var].mac,_mac)) {
			return var;
		}
	}
	return -1;
}
s16 fl_master_SlaveID_find(u8 _id) {
	for (u8 var = 0; var < G_NODE_LIST.slot_inused && G_NODE_LIST.slot_inused != 0xFF; ++var) {
		if (G_NODE_LIST.sla_info[var].slaveID == _id) {
			return var;
		}
	}
	return -1;
}
u8 fl_master_SlaveID_get(u8* _mac) {
	s8 indx = fl_master_Node_find(_mac);
	if (indx != -1) {
		return G_NODE_LIST.sla_info[indx].slaveID;
	}
	return 0xFF;
}

s8 fl_master_SlaveMAC_get(u8 _slaveid,u8* mac){
	s8 indx = fl_master_SlaveID_find(_slaveid);
	//broadcast mac
	if(_slaveid==0xFF && G_NODE_LIST.slot_inused != 0xFF ){
		memset(mac,0xFF,6);
		return 1;
	}
	//slave mac
	if(indx != -1){
		memcpy(mac,G_NODE_LIST.sla_info[indx].mac,SIZEU8(G_NODE_LIST.sla_info[indx].mac));
		return 1;
	}
	return -1;
}
/***************************************************
 * @brief 		: create and send packet request to slave via the freelux protocol
 *
 * @param[in] 	: _cmdid : id cmd
 * 				  _data : pointer to data
 * 				  _len  : size data
 *
 * @return	  	: 0: fail, otherwise seqtimetamp
 *
 ***************************************************/
u64 fl_req_master_packet_createNsend(u8* _slave_mac,u8 _cmdid,u8* _data, u8 _len){
	/*****************************************************************/
	/* | HDR | Timetamp | Mill_time | SlaveID | payload | crc8 | Ep | */
	/* | 1B  |   4Bs    |    1B     |    1B   |   22Bs  |  1B  | 1B | -> .master = FL_FROM_SLAVE_ACK / FL_FROM_SLAVE */
	/*****************************************************************/
	//**todo: Need to convert _data to payload base on special command ID
	//
	fl_pack_t rslt = {.length = 0};
	fl_hdr_nwk_type_e cmdid = (fl_hdr_nwk_type_e)_cmdid;
	if(!IsREQHDR(cmdid)){
		ERR(API,"REQ CMD ID hasn't been registered!!\r\n");
		return 0;
	}
	s8 slaveID = fl_master_Node_find(_slave_mac);
	if(slaveID == -1){
		ERR(API,"SlaveID NOT FOUND!!\r\n");
		return 0;
	}
	fl_send_heartbeat();
	delay_ms(5);
	//generate seqtimetamp
	fl_timetamp_withstep_t timetampStep = fl_rtc_getWithMilliStep();
//	timetampStep.milstep++;
	fl_data_frame_u req_pack;
	/*Create common packet */
	req_pack.frame.hdr = cmdid;
	req_pack.frame.timetamp[0] = U32_BYTE0(timetampStep.timetamp);
	req_pack.frame.timetamp[1] = U32_BYTE1(timetampStep.timetamp);
	req_pack.frame.timetamp[2] = U32_BYTE2(timetampStep.timetamp);
	req_pack.frame.timetamp[3] = U32_BYTE3(timetampStep.timetamp);

	//Add new mill-step
	req_pack.frame.milltamp = timetampStep.milstep;
	LOGA(INF,"Send %02X REQ to Slave %d:%d/%d\r\n",req_pack.frame.hdr,slaveID,timetampStep.timetamp,timetampStep.milstep);

	req_pack.frame.slaveID = slaveID;
	//Create payload
	memset(req_pack.frame.payload,0x0,SIZEU8(req_pack.frame.payload));
	memcpy(req_pack.frame.payload,_data,_len);
	//crc
	req_pack.frame.crc8 = fl_crc8(req_pack.frame.payload,SIZEU8(req_pack.frame.payload));

	//create endpoint
	req_pack.frame.endpoint.dbg = NWK_DEBUG_STT;
	req_pack.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;
	req_pack.frame.endpoint.rep_settings = NWK_REPEAT_LEVEL;
	req_pack.frame.endpoint.repeat_mode = NWK_REPEAT_MODE;
	//Create packet from slave
	req_pack.frame.endpoint.master = FL_FROM_MASTER;
	/*======================*/
	switch (cmdid) {
		case NWK_HDR_A5_HIS:{
			//Create packet from slave
			req_pack.frame.endpoint.master = FL_FROM_MASTER;
		}
			break;
		case NWK_HDR_F6_SENDMESS: {
			if (G_NODE_LIST.sla_info[slaveID].dev_type == TBS_COUNTER) {
				req_pack.frame.endpoint.master = FL_FROM_MASTER_ACK;
			}
		}
		break;
		case NWK_HDR_F7_RSTPWMETER:
		case NWK_HDR_F8_PWMETER_SET:
		{
			if(G_NODE_LIST.sla_info[slaveID].dev_type==TBS_POWERMETER){
				req_pack.frame.endpoint.master = FL_FROM_MASTER_ACK;
			}else{
				return 0;
			}
		}
		break;
		case NWK_HDR_22_PING: {
			req_pack.frame.endpoint.master = FL_FROM_MASTER_ACK;
		}
		break;
		default:
			return 0;
		break;
	}
	//copy to data struct
	rslt.length = SIZEU8(req_pack.bytes) - 1; //skip rssi
	memcpy(rslt.data_arr,req_pack.bytes,rslt.length );
	P_PRINTFHEX_A(INF,rslt.data_arr,rslt.length,"REQ %X ",_cmdid);
	//Send ADV
	s8 add_rslt = fl_adv_sendFIFO_add(rslt);
	u64 seq_timetamp = 0;
	if(add_rslt!=-1){
		//seqtimetamp
		fl_timetamp_withstep_t  timetamp_inpack = fl_adv_timetampStepInPack(rslt);
		seq_timetamp = fl_rtc_timetamp2milltampStep(timetamp_inpack);
//		LOGA(API,"API REQ Synchronization TimeTamp:%d(%d)\r\n",timetamp_inpack.timetamp,timetamp_inpack.milstep);
//		SYNC_ORIGIN_MASTER(timetamp_inpack.timetamp,timetamp_inpack.milstep);
		fl_nwk_master_heartbeat_run();
	}
	return seq_timetamp;
}

s8 fl_api_master_req(u8* _mac_slave,u8 _cmdid, u8* _data, u8 _len, fl_rsp_callback_fnc _cb, u32 _timeout_ms,u8 _retry) {
	//register timeout cb

	if (_cb != 0 && ( _timeout_ms*1000 >= 2*QUEUQ_REQcRSP_INTERVAL || _timeout_ms==0)) {
		u64 seq_timetamp = fl_req_master_packet_createNsend(_mac_slave,_cmdid,_data,_len);
		if(seq_timetamp){
			return fl_queueREQcRSP_add(fl_master_Node_find(_mac_slave),_cmdid,seq_timetamp,_data,_len,&_cb,_timeout_ms,_retry);
		}
	} else if(_timeout_ms ==0){
		return (fl_req_master_packet_createNsend(_mac_slave,_cmdid,_data,_len) == 0?-1:0); // none rsp
	}
	ERR(API,"Can't register REQ (%d/%d ms)!!\r\n",(u32)_cb,_timeout_ms);
	return -1;
}
/***************************************************
 * @brief 		:soft-timer callback for the processing response
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
int fl_send_collection_req(void) {
	datetime_t cur_dt;
	u32 cur_timetamp = fl_rtc_get();
	fl_rtc_timestamp_to_datetime(cur_timetamp,&cur_dt);
	LOGA(APP,"CollectREQ:%02d/%02d/%02d-%02d:%02d:%02d\r\n",cur_dt.year,cur_dt.month,cur_dt.day,cur_dt.hour,cur_dt.minute,cur_dt.second);
	//fl_master_node_printf();
	fl_pack_t packet_built = fl_master_packet_collect_build();
	fl_adv_sendFIFO_add(packet_built);
	return 1200 * 1000;
}
/***************************************************
 * @brief 		:send heartbeat packet
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
int fl_send_heartbeat(void) {
	//if(G_NODE_LIST.slot_inused == 0xFF){return 0;}
//	datetime_t cur_dt;
//	u32 cur_timetamp = fl_rtc_get();
//	fl_rtc_timestamp_to_datetime(cur_timetamp,&cur_dt);

//	fl_master_node_printf();
//	LOGA(APP,"[%02d/%02d/%02d-%02d:%02d:%02d] HeartBeat Sync(%d ms)\r\n",cur_dt.year,cur_dt.month,cur_dt.day,cur_dt.hour,cur_dt.minute,cur_dt.second,
//			PERIOD_HEARTBEAT);
	fl_pack_t packet_built = fl_master_packet_heartbeat_build();
	fl_adv_sendFIFO_add(packet_built);
	return 0; //
}
/***************************************************
 * @brief 		: check change and store
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
int _nwk_master_backup(void) {
	static u32 pre_crc32 = 0;
	u32 crc32 = fl_db_crc32((u8*) &G_MASTER_INFO,sizeof(fl_master_config_t) / sizeof(u8));
	if (crc32 != pre_crc32) {
		pre_crc32 = crc32;
		fl_db_master_profile_t profile;
		profile.nwk.chn[0] = G_MASTER_INFO.nwk.chn[0];
		profile.nwk.chn[1] = G_MASTER_INFO.nwk.chn[1];
		profile.nwk.chn[2] = G_MASTER_INFO.nwk.chn[2];
		memcpy(profile.nwk.private_key,G_MASTER_INFO.nwk.private_key,SIZEU8(G_MASTER_INFO.nwk.private_key));
		fl_db_masterprofile_save(profile);
		LOGA(INF,"Channels:%d |%d |%d\r\n",profile.nwk.chn[0],profile.nwk.chn[1],profile.nwk.chn[2])
	}
	return 0;
}
//
//int _nwk_master_checkSlaveStatus(void) {
//	extern volatile fl_timetamp_withstep_t WIFI_ORIGINAL_GETALL;
//	extern const u32 ORIGINAL_TIME_TRUST;
//#define INTERVAL_REFESH_STATUS 45
//	if (G_NODE_LIST.slot_inused != 0xFF) {
//		if (WIFI_ORIGINAL_GETALL.timetamp < fl_rtc_get() && WIFI_ORIGINAL_GETALL.timetamp > ORIGINAL_TIME_TRUST) {
//			if (fl_rtc_get() - WIFI_ORIGINAL_GETALL.timetamp >= INTERVAL_REFESH_STATUS) {
//				for (u8 var = 0; var < G_NODE_LIST.slot_inused; ++var) {
//					G_NODE_LIST.sla_info[var].active = false;
//				}
//				return INTERVAL_REFESH_STATUS*1000*1000;
//			}
//		}
//	}
//#undef INTERVAL_REFESH_STATUS
//	return 98 * 1001;
//}
/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/
void fl_nwk_master_StatusNodesRefesh(void) {
	for (u8 i = 0; i < G_NODE_LIST.slot_inused && G_NODE_LIST.slot_inused != 0xFF; i++) {
		G_NODE_LIST.sla_info[i].active = (fl_rtc_get() - G_NODE_LIST.sla_info[i].timelife <= 40) ? 1 : 0;
	}
}

/***************************************************
 * @brief 		:soft-timer callback for the processing response
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
static void _master_updateDB_for_Node(u8 node_indx ,fl_data_frame_u *packet)  {
	G_NODE_LIST.sla_info[node_indx].active = true;
	G_NODE_LIST.sla_info[node_indx].timelife = fl_rtc_get();
	P_INFO_HEX(G_NODE_LIST.sla_info[node_indx].mac,6,"%d->[%02d]",G_NODE_LIST.sla_info[node_indx].timelife,G_NODE_LIST.sla_info[node_indx].slaveID);
	//create MAC + TIMETAMP + DEV_TYPE
	u8 size_mac = SIZEU8(G_NODE_LIST.sla_info[node_indx].mac);
	//memcpy(&G_NODE_LIST.sla_info[node_indx].data[0],G_NODE_LIST.sla_info[node_indx].mac,size_mac); //update mac to pointer data
	/*Timetamp*/
//	fl_timetamp_withstep_t timetampStep = fl_rtc_getWithMilliStep();
//	//	u32 timetamp = fl_rtc_get();
//	G_NODE_LIST.sla_info[node_indx].data[size_mac] = U32_BYTE0(timetampStep.timetamp);
//	G_NODE_LIST.sla_info[node_indx].data[size_mac + 1] = U32_BYTE1(timetampStep.timetamp);
//	G_NODE_LIST.sla_info[node_indx].data[size_mac + 2] = U32_BYTE2(timetampStep.timetamp);
//	G_NODE_LIST.sla_info[node_indx].data[size_mac + 3] = U32_BYTE3(timetampStep.timetamp);
	memcpy(&G_NODE_LIST.sla_info[node_indx].data[size_mac],packet->frame.timetamp,SIZEU8(packet->frame.timetamp));
	/*Dev type*/
	G_NODE_LIST.sla_info[node_indx].data[size_mac + 4] = G_NODE_LIST.sla_info[node_indx].dev_type;
	/*Data*/
	P_PRINTFHEX_A(INF,packet->frame.payload,SIZEU8(packet->frame.payload),"PACK:");
	if (G_NODE_LIST.sla_info[node_indx].dev_type == TBS_COUNTER) {
		memcpy(&G_NODE_LIST.sla_info[node_indx].data[size_mac + 5],&packet->frame.payload[0],SIZEU8(packet->frame.payload));
		tbs_counter_printf(INF,(void*) G_NODE_LIST.sla_info[node_indx].data);
	}
	if (G_NODE_LIST.sla_info[node_indx].dev_type == TBS_POWERMETER) {
		memcpy(&G_NODE_LIST.sla_info[node_indx].data[size_mac + 5],&packet->frame.payload[0],SIZEU8(packet->frame.payload));
		//for test
		tbs_device_powermeter_t received;
		tbs_unpack_powermeter_data(&received,G_NODE_LIST.sla_info[node_indx].data);
		tbs_power_meter_printf(INF,(void*) &received);
	}
}

int fl_master_ProccesRSP_cbk(void) {
	fl_pack_t data_in_queue;
	u16 numofrsp = 0;
	if ((numofrsp=FL_QUEUE_GET(&G_HANDLE_MASTER_CONTAINER,&data_in_queue))) {

		fl_data_frame_u packet;
		extern u8 fl_packet_parse(fl_pack_t _pack, fl_dataframe_format_t *rslt);

		if(!fl_packet_parse(data_in_queue,&packet.frame)) return -1;
		if(!MASTER_INSTALL_STATE && G_NODE_LIST.slot_inused == 0xFF){return -1;}
		//LOGA(APP,"NumOfRSP:%d\r\n",numofrsp);
		P_PRINTFHEX_A(INF,packet.bytes,SIZEU8(packet.bytes),"Slave(%d|0x%02X)-hdr(%02X):",packet.frame.slaveID,packet.frame.slaveID,
				packet.frame.hdr);
		fl_timetamp_withstep_t  timetamp_inpack = fl_adv_timetampStepInPack(data_in_queue);
		u64 seq_timetamp =fl_rtc_timetamp2milltampStep(timetamp_inpack);
		LOGA(INF,"TT inpack:%lld\r\n",seq_timetamp);
		//Todo:Process RSP with API REQ registered
		fl_queue_REQcRSP_ScanRec(data_in_queue);

		switch ((fl_hdr_nwk_type_e) packet.frame.hdr) {
			case NWK_HDR_HEARTBEAT: {
				/** - NON-RSP
				 u32 mac_short = MAKE_U32(packet.frame.payload[0],packet.frame.payload[1],packet.frame.payload[2],packet.frame.payload[3]);
				 s16 node_indx = fl_master_node_find(mac_short);
				 if (node_indx != -1) {
				 G_NODE_LIST[node_indx].active = true;
				 G_NODE_LIST[node_indx].timelife = (clock_time() - G_NODE_LIST[node_indx].timelife) / SYSTEM_TIMER_TICK_1MS;
				 P_PRINTFHEX_A(INF,packet.frame.payload,4,"0x%08X(%d ms)(%d):",mac_short,G_NODE_LIST[node_indx].timelife,
				 packet.frame.endpoint.repeat_cnt);
				 } else {
				 P_PRINTFHEX_A(INF,packet.frame.payload,4,"0x%08X:",mac_short);
				 }
				 **/
			}
			break;

			case NWK_HDR_A5_HIS: {
				u8 slave_id = packet.frame.slaveID;
				u8 node_indx = fl_master_SlaveID_find(slave_id);
				if (node_indx != -1) {
					if (packet.frame.endpoint.master == FL_FROM_SLAVE) {
						fl_ble2wifi_HISTORY_SEND(G_NODE_LIST.sla_info[node_indx].mac,packet.frame.timetamp,packet.frame.payload);
					}
				}
			}
			break;
			case NWK_HDR_11_REACTIVE: {
				u8 slave_id = packet.frame.slaveID;
				u8 node_indx = fl_master_SlaveID_find(slave_id);
				if (node_indx != -1) {
					if (packet.frame.endpoint.master == FL_FROM_SLAVE_ACK) {
						_master_updateDB_for_Node(node_indx,&packet);
						G_NODE_LIST.sla_info[node_indx].active = true;
						//Send rsp to slave
						u8 seq_timetamp[5];
						memcpy(seq_timetamp,packet.frame.timetamp,SIZEU8(packet.frame.timetamp));
						seq_timetamp[SIZEU8(packet.frame.timetamp)] = packet.frame.milltamp;
						fl_adv_sendFIFO_add(fl_master_packet_RSP_TimetampREQ_build(NWK_HDR_11_REACTIVE,slave_id,seq_timetamp));
					}
				} else {
					ERR(INF,"ID not foud:%02X\r\n",slave_id);
					return -1;
				}
			}
			break;
			case NWK_HDR_22_PING: {
				u8 slave_id = packet.frame.slaveID;
				u8 node_indx = fl_master_SlaveID_find(slave_id);
				if (node_indx != -1) {
					if (packet.frame.endpoint.master == FL_FROM_SLAVE_ACK) {
						//Send rsp to slave
						u8 seq_timetamp[5];
						memcpy(seq_timetamp,packet.frame.timetamp,SIZEU8(packet.frame.timetamp));
						seq_timetamp[SIZEU8(packet.frame.timetamp)] = packet.frame.milltamp;
						fl_adv_sendFIFO_add(fl_master_packet_RSP_TimetampREQ_build(NWK_HDR_22_PING,slave_id,seq_timetamp));
					}
				} else {
					ERR(INF,"ID not foud:%02X\r\n",slave_id);
					return -1;
				}
			}
			break;
			case NWK_HDR_55: {
				/*****************************************************************/
				/* | HDR | Timetamp | Mill_time | SlaveID | payload | crc8 | Ep | */
				/* | 1B  |   4Bs    |    1B     |    1B   |   20Bs  |  1B  | 1B | -> .master = FL_FROM_SLAVE_ACK / FL_FROM_SLAVE */
				/*****************************************************************/
				//Todo: parse payload response
				u8 slave_id = packet.frame.slaveID;
				u8 node_indx = fl_master_SlaveID_find(slave_id);
				if (node_indx != -1) {
					if (packet.frame.endpoint.master == FL_FROM_SLAVE_ACK) {
					//send to WIFI the previous data
//					fl_ble2wifi_EVENT_SEND(G_NODE_LIST.sla_info[node_indx].mac);
//					G_NODE_LIST.sla_info[node_indx].active = true;
//					G_NODE_LIST.sla_info[node_indx].timelife = (clock_time() - G_NODE_LIST.sla_info[node_indx].timelife);
//					u32 cnt_inpack = MAKE_U32(packet.frame.payload[3],packet.frame.payload[2],packet.frame.payload[1],packet.frame.payload[0]);
//					LOGA(INF,"CMD_55(%d)0x%02X%02X%02X%02X%02X%02X(%d):%d\r\n",slave_id,G_NODE_LIST.sla_info[node_indx].mac[0],
//							G_NODE_LIST.sla_info[node_indx].mac[1],G_NODE_LIST.sla_info[node_indx].mac[2],G_NODE_LIST.sla_info[node_indx].mac[3],
//							G_NODE_LIST.sla_info[node_indx].mac[4],G_NODE_LIST.sla_info[node_indx].mac[5],packet.frame.endpoint.repeat_cnt,cnt_inpack);
						_master_updateDB_for_Node(node_indx,&packet);
						//Send rsp to slave
						u8 seq_timetamp_arr[5];
						memcpy(seq_timetamp_arr,packet.frame.timetamp,SIZEU8(packet.frame.timetamp));
						seq_timetamp_arr[SIZEU8(packet.frame.timetamp)] = packet.frame.milltamp;
						fl_adv_sendFIFO_add(fl_master_packet_RSP_55_build(slave_id,seq_timetamp_arr));
						//send to WIFI
						fl_ble2wifi_EVENT_SEND(G_NODE_LIST.sla_info[node_indx].mac);
					}
				} else {
					ERR(INF,"ID not foud:%02X\r\n",slave_id);
					return -1;
				}
			}
			break;
			case NWK_HDR_F5_INFO: {
				/* F5|timetamp|slaveID| Info |CRC|TTL */
				/* 1B|  4Bs   |  1B   | 18Bs |1B |1B  */
				u8 crc = fl_crc8(packet.frame.payload,SIZEU8(packet.frame.payload));
				s16 node_indx = -1;
				if (crc == packet.frame.crc8) {
					//Todo: parse payload response
					u8 slave_id = packet.frame.slaveID;
					node_indx = fl_master_SlaveID_find(slave_id);
					if (node_indx != -1) {
						_master_updateDB_for_Node(node_indx,&packet);
						//send to WIFI
//						fl_ble2wifi_EVENT_SEND(G_NODE_LIST.sla_info[node_indx].mac);
					} else {
						ERR(INF,"ID not foud:%02X\r\n",slave_id);
						return -1;
					}
				} else {
					ERR(INF,"CRC Fail!!\r\n");
					P_PRINTFHEX_A(INF,packet.bytes,SIZEU8(packet.bytes),"[ERR]PACK:");
					return -1;
				}
			}
			break;
			case NWK_HDR_COLLECT: {
				//u32 mac_short = MAKE_U32(packet.frame.payload[0],packet.frame.payload[1],packet.frame.payload[2],packet.frame.payload[3]);
				//u8 mac[6] = {packet.frame.payload[0],packet.frame.payload[1],packet.frame.payload[2],packet.frame.payload[3],packet.frame.payload[4],packet.frame.payload[5]};
				u8 mac[6];
				MAC_ZERO_CLEAR(mac,0);
				memcpy(mac,&packet.frame.payload[0],sizeof(mac));
				if (IS_MAC_INVALID(mac,0) || IS_MAC_INVALID(mac,0xFF)) {
					return -1;
				}
				s16 node_indx = fl_master_Node_find(mac);
				//Full and node is a new => skip
				if (G_NODE_LIST.slot_inused > MAX_NODES -1 && G_NODE_LIST.slot_inused != 0xFF && node_indx == -1) {
					ERR(APP,"Network Full!!!\r\n");
					break;
				}
				if (node_indx == -1) {
					fl_nodeinnetwork_t new_slave;
					new_slave.active = true;
					new_slave.timelife = fl_rtc_get();
//					new_slave.mac_short.mac_u32 = mac_short;
					memcpy(new_slave.mac,mac,sizeof(mac));
					new_slave.dev_type = packet.frame.payload[SIZEU8(new_slave.mac)];
					P_PRINTFHEX_A(INF,packet.frame.payload,sizeof(mac),"0x%02X%02X%02X%02X%02X%02X\r\n(%d):",new_slave.mac[0],new_slave.mac[1],
							new_slave.mac[2],new_slave.mac[3],new_slave.mac[4],new_slave.mac[5],packet.frame.endpoint.repeat_cnt);
					//assign SlaveID (grpID + memID) to the slave
					fl_master_nodelist_AddRefesh(new_slave);
				}
				//Send assignment to slave
				fl_adv_sendFIFO_add(fl_master_packet_assignSlaveID_build(mac));
			}
			break;
			default:
			break;
		}
	} else {
		ERR(INF,"Err <QUEUE GET>-G_HANDLE_MASTER_CONTAINER!!\r\n");
		return -1;
	}
	return 0;
}
/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/
void fl_nwk_master_init(void) {
	LOG_P(INF,"Freelux network MASTER init -> ok\r\n");
	FL_QUEUE_CLEAR(&G_HANDLE_MASTER_CONTAINER,PACK_HANDLE_MASTER_SIZE);
	//todo: load database into the flash
	fl_nwk_master_nodelist_init();
	fl_nwk_master_nodelist_load();
	//todo: load master profile
	fl_db_master_profile_t master_profile = fl_db_masterprofile_init();
	G_MASTER_INFO.nwk.chn[0] = master_profile.nwk.chn[0];
	G_MASTER_INFO.nwk.chn[1] = master_profile.nwk.chn[1];
	G_MASTER_INFO.nwk.chn[2] = master_profile.nwk.chn[2];
	memcpy(G_MASTER_INFO.nwk.private_key,master_profile.nwk.private_key,SIZEU8(master_profile.nwk.private_key));
	blt_soft_timer_add(_nwk_master_backup,2 * 1000 * 1000);
	fl_nwk_master_heartbeat_init();
	//Recheck status of the all network
//	blt_soft_timer_add(_nwk_master_checkSlaveStatus,100*1000);
}
/***************************************************
 * @brief 		: init nodelist
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
void fl_nwk_master_nodelist_init(void) {
	G_NODE_LIST.slot_inused = 0xFF;
	for (u8 i = 0; i < MAX_NODES; i++) {
		G_NODE_LIST.sla_info[i].active = false;
		G_NODE_LIST.sla_info[i].slaveID = 0xFF;
		G_NODE_LIST.sla_info[i].timelife = 0;
		MAC_ZERO_CLEAR(G_NODE_LIST.sla_info[i].mac,0);
	}
}
/***************************************************
 * @brief 		: store nodelist into the flash
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
void fl_nwk_master_nodelist_store(void) {
	fl_nodelist_db_t nodelist;
	nodelist.num_slave = G_NODE_LIST.slot_inused;
	for (u8 var = 0; var < nodelist.num_slave && nodelist.num_slave != 0xFF; ++var) {
		nodelist.slave[var].slaveid = G_NODE_LIST.sla_info[var].slaveID;
		//nodelist.slave[var].mac_u32 = G_NODE_LIST.sla_info[var].mac_short.mac_u32;
		memcpy(nodelist.slave[var].mac,G_NODE_LIST.sla_info[var].mac,6);
		nodelist.slave[var].dev_type = (u8)G_NODE_LIST.sla_info[var].dev_type;
	}
//	//For testing
//	nodelist.num_slave = NODELIST_SLAVE_MAX;
//	nodelist.slave[0].slaveid = 0;
//	nodelist.slave[0].mac_u32 = 0x41232e24; //
//	for (u8 i = 1; i < nodelist.num_slave; ++i) {
//		nodelist.slave[i].slaveid = i;
//		nodelist.slave[i].mac_u32 = 0x2F2245D1; //
//	}
	if (nodelist.num_slave && nodelist.num_slave != 0xFF) {
		fl_db_nodelist_save(&nodelist);
	}
}
/***************************************************
 * @brief 		:load all nodelist from the flash and assign to G_NODE_LIST
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
void fl_nwk_master_nodelist_load(void) {
	fl_nodelist_db_t nodelist = { .num_slave = 0xFF };
	if (fl_db_nodelist_load(&nodelist) && nodelist.num_slave != 0xFF) {
		G_NODE_LIST.slot_inused = nodelist.num_slave;
		for (u8 var = 0; var < nodelist.num_slave; ++var) {
			G_NODE_LIST.sla_info[var].slaveID = nodelist.slave[var].slaveid;
			//G_NODE_LIST.sla_info[var].mac_short.mac_u32 = nodelist.slave[var].mac_u32;
			memcpy(G_NODE_LIST.sla_info[var].mac,nodelist.slave[var].mac,6);
			G_NODE_LIST.sla_info[var].dev_type=nodelist.slave[var].dev_type;
			//update data_struct dev
			memset(G_NODE_LIST.sla_info[var].data,0,SIZEU8(G_NODE_LIST.sla_info[var].data));
			//create MAC + TIMETAMP + DEV_TYPE
			u8 size_mac = SIZEU8(G_NODE_LIST.sla_info[var].mac);
			memcpy(&G_NODE_LIST.sla_info[var].data[0],G_NODE_LIST.sla_info[var].mac,size_mac);
			/*Dev type*/
			G_NODE_LIST.sla_info[var].data[size_mac + 4] = G_NODE_LIST.sla_info[var].dev_type;
		}
	}
}
/***************************************************
 * @brief 		:CLEAR ALL NETWORK
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/

void fl_nwk_master_CLEARALL_NETWORK(void) {
	extern u8 MASTER_CLEARNETWORK[18] ;
	fl_pack_t packet_built;
	fl_data_frame_u packet;
	memset(packet.bytes,0,SIZEU8(packet.bytes));
	memcpy(packet.bytes,MASTER_CLEARNETWORK,SIZEU8(MASTER_CLEARNETWORK));
	memcpy(&packet.bytes[SIZEU8(MASTER_CLEARNETWORK)],blc_ll_get_macAddrPublic(),6);
	packet_built.length = SIZEU8(packet.bytes) - 1; //skip rssi
	memcpy(packet_built.data_arr,packet.bytes,packet_built.length);
	fl_adv_sendFIFO_add(packet_built);
}

/***************************************************
 * @brief 		: excution heartbeat for the network
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
void fl_nwk_master_heartbeat_init(void){
	fl_send_heartbeat();
	blt_soft_timer_add(&fl_send_heartbeat,PERIOD_HEARTBEAT * 1000);
}
void fl_nwk_master_heartbeat_run(void) {
	blt_soft_timer_restart(&fl_send_heartbeat,PERIOD_HEARTBEAT * 1000);
}
/***************************************************
 * @brief 		: collection slave (install mode)
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
void fl_nwk_master_collection_run(void) {
	if (MASTER_INSTALL_STATE) {
		if (blt_soft_timer_find(&fl_send_collection_req) == -1) {
			blt_soft_timer_add(&fl_send_collection_req,50*1000);
			fl_adv_collection_channel_init();
			fl_nwk_master_nodelist_load();
		}
	}
	else
	{
		if (blt_soft_timer_find(&fl_send_collection_req) != -1) {
			blt_soft_timer_delete(&fl_send_collection_req);
			fl_adv_collection_channel_deinit();
			//store nodelist
			fl_nwk_master_nodelist_store();
		}
	}
}

/***************************************************
 * @brief 		:process about monitoring network via sending cmd
 *
 * @param[in] 	:none
 *
 * @return	  	:
 *
 ***************************************************/
void fl_nwk_master_process(void) {
	//install mode
	fl_nwk_master_collection_run();
	//todo: something tasks
}
/***************************************************
 * @brief 		: execute response from nodes
 *
 * @param[in] 	:none
 *
 * @return	  	:
 *
 ***************************************************/
void fl_nwk_master_run(fl_pack_t *_pack_handle) {
	if (FL_QUEUE_FIND(&G_HANDLE_MASTER_CONTAINER,_pack_handle,_pack_handle->length - 2/*skip rssi + repeat_cnt*/) == -1) {
		if (FL_QUEUE_ADD(&G_HANDLE_MASTER_CONTAINER,_pack_handle) < 0) {
			ERR(INF,"Err <QUEUE ADD>-G_HANDLE_MASTER_CONTAINER!!\r\n");
		} else {
			if(fl_master_ProccesRSP_cbk() != -1)
			{
				fl_nwk_master_heartbeat_run();
			}
		}
	}
}
#endif
