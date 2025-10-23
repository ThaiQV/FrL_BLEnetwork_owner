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
//#include "fl_nwk_protocol.h"
#include "fl_nwk_handler.h"
#include "fl_wifi2ble_fota.h"
//Test api
#include "test_api.h"
#include "../TBS_dev/TBS_dev_config.h"

/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
/*---------------- Synchronization Master RTC --------------------------*/
volatile fl_timetamp_withstep_t ORIGINAL_MASTER_TIME = {.timetamp = 0,.milstep = 0};

#ifndef MASTER_CORE
#define SYNC_ORIGIN_MASTER(x,y) 			do{	\
												ORIGINAL_MASTER_TIME.timetamp = x;\
												ORIGINAL_MASTER_TIME.milstep = y;\
											}while(0) //Sync original time-master req
u8 GETINFO_FLAG_EVENTTEST = 0;
#define JOIN_NETWORK_TIME 			30*1012 			//ms
#define RECHECKING_NETWOK_TIME 		30*1021 		    //ms
#define RECONNECT_TIME				62*1000*1020		//s
#define INFORM_MASTER				5*1001*1004
fl_hdr_nwk_type_e G_NWK_HDR_LIST[] = {NWK_HDR_FOTA,NWK_HDR_A5_HIS,NWK_HDR_F6_SENDMESS,NWK_HDR_F7_RSTPWMETER,NWK_HDR_F8_PWMETER_SET,NWK_HDR_F5_INFO, NWK_HDR_COLLECT, NWK_HDR_HEARTBEAT,NWK_HDR_ASSIGN }; // register cmdid RSP
fl_hdr_nwk_type_e G_NWK_HDR_REQLIST[] = {NWK_HDR_FOTA,NWK_HDR_A5_HIS,NWK_HDR_55,NWK_HDR_11_REACTIVE,NWK_HDR_22_PING}; // register cmdid REQ

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

fl_timetamp_withstep_t GenerateTimetampField(void){
	fl_timetamp_withstep_t cur_timetamp = fl_rtc_getWithMilliStep();
	u32 mill_sys = fl_rtc_timetamp2milltampStep(cur_timetamp);
	u32 origin_master = fl_rtc_timetamp2milltampStep(ORIGINAL_MASTER_TIME);
	if(mill_sys < origin_master){
		cur_timetamp = ORIGINAL_MASTER_TIME;
	}
	cur_timetamp.milstep++;
	return cur_timetamp;
}

/*---------------- Total Packet handling --------------------------*/

fl_pack_t g_handle_array[PACK_HANDLE_SIZE];
fl_data_container_t G_HANDLE_CONTAINER = { .data = g_handle_array, .head_index = 0, .tail_index = 0, .mask = PACK_HANDLE_SIZE - 1, .count = 0 };

//My information
fl_nodeinnetwork_t G_INFORMATION ={.active=false};
#ifndef MASTER_CORE
#ifdef COUNTER_DEVICE
extern tbs_device_counter_t G_COUNTER_DEV ;
extern u8 G_COUNTER_LCD[COUNTER_LCD_MESS_MAX][LCD_MESSAGE_SIZE];
#endif
#ifdef POWER_METER_DEVICE
tbs_device_powermeter_t G_POWER_METER;

#endif
//flag debug of the network
volatile u8 NWK_DEBUG_STT = 0; // it will be assigned into end-point byte (dbg :1bit)
volatile u8 NWK_REPEAT_MODE = 0; // 1: level | 0 : non-level
volatile u8  NWK_REPEAT_LEVEL = 3;
/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
int fl_nwk_joinnwk_timeout(void) ;
int _informMaster(void);
int _slave_reconnect(void);
/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/
u8 fl_nwk_mySlaveID(void){
	return G_INFORMATION.slaveID.id_u8;
}

bool IsJoinedNetwork(void)	{
	return(G_INFORMATION.slaveID.id_u8 != 0xFF);
}
bool IsOnline(void){
	return G_INFORMATION.active;
}
void _Inform11_rsp_callback(void *_data,void* _data2){
	fl_rsp_container_t *data =  (fl_rsp_container_t*)_data;
	LOGA(API,"Timeout:%d\r\n",data->timeout);
	LOGA(API,"cmdID  :%0X\r\n",data->rsp_check.hdr_cmdid);
	LOGA(API,"SlaveID:%0X\r\n",data->rsp_check.slaveID);
	//rsp data
	if(data->timeout > 0){
		blt_soft_timer_delete(_informMaster);
	}else{
//		P_INFO("Master RSP: NON-RSP \r\n");
		blt_soft_timer_restart(_informMaster,INFORM_MASTER);
	}
}

