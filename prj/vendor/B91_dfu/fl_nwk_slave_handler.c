/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_nwk_handler.c
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

#include "fl_adv_repeat.h"
#include "fl_adv_proc.h"
#include "fl_nwk_handler.h"
//#include "fl_nwk_protocol.h"

//Test api
#include "test_api.h"

#ifndef MASTER_CORE
/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
/*---------------- Synchronization Master RTC --------------------------*/
_attribute_data_retention_ volatile fl_timetamp_withstep_t ORIGINAL_MASTER_TIME = {.timetamp = 0,.milstep = 0};

#define SYNC_ORIGIN_MASTER(x,y) 			do{	\
												ORIGINAL_MASTER_TIME.timetamp = x;\
												ORIGINAL_MASTER_TIME.milstep = y;\
											}while(0) //Sync original time-master req

#define JOIN_NETWORK_TIME 			30*1000 	//ms
#define RECHECKING_NETWOK_TIME 		1*60*1000 	//ms - 2mins

fl_hdr_nwk_type_e G_NWK_HDR_LIST[] = {NWK_HDR_F5_INFO, NWK_HDR_COLLECT, NWK_HDR_HEARTBEAT,NWK_HDR_ASSIGN }; // register cmdid RSP
fl_hdr_nwk_type_e G_NWK_HDR_REQLIST[] = {NWK_HDR_55,NWK_HDR_RECONNECT}; // register cmdid REQ

#define NWK_HDR_SIZE (sizeof(G_NWK_HDR_LIST)/sizeof(G_NWK_HDR_LIST[0]))
#define NWK_HDR_REQ_SIZE (sizeof(G_NWK_HDR_REQLIST)/sizeof(G_NWK_HDR_REQLIST[0]))

static inline u8 IsREQHDR(fl_hdr_nwk_type_e cmdid) {
	for (u8 i = 0; i < NWK_HDR_REQ_SIZE; i++) {
		if (cmdid == G_NWK_HDR_REQLIST[i]) {
			return 1;
		}
	}
	return 0;
}

static inline fl_timetamp_withstep_t GenerateTimetampField(void){
	fl_timetamp_withstep_t cur_timetamp = fl_rtc_getWithMilliStep();
	u32 mill_sys = fl_rtc_timetamp2milltampStep(cur_timetamp);
	u32 origin_master = fl_rtc_timetamp2milltampStep(ORIGINAL_MASTER_TIME);
	if(mill_sys < origin_master){
		cur_timetamp = ORIGINAL_MASTER_TIME;
	}
	return cur_timetamp;
}

/*---------------- Total Packet handling --------------------------*/

#define PACK_HANDLE_SIZE 		32 // bcs : slave need to rec its req and repeater of the neighbors
fl_pack_t g_handle_array[PACK_HANDLE_SIZE];
fl_data_container_t G_HANDLE_CONTAINER = { .data = g_handle_array, .head_index = 0, .tail_index = 0, .mask = PACK_HANDLE_SIZE - 1, .count = 0 };

//My information
fl_nodeinnetwork_t G_INFORMATION ={.active = true};

//flag debug of the network
volatile u8 NWK_DEBUG_STT = 1; // it will be assigned into end-point byte (dbg :1bit)
//volatile u8 NWK_REPEAT_MODE = 1; // slave repeat?
volatile u8  REPEAT_LEVEL = 3;
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

bool IsJoinedNetwork(void)	{
	return(G_INFORMATION.slaveID.id_u8 != 0xFF);
}
bool IsOnline(void){
	return G_INFORMATION.active;
}

