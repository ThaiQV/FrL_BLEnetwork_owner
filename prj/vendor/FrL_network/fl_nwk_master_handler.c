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

#ifdef MASTER_CORE
/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
#define TIME_COLLECT_NODE				5*1000 //

#define PACK_HANDLE_MASTER_SIZE 		256
fl_pack_t g_handle_master_array[PACK_HANDLE_MASTER_SIZE];
fl_data_container_t G_HANDLE_MASTER_CONTAINER = {
		.data = g_handle_master_array, .head_index = 0, .tail_index = 0, .mask = PACK_HANDLE_MASTER_SIZE - 1, .count = 0 };

fl_slaves_list_t G_NODE_LIST = { .slot_inused = 0xFF };

volatile u8 MASTER_INSTALL_STATE = 0;
volatile u8 MASTER_GETINNFO_AUTORUN = 0;

//Period of the heartbeat
u16 PERIOD_HEARTBEAT = 0 * 1000; //
//flag debug of the network
volatile u8 NWK_DEBUG_STT = 0; // it will be assigned into endpoint byte (dbg :1bit)
volatile u8 NWK_REPEAT_MODE = 1; // slave repeat?

//For getting automatic information
typedef struct {
	u8 num_sla;
	fl_nodeinnetwork_t* id[10];
}__attribute__((packed)) fl_master_getinfo_pointer_t;

fl_master_getinfo_pointer_t G_SLA_INFO_RSP = { .num_sla = 0 };
/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
u8 fl_master_SlaveID_get(u32 _mac_short);
s16 fl_master_SlaveID_find(u8 _id);
void fl_nwk_master_nodelist_load(void);
void fl_nwk_master_nodelist_store(void);