int _informMaster(void){
	if(IsJoinedNetwork()){
		LOGA(INF,"Inform to master (%d s)!!!\r\n",INFORM_MASTER/1000/1000);
#ifdef COUNTER_DEVICE
		fl_api_slave_req(NWK_HDR_11_REACTIVE,(u8*)&G_COUNTER_DEV.data,SIZEU8(G_COUNTER_DEV.data),_Inform11_rsp_callback,0,0);
#else //POWERMETER
		u8 _payload[SIZEU8(tbs_device_powermeter_t)];
		tbs_device_powermeter_t *pwmeter_data = (tbs_device_powermeter_t*) G_INFORMATION.data;
		tbs_pack_powermeter_data(pwmeter_data,_payload);
		u8 indx_data = SIZEU8(pwmeter_data->type) + SIZEU8(pwmeter_data->mac) + SIZEU8(pwmeter_data->timetamp);
		fl_api_slave_req(NWK_HDR_11_REACTIVE,&_payload[indx_data],SIZEU8(pwmeter_data->data),_Inform11_rsp_callback,0,0);
#endif
	}
	return -1;
}
int _isOnline_check(void) {
	ERR(INF,"Device -> offline\r\n");
	G_INFORMATION.active = false;
	blt_soft_timer_restart(_informMaster,INFORM_MASTER);
	return -1;
}

int _nwk_slave_backup(void){
	static u32 pre_crc32 = 0;
	u32 crc32  = fl_db_crc32((u8*)&G_INFORMATION.profile,SLAVE_PROFILE_ENTRY_SIZE);
	if(crc32 != pre_crc32){
		pre_crc32 = crc32;
		fl_db_slaveprofile_save(G_INFORMATION.profile);
		LOGA(FLA,"** MAC     :%02X%02X%02X%02X%02X%02X\r\n",G_INFORMATION.mac[0],G_INFORMATION.mac[1],G_INFORMATION.mac[2],
				G_INFORMATION.mac[3],G_INFORMATION.mac[4],G_INFORMATION.mac[5]);
		LOGA(FLA,"** SlaveID :%d\r\n",G_INFORMATION.slaveID.id_u8);
		LOGA(FLA,"** grpID   :%d\r\n",G_INFORMATION.slaveID.grpID);
		LOGA(FLA,"** memID   :%d\r\n",G_INFORMATION.slaveID.memID);
		LOGA(FLA,"** JoinNWK :%d\r\n",G_INFORMATION.profile.run_stt.join_nwk);
		LOGA(FLA,"** RstFac  :%d\r\n",G_INFORMATION.profile.run_stt.rst_factory);
		LOGA(FLA,"** Channels:%d |%d |%d\r\n",G_INFORMATION.profile.nwk.chn[0],G_INFORMATION.profile.nwk.chn[1],G_INFORMATION.profile.nwk.chn[2])
		LOGA(FLA,"** NWK Key :%s(%02X%02X)\r\n",(G_INFORMATION.profile.nwk.private_key[0] != 0xFF && G_INFORMATION.profile.nwk.private_key[1] != 0xFF )?"*****":"NULL",
				G_INFORMATION.profile.nwk.private_key[0],G_INFORMATION.profile.nwk.private_key[1]);
		LOGA(FLA,"** MAC GW :%02X%02X%02X%02X\r\n",U32_BYTE0( G_INFORMATION.profile.nwk.mac_parent),U32_BYTE1( G_INFORMATION.profile.nwk.mac_parent),
				U32_BYTE2( G_INFORMATION.profile.nwk.mac_parent),U32_BYTE3( G_INFORMATION.profile.nwk.mac_parent));
//		LOGA(INF,"** Key     :0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
//				G_INFORMATION.profile.nwk.private_key[0],G_INFORMATION.profile.nwk.private_key[1],G_INFORMATION.profile.nwk.private_key[2],G_INFORMATION.profile.nwk.private_key[3],
//				G_INFORMATION.profile.nwk.private_key[4],G_INFORMATION.profile.nwk.private_key[5],G_INFORMATION.profile.nwk.private_key[6],G_INFORMATION.profile.nwk.private_key[7],
//				G_INFORMATION.profile.nwk.private_key[8],G_INFORMATION.profile.nwk.private_key[9],G_INFORMATION.profile.nwk.private_key[10],G_INFORMATION.profile.nwk.private_key[11],
//				G_INFORMATION.profile.nwk.private_key[12],G_INFORMATION.profile.nwk.private_key[13],G_INFORMATION.profile.nwk.private_key[14],G_INFORMATION.profile.nwk.private_key[15]);

	}
	return 0;
}