int _nwk_slave_backup(void){
	static u32 pre_crc32 = 0;
	u32 crc32  = fl_db_crc32((u8*)&G_INFORMATION.profile,SLAVE_PROFILE_ENTRY_SIZE);
	if(crc32 != pre_crc32){
		pre_crc32 = crc32;
		fl_db_slaveprofile_save(G_INFORMATION.profile);
		LOGA(INF,"** MAC     :%08X\r\n",G_INFORMATION.mac_short.mac_u32);
		LOGA(INF,"** SlaveID :%d\r\n",G_INFORMATION.slaveID.id_u8);
		LOGA(INF,"** grpID   :%d\r\n",G_INFORMATION.slaveID.grpID);
		LOGA(INF,"** memID   :%d\r\n",G_INFORMATION.slaveID.memID);
		LOGA(INF,"** JoinNWK :%d\r\n",G_INFORMATION.profile.run_stt.join_nwk);
		LOGA(INF,"** RstFac  :%d\r\n",G_INFORMATION.profile.run_stt.rst_factory);
		LOGA(INF,"** Channels:%d |%d |%d\r\n",G_INFORMATION.profile.nwk.chn[0],G_INFORMATION.profile.nwk.chn[1],G_INFORMATION.profile.nwk.chn[2])
	}
	return 0;
}

void fl_nwk_slave_init(void) {
	DEBUG_TURN(NWK_DEBUG_STT);

	FL_QUEUE_CLEAR(&G_HANDLE_CONTAINER,PACK_HANDLE_SIZE);
	//Generate information
	G_INFORMATION.active = false;
	G_INFORMATION.timelife = 0;
	memcpy(G_INFORMATION.mac_short.byte,blc_ll_get_macAddrPublic(),SIZEU8(G_INFORMATION.mac_short.byte));

	//Load from db
	fl_slave_profiles_t my_profile = fl_db_slaveprofile_init();
	G_INFORMATION.slaveID.id_u8 = my_profile.slaveid;
	G_INFORMATION.profile = my_profile;

//	//Test join network + factory
	if (G_INFORMATION.slaveID.id_u8 == 0xFF && G_INFORMATION.profile.slaveid==G_INFORMATION.slaveID.id_u8) {
		G_INFORMATION.profile.run_stt.join_nwk = 1; //access to join network
		G_INFORMATION.profile.run_stt.rst_factory = 1; //has reset factory device
	}
	fl_timetamp_withstep_t cur_timetamp = fl_rtc_getWithMilliStep();
	SYNC_ORIGIN_MASTER(cur_timetamp.timetamp,cur_timetamp.milstep);

	LOG_P(INF,"Freelux network SLAVE init\r\n");
	LOGA(INF,"** MAC    :%08X\r\n",G_INFORMATION.mac_short.mac_u32);
	LOGA(INF,"** SlaveID:%d\r\n",G_INFORMATION.slaveID.id_u8);
	LOGA(INF,"** grpID  :%d\r\n",G_INFORMATION.slaveID.grpID);
	LOGA(INF,"** memID  :%d\r\n",G_INFORMATION.slaveID.memID);
	LOGA(INF,"** JoinNWK:%d\r\n",G_INFORMATION.profile.run_stt.join_nwk);
	LOGA(INF,"** RstFac :%d\r\n",G_INFORMATION.profile.run_stt.rst_factory);


	//test
	if(G_INFORMATION.slaveID.id_u8 == G_INFORMATION.profile.slaveid && G_INFORMATION.slaveID.id_u8 == 0xFF){
		ERR(APP,"Turn on install mode\r\n");
		G_INFORMATION.profile.run_stt.join_nwk = 1;
		G_INFORMATION.profile.run_stt.rst_factory  = 1 ; //has reset factory device
	}

	blt_soft_timer_add(_nwk_slave_backup,2*1000*1000);

	//Interval checking network
	fl_nwk_slave_reconnect();

	//test random send req
//	TEST_slave_sendREQ();
}
/***************************************************
 * @brief 		:synchronization status from packet
 *
 * @param[in] 	:none
 *
 * @return	  	:
 *
 ***************************************************/