void fl_master_nodelist_AddRefesh(fl_nodeinnetwork_t _node) {
	for (u8 var = 0; var < MAX_NODES; ++var) {
		if (G_NODE_LIST.sla_info[var].mac_short.mac_u32 != 0) {
			G_NODE_LIST.slot_inused = var + 1;
		} else {
			//debug
			u8 inused = 0xFF; //Empty
			u8 grp_inused = 0xFF;
			u8 mem_inused = 0xFF;
			if (var != 0) {
				inused = G_NODE_LIST.slot_inused;
				grp_inused = G_NODE_LIST.sla_info[var - 1].slaveID.grpID;
				mem_inused = G_NODE_LIST.sla_info[var - 1].slaveID.memID;
			}
			LOGA(FLA,"NODE LIST: %d(grpID:%d,memID:%d)\r\n",inused,grp_inused,mem_inused);
			break;
		}
	}
	if (_node.mac_short.mac_u32 != 0) {
		//update
		u8 slot_ins = G_NODE_LIST.slot_inused++;
		G_NODE_LIST.sla_info[slot_ins] = _node;
		G_NODE_LIST.sla_info[slot_ins].slaveID.id_u8 = slot_ins;
//		G_NODE_LIST.slot_inused++;
		LOGA(FLA,"Update Node [%d]slaveID:%d\r\n",slot_ins,G_NODE_LIST.sla_info[slot_ins].slaveID.id_u8);

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
	fl_pack_t packet_built;
	fl_data_frame_u packet;
	memset(packet.bytes,0,SIZEU8(packet.bytes));
	packet.frame.hdr = NWK_HDR_HEARTBEAT;

	fl_timetamp_withstep_t timetampStep = fl_rtc_getWithMilliStep();
//	u32 timetamp = fl_rtc_get();
	packet.frame.timetamp[0] = U32_BYTE0(timetampStep.timetamp);
	packet.frame.timetamp[1] = U32_BYTE1(timetampStep.timetamp);
	packet.frame.timetamp[2] = U32_BYTE2(timetampStep.timetamp);
	packet.frame.timetamp[3] = U32_BYTE3(timetampStep.timetamp);

	//Add new mill-step
	packet.frame.milltamp = timetampStep.milstep;

	packet.frame.slaveID.id_u8 = 0xFF; // all grps + all members

	memset(packet.frame.payload,0xFF,SIZEU8(packet.frame.payload));

	packet.frame.endpoint.repeat_cnt = 0;
	packet.frame.endpoint.master = FL_FROM_MASTER; //non-ack
	packet.frame.endpoint.dbg = NWK_DEBUG_STT;
	packet.frame.endpoint.rep_mode = NWK_REPEAT_MODE;

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

	packet.frame.slaveID.id_u8 = 0xFF;
	memset(packet.frame.payload,0xFF,SIZEU8(packet.frame.payload));

	packet.frame.endpoint.repeat_cnt = 0;
	packet.frame.endpoint.master = FL_FROM_MASTER_ACK; //ack
	packet.frame.endpoint.dbg = NWK_DEBUG_STT;
	packet.frame.endpoint.rep_mode = NWK_REPEAT_MODE;

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
fl_pack_t fl_master_packet_assignSlaveID_build(u32 _mac_short) {
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

	//Add

	memset(packet.frame.payload,0xFF,SIZEU8(packet.frame.payload));
	packet.frame.payload[0] = U32_BYTE3(_mac_short);
	packet.frame.payload[1] = U32_BYTE2(_mac_short);
	packet.frame.payload[2] = U32_BYTE1(_mac_short);
	packet.frame.payload[3] = U32_BYTE0(_mac_short);

	//Create payload assignment
	packet.frame.slaveID.id_u8 = fl_master_SlaveID_get(_mac_short);

	packet.frame.endpoint.repeat_cnt = 0;
	packet.frame.endpoint.master = FL_FROM_MASTER; //non-ack
	packet.frame.endpoint.dbg = NWK_DEBUG_STT;
	packet.frame.endpoint.rep_mode = NWK_REPEAT_MODE;

	packet_built.length = SIZEU8(packet.bytes) - 1; //skip rssi
	memcpy(packet_built.data_arr,packet.bytes,packet_built.length);
	return packet_built;
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

	//clear lastest time rec
	for (int var = 0; var < _slave_num; ++var) {
		s16 idx = fl_master_SlaveID_find(_slave_mac_arr[var]);
		if (idx != -1) {
			G_NODE_LIST.sla_info[idx].timelife = clock_time();
			G_NODE_LIST.sla_info[idx].active = false;
		}
	}

	/* F5|timetamp|slaveID 1|slaveID 2.......|CRC|TTL */
	/* 1B|  4Bs   |  1Bs  	|     18Bs  	 |1B |1B  */
	memset(packet.bytes,0,SIZEU8(packet.bytes));
	packet.frame.hdr = NWK_HDR_F5_INFO; //testing

	fl_timetamp_withstep_t timetampStep = fl_rtc_getWithMilliStep();
//	u32 timetamp = fl_rtc_get();
	packet.frame.timetamp[0] = U32_BYTE0(timetampStep.timetamp);
	packet.frame.timetamp[1] = U32_BYTE1(timetampStep.timetamp);
	packet.frame.timetamp[2] = U32_BYTE2(timetampStep.timetamp);
	packet.frame.timetamp[3] = U32_BYTE3(timetampStep.timetamp);

	//Add new mill-step
	packet.frame.milltamp = timetampStep.milstep;

	packet.frame.slaveID.id_u8 = 0xFF;

	memset(packet.frame.payload,0xFF,SIZEU8(packet.frame.payload));
	//create payload req
	memcpy(packet.frame.payload,_slave_mac_arr,_slave_num);
	//crc
	packet.frame.crc8 = fl_crc8(packet.frame.payload,SIZEU8(packet.frame.payload));

	packet.frame.endpoint.repeat_cnt = 0;
	packet.frame.endpoint.master = FL_FROM_MASTER_ACK;
	packet.frame.endpoint.dbg = NWK_DEBUG_STT;
	packet.frame.endpoint.rep_mode = NWK_REPEAT_MODE;

	packet_built.length = SIZEU8(packet.bytes) - 1; //skip rssi
	memcpy(packet_built.data_arr,packet.bytes,packet_built.length);
	return packet_built;
}
/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/

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
///
//void fl_master_node_printf(void) {
//	u8 online_cnt = 0;
//	u32 max_time = 0;
//	u8 var = 0;
//	for (var = 0; var < MAX_NODES && G_NODE_LIST[var].mac_short != 0; ++var) {
//		if (G_NODE_LIST[var].active) {
//			//LOGA(APP,"[%d] %08X:%d ms\r\n",var,G_NODE_LIST[var].mac,G_NODE_LIST[var].timelife);
//			online_cnt++;
//			if (max_time < G_NODE_LIST[var].timelife) {
//				max_time = G_NODE_LIST[var].timelife;
//			}
//		} else {
//			//ERR(APP,"%s[%d] %08X:%d ms\r\n",(G_NODE_LIST[var].active) ? "Onl " : "Off",var,G_NODE_LIST[var].mac_short,G_NODE_LIST[var].timelife);
//		}
//	}
//	P_INFO("Online :%d/%d\r\n",online_cnt,var);
//	P_INFO("RspTime:%d ms\r\n",max_time);
//}
s16 fl_master_Node_find(u32 _mac_short) {
	for (u8 var = 0; var < G_NODE_LIST.slot_inused && G_NODE_LIST.slot_inused != 0xFF; ++var) {
		if (G_NODE_LIST.sla_info[var].mac_short.mac_u32 == _mac_short) {
			return var;
		}
	}
	return -1;
}
s16 fl_master_SlaveID_find(u8 _id) {
	for (u8 var = 0; var < G_NODE_LIST.slot_inused && G_NODE_LIST.slot_inused != 0xFF; ++var) {
		if (G_NODE_LIST.sla_info[var].slaveID.id_u8 == _id) {
			return var;
		}
	}
	return -1;
}
u8 fl_master_SlaveID_get(u32 _mac_short) {
	s8 indx = fl_master_Node_find(_mac_short);
	if (indx != -1) {
		return G_NODE_LIST.sla_info[indx].slaveID.id_u8;
	}
	return 0xFF;
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
	datetime_t cur_dt;
	u32 cur_timetamp = fl_rtc_get();
	fl_rtc_timestamp_to_datetime(cur_timetamp,&cur_dt);
//	fl_master_node_printf();
	LOGA(APP,"[%02d/%02d/%02d-%02d:%02d:%02d] HeartBeat Sync(%d ms)\r\n",cur_dt.year,cur_dt.month,cur_dt.day,cur_dt.hour,cur_dt.minute,cur_dt.second,
			PERIOD_HEARTBEAT);
	fl_pack_t packet_built = fl_master_packet_heartbeat_build();
	fl_adv_sendFIFO_add(packet_built);
	return -1; //
}

/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/

/***************************************************
 * @brief 		:soft-timer callback for the processing response
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
int fl_master_ProccesRSP_cbk(void) {
	fl_pack_t data_in_queue;
	if (FL_QUEUE_GET(&G_HANDLE_MASTER_CONTAINER,&data_in_queue)) {
		fl_data_frame_u packet;
		extern u8 fl_packet_parse(fl_pack_t _pack, fl_dataframe_format_t *rslt);
		fl_packet_parse(data_in_queue,&packet.frame);
//		LOGA(INF,"HDR_RSP ID: %02X\r\n",packet.frame.hdr);
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
			case NWK_HDR_F5_INFO: {
				/* F5|timetamp|slaveID| Info |CRC|TTL */
				/* 1B|  4Bs   |  1B   | 18Bs |1B |1B  */
				u8 crc = fl_crc8(packet.frame.payload,SIZEU8(packet.frame.payload));
				s16 node_indx = -1;
				if (crc == packet.frame.crc8) {
					//Todo: parse payload response
					u8 slave_id = packet.frame.slaveID.id_u8;
					node_indx = fl_master_SlaveID_find(slave_id);
					if (node_indx != -1) {
						G_NODE_LIST.sla_info[node_indx].active = true;
						G_NODE_LIST.sla_info[node_indx].timelife = (clock_time() - G_NODE_LIST.sla_info[node_indx].timelife) / SYSTEM_TIMER_TICK_1MS;
						LOGA(INF,"INFO(%d)(%02X)(%d ms)(%d):%s\r\n",packet.frame.endpoint.rep_mode,slave_id,G_NODE_LIST.sla_info[node_indx].timelife,
								packet.frame.endpoint.repeat_cnt,packet.frame.payload);
					} else {
						ERR(INF,"ID not foud:%02X\r\n",slave_id);
					}
				} else {
					ERR(INF,"CRC Fail!!\r\n");
				}
			}
			break;
			case NWK_HDR_COLLECT: {
				u32 mac_short = MAKE_U32(packet.frame.payload[0],packet.frame.payload[1],packet.frame.payload[2],packet.frame.payload[3]);
				s16 node_indx = fl_master_Node_find(mac_short);
				if (node_indx == -1) {
					fl_nodeinnetwork_t new_slave;
					new_slave.active = true;
					new_slave.timelife = clock_time() / SYSTEM_TIMER_TICK_1MS;
					new_slave.mac_short.mac_u32 = mac_short;
					P_PRINTFHEX_A(INF,packet.frame.payload,4,"0x%04X(%d):",mac_short,packet.frame.endpoint.repeat_cnt);
					//assign SlaveID (grpID + memID) to the slave
					fl_master_nodelist_AddRefesh(new_slave);
				}
				//Send assignment to slave
				fl_adv_sendFIFO_add(fl_master_packet_assignSlaveID_build(mac_short));
			}
			break;
			default:
			break;
		}
	} else {
		ERR(INF,"Err <QUEUE GET>-G_HANDLE_MASTER_CONTAINER!!\r\n");
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
	fl_nwk_master_nodelist_load();
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
		nodelist.slave[var].slaveid = G_NODE_LIST.sla_info[var].slaveID.id_u8;
		nodelist.slave[var].mac_u32 = G_NODE_LIST.sla_info[var].mac_short.mac_u32;
	}
	if (nodelist.num_slave && nodelist.num_slave!= 0xFF)
		fl_db_nodelist_save(&nodelist);
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
	fl_nodelist_db_t nodelist = { .num_slave = 0xFF};
	if (fl_db_nodelist_load(&nodelist) && nodelist.num_slave != 0xFF) {
		G_NODE_LIST.slot_inused = nodelist.num_slave;
		for (u8 var = 0; var < nodelist.num_slave; ++var) {
			G_NODE_LIST.sla_info[var].slaveID.id_u8 = nodelist.slave[var].slaveid;
			G_NODE_LIST.sla_info[var].mac_short.mac_u32 = nodelist.slave[var].mac_u32;
		}
	}
}