void fl_nwk_slave_init(void) {
	DEBUG_TURN(NWK_DEBUG_STT);
//	fl_input_external_init();
	FL_QUEUE_CLEAR(&G_HANDLE_CONTAINER,PACK_HANDLE_SIZE);
	//Generate information
	G_INFORMATION.active = false;
	G_INFORMATION.timelife = 0;
	memcpy(G_INFORMATION.mac,blc_ll_get_macAddrPublic(),SIZEU8(G_INFORMATION.mac));
	//Load from db
	fl_slave_profiles_t my_profile = fl_db_slaveprofile_init();
	G_INFORMATION.slaveID.id_u8 = my_profile.slaveid;
	G_INFORMATION.profile = my_profile;
//	//Test join network + factory
	if (G_INFORMATION.slaveID.id_u8 == 0xFF && G_INFORMATION.profile.slaveid==G_INFORMATION.slaveID.id_u8)
	{
		G_INFORMATION.profile.run_stt.join_nwk = 1; //access to join network
		G_INFORMATION.profile.run_stt.rst_factory = 1; //has reset factory device
	}
	fl_timetamp_withstep_t cur_timetamp = fl_rtc_getWithMilliStep();
	SYNC_ORIGIN_MASTER(cur_timetamp.timetamp,cur_timetamp.milstep);

#ifdef COUNTER_DEVICE
	G_INFORMATION.dev_type = TBS_COUNTER;
	G_INFORMATION.data =(u8*)&G_COUNTER_DEV;
	for (u8 i = 0; i < COUNTER_LCD_MESS_MAX; i++) {
		G_INFORMATION.lcd_mess[i] = &G_COUNTER_LCD[i][0];
	}
#endif
#ifdef POWER_METER_DEVICE
	G_INFORMATION.dev_type = TBS_POWERMETER;
	G_INFORMATION.data =(u8*)&G_POWER_METER;
#endif
	//todo: TBS Device initialization
	TBS_Device_Init();

	LOG_P(INF,"Freelux network SLAVE init\r\n");
	LOGA(INF,"** MAC     :%02X%02X%02X%02X%02X%02X\r\n",G_INFORMATION.mac[0],G_INFORMATION.mac[1],G_INFORMATION.mac[2],
			G_INFORMATION.mac[3],G_INFORMATION.mac[4],G_INFORMATION.mac[5]);
	LOGA(INF,"** DevType:%d\r\n",G_INFORMATION.dev_type);
	LOGA(INF,"** SlaveID:%d\r\n",G_INFORMATION.slaveID.id_u8);
	LOGA(INF,"** grpID  :%d\r\n",G_INFORMATION.slaveID.grpID);
	LOGA(INF,"** memID  :%d\r\n",G_INFORMATION.slaveID.memID);
	LOGA(INF,"** JoinNWK:%d\r\n",G_INFORMATION.profile.run_stt.join_nwk);
	LOGA(INF,"** RstFac :%d\r\n",G_INFORMATION.profile.run_stt.rst_factory);
	LOGA(INF,"** MAC GW :%02X%02X%02X%02X\r\n",U32_BYTE0( G_INFORMATION.profile.nwk.mac_parent),U32_BYTE1( G_INFORMATION.profile.nwk.mac_parent),
			U32_BYTE2( G_INFORMATION.profile.nwk.mac_parent),U32_BYTE3( G_INFORMATION.profile.nwk.mac_parent));
	//test
	if(G_INFORMATION.slaveID.id_u8 == G_INFORMATION.profile.slaveid && G_INFORMATION.slaveID.id_u8 == 0xFF){
		ERR(APP,"Turn on install mode\r\n");
		G_INFORMATION.profile.run_stt.join_nwk = 1;
		G_INFORMATION.profile.run_stt.rst_factory  = 1 ; //has reset factory device
	}

	blt_soft_timer_add(_nwk_slave_backup,2*1000*1000);
	//Interval checking network
	fl_nwk_slave_reconnectNstoragedata();
	//inform to master I'm online
//	blt_soft_timer_add(&_slave_reconnect,2*1000*1000);
	blt_soft_timer_add(&_informMaster,INFORM_MASTER);
	G_INFORMATION.active = false;
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
	DEBUG_TURN(NWK_DEBUG_STT);
	//Sync repeat mode
	NWK_REPEAT_MODE = packet->endpoint.repeat_mode;
	//add repeat_cnt
	NWK_REPEAT_LEVEL = packet->endpoint.rep_settings;

	//Sync mastertime origin
	//if (packet->hdr == NWK_HDR_HEARTBEAT)
	{
		SYNC_ORIGIN_MASTER(master_timetamp,packet->milltamp);
		LOGA(INF,"ORIGINAL MASTER-TIME:%d\r\n",ORIGINAL_MASTER_TIME.milstep);
	}
	//Sync network status
	//if(packet->slaveID.id_u8 == G_INFORMATION.slaveID.id_u8)
	{
		G_INFORMATION.active = true;
		blt_soft_timer_restart(&_isOnline_check,RECHECKING_NETWOK_TIME * 1000);
	}
}