void _nwk_slave_syncFromPack(fl_dataframe_format_t *packet){
	u32 master_timetamp; //, slave_timetamp;
	//Synchronize time master
	master_timetamp = MAKE_U32(packet->timetamp[3],packet->timetamp[2],packet->timetamp[1],packet->timetamp[0]);
	datetime_t cur_dt;
	fl_rtc_timestamp_to_datetime(master_timetamp,&cur_dt);
	LOGA(APP,"(%d)MASTER-TIME:%02d/%02d/%02d - %02d:%02d:%02d\r\n",packet->endpoint.dbg,cur_dt.year,cur_dt.month,cur_dt.day,cur_dt.hour,
			cur_dt.minute,cur_dt.second);
	fl_rtc_sync(master_timetamp);
	//Synchronize debug log
	NWK_DEBUG_STT = packet->endpoint.dbg;
//	DEBUG_TURN(NWK_DEBUG_STT);
	//Sync repeat mode
//	NWK_REPEAT_MODE = packet->endpoint.rep_mode;
	//add repeat_cnt
	REPEAT_LEVEL = packet->endpoint.rep_mode;
	//Sync mastertime origin
	SYNC_ORIGIN_MASTER(master_timetamp,packet->milltamp);
	LOGA(INF,"ORIGINAL MASTER-TIME:%d\r\n",ORIGINAL_MASTER_TIME.milstep);
	//Sync network status
	//if(packet->slaveID.id_u8 == G_INFORMATION.slaveID.id_u8)
	{
		G_INFORMATION.active=true;
	}
}

s8 fl_api_slave_req(u8 _cmdid, u8* _data, u8 _len, fl_rsp_callback_fnc _cb, u32 _timeout_ms) {
	//register timeout cb
	if (_cb != 0 && _timeout_ms*1000 >= 2*QUEUQ_REQcRSP_INTERVAL) {
		if(fl_req_slave_packet_createNsend(_cmdid,_data,_len)){
			fl_queueREQcRSP_add(_cmdid,G_INFORMATION.slaveID.id_u8,&_cb,_timeout_ms*1000);
		}
	} else if(_cb == 0 && _timeout_ms ==0){
		return (fl_req_slave_packet_createNsend(_cmdid,_data,_len) == 0?-1:0); // none rsp
	}
	else {
		ERR(API,"Cann't set timeout (%d/%d ms)!!\r\n",(u32)_cb,_timeout_ms);
	}
	return -1;
}

/***************************************************
 * @brief 		: create and send packet request to master via the freelux protocol
 *
 * @param[in] 	: _cmdid : id cmd
 * 				  _data : pointer to data
 * 				  _len  : size data
 *
 * @return	  	: 0: fail, 1: success
 *
 ***************************************************/