/***************************************************
 * @brief 		: excution heartbeat for the network
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
void fl_nwk_master_heartbeat_run(void) {
	if (!MASTER_INSTALL_STATE && PERIOD_HEARTBEAT != 0) {
		if (blt_soft_timer_find(&fl_send_heartbeat) < 0) {
			blt_soft_timer_add(&fl_send_heartbeat,PERIOD_HEARTBEAT * 1000);
		}
	} else {
		blt_soft_timer_delete(&fl_send_heartbeat);
	}
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
	static u8 previous_mode = 0;
	if (MASTER_INSTALL_STATE) {
		if (blt_soft_timer_find(&fl_send_collection_req) < 0) {
			LOG_P(APP,"Install mode : On\r\n");
			fl_nwk_master_nodelist_load();
			fl_adv_collection_channel_init();
			blt_soft_timer_add(&fl_send_collection_req,1000 * 1000); // 1s
		}
		previous_mode = MASTER_INSTALL_STATE;
	} else {
		if (previous_mode == 1 && MASTER_INSTALL_STATE == 0) {
			LOG_P(APP,"Install mode : Off\r\n");
			blt_soft_timer_delete(&fl_send_collection_req);
			fl_adv_collection_channel_deinit();
			previous_mode = MASTER_INSTALL_STATE;
			//store nodelist
			fl_nwk_master_nodelist_store();
		}
	}
}
/***************************************************
 * @brief 		:automatically get all information of slaves
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
void fl_nwk_master_getInfo_autorun(void) {
	const u8 max_slave = 8;
	static u8 node_got = 0;
	u8 slave_arr[10];
	fl_pack_t pack;
	u8 indx = 0;
	static u32 test_saving = 0;
	//store time for getting
	static u32 time_start = 0;
	if(MASTER_GETINNFO_AUTORUN == 0){
		goto END_PROCESS;
	}

	if (G_NODE_LIST.slot_inused != 0xFF || !MASTER_INSTALL_STATE) {
		if (node_got == 0) {
			//re-get
			time_start = clock_time();
			//Clear all status of slaves => make sure
			for (u8 stt = 0; stt < G_NODE_LIST.slot_inused; ++stt) {
				G_NODE_LIST.sla_info[stt].active = false;
			}

		} else if (node_got >= G_NODE_LIST.slot_inused && time_start != 0) {
			u8 check = 0;
			for (check = 0; check < G_NODE_LIST.slot_inused; ++check) {
				if (G_NODE_LIST.sla_info[check].active == false)
					break;
			}
			if (check == G_NODE_LIST.slot_inused) {
				P_INFO("Total time:%d ms\r\n",(clock_time()-time_start)/SYSTEM_TIMER_TICK_1MS);
				time_start = 0;
			}
		}
		///
		if (G_SLA_INFO_RSP.num_sla == 0 && node_got < G_NODE_LIST.slot_inused) {
			for (indx = 0; indx < max_slave && ((indx + node_got) < G_NODE_LIST.slot_inused); ++indx) {
				slave_arr[indx] = G_NODE_LIST.sla_info[indx + node_got].slaveID.id_u8;
				G_SLA_INFO_RSP.id[indx] = &G_NODE_LIST.sla_info[indx + node_got];
//				LOGA(INF,"Mac:0x%04X\r\n",G_SLA_INFO_RSP.id[indx]->mac_short);
			}
			G_SLA_INFO_RSP.num_sla = indx;
			pack = fl_master_packet_GetInfo_build(slave_arr,indx);
			fl_adv_sendFIFO_add(pack);
			LOGA(INF,"GetInfo: %d->%d/%d\r\n",node_got,indx + node_got,G_NODE_LIST.slot_inused);
			node_got = node_got + indx;
		} else {
			if (G_SLA_INFO_RSP.num_sla != 0) {
				u8 i = 0;
				for (i = 0; i < G_SLA_INFO_RSP.num_sla; i++) {
					if (G_SLA_INFO_RSP.id[i]->active == false)
						break;
				}
				if (i == G_SLA_INFO_RSP.num_sla)
					G_SLA_INFO_RSP.num_sla = 0;
			}
			//got full slaves
			if (node_got >= G_NODE_LIST.slot_inused) {
				//add timeout period getting
				if (test_saving == 0) {
					test_saving = clock_time();
				} else {
					if (clock_time_exceed(test_saving,MASTER_GETINNFO_AUTORUN * 1000 * 1000)) {
						node_got = 0;
						test_saving = 0;
					}
				}
				//manage time
			}
		}
	} else {
		END_PROCESS:
		node_got = 0;
		test_saving = 0;
		time_start = 0;
		G_SLA_INFO_RSP.num_sla = 0;
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
	//refesh node list
//	fl_input_collection_node_handle(&fl_send_collection_req,TIME_COLLECT_NODE + RAND(-2500,2500));
	//install mode
	fl_nwk_master_collection_run();
	//Heartbeat processor
	fl_nwk_master_heartbeat_run();
	//auto get information
	fl_nwk_master_getInfo_autorun();
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
			fl_master_ProccesRSP_cbk();
		}
	}
	//TEST
//	fl_master_StatusNodePrintf();
}
#endif