s8 fl_api_slave_req(u8 _cmdid, u8* _data, u8 _len, fl_rsp_callback_fnc _cb, u32 _timeout_ms,u8 _retry) {
	//register timeout cb
	if (_cb != 0 && ( _timeout_ms*1000 >= 2*QUEUQ_REQcRSP_INTERVAL || _timeout_ms==0)) {
		u32 seq_timetamp=fl_req_slave_packet_createNsend(_cmdid,_data,_len);
		if(seq_timetamp){
			return fl_queueREQcRSP_add(G_INFORMATION.slaveID.id_u8,_cmdid,seq_timetamp,_data,_len,&_cb,_timeout_ms,_retry);
		}
	} else if(_cb == 0 && _timeout_ms ==0){
		return (fl_req_slave_packet_createNsend(_cmdid,_data,_len) == 0?-1:0); // none rsp
	}
	ERR(API,"Can't register REQ (%d/%d ms)!!\r\n",(u32)_cb,_timeout_ms);
	return -1;
}

/***************************************************
 * @brief 		: create and send packet request to master via the freelux protocol
 *
 * @param[in] 	: _cmdid : id cmd
 * 				  _data : pointer to data
 * 				  _len  : size data
 *
 * @return	  	: 0: fail otherwise seq_timetamp
 *
 ***************************************************/