bool fl_req_slave_packet_createNsend(u8 _cmdid,u8* _data, u8 _len){
	/*****************************************************************/
	/* | HDR | Timetamp | Mill_time | SlaveID | payload | crc8 | Ep | */
	/* | 1B  |   4Bs    |    1B     |    1B   |   20Bs  |  1B  | 1B | -> .master = FL_FROM_SLAVE_ACK / FL_FROM_SLAVE */
	/*****************************************************************/
	//**todo: Need to convert _data to payload base on special command ID
	//
	fl_pack_t rslt = {.length = 0};
	fl_hdr_nwk_type_e cmdid = (fl_hdr_nwk_type_e)_cmdid;
	if(!IsREQHDR(cmdid)){
		ERR(API,"REQ CMD ID has been registered!!\r\n");
		return false;
	}
	fl_data_frame_u req_pack;
	switch (cmdid) {
		case NWK_HDR_55: {
			LOGA(INF,"Send 55 REQ to Master:%d/%d\r\n",ORIGINAL_MASTER_TIME.timetamp,ORIGINAL_MASTER_TIME.milstep);
			req_pack.frame.hdr = NWK_HDR_55;

//			req_pack.frame.timetamp[0] = U32_BYTE0(ORIGINAL_MASTER_TIME.timetamp);
//			req_pack.frame.timetamp[1] = U32_BYTE1(ORIGINAL_MASTER_TIME.timetamp);
//			req_pack.frame.timetamp[2] = U32_BYTE2(ORIGINAL_MASTER_TIME.timetamp);
//			req_pack.frame.timetamp[3] = U32_BYTE3(ORIGINAL_MASTER_TIME.timetamp);

//			req_pack.frame.milltamp = ORIGINAL_MASTER_TIME.milstep;

			fl_timetamp_withstep_t field_timetamp = GenerateTimetampField();

			req_pack.frame.timetamp[0] = U32_BYTE0(field_timetamp.timetamp);
			req_pack.frame.timetamp[1] = U32_BYTE1(field_timetamp.timetamp);
			req_pack.frame.timetamp[2] = U32_BYTE2(field_timetamp.timetamp);
			req_pack.frame.timetamp[3] = U32_BYTE3(field_timetamp.timetamp);
			req_pack.frame.milltamp = field_timetamp.milstep;

			req_pack.frame.slaveID.id_u8 = G_INFORMATION.slaveID.id_u8;
			//Create payload
			memset(req_pack.frame.payload,0xFF,SIZEU8(req_pack.frame.payload));
			memcpy(req_pack.frame.payload,_data,_len);
			//create endpoint
			req_pack.frame.endpoint.dbg = NWK_DEBUG_STT;
			req_pack.frame.endpoint.repeat_cnt = REPEAT_LEVEL;
			req_pack.frame.endpoint.rep_mode = REPEAT_LEVEL;
			//Create packet from slave
			req_pack.frame.endpoint.master = FL_FROM_SLAVE_ACK;
		}
		break;
		case NWK_HDR_RECONNECT: {

			//LOGA(INF,"Send 55 REQ to Master:%d/%d\r\n",ORIGINAL_MASTER_TIME.timetamp,ORIGINAL_MASTER_TIME.milstep);
			req_pack.frame.hdr = NWK_HDR_RECONNECT;

			fl_timetamp_withstep_t field_timetamp = GenerateTimetampField();

			req_pack.frame.timetamp[0] = U32_BYTE0(field_timetamp.timetamp);
			req_pack.frame.timetamp[1] = U32_BYTE1(field_timetamp.timetamp);
			req_pack.frame.timetamp[2] = U32_BYTE2(field_timetamp.timetamp);
			req_pack.frame.timetamp[3] = U32_BYTE3(field_timetamp.timetamp);
			req_pack.frame.milltamp = field_timetamp.milstep;

			req_pack.frame.slaveID.id_u8 = G_INFORMATION.slaveID.id_u8;
			//Create payload
			memset(req_pack.frame.payload,0xFF,SIZEU8(req_pack.frame.payload));
			memcpy(req_pack.frame.payload,_data,_len);
			//create endpoint
			req_pack.frame.endpoint.dbg = NWK_DEBUG_STT;
			req_pack.frame.endpoint.repeat_cnt = REPEAT_LEVEL;
			req_pack.frame.endpoint.rep_mode = REPEAT_LEVEL;
			//Create packet from slave
			req_pack.frame.endpoint.master = FL_FROM_SLAVE;
		}
		break;
		default:
		break;
	}
	//copy to data struct
	rslt.length = SIZEU8(req_pack.bytes) - 1; //skip rssi
	memcpy(rslt.data_arr,req_pack.bytes,rslt.length );

	P_PRINTFHEX_A(INF,rslt.data_arr,rslt.length,"REQ %X ",_cmdid);

	//Send ADV
	fl_adv_sendFIFO_add(rslt);
	return true;
}
/***************************************************
 * @brief 		: build packet response via the freelux protocol
 *
 * @param[in] 	: none
 *
 * @return	  	: none
 *
 ***************************************************/