u32 fl_req_slave_packet_createNsend(u8 _cmdid,u8* _data, u8 _len){
	/*****************************************************************/
	/* | HDR | Timetamp | Mill_time | SlaveID | payload | crc8 | Ep | */
	/* | 1B  |   4Bs    |    1B     |    1B   |   20Bs  |  1B  | 1B | -> .master = FL_FROM_SLAVE_ACK / FL_FROM_SLAVE */
	/*****************************************************************/
	//**todo: Need to convert _data to payload base on special command ID
	fl_pack_t rslt = {.length = 0};
	fl_hdr_nwk_type_e cmdid = (fl_hdr_nwk_type_e)_cmdid;
	if(!IsREQHDR(cmdid)){
		ERR(API,"REQ CMD ID hasn't been registered!!\r\n");
		return false;
	}
	fl_data_frame_u req_pack;
	switch (cmdid) {
		case NWK_HDR_55: {
			LOGA(INF,"Send 55 REQ to Master:%d/%d\r\n",ORIGINAL_MASTER_TIME.timetamp,ORIGINAL_MASTER_TIME.milstep);
			req_pack.frame.hdr = NWK_HDR_55;

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
			//crc
			req_pack.frame.crc8 = fl_crc8(req_pack.frame.payload,SIZEU8(req_pack.frame.payload));

			//create endpoint
			req_pack.frame.endpoint.dbg = NWK_DEBUG_STT;
			req_pack.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;
			req_pack.frame.endpoint.rep_settings = NWK_REPEAT_LEVEL;
			req_pack.frame.endpoint.repeat_mode = NWK_REPEAT_MODE;
			//Create packet from slave
			req_pack.frame.endpoint.master = FL_FROM_SLAVE_ACK;
			//tbs index manage
			TBS_Device_Index_manage(NWK_HDR_55);
		}
		break;
		case NWK_HDR_11_REACTIVE: {

			req_pack.frame.hdr = NWK_HDR_11_REACTIVE;

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
			//crc
			req_pack.frame.crc8 = fl_crc8(req_pack.frame.payload,SIZEU8(req_pack.frame.payload));
			//create endpoint
			req_pack.frame.endpoint.dbg = NWK_DEBUG_STT;
			req_pack.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;
			req_pack.frame.endpoint.rep_settings = NWK_REPEAT_LEVEL;
			req_pack.frame.endpoint.repeat_mode = NWK_REPEAT_MODE;
			//Create packet from slave
			req_pack.frame.endpoint.master = FL_FROM_SLAVE_ACK;
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
	fl_timetamp_withstep_t  timetamp_inpack = fl_adv_timetampStepInPack(rslt);
	u32 seq_timetamp = fl_rtc_timetamp2milltampStep(timetamp_inpack);
	//Synch original time
	SYNC_ORIGIN_MASTER(timetamp_inpack.timetamp,timetamp_inpack.milstep);
	return seq_timetamp;
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
	//check crc_pack
	u8 crc = fl_crc8(packet.frame.payload,SIZEU8(packet.frame.payload));
	if(crc != packet.frame.crc8){
		ERR(INF,"ERR CRC 0x%02X | 0x%02X\r\n",packet.frame.crc8,crc);
		packet_built.length = 0;
		return packet_built;
	}
	LOGA(INF,"(%d|%x)HDR_REQ ID: %02X - ACK:%d\r\n",IsJoinedNetwork(),G_INFORMATION.slaveID.id_u8,packet.frame.hdr,packet.frame.endpoint.master);

	switch ((fl_hdr_nwk_type_e) packet.frame.hdr) {
		case NWK_HDR_HEARTBEAT:
			_nwk_slave_syncFromPack(&packet.frame);
			GETINFO_FLAG_EVENTTEST = packet.frame.payload[0];
			if (packet.frame.endpoint.master == FL_FROM_MASTER_ACK) {
				//Process rsp
				memset(packet.frame.payload,0,SIZEU8(packet.frame.payload));
				memcpy(packet.frame.payload,blc_ll_get_macAddrPublic(),6);
				packet.frame.endpoint.dbg = NWK_DEBUG_STT;
				//change endpoint to node source
				packet.frame.endpoint.master = FL_FROM_SLAVE;
				//add repeat_cnt
				packet.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;
			} else {
				//Non-rsp
				packet_built.length = 0;
				return packet_built;
			}
		break;
		case NWK_HDR_F5_INFO: {
//			_nwk_slave_syncFromPack(&packet.frame);
			if (packet.frame.endpoint.master == FL_FROM_MASTER_ACK && IsJoinedNetwork()) {
				//Process rsp
				s8 memid_idx = plog_IndexOf(packet.frame.payload,(u8*)&G_INFORMATION.slaveID.id_u8,1,sizeof(packet.frame.payload)-1); //skip lastbyte int the payload
				u32 master_timetamp = MAKE_U32(packet.frame.timetamp[3],packet.frame.timetamp[2],packet.frame.timetamp[1],packet.frame.timetamp[0]);
				datetime_t cur_dt;
				fl_rtc_timestamp_to_datetime(master_timetamp,&cur_dt);
				u8 _payload[POWER_METER_STRUCT_BYTESIZE];
				memset(_payload,0xFF,SIZEU8(_payload));
				//u8 len_payload=0;
				LOGA(APP,"(%d)SlaveID:%X | inPack:%X | TestEvent:%d\r\n",memid_idx,G_INFORMATION.slaveID.id_u8,packet.frame.payload[memid_idx],GETINFO_FLAG_EVENTTEST);
				packet.frame.endpoint.dbg = NWK_DEBUG_STT;
				u8 indx_data = 0;
				if (memid_idx != -1) {
					fl_rtc_sync(master_timetamp);
#ifdef COUNTER_DEVICE
					tbs_device_counter_t *counter_data = (tbs_device_counter_t*)G_INFORMATION.data;
					counter_data->timetamp = master_timetamp;
					memcpy(_payload,(u8*)&counter_data->data,SIZEU8(counter_data->data));
					tbs_counter_printf(APP,(void*)counter_data);
#endif
#ifdef POWER_METER_DEVICE
					tbs_power_meter_printf(APP,(void*)G_INFORMATION.data);
					tbs_device_powermeter_t *pwmeter_data = (tbs_device_powermeter_t*)G_INFORMATION.data;
					pwmeter_data->timetamp = master_timetamp;
					tbs_pack_powermeter_data(pwmeter_data,_payload);
					indx_data = SIZEU8(pwmeter_data->type) + SIZEU8(pwmeter_data->mac) + SIZEU8(pwmeter_data->timetamp);
#endif
					packet.frame.slaveID.id_u8 = G_INFORMATION.slaveID.id_u8;
					memset(packet.frame.payload,0,SIZEU8(packet.frame.payload));
					memcpy(&packet.frame.payload,&_payload[indx_data],SIZEU8(packet.frame.payload));
					//CRC
					packet.frame.crc8 = fl_crc8(packet.frame.payload,SIZEU8(packet.frame.payload));
					//increase index tbs
					TBS_Device_Index_manage(NWK_HDR_F5_INFO);
					//start period storage data
					fl_nwk_slave_reconnectNstoragedata();
				} else {
					packet_built.length = 0;
					return packet_built;
				}
				//change endpoint to node source
				packet.frame.endpoint.master = FL_FROM_SLAVE;
				//add repeat_cnt
				packet.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;
			} else {
				//Non-rsp
				packet_built.length = 0;
				return packet_built;
			}
		}
		break;
		case NWK_HDR_A5_HIS:{
			if (IsJoinedNetwork()) {
				if(packet.frame.slaveID.id_u8 == G_INFORMATION.slaveID.id_u8){
					//get mac
					u8 mac[6];
					memcpy(mac,&packet.frame.payload[0],SIZEU8(mac));
					u8 ind_value = SIZEU8(mac);
					//get from_index and to_index
					u16 from_index = MAKE_U32(packet.frame.payload[ind_value+3],packet.frame.payload[ind_value+2],packet.frame.payload[ind_value+1],packet.frame.payload[ind_value]);
					ind_value = ind_value+4;
					u16 to_index =MAKE_U32(packet.frame.payload[ind_value+3],packet.frame.payload[ind_value+2],packet.frame.payload[ind_value+1],packet.frame.payload[ind_value]);
					TBS_History_Get((u16)from_index,(u16)to_index);
				}
				//Non-rsp
				packet_built.length = 0;
				return packet_built;
			}
			break;
		}
		case NWK_HDR_F6_SENDMESS: {
//			_nwk_slave_syncFromPack(&packet.frame);
			if (IsJoinedNetwork()) {
#ifdef COUNTER_DEVICE
				//check packet_slaveid
				if(packet.frame.slaveID.id_u8 == G_INFORMATION.slaveID.id_u8){
					u8 slot_indx = packet.frame.payload[0];
					memset(G_INFORMATION.lcd_mess[slot_indx],0,SIZEU8(G_INFORMATION.lcd_mess[slot_indx]));
					memcpy(G_INFORMATION.lcd_mess[slot_indx],&packet.frame.payload[1],SIZEU8(packet.frame.payload)-1);
					G_INFORMATION.lcd_mess[slot_indx][LCD_MESSAGE_SIZE-1] = 1; //update lsb f_new =1

//					u8 slot_indx = packet.frame.payload[0];
//					memset(G_INFORMATION.lcd_mess[slot_indx],0,SIZEU8(G_INFORMATION.lcd_mess[slot_indx]));
//					memcpy(G_INFORMATION.lcd_mess[slot_indx],&packet.frame.payload[1],SIZEU8(packet.frame.payload) -1);

					if(packet.frame.endpoint.master == FL_FROM_MASTER_ACK){
						u8 ok[2] = {'o','k'};
						memset(packet.frame.payload,0,SIZEU8(packet.frame.payload));
						memcpy(packet.frame.payload,ok,SIZEU8(ok));
						//change endpoint to node source
						packet.frame.endpoint.master = FL_FROM_SLAVE;
						//add repeat_cnt
						packet.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;

					}
					else{
						//Non-rsp
						packet_built.length = 0;
						return packet_built;
					}
				}
#endif
			}
		}
		break;
		case NWK_HDR_F7_RSTPWMETER: {
			if (IsJoinedNetwork()) {
#ifdef POWER_METER_DEVICE
				//check packet_slaveid
				if (packet.frame.slaveID.id_u8 == G_INFORMATION.slaveID.id_u8) {
					TBS_PowerMeter_RESETbyMaster(packet.frame.payload[0],packet.frame.payload[1],packet.frame.payload[2]);
					if (packet.frame.endpoint.master == FL_FROM_MASTER_ACK) {
						u8 ok[2] = { 'o', 'k' };
						memset(packet.frame.payload,0,SIZEU8(packet.frame.payload));
						memcpy(packet.frame.payload,ok,SIZEU8(ok));
						//change endpoint to node source
						packet.frame.endpoint.master = FL_FROM_SLAVE;
						//add repeat_cnt
						packet.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;
					} else {
						//Non-rsp
						packet_built.length = 0;
						return packet_built;
					}
				}
#endif
			}
		}
		break;
		case NWK_HDR_F8_PWMETER_SET: {
			if (IsJoinedNetwork()) {
#ifdef POWER_METER_DEVICE
				//check packet_slaveid
				if (packet.frame.slaveID.id_u8 == G_INFORMATION.slaveID.id_u8) {
					u16 chn1 = MAKE_U16(packet.frame.payload[1],packet.frame.payload[0]);
					u16 chn2= MAKE_U16(packet.frame.payload[3],packet.frame.payload[2]);
					u16 chn3= MAKE_U16(packet.frame.payload[5],packet.frame.payload[4]);
					TBS_PwMeter_SetThreshod(chn1,chn2,chn3);
					if (packet.frame.endpoint.master == FL_FROM_MASTER_ACK) {
						u8 ok[2] = { 'o', 'k' };
						memset(packet.frame.payload,0,SIZEU8(packet.frame.payload));
						memcpy(packet.frame.payload,ok,SIZEU8(ok));
						//change endpoint to node source
						packet.frame.endpoint.master = FL_FROM_SLAVE;
						//add repeat_cnt
						packet.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;

					} else {
						//Non-rsp
						packet_built.length = 0;
						return packet_built;
					}
				}
#endif
			}
		}
		break;
		case NWK_HDR_22_PING: {
			if (IsJoinedNetwork()) {
				//check packet_slaveid
				if (packet.frame.slaveID.id_u8 == G_INFORMATION.slaveID.id_u8) {
					if (packet.frame.endpoint.master == FL_FROM_MASTER_ACK) {
						u8 ok[2] = { 'o', 'k' };
						memset(packet.frame.payload,0,SIZEU8(packet.frame.payload));
						memcpy(packet.frame.payload,ok,SIZEU8(ok));
						//change endpoint to node source
						packet.frame.endpoint.master = FL_FROM_SLAVE;
						//add repeat_cnt
						packet.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;
					} else {
						//Non-rsp
						packet_built.length = 0;
						return packet_built;
					}
				}
			}
		}
		break;
		/*============================================================================================*/
		case NWK_HDR_COLLECT: {
//			_nwk_slave_syncFromPack(&packet.frame);
			if (IsJoinedNetwork() == 0) {
				if (packet.frame.endpoint.master == FL_FROM_MASTER_ACK) {
					//get master's mac
					G_INFORMATION.profile.nwk.mac_parent = MAKE_U32(packet.frame.payload[3],packet.frame.payload[2],packet.frame.payload[1],packet.frame.payload[0]);
					//Process rsp
					memset(packet.frame.payload,0,SIZEU8(packet.frame.payload));
					memcpy(packet.frame.payload,G_INFORMATION.mac,SIZEU8(G_INFORMATION.mac));
					packet.frame.payload[SIZEU8(G_INFORMATION.mac)] = G_INFORMATION.dev_type;
					//change endpoint to node source
					packet.frame.endpoint.master = FL_FROM_SLAVE;
					//add repeat_cnt
					packet.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;
				}
			} else {
				//Joined network -> exit collection mode if the master stopped broadcast Collection packet
				blt_soft_timer_restart(fl_nwk_joinnwk_timeout,3*1000*1000); //exit after 3s
				//Non-rsp
				packet_built.length = 0;
				return packet_built;
			}
		}
		break;
		case NWK_HDR_ASSIGN:
		{
//			_nwk_slave_syncFromPack(&packet.frame);
			//Process rsp
			s8 mymac_idx = plog_IndexOf(packet.frame.payload,G_INFORMATION.mac,SIZEU8(G_INFORMATION.mac),sizeof(packet.frame.payload));
			if (mymac_idx != -1) {
				G_INFORMATION.slaveID.id_u8 = packet.frame.slaveID.id_u8;
				LOGA(INF,"UPDATE SlaveID: %d(grpID:%d|memID:%d)\r\n",G_INFORMATION.slaveID.id_u8,G_INFORMATION.slaveID.grpID,G_INFORMATION.slaveID.memID);
				G_INFORMATION.profile.slaveid = G_INFORMATION.slaveID.id_u8 ;
				G_INFORMATION.profile.run_stt.rst_factory = 0;
				G_INFORMATION.profile.nwk.chn[0] = packet.frame.payload[mymac_idx+SIZEU8(G_INFORMATION.mac)];
				G_INFORMATION.profile.nwk.chn[1] = packet.frame.payload[mymac_idx+SIZEU8(G_INFORMATION.mac) + 1];
				G_INFORMATION.profile.nwk.chn[2] = packet.frame.payload[mymac_idx+SIZEU8(G_INFORMATION.mac) + 2];
				//Get private_key
				memcpy(G_INFORMATION.profile.nwk.private_key,&packet.frame.payload[mymac_idx+SIZEU8(G_INFORMATION.mac) + 3],NWK_PRIVATE_KEY_SIZE);
			}
			//debug
			else{
				P_PRINTFHEX_A(INF,G_INFORMATION.mac,SIZEU8(G_INFORMATION.mac),"Mac:");
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
	//crc
	packet.frame.crc8 = fl_crc8(packet.frame.payload,SIZEU8(packet.frame.payload));
	packet_built.length = SIZEU8(packet.bytes) - 1; //skip rssi
	memcpy(packet_built.data_arr,packet.bytes,packet_built.length);

	return packet_built;
}
/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/
fl_pack_t fl_slave_fota_rsp_packet_build(u8* _data, u8 _len,fl_data_frame_u _REQpack){
	/**************************************************************************/
	/* | HDR | Timetamp | Mill_time | SlaveID | payload | crc8_payload | Ep | */
	/* | 1B  |   4Bs    |    1B     |    1B   |   22Bs  |   	1B	   | 1B | -> .master = FL_FROM_SLAVE */
	/**************************************************************************/
	fl_pack_t rslt = {.length = 0};
	fl_data_frame_u rsp_pack;
	/*Create common packet */
	rsp_pack.frame.hdr = NWK_HDR_FOTA;
	memcpy(rsp_pack.frame.timetamp,_REQpack.frame.timetamp,SIZEU8(rsp_pack.frame.timetamp));
	//Add new mill-step
	rsp_pack.frame.milltamp = _REQpack.frame.milltamp;

	rsp_pack.frame.slaveID.id_u8 = G_INFORMATION.slaveID.id_u8;
	//Create payload
	memset(rsp_pack.frame.payload,0x0,SIZEU8(rsp_pack.frame.payload));
	memcpy(rsp_pack.frame.payload,_data,_len);
	//crc
	rsp_pack.frame.crc8 = fl_crc8(rsp_pack.frame.payload,SIZEU8(rsp_pack.frame.payload));

	//create endpoint => always set below
	rsp_pack.frame.endpoint = _REQpack.frame.endpoint;
	//Create packet from slave
	rsp_pack.frame.endpoint.master = FL_FROM_SLAVE;

	//copy to resutl data struct
	rslt.length = SIZEU8(rsp_pack.bytes) - 1; //skip rssi
	memcpy(rslt.data_arr,rsp_pack.bytes,rslt.length );
//	LOGA(FILE,"Send %02X REQ to Slave %d:%d/%d\r\n",rsp_pack.frame.hdr,slaveID,timetampStep.timetamp,timetampStep.milstep);
	P_PRINTFHEX_A(INF_FILE,rslt.data_arr,rslt.length,"RSP %X:",rsp_pack.frame.hdr);
	return rslt;
}

void fl_slave_fota_proc(fl_pack_t _fota_pack){
	static u32 count_echo=0;
	static u8 flag_begin_end=0;
	static u32 rtt=0;
	static u32 fw_size=0;
	extern u8 fl_packet_parse(fl_pack_t _pack, fl_dataframe_format_t *rslt);
	fl_dataframe_format_t packet;
	if(!fl_packet_parse(_fota_pack,&packet)){
		ERR(INF,"Packet parse fail!!!\r\n");
		return;
	}
	if (packet.hdr == NWK_HDR_FOTA) {
		if(packet.endpoint.master == FL_FROM_MASTER_ACK){
			//TEST
//			u8 version_typefw[4]={'1','2','3',G_INFORMATION.dev_type};
//			fl_adv_sendFIFO_add(fl_slave_fota_rsp_packet_build(version_typefw,SIZEU8(version_typefw),packet));
		}else{
			u8 OTA_BEGIN[3] = { 0, G_INFORMATION.dev_type, 2 };
			u8 OTA_END[3] = { 2, G_INFORMATION.dev_type, 2 };
			if (plog_IndexOf(packet.payload,OTA_BEGIN,SIZEU8(OTA_BEGIN),SIZEU8(OTA_BEGIN)) != -1) {
				count_echo = 0;
				flag_begin_end = 1;
				rtt = fl_rtc_get();
				fw_size = MAKE_U32(0,packet.payload[5],packet.payload[4],packet.payload[3]);
				DFU_OTA_CRC128_INIT();
				P_INFO("\r\n============ FOTA BEGIN ============ \r\n");

			} else if (flag_begin_end && plog_IndexOf(packet.payload,OTA_END,SIZEU8(OTA_END),SIZEU8(OTA_END)) != -1) {
				P_INFO("\r\n============ FOTA END ==============\r\n");
				u8 crc128[16];
				memcpy(crc128,DFU_OTA_CRC128_GET(),16);
				P_INFO_HEX(crc128,16,"** CRC      :");
				P_INFO_HEX(packet.payload+6,16,"** CRC CHECK:");
				P_INFO("** File     : %d/%d (%d)\r\n",count_echo*16,fw_size,count_echo);
				P_INFO("** RTT      : %d s\r\n",(u32)(fl_rtc_get()-rtt));
				P_INFO("=====================================\r\n");
				count_echo = 0;
				flag_begin_end = 0;
			} else {
				if (flag_begin_end) {
					DFU_OTA_CRC128_CAL(&packet.payload[6]);
					count_echo++;
					P_INFO("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
					P_INFO("Downloading.......%d",16*count_echo);
				}
			}
		}
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
int fl_slave_ProccesRSP_cbk(void) {
	fl_pack_t data_in_queue;
	if (FL_QUEUE_GET(&G_HANDLE_CONTAINER,&data_in_queue)) {
		//Scan req call rsp
		if(-1!=fl_queue_REQcRSP_ScanRec(data_in_queue,&G_INFORMATION))
		{
			LOGA(API,"Refesh online status (%d ms)\r\n",RECHECKING_NETWOK_TIME);
			blt_soft_timer_restart(&_isOnline_check,RECHECKING_NETWOK_TIME * 1000);
		}
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
	if (IsJoinedNetwork()) {
		G_INFORMATION.profile.run_stt.join_nwk = 0;
		fl_adv_collection_channel_deinit();
		return -1;
	} else {
		ERR(INF,"Join-network timeout and re-scan!!!\r\n");
		return 0;
	}
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
int _slave_reconnect(void){
	if(IsJoinedNetwork()){
		LOGA(INF,"Reconnect network (%d s)!!!\r\n",RECONNECT_TIME/1000/1000);
		//
#ifdef COUNTER_DEVICE
		fl_api_slave_req(NWK_HDR_55,(u8*)&G_COUNTER_DEV.data,SIZEU8(G_COUNTER_DEV.data),0,0,0);
#else //POWERMETER
		u8 _payload[SIZEU8(tbs_device_powermeter_t)];
		tbs_device_powermeter_t *pwmeter_data = (tbs_device_powermeter_t*) G_INFORMATION.data;
		tbs_pack_powermeter_data(pwmeter_data,_payload);
		u8 indx_data = SIZEU8(pwmeter_data->type) + SIZEU8(pwmeter_data->mac) + SIZEU8(pwmeter_data->timetamp);
		fl_api_slave_req(NWK_HDR_55,&_payload[indx_data],SIZEU8(pwmeter_data->data),0,0,0);
#endif
		return RECONNECT_TIME;
	}
	return 0;
}

void fl_nwk_slave_reconnectNstoragedata(void){
	//Restart timeout reconnect
	blt_soft_timer_restart(_slave_reconnect,RECONNECT_TIME);
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
	//todo TBS_device process
	TBS_Device_Run();
	//For debuging
	static bool debug_on_offline = false;
	if(debug_on_offline != G_INFORMATION.active){
		debug_on_offline = G_INFORMATION.active;
		ERR(INF,"Device -> %s\r\n",debug_on_offline==true?"Online":"Offline");
	}
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
#endif