fl_pack_t fl_rsp_slave_packet_build(fl_pack_t _pack) {
	fl_pack_t packet_built;
	packet_built.length = 0;
	memset(packet_built.data_arr,0,SIZEU8(packet_built.data_arr));

	fl_data_frame_u packet;
	extern u8 fl_packet_parse(fl_pack_t _pack, fl_dataframe_format_t *rslt);
	if(!fl_packet_parse(_pack,&packet.frame)){
		ERR(INF,"Packet parse fail!!!\r\n");
		packet_built.length = 0;
		return packet_built;
	}
	LOGA(INF,"(%d|%x)HDR_REQ ID: %02X - ACK:%d\r\n",IsJoinedNetwork(),G_INFORMATION.slaveID.id_u8,packet.frame.hdr,packet.frame.endpoint.master);
	switch ((fl_hdr_nwk_type_e) packet.frame.hdr) {
		case NWK_HDR_HEARTBEAT:
			_nwk_slave_syncFromPack(&packet.frame);
			if (packet.frame.endpoint.master == FL_FROM_MASTER_ACK) {
				//Process rsp
				memset(packet.frame.payload,0,SIZEU8(packet.frame.payload));
				memcpy(packet.frame.payload,blc_ll_get_macAddrPublic(),4);
				packet.frame.endpoint.dbg = NWK_DEBUG_STT;
				//change endpoint to node source
				packet.frame.endpoint.master = FL_FROM_SLAVE;
				//add repeat_cnt
				packet.frame.endpoint.repeat_cnt = REPEAT_LEVEL;
			} else {
				//Non-rsp
				packet_built.length = 0;
				return packet_built;
			}
		break;
		case NWK_HDR_F5_INFO: {
			_nwk_slave_syncFromPack(&packet.frame);
			if (packet.frame.endpoint.master == FL_FROM_MASTER_ACK && IsJoinedNetwork()) {
				//Process rsp
				s8 memid_idx = plog_IndexOf(packet.frame.payload,(u8*)&G_INFORMATION.slaveID.id_u8,1,sizeof(packet.frame.payload));
				u32 master_timetamp; //, slave_timetamp;
				master_timetamp = MAKE_U32(packet.frame.timetamp[3],packet.frame.timetamp[2],packet.frame.timetamp[1],packet.frame.timetamp[0]);
				datetime_t cur_dt;
				fl_rtc_timestamp_to_datetime(master_timetamp,&cur_dt);

				fl_timetamp_withstep_t millstep = fl_rtc_getWithMilliStep();
				u8 test_payload[SIZEU8(packet.frame.payload)];
				memset(test_payload,0,sizeof(test_payload));
				sprintf((char*) test_payload,"%02d:%02d:%02d-%03d/%03d",cur_dt.hour,cur_dt.minute,cur_dt.second,millstep.milstep, packet.frame.milltamp);
				LOGA(APP,"(%d)SlaveID:%X | inPack:%X\r\n",memid_idx,G_INFORMATION.slaveID.id_u8,packet.frame.payload[memid_idx]);
				packet.frame.endpoint.dbg = NWK_DEBUG_STT;

				if (memid_idx != -1) {
					packet.frame.slaveID.id_u8 = G_INFORMATION.slaveID.id_u8;
					memset(packet.frame.payload,0,SIZEU8(packet.frame.payload));
					//TEST payload
					memcpy(&packet.frame.payload,test_payload,SIZEU8(test_payload));
					//CRC
					packet.frame.crc8 = fl_crc8(packet.frame.payload,SIZEU8(packet.frame.payload));
				} else {
					packet_built.length = 0;
					return packet_built;
				}
				//change endpoint to node source
				packet.frame.endpoint.master = FL_FROM_SLAVE;
				//add repeat_cnt
				packet.frame.endpoint.repeat_cnt = REPEAT_LEVEL;
			} else {
				//Non-rsp
				packet_built.length = 0;
				return packet_built;
			}
		}
		break;
		case NWK_HDR_COLLECT: {
			_nwk_slave_syncFromPack(&packet.frame);
			if (IsJoinedNetwork() == 0) {
				if (packet.frame.endpoint.master == FL_FROM_MASTER_ACK) {
					//Process rsp
					memset(packet.frame.payload,0,SIZEU8(packet.frame.payload));
					memcpy(packet.frame.payload,G_INFORMATION.mac_short.byte,SIZEU8(G_INFORMATION.mac_short.byte));
					//change endpoint to node source
					packet.frame.endpoint.master = FL_FROM_SLAVE;
					//add repeat_cnt
					packet.frame.endpoint.repeat_cnt = REPEAT_LEVEL;
				}
			} else {
				//Non-rsp
				packet_built.length = 0;
				return packet_built;
			}
		}
		break;
		case NWK_HDR_ASSIGN:
		{
			_nwk_slave_syncFromPack(&packet.frame);
			//Process rsp
			s8 mymac_idx = plog_IndexOf(packet.frame.payload,G_INFORMATION.mac_short.byte,SIZEU8(G_INFORMATION.mac_short.byte),sizeof(packet.frame.payload));
			if (mymac_idx != -1) {
				G_INFORMATION.slaveID.id_u8 = packet.frame.slaveID.id_u8;
				LOGA(INF,"UPDATE SlaveID: %d(grpID:%d|memID:%d)\r\n",G_INFORMATION.slaveID.id_u8,G_INFORMATION.slaveID.grpID,G_INFORMATION.slaveID.memID);
				G_INFORMATION.profile.slaveid = G_INFORMATION.slaveID.id_u8 ;
				G_INFORMATION.profile.run_stt.rst_factory = 0;
				G_INFORMATION.profile.nwk.chn[0] = packet.frame.payload[mymac_idx+SIZEU8(G_INFORMATION.mac_short.byte)];
				G_INFORMATION.profile.nwk.chn[1] = packet.frame.payload[mymac_idx+SIZEU8(G_INFORMATION.mac_short.byte) + 1];
				G_INFORMATION.profile.nwk.chn[2] = packet.frame.payload[mymac_idx+SIZEU8(G_INFORMATION.mac_short.byte) + 2];
				//get master's mac
				G_INFORMATION.profile.nwk.mac_parent = MAKE_U32(packet.frame.payload[7],packet.frame.payload[8],packet.frame.payload[9],packet.frame.payload[10]);
			}
			//debug
			else{
				P_PRINTFHEX_A(INF,G_INFORMATION.mac_short.byte,4,"Mac:");
				P_PRINTFHEX_A(INF,packet.frame.payload,19,"PACK");
			}
			//Non-rsp
			packet_built.length = 0;
			return packet_built;
		}
		break;
		default:
			return packet_built;
		break;
	}

	packet_built.length = SIZEU8(packet.bytes) - 1; //skip rssi
	memcpy(packet_built.data_arr,packet.bytes,packet_built.length);

	return packet_built;
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
int fl_slave_ProccesRSP_cbk(void) {
	fl_pack_t data_in_queue;
	if (FL_QUEUE_GET(&G_HANDLE_CONTAINER,&data_in_queue)) {
		//Scan req call rsp
		fl_queue_REQcRSP_ScanRec(data_in_queue,&G_INFORMATION);
		//process rsp of the protocol
		fl_pack_t packet_build;
		packet_build = fl_rsp_slave_packet_build(data_in_queue);
		if (packet_build.length > 0) {
			P_PRINTFHEX_A(INF,packet_build.data_arr,packet_build.length,"%s:","RSP");
			fl_adv_sendFIFO_add(packet_build);
		}
	} else {
		ERR(INF,"Err <QUEUE GET> - G_HANDLE_CONTAINER!!\r\n");
	}
	return 0;
}
/***************************************************
 * @brief 		:Join-network function
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
int fl_nwk_joinnwk_timeout(void) {
	ERR(INF,"Join-network timeout and re-scan!!!\r\n");
	if (IsJoinedNetwork()) {
		G_INFORMATION.profile.run_stt.join_nwk = 0;
		fl_adv_collection_channel_deinit();
		return -1;
	} else
		return 0;
}
void fl_nwk_slave_joinnwk_exc(void) {
	if (G_INFORMATION.profile.run_stt.join_nwk) {
		if (blt_soft_timer_find(&fl_nwk_joinnwk_timeout) == -1) {
			fl_adv_collection_channel_init();
			blt_soft_timer_add(&fl_nwk_joinnwk_timeout,JOIN_NETWORK_TIME * 1000);
		}
	}
}
/***************************************************
 * @brief 		:Reconnect process - inform to master that re-online (reboot,change anything configurations,...)
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
int fl_nwk_slave_reconnect(void){
	//create timer interval for checking online
	if(blt_soft_timer_find(&fl_nwk_slave_reconnect)==-1){
		blt_soft_timer_add(&fl_nwk_slave_reconnect,RECHECKING_NETWOK_TIME*1000);
	}
	if(IsJoinedNetwork() && G_INFORMATION.active == false){
		LOGA(INF,"Reconnect network (%d s)!!!\r\n",RECHECKING_NETWOK_TIME/1000);
		fl_api_slave_req(NWK_HDR_RECONNECT,G_INFORMATION.mac_short.byte,SIZEU8(G_INFORMATION.mac_short.byte),0,0);
	}
	return 0;
}

/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/
/***************************************************
 * @brief 		:filter adv packet with the Freelux HDR
 *
 * @param[in] 	:pointer pack
 *
 * @return	  	:true / false
 *
 ***************************************************/
bool fl_nwk_slave_checkHDR(u8 _hdr) {
	//scan RSP HDR
	for (int var = 0; var < SIZEU8(G_NWK_HDR_LIST); ++var) {
		if (G_NWK_HDR_LIST[var] == _hdr)
			return true;
	}
	//scan REQ HDR
	for (int var = 0; var < SIZEU8(G_NWK_HDR_REQLIST); ++var) {
		if (G_NWK_HDR_REQLIST[var] == _hdr)
			return true;
	}
	return false;
}
/***************************************************
 * @brief 		: parse data array to get timetamp
 *
 * @param[in] 	: pointer pack
 *
 * @return	  	: 1 if success, 0 otherwise
 *
 ***************************************************/
u32 fl_adv_timetampInPack(fl_pack_t _pack) {
	fl_data_frame_u data_parsed;
	if (_pack.length > 5) {
		memcpy(data_parsed.bytes,_pack.data_arr,_pack.length);
		if (fl_nwk_slave_checkHDR(data_parsed.frame.hdr)) {
			return MAKE_U32(data_parsed.frame.timetamp[3],data_parsed.frame.timetamp[2],data_parsed.frame.timetamp[1],data_parsed.frame.timetamp[0]);
		}
	}
	return 0;
}
/***************************************************
 * @brief 		: parse data array to get timetampmillStep
 *
 * @param[in] 	: pointer pack
 *
 * @return	  	: 1 if success, 0 otherwise
 *
 ***************************************************/
fl_timetamp_withstep_t fl_adv_timetampStepInPack(fl_pack_t _pack) {
	fl_timetamp_withstep_t rslt = {.timetamp = 0, .milstep =0} ;
	fl_data_frame_u data_parsed;
	if (_pack.length > 5) {
		memcpy(data_parsed.bytes,_pack.data_arr,_pack.length);
		if (fl_nwk_slave_checkHDR(data_parsed.frame.hdr)) {
			rslt.timetamp = MAKE_U32(data_parsed.frame.timetamp[3],data_parsed.frame.timetamp[2],data_parsed.frame.timetamp[1],data_parsed.frame.timetamp[0]);
			rslt.milstep = data_parsed.frame.milltamp;
			return rslt;
		}
	}
	return rslt;
}
/***************************************************
 * @brief 		:excute features (join-network, reset factory,...)
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
void fl_nwk_slave_process(void){
	//todo : join- network
	fl_nwk_slave_joinnwk_exc();

}
/***************************************************
 * @brief 		:Main functions to process income packet
 *
 * @param[in] 	:pointer packet
 *
 * @return	  	:none
 *
 ***************************************************/
void fl_nwk_slave_run(fl_pack_t *_pack_handle) {
	if (FL_QUEUE_FIND(&G_HANDLE_CONTAINER,_pack_handle,_pack_handle->length - 2/*skip rssi*/) == -1) { // + repeat_cnt
		if (FL_QUEUE_ADD(&G_HANDLE_CONTAINER,_pack_handle) < 0) {
			ERR(INF,"Err <QUEUE ADD> - G_HANDLE_CONTAINER!!\r\n");
		} else {
			s8 rssi = _pack_handle->data_arr[_pack_handle->length - 1];
			LOGA(INF,"QUEUE HANDLE ADD (len:%d|RSSI:%d): (%d)%d-%d\r\n",_pack_handle->length,rssi,G_HANDLE_CONTAINER.count,
					G_HANDLE_CONTAINER.head_index,G_HANDLE_CONTAINER.tail_index);

			//process rsp of the protocols
			fl_slave_ProccesRSP_cbk();
		}
	} else {
		LOG_P(INF,"Packet has processed!!!\r\n");
	}
}
#endif
