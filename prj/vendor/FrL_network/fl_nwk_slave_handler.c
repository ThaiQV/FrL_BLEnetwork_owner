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
#include "fl_nwk_protocol.h"
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
extern volatile fl_timetamp_withstep_t WIFI_ORIGINAL_GETALL;

#ifndef MASTER_CORE
#define SYNC_ORIGIN_MASTER(x,y) 			do{	\
												ORIGINAL_MASTER_TIME.timetamp = x;\
												ORIGINAL_MASTER_TIME.milstep = y;\
											}while(0) //Sync original time-master req
u8 GETINFO_FLAG_EVENTTEST = 0;
#define JOIN_NETWORK_TIME 			60*1012 			//ms
#define RECHECKING_NETWOK_TIME 		14*1021 		    //ms
#define RECONNECT_TIME				62*1000*1020		//s
#define INFORM_MASTER				9*1001*1004
fl_hdr_nwk_type_e G_NWK_HDR_LIST[] = {NWK_HDR_NODETALBE_UPDATE,NWK_HDR_REMOVE,NWK_HDR_MASTER_CMD,NWK_HDR_FOTA,NWK_HDR_A5_HIS,NWK_HDR_F6_SENDMESS,NWK_HDR_F7_RSTPWMETER,NWK_HDR_F8_PWMETER_SET,NWK_HDR_F5_INFO, NWK_HDR_COLLECT, NWK_HDR_HEARTBEAT,NWK_HDR_ASSIGN }; // register cmdid RSP
fl_hdr_nwk_type_e G_NWK_HDR_REQLIST[] = {NWK_HDR_NODETALBE_UPDATE,NWK_HDR_REMOVE,NWK_HDR_MASTER_CMD,NWK_HDR_FOTA,NWK_HDR_A5_HIS,NWK_HDR_55,NWK_HDR_11_REACTIVE,NWK_HDR_22_PING}; // register cmdid REQ

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
	u64 mill_sys = fl_rtc_timetamp2milltampStep(cur_timetamp);
	u64 origin_master = fl_rtc_timetamp2milltampStep(ORIGINAL_MASTER_TIME);
	if(mill_sys < origin_master){
		cur_timetamp = ORIGINAL_MASTER_TIME;
	}
	cur_timetamp.milstep++;
	return cur_timetamp;
}

void fl_nwk_slave_SYNC_ORIGIN_MASTER(u32 _timetamp,u8 _mil) {
	ORIGINAL_MASTER_TIME.timetamp = _timetamp;
	ORIGINAL_MASTER_TIME.milstep = _mil;
} //Sync original time-master req

/*---------------- Total Packet handling --------------------------*/

fl_pack_t g_handle_array[PACK_HANDLE_SIZE];
fl_data_container_t G_HANDLE_CONTAINER = { .data = g_handle_array, .head_index = 0, .tail_index = 0, .mask = PACK_HANDLE_SIZE - 1, .count = 0 };
/*---------------- Priority Sending --------------------------*/
fl_pack_t g_priority_sending_array[8];
fl_data_container_t G_PRIORITY_QUEUE_SENDING = { .data = g_priority_sending_array, .head_index = 0, .tail_index = 0, .mask = 8 - 1, .count = 0 };

/*---------------- FW ADV REC QUEUE --------------------------*/
fl_pack_t g_fw_receiving_array[FOTA_REC_SIZE];
fl_data_container_t G_FW_QUEUE_REC = { .data = g_fw_receiving_array, .head_index = 0, .tail_index = 0, .mask = FOTA_REC_SIZE - 1, .count = 0 };

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
int _interval_report(void);
/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/
void fl_nwk_LedSignal_init(void) {
#ifdef COUNTER_DEVICE
#ifndef HW_SAMPLE_TEST
	gpio_function_en(GPIO_PA6);
	gpio_set_output(GPIO_PA6,1);	//enable output
	gpio_set_input(GPIO_PA6,0);		//disable input
	gpio_set_level(GPIO_PA6,0);
#endif
#endif
}
void fl_nwk_LedSignal_run(void){
#ifdef COUNTER_DEVICE
#ifndef HW_SAMPLE_TEST
	gpio_toggle(GPIO_PA6);
#endif
#endif
}

u8 fl_nwk_mySlaveID(void){
	return G_INFORMATION.slaveID;
}
u8* fl_nwk_mySlaveMac(void){
	return G_INFORMATION.mac;
}

bool IsPairing(void)	{
	return(G_INFORMATION.profile.run_stt.join_nwk);
}
bool IsJoinedNetwork(void)	{
	return(G_INFORMATION.slaveID != 0xFF);
}
bool IsOnline(void){
	return (G_INFORMATION.active && IsJoinedNetwork());
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
	if(IsOnline() && G_INFORMATION.profile.run_stt.join_nwk == 0){
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
//	ERR(INF,"Device -> offline\r\n");
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
		LOGA(FLA,"** SlaveID :%d\r\n",G_INFORMATION.slaveID);
		LOGA(FLA,"** grpID   :%d\r\n",FL_SLAVEID_GRPID(G_INFORMATION.slaveID));
		LOGA(FLA,"** memID   :%d\r\n",FL_SLAVEID_MEMID(G_INFORMATION.slaveID));
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
void fl_slave_network_checkSTT(void){
	G_INFORMATION.active = true;
	blt_soft_timer_restart(&_isOnline_check,RECHECKING_NETWOK_TIME * 1000);
}
int fl_nwk_slave_nwkclear(void){
	fl_db_Pairing_Clear();
	fl_slave_profiles_t my_profile =fl_db_slaveprofile_init();
	G_INFORMATION.slaveID = my_profile.slaveid;
	G_INFORMATION.profile = my_profile;
	G_INFORMATION.profile.run_stt.join_nwk =1;
	fl_db_slaveprofile_save(G_INFORMATION.profile);
	blt_soft_timer_restart(_interval_report,100*100);
	return -1;
}

int fl_nwk_slave_nwkRemove(void){
	extern int REBOOT_DEV(void) ;
	fl_db_Pairing_Clear();
	fl_slave_profiles_t my_profile =fl_db_slaveprofile_init();
	G_INFORMATION.slaveID = my_profile.slaveid;
	G_INFORMATION.profile = my_profile;
#ifdef HW_SAMPLE_TEST
	G_INFORMATION.profile.run_stt.join_nwk =1;
#else
	G_INFORMATION.profile.run_stt.join_nwk =0;
#endif
	fl_db_slaveprofile_save(G_INFORMATION.profile);
//	REBOOT_DEV();
	blt_soft_timer_restart(_interval_report,100*100);
	return -1;
}
#ifdef COUNTER_DEVICE
void fl_nwk_slave_displayLCD_Refesh(void){
#ifndef HW_SAMPLE_TEST
	//display version
	extern fl_version_t _fw;
	extern fl_version_t _hw;
	char version_c[16];
	memset((u8*)version_c,0,SIZEU8(version_c));
	//_fw.patch = DFU_OTA_VERISON_GET();
	sprintf(version_c,"FW :%d.%d.%d (%d)", _fw.major,_fw.minor,_fw.patch,DFU_OTA_VERISON_GET());
	Counter_LCD_ENDCALL_Display(1,version_c);
	memset((u8*)version_c,0,SIZEU8(version_c));
	sprintf(version_c,"HW :%d.%d.%d", _hw.major,_hw.minor,_hw.patch );
	Counter_LCD_ENDCALL_Display(0,version_c);
	char mess_c[16];
	memset((u8*)mess_c,0,SIZEU8(mess_c));
	sprintf(mess_c,"GW : %02X%02X%02X%02X",U32_BYTE0( G_INFORMATION.profile.nwk.mac_parent),U32_BYTE1( G_INFORMATION.profile.nwk.mac_parent),
			U32_BYTE2( G_INFORMATION.profile.nwk.mac_parent),U32_BYTE3( G_INFORMATION.profile.nwk.mac_parent));
	Counter_LCD_PEU_Display(0,mess_c);
	char mess_c1[16];
	memset((u8*)mess_c1,0,SIZEU8(mess_c1));
	sprintf(mess_c1,"ID : %d",G_INFORMATION.slaveID);
	Counter_LCD_PEU_Display(1,mess_c1);
	char mess_c2[16];
	memset((u8*) mess_c2,0,SIZEU8(mess_c2));
	sprintf(mess_c2,"Chn:%d,%d,%d",G_INFORMATION.profile.nwk.chn[0],G_INFORMATION.profile.nwk.chn[1],G_INFORMATION.profile.nwk.chn[2]);
	Counter_LCD_PED_Display(0,mess_c2);
	memset((u8*) mess_c2,0,SIZEU8(mess_c2));
	sprintf(mess_c2,"Key:***%02X%02X%02X%02X",G_INFORMATION.profile.nwk.private_key[12],G_INFORMATION.profile.nwk.private_key[11],
			G_INFORMATION.profile.nwk.private_key[10],G_INFORMATION.profile.nwk.private_key[9]);
	Counter_LCD_PED_Display(1,mess_c2);
#endif
}
#endif
void fl_nwk_slave_init(void) {
//	PLOG_Start(APP);
//	PLOG_Start(FLA);
	DEBUG_TURN(NWK_DEBUG_STT);
//	fl_input_external_init();
	FL_QUEUE_CLEAR(&G_HANDLE_CONTAINER,PACK_HANDLE_SIZE);
	//Generate information
	G_INFORMATION.active = false;
	G_INFORMATION.timelife = 0;
	memcpy(G_INFORMATION.mac,blc_ll_get_macAddrPublic(),SIZEU8(G_INFORMATION.mac));
	//Load from db
	fl_slave_profiles_t my_profile = fl_db_slaveprofile_init();
	G_INFORMATION.slaveID = my_profile.slaveid;
	G_INFORMATION.profile = my_profile;
	fl_timetamp_withstep_t cur_timetamp = fl_rtc_getWithMilliStep();
	SYNC_ORIGIN_MASTER(cur_timetamp.timetamp,cur_timetamp.milstep);

#ifdef COUNTER_DEVICE
	G_INFORMATION.dev_type = TBS_COUNTER;
	G_INFORMATION.data =(u8*)&G_COUNTER_DEV;
	memcpy(G_COUNTER_DEV.mac,G_INFORMATION.mac,6);
	for (u8 i = 0; i < COUNTER_LCD_MESS_MAX; i++) {
		G_INFORMATION.lcd_mess[i] = &G_COUNTER_LCD[i][0];
	}
#endif
#ifdef POWER_METER_DEVICE
	G_INFORMATION.dev_type = TBS_POWERMETER;
	G_INFORMATION.data =(u8*)&G_POWER_METER;
#endif
	fl_nwk_LedSignal_init();

	memcpy(G_INFORMATION.mac,blc_ll_get_macAddrPublic(),SIZEU8(G_INFORMATION.mac));
	memcpy(&G_INFORMATION.data[0],G_INFORMATION.mac,6);
	P_INFO("** Freelux network SLAVE Init \r\n");
	P_INFO("** MAC     :%02X%02X%02X%02X%02X%02X\r\n",G_INFORMATION.mac[0],G_INFORMATION.mac[1],G_INFORMATION.mac[2],
			G_INFORMATION.mac[3],G_INFORMATION.mac[4],G_INFORMATION.mac[5]);
	P_INFO("** DevType:%d\r\n",G_INFORMATION.dev_type);
	P_INFO("** SlaveID: %d\r\n",G_INFORMATION.slaveID);
	P_INFO("** grpID  :%d\r\n",FL_SLAVEID_GRPID(G_INFORMATION.slaveID));
	P_INFO("** memID  :%d\r\n",FL_SLAVEID_MEMID(G_INFORMATION.slaveID));
	P_INFO("** JoinNWK:%d\r\n",G_INFORMATION.profile.run_stt.join_nwk);
	P_INFO("** RstFac :%d\r\n",G_INFORMATION.profile.run_stt.rst_factory);
	P_INFO("** MAC GW :%02X%02X%02X%02X\r\n",U32_BYTE0( G_INFORMATION.profile.nwk.mac_parent),U32_BYTE1( G_INFORMATION.profile.nwk.mac_parent),
			U32_BYTE2( G_INFORMATION.profile.nwk.mac_parent),U32_BYTE3( G_INFORMATION.profile.nwk.mac_parent));
	P_INFO("** Channel:%d,%d,%d\r\n",G_INFORMATION.profile.nwk.chn[0],G_INFORMATION.profile.nwk.chn[1],G_INFORMATION.profile.nwk.chn[2]);
#ifdef COUNTER_DEVICE
	fl_nwk_slave_displayLCD_Refesh();
#endif
#ifdef HW_SAMPLE_TEST
	if(G_INFORMATION.slaveID == G_INFORMATION.profile.slaveid && G_INFORMATION.slaveID == 0xFF){
		ERR(APP,"Turn on install mode\r\n");
		G_INFORMATION.profile.run_stt.join_nwk = 1;
		G_INFORMATION.profile.run_stt.rst_factory  = 1 ; //has reset factory device
	}
#endif

	blt_soft_timer_add(_nwk_slave_backup,2*1020*999);
	//Interval checking network
//	fl_nwk_slave_reconnectNstoragedata();
	WIFI_ORIGINAL_GETALL = fl_rtc_getWithMilliStep();
	blt_soft_timer_add(_interval_report,100*1000);
	//inform to master I'm online
//	blt_soft_timer_add(&_slave_reconnect,2*1000*1000);
	blt_soft_timer_add(&_informMaster,5*1000*1000);
	G_INFORMATION.active = false;
	//todo: TBS Device initialization
	TBS_Device_Init();
	//test random send req slave
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
//	datetime_t cur_dt;
//	fl_rtc_timestamp_to_datetime(master_timetamp,&cur_dt);
//	P_INFO("(%d)MASTER-TIME:%02d/%02d/%02d - %02d:%02d:%02d\r\n",packet->endpoint.dbg,cur_dt.year,cur_dt.month,cur_dt.day,cur_dt.hour,cur_dt.minute,
//			cur_dt.second);
//	fl_rtc_timestamp_to_datetime(WIFI_ORIGINAL_GETALL.timetamp,&cur_dt);
//	P_INFO("ORIGINAL_GETALL:%02d/%02d/%02d - %02d:%02d:%02d\r\n",cur_dt.year,cur_dt.month,cur_dt.day,cur_dt.hour,cur_dt.minute,cur_dt.second);
//
//	u32 cur_timetamp = fl_rtc_get();
//	fl_rtc_timestamp_to_datetime(cur_timetamp,&cur_dt);
//	P_INFO("SYSTIME:%02d/%02d/%02d - %02d:%02d:%02d\r\n",cur_dt.year,cur_dt.month,cur_dt.day,cur_dt.hour,cur_dt.minute,cur_dt.second);

	fl_rtc_sync(master_timetamp);
	//Synchronize debug log
	NWK_DEBUG_STT = packet->endpoint.dbg;
	DEBUG_TURN(NWK_DEBUG_STT);
	//Sync repeat mode
	NWK_REPEAT_MODE = packet->endpoint.repeat_mode;
	//add repeat_cnt
	NWK_REPEAT_LEVEL = packet->endpoint.rep_settings;

	SYNC_ORIGIN_MASTER(master_timetamp,packet->milltamp);
	LOGA(INF,"ORIGINAL MASTER-TIME:%d\r\n",ORIGINAL_MASTER_TIME.milstep);

//	G_INFORMATION.active = true;
//	blt_soft_timer_restart(&_isOnline_check,RECHECKING_NETWOK_TIME * 1000);
	fl_slave_network_checkSTT();
}

s8 fl_api_slave_req(u8 _cmdid, u8* _data, u8 _len, fl_rsp_callback_fnc _cb, u32 _timeout_ms,u8 _retry) {
	extern fl_adv_settings_t G_ADV_SETTINGS;
	//register timeout cb
	u32 waittime=0;
	s8 rslt=-1;
	if (_cb != 0 && ( _timeout_ms*1000 >= 2*QUEUQ_REQcRSP_INTERVAL || _timeout_ms==0)) {
		u64 seq_timetamp=fl_req_slave_packet_createNsend(_cmdid,_data,_len,&waittime);
		if(seq_timetamp){
//			P_INFO("REQ waittime:%d\r\n",waittime);
			waittime = _timeout_ms+waittime*G_ADV_SETTINGS.adv_duration;
			rslt=fl_queueREQcRSP_add(G_INFORMATION.slaveID,_cmdid,seq_timetamp,_data,_len,&_cb,waittime,_retry);
		}
	} else if(_cb == 0 && _timeout_ms ==0){
		rslt = (fl_req_slave_packet_createNsend(_cmdid,_data,_len,&waittime) == 0?-1:0); // none rsp
	}

	if (rslt > -1) {
//		fl_timetamp_withstep_t timetamp_inpack = fl_adv_timetampStepInPack(rslt);
//		u64 seq_timetamp = fl_rtc_timetamp2milltampStep(timetamp_inpack);
//		//Synch original time
//		SYNC_ORIGIN_MASTER(timetamp_inpack.timetamp,timetamp_inpack.milstep);
	} else {
		ERR(API,"Can't register REQ (%d/%d ms)!!\r\n",(u32 )_cb,_timeout_ms);
	}
	return rslt;;
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
u64 fl_req_slave_packet_createNsend(u8 _cmdid,u8* _data, u8 _len,u32 *_timequeues){
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

			req_pack.frame.slaveID = G_INFORMATION.slaveID;

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
			TBS_Device_Index_manage();
//			P_INFO("EVENT update - indx:%d \r\n",G_COUNTER_DEV.data.index-1);
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

			req_pack.frame.slaveID = G_INFORMATION.slaveID;
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

	//report head and tail
//	extern fl_data_container_t G_PRIORITY_QUEUE_SENDING;
	extern fl_data_container_t G_QUEUE_SENDING;
	s16 cur_tail =fl_adv_sendFIFO_add(rslt);
//	s16 cur_tail = fl_nwk_slave_PriorityADV_Add(rslt);
//	if(cur_tail > 0){
//		*_timequeues = (cur_tail - G_PRIORITY_QUEUE_SENDING.head_index + G_PRIORITY_QUEUE_SENDING.mask) % G_PRIORITY_QUEUE_SENDING.mask;
//	}
	if (cur_tail > 0) {
		*_timequeues = (cur_tail - G_QUEUE_SENDING.head_index + G_QUEUE_SENDING.mask) % G_QUEUE_SENDING.mask;
	}
	fl_timetamp_withstep_t  timetamp_inpack = fl_adv_timetampStepInPack(rslt);
	u64 seq_timetamp = fl_rtc_timetamp2milltampStep(timetamp_inpack);
//	//Synch original time
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
	extern u8 F_EXTITFOTA_TIME;
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
	LOGA(INF,"(%d|%x)HDR_REQ ID: %02X - ACK:%d\r\n",IsJoinedNetwork(),G_INFORMATION.slaveID,packet.frame.hdr,packet.frame.endpoint.master);

	switch ((fl_hdr_nwk_type_e) packet.frame.hdr) {
		case NWK_HDR_HEARTBEAT:
			_nwk_slave_syncFromPack(&packet.frame);
			GETINFO_FLAG_EVENTTEST = packet.frame.payload[0];
//			memcpy((u8*)WIFI_ORIGINAL_GETALL.timetamp,&packet.frame.payload[1],4);
			WIFI_ORIGINAL_GETALL.timetamp = MAKE_U32(packet.frame.payload[4],packet.frame.payload[3],packet.frame.payload[2],packet.frame.payload[1]);
			WIFI_ORIGINAL_GETALL.milstep = packet.frame.payload[5];
			LOGA(INF,"WIFI_ORIGINAL_GETALL :%d (%d) \r\n",WIFI_ORIGINAL_GETALL.timetamp,WIFI_ORIGINAL_GETALL.milstep);
			F_EXTITFOTA_TIME = packet.frame.payload[6];
			////
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
				s8 memid_idx = plog_IndexOf(packet.frame.payload,(u8*)&G_INFORMATION.slaveID,1,sizeof(packet.frame.payload)-1); //skip lastbyte int the payload
				u32 master_timetamp = MAKE_U32(packet.frame.timetamp[3],packet.frame.timetamp[2],packet.frame.timetamp[1],packet.frame.timetamp[0]);
				datetime_t cur_dt;
				fl_rtc_timestamp_to_datetime(master_timetamp,&cur_dt);
				u8 _payload[POWER_METER_STRUCT_BYTESIZE];
				memset(_payload,0xFF,SIZEU8(_payload));
				//u8 len_payload=0;
				LOGA(APP,"(%d)SlaveID:%X | inPack:%X | TestEvent:%d\r\n",memid_idx,G_INFORMATION.slaveID,packet.frame.payload[memid_idx],GETINFO_FLAG_EVENTTEST);
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
					packet.frame.slaveID = G_INFORMATION.slaveID;
					memset(packet.frame.payload,0,SIZEU8(packet.frame.payload));
					memcpy(&packet.frame.payload,&_payload[indx_data],SIZEU8(packet.frame.payload));
					//CRC
					packet.frame.crc8 = fl_crc8(packet.frame.payload,SIZEU8(packet.frame.payload));
					//increase index tbs
//					TBS_Device_Index_manage();
					//start period storage data
//					fl_nwk_slave_reconnectNstoragedata();
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
				if(packet.frame.slaveID == G_INFORMATION.slaveID){
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
				if(packet.frame.slaveID == G_INFORMATION.slaveID){
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
				if (packet.frame.slaveID == G_INFORMATION.slaveID) {
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
				if (packet.frame.slaveID == G_INFORMATION.slaveID) {
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
				if (packet.frame.slaveID == G_INFORMATION.slaveID) {
					if (packet.frame.endpoint.master == FL_FROM_MASTER_ACK) {
						u8 ok[2] = { 'o', 'k' };
						memset(packet.frame.payload,0,SIZEU8(packet.frame.payload));
						packet.frame.payload[0]= DFU_OTA_VERISON_GET();//test version
						memcpy(packet.frame.payload+1,ok,SIZEU8(ok));
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
		case NWK_HDR_MASTER_CMD: {
			if (IsJoinedNetwork()) {
				//check packet_slaveid
				if (packet.frame.slaveID == 0xFF || packet.frame.slaveID == G_INFORMATION.slaveID) {
					if (packet.frame.endpoint.master == FL_FROM_MASTER_ACK) {
						u8 ok[2] = { 'o', 'k' };
						memset(packet.frame.payload,0,SIZEU8(packet.frame.payload));
						packet.frame.payload[0]= DFU_OTA_VERISON_GET();//test version
						memcpy(packet.frame.payload+1,ok,SIZEU8(ok));
						//change endpoint to node source
						packet.frame.endpoint.master = FL_FROM_SLAVE;
						//add repeat_cnt
						packet.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;
					} else {
						u8 master_cmd_settings[22];
						memcpy(master_cmd_settings,packet.frame.payload,SIZEU8(packet.frame.payload));
						//Excute cmd
						P_INFO("CMD:%s\r\n",master_cmd_settings);
						PLOG_Parser_Cmd(master_cmd_settings);//p log
						//Non-rsp
						packet_built.length = 0;
						return packet_built;
					}
				}
			}
		}
		break;
		/*============================================================================================*/
		case NWK_HDR_REMOVE: {
			if (IsJoinedNetwork() && packet.frame.slaveID == G_INFORMATION.slaveID) {
				if (packet.frame.endpoint.master == FL_FROM_MASTER_ACK) {
					//get master's mac
					u32 mac_parent = MAKE_U32(packet.frame.payload[3],packet.frame.payload[2],packet.frame.payload[1],packet.frame.payload[0]);
					memcpy(G_INFORMATION.mac,blc_ll_get_macAddrPublic(),SIZEU8(G_INFORMATION.mac));
					if (mac_parent == G_INFORMATION.profile.nwk.mac_parent && -1!=plog_IndexOf(packet.frame.payload,G_INFORMATION.mac,6,SIZEU8(packet.frame.payload))) {
						ERR(APP,"Network leaving.....(%d )s\r\n",1);
#ifdef COUNTER_DEVICE
						Counter_LCD_RemoveDisplay();
#endif
						fl_nwk_slave_nwkRemove();
						blt_soft_timer_add(REBOOT_DEV,NWK_LEAVE_TIME_DISPLAY);
						//Process rsp
						memset(packet.frame.payload,0,SIZEU8(packet.frame.payload));
						memcpy(packet.frame.payload,G_INFORMATION.mac,SIZEU8(G_INFORMATION.mac));
						packet.frame.payload[SIZEU8(G_INFORMATION.mac)] = G_INFORMATION.dev_type;
						//change endpoint to node source
						packet.frame.endpoint.master = FL_FROM_SLAVE;
						//add repeat_cnt
						packet.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;
						break;
					}
				}
			}
			packet_built.length = 0;
			return packet_built;
		}
		break;
		case NWK_HDR_COLLECT: {
//			_nwk_slave_syncFromPack(&packet.frame);
			fl_nwk_LedSignal_run();
			/***/
			u32 master_timetamp = MAKE_U32(packet.frame.timetamp[3],packet.frame.timetamp[2],packet.frame.timetamp[1],packet.frame.timetamp[0]);
			SYNC_ORIGIN_MASTER(master_timetamp,packet.frame.milltamp);
			fl_rtc_sync(master_timetamp);
			//Synchronize debug log
			NWK_DEBUG_STT = packet.frame.endpoint.dbg;
			DEBUG_TURN(NWK_DEBUG_STT);
			/**/
			//LOGA(INF,"ORIGINAL MASTER-TIME:%d\r\n",ORIGINAL_MASTER_TIME.milstep);
			if (IsJoinedNetwork() == 0) {
				if (packet.frame.endpoint.master == FL_FROM_MASTER_ACK) {
					//get master's mac
					u8 mac_parent[4];
					memcpy(mac_parent,packet.frame.payload,SIZEU8(mac_parent));
					G_INFORMATION.profile.nwk.mac_parent = MAKE_U32(packet.frame.payload[3],packet.frame.payload[2],packet.frame.payload[1],packet.frame.payload[0]);
					//Process rsp
					memcpy(G_INFORMATION.mac,blc_ll_get_macAddrPublic(),SIZEU8(G_INFORMATION.mac));

					memset(packet.frame.payload,0,SIZEU8(packet.frame.payload));
					memcpy(packet.frame.payload,G_INFORMATION.mac,SIZEU8(G_INFORMATION.mac));
					packet.frame.payload[SIZEU8(G_INFORMATION.mac)] = G_INFORMATION.dev_type;
					//Add MAC PARENT to confirm
					memcpy(&packet.frame.payload[SIZEU8(G_INFORMATION.mac)+1],mac_parent,SIZEU8(mac_parent));
					//change endpoint to node source
					packet.frame.endpoint.master = FL_FROM_SLAVE;
					//add repeat_cnt
					packet.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;
				}
			} else {
				//Joined network -> exit collection mode if the master stopped broadcast Collection packet
				blt_soft_timer_restart(fl_nwk_joinnwk_timeout,3*909*1010); //exit after 3s
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
			memcpy(G_INFORMATION.mac,blc_ll_get_macAddrPublic(),SIZEU8(G_INFORMATION.mac));
			s8 mymac_idx = plog_IndexOf(packet.frame.payload,G_INFORMATION.mac,SIZEU8(G_INFORMATION.mac),sizeof(packet.frame.payload));
			if (mymac_idx != -1) {
				G_INFORMATION.slaveID = packet.frame.slaveID;
				P_INFO("UPDATE SlaveID: %d(grpID:%d|memID:%d)\r\n",G_INFORMATION.slaveID,FL_SLAVEID_GRPID(G_INFORMATION.slaveID),FL_SLAVEID_MEMID(G_INFORMATION.slaveID));
				G_INFORMATION.profile.slaveid = G_INFORMATION.slaveID ;
				G_INFORMATION.profile.run_stt.rst_factory = 0;
				G_INFORMATION.profile.nwk.chn[0] = packet.frame.payload[mymac_idx+SIZEU8(G_INFORMATION.mac)];
				G_INFORMATION.profile.nwk.chn[1] = packet.frame.payload[mymac_idx+SIZEU8(G_INFORMATION.mac) + 1];
				G_INFORMATION.profile.nwk.chn[2] = packet.frame.payload[mymac_idx+SIZEU8(G_INFORMATION.mac) + 2];
				//Get private_key
				memcpy(G_INFORMATION.profile.nwk.private_key,&packet.frame.payload[mymac_idx+SIZEU8(G_INFORMATION.mac) + 3],NWK_PRIVATE_KEY_SIZE);
				//Joined network -> exit collection mode if the master stopped broadcast Collection packet
				blt_soft_timer_restart(fl_nwk_joinnwk_timeout,3*909*1010); //exit after 3s
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

	rsp_pack.frame.slaveID = G_INFORMATION.slaveID;
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
void fl_slave_fota_RECclear(void){
	FL_QUEUE_CLEAR(&G_FW_QUEUE_REC,G_FW_QUEUE_REC.mask+1);
}
s16 fl_slave_fota_rec(fl_pack_t *_fw_pack, u8* _mac_source) {
	if (FL_QUEUE_DATA_FIND(&G_FW_QUEUE_REC,&_fw_pack->data_arr[FOTA_FW_DATA_POSITION],FOTA_FW_DATA_POSITION,FOTA_PACK_FW_SIZE) == -1)
	{
		if (FL_QUEUE_ADD(&G_FW_QUEUE_REC,_fw_pack) < 0) {
			ERR(BLE,"Err <G_FW_QUEUE_REC ADD>!!\r\n");
			return -1;
		} else {
			//Add mac incomming
			u16 current_slot = (G_FW_QUEUE_REC.tail_index - 1) & (G_FW_QUEUE_REC.mask);
			memcpy(&G_FW_QUEUE_REC.data[current_slot].data_arr[FOTA_MAC_INCOM_POSITION],_mac_source,FOTA_MAC_INCOM_SIZE);
//			P_INFO_HEX(G_FW_QUEUE_REC.data[current_slot].data_arr,SIZEU8(G_FW_QUEUE_REC.data[current_slot].data_arr),"FOTA-ADD(%d):",G_FW_QUEUE_REC.data[current_slot].length);
			return current_slot;
		}
	}
	return 0;
}
//DEBUG
fl_ble2wif_fota_info_t FOTA_SLAVE_INFO;

s16 fl_slave_fota_proc(void) {
	extern u8 fl_packet_parse(fl_pack_t _pack, fl_dataframe_format_t *rslt);
	fl_dataframe_format_t packet;
	fl_pack_t fota_pack;
//	u16 curr_head = G_FW_QUEUE_REC.head_index;
	//For debuging log
//	static u16 head_err =0;
	while (FL_QUEUE_GET(&G_FW_QUEUE_REC,&fota_pack) > -1) {
		//
		//blt_soft_timer_restart(&_isOnline_check,RECHECKING_NETWOK_TIME * 1000);
		fl_slave_network_checkSTT();

		if (!fl_packet_parse(fota_pack,&packet)) {
			ERR(INF,"Packet parse fail!!!\r\n");
			P_INFO_HEX(fota_pack.data_arr,SIZEU8(fota_pack.data_arr),"FOTA-ERR(%d):",fota_pack.length);
			return -1;
		}
		if (packet.hdr == NWK_HDR_FOTA) {
			fl_nwk_LedSignal_run();
			//Move END_PACK to the last position
			if (packet.payload[0] == FOTA_PACKET_END && G_FW_QUEUE_REC.count > 0) {
				FL_QUEUE_ADD(&G_FW_QUEUE_REC,&fota_pack);
				continue;
			}
			/*todo: load fw into the dfu*/
			ota_ret_t rslt_ota = OTA_RET_ERROR;
			u8 crc =0;
			if ((packet.payload[0] <= FOTA_PACKET_END)) {
				if (packet.payload[1] == G_INFORMATION.dev_type) {
					crc = fl_crc8(packet.payload,SIZEU8(packet.payload));
					rslt_ota = DFU_OTA_FW_PUT(packet.payload,crc);
					if (OTA_RET_OK != rslt_ota) {
						ERR(APP,"FOTA DFU Err <RET ERR>\r\n");
						P_INFO_HEX(packet.payload,SIZEU8(packet.payload),"FW(crc:%02X):",crc);
					}
					/*DEBUG*/
					FOTA_SLAVE_INFO.version = packet.payload[2];
					FOTA_SLAVE_INFO.fw_type = packet.payload[1];
					FOTA_SLAVE_INFO.pack_type = packet.payload[0];
					if (FOTA_SLAVE_INFO.pack_type == FOTA_PACKET_BEGIN) {
						FOTA_SLAVE_INFO.begin++;
						FOTA_SLAVE_INFO.fw_size = MAKE_U32(0,packet.payload[5],packet.payload[4],packet.payload[3]);
						FOTA_SLAVE_INFO.rtt = fl_rtc_get();
						FOTA_SLAVE_INFO.body = 0;
						FOTA_SLAVE_INFO.end = 0;
					}
					if (FOTA_SLAVE_INFO.pack_type == FOTA_PACKET_DATA) {
						FOTA_SLAVE_INFO.body++;
						P_INFO("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
						P_INFO("[T%d,v%d]Downloading:%d/%d (%d/%d)",FOTA_SLAVE_INFO.fw_type,FOTA_SLAVE_INFO.version,
								FOTA_SLAVE_INFO.body*OTA_PACKET_LENGTH,FOTA_SLAVE_INFO.fw_size,FL_NWK_FOTA_IsReady(),G_FW_QUEUE_REC.count);
					}
					if (FOTA_SLAVE_INFO.pack_type == FOTA_PACKET_END) {
						FOTA_SLAVE_INFO.end++;
						FOTA_SLAVE_INFO.rtt = fl_rtc_get() - FOTA_SLAVE_INFO.rtt;
						P_INFO("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
						P_INFO("[T%d,v%d]Begin:%d,Body:%d,End:%d,RTT:%d s\r\n",FOTA_SLAVE_INFO.fw_type,FOTA_SLAVE_INFO.version,FOTA_SLAVE_INFO.begin,
								FOTA_SLAVE_INFO.body,FOTA_SLAVE_INFO.end,FOTA_SLAVE_INFO.rtt);
						FOTA_SLAVE_INFO.rtt = fl_rtc_get();
						FOTA_SLAVE_INFO.begin = 0;
					}
					/*END DEBUG*/
//					else {
					//add send repeat and check echo
					if (fl_wifi2ble_fota_fwpush(&fota_pack,packet.payload[0]) == -1) {
						//re-send this slot
						//				G_FW_QUEUE_REC.head_index = curr_head;
						//				G_FW_QUEUE_REC.count++;
						//				FL_QUEUE_ADD(&G_FW_QUEUE_REC,&fota_pack);

						P_INFO("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
						P_INFO("[T%d,v%d]Downloading:%d/%d (%d/%d)",FOTA_SLAVE_INFO.fw_type,FOTA_SLAVE_INFO.version,
								FOTA_SLAVE_INFO.body*OTA_PACKET_LENGTH,FOTA_SLAVE_INFO.fw_size,FL_NWK_FOTA_IsReady(),G_FW_QUEUE_REC.count);
						//				if (head_err ==0) {
						//					head_err=1;
						//					ERR(APP,"FOTA ECHO Err <Full>\r\n");
						//				}
						//				return -1;
						break;
					}
//						else {
					//				head_err = 0;
					//				curr_head = G_FW_QUEUE_REC.head_index;
//						}
//					}
				}
			}
		}
	}
	//ECHO
	if (FOTA_SLAVE_INFO.end > 0) {
		if (FL_NWK_FOTA_IsReady() == G_FW_QUEUE_REC.count && G_FW_QUEUE_REC.count == 0) {
			FOTA_SLAVE_INFO.rtt = fl_rtc_get() - FOTA_SLAVE_INFO.rtt;
			P_INFO("\r\n[T%d,v%d]====> FOTA Done (%d s)\r\n",FOTA_SLAVE_INFO.fw_type,FOTA_SLAVE_INFO.version,FOTA_SLAVE_INFO.rtt);
			FOTA_SLAVE_INFO.end = 0;
//			fl_slave_fota_RECclear();
		} else {
			P_INFO("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
			P_INFO("[T%d,v%d]Echo:%d/%d",FOTA_SLAVE_INFO.fw_type,FOTA_SLAVE_INFO.version,FL_NWK_FOTA_IsReady(),G_FW_QUEUE_REC.count);
		}
	}
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
int fl_slave_ProccesRSP_cbk(void) {
	fl_pack_t data_in_queue;
	if (FL_QUEUE_GET(&G_HANDLE_CONTAINER,&data_in_queue)>-1) {
		//Scan req call rsp
		if(-1!=fl_queue_REQcRSP_ScanRec(data_in_queue,&G_INFORMATION))
		{
			LOGA(API,"Refesh online status (%d ms)\r\n",RECHECKING_NETWOK_TIME);
//			blt_soft_timer_restart(&_isOnline_check,RECHECKING_NETWOK_TIME * 1000);
			fl_slave_network_checkSTT();
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
		G_INFORMATION.profile.run_stt.join_nwk = 1;
		return 0;
	}
}
void fl_nwk_slave_joinnwk_exc(void) {
	if (IsPairing()) {
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
		fl_api_slave_req(NWK_HDR_55,(u8*)&G_COUNTER_DEV.data,SIZEU8(G_COUNTER_DEV.data),0,0,1);
#else //POWERMETER
		u8 _payload[SIZEU8(tbs_device_powermeter_t)];
		tbs_device_powermeter_t *pwmeter_data = (tbs_device_powermeter_t*) G_INFORMATION.data;
		tbs_pack_powermeter_data(pwmeter_data,_payload);
		u8 indx_data = SIZEU8(pwmeter_data->type) + SIZEU8(pwmeter_data->mac) + SIZEU8(pwmeter_data->timetamp);
		fl_api_slave_req(NWK_HDR_55,&_payload[indx_data],SIZEU8(pwmeter_data->data),0,0,1);
#endif
		return RECONNECT_TIME;
	}
	return 0;
}

int _interval_report(void) {
#ifdef COUNTER_DEVICE
	fl_nwk_slave_displayLCD_Refesh();
#endif
	int offset_spread = (fl_rtc_getWithMilliStep().milstep - WIFI_ORIGINAL_GETALL.milstep)*10;
#define INTERVAL_REPORT_TIME (55 - FL_SLAVEID_MEMID(G_INFORMATION.slaveID))
	extern const u32 ORIGINAL_TIME_TRUST;
	if (IsJoinedNetwork()) {
		if (WIFI_ORIGINAL_GETALL.timetamp < fl_rtc_get() && WIFI_ORIGINAL_GETALL.timetamp > ORIGINAL_TIME_TRUST) {
			if (fl_rtc_get() - WIFI_ORIGINAL_GETALL.timetamp >= INTERVAL_REPORT_TIME) {
#ifdef COUNTER_DEVICE
				fl_api_slave_req(NWK_HDR_55,(u8*) &G_COUNTER_DEV.data,SIZEU8(G_COUNTER_DEV.data),0,0,1);//NWK_HDR_11_REACTIVE
#else //POWERMETER
				u8 _payload[SIZEU8(tbs_device_powermeter_t)];
				tbs_device_powermeter_t *pwmeter_data = (tbs_device_powermeter_t*) G_INFORMATION.data;
				tbs_pack_powermeter_data(pwmeter_data,_payload);
				u8 indx_data = SIZEU8(pwmeter_data->type) + SIZEU8(pwmeter_data->mac) + SIZEU8(pwmeter_data->timetamp);
				fl_api_slave_req(NWK_HDR_55,&_payload[indx_data],SIZEU8(pwmeter_data->data),0,0,1);
#endif
				//increase index if using NWK_HDR_11_REACTIVE
				//TBS_Device_Index_manage();
//				P_INFO("Auto update (%d s) - indx:%d\r\n",INTERVAL_REPORT_TIME*1000*1000 + offset_spread,G_COUNTER_DEV.data.index-1);
				return INTERVAL_REPORT_TIME*1000*1000 + offset_spread;
			}
		}
	}else{
		if(IsPairing()){
			fl_nwk_LedSignal_run();
			return 500 * 1000;
		}
	}
#undef INTERVAL_REPORT_TIME
	return 100 * 1000 + offset_spread;
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
	if(blt_soft_timer_find(_interval_report) == -1){
		blt_soft_timer_restart(_interval_report,100*100);
	}
}
/***************************************************
 * @brief 		: priority request
 *
 * @param[in] 	:none
 *
 * @return	  	:
 *
 ***************************************************/
s16 fl_nwk_slave_PriorityADV_Add(fl_pack_t _pack) {
	if (FL_QUEUE_FIND(&G_PRIORITY_QUEUE_SENDING,&_pack,_pack.length - 1) == -1) {
		if (FL_QUEUE_ADD(&G_PRIORITY_QUEUE_SENDING,&_pack) < 0) {
			ERR(BLE,"Err FULL <QUEUE ADD PRIORITY ADV SENDING>!!\r\n");
			return -1;
		} else {
			P_PRINTFHEX_A(BLE,_pack.data_arr,_pack.length,"%s(%d):","QUEUE PRIORITY ADV",_pack.length);
			LOGA(BLE,"QUEUE PRIORITY SEND ADD: %d/%d (cnt:%d)\r\n",G_PRIORITY_QUEUE_SENDING.head_index,G_PRIORITY_QUEUE_SENDING.tail_index,G_PRIORITY_QUEUE_SENDING.count);
			return G_PRIORITY_QUEUE_SENDING.tail_index;
		}
	}
	ERR(BLE,"Err <QUEUE ALREADY PRIORITY ADV SENDING>!!\r\n");
	return -1;
}

u8 fl_adv_sendFIFO_PriorityADV_run(void) {
	extern fl_adv_settings_t G_ADV_SETTINGS ;
	extern volatile u8 F_SENDING_STATE;
	fl_pack_t data_in_queue;
	if (!F_SENDING_STATE) {
		if (FL_QUEUE_GET(&G_PRIORITY_QUEUE_SENDING,&data_in_queue)>-1) {
			fl_adv_send(data_in_queue.data_arr,data_in_queue.length,G_ADV_SETTINGS.adv_duration);
			P_INFO_HEX(data_in_queue.data_arr,data_in_queue.length,"[%d-%d/%d]PRIORITY(%d):",G_PRIORITY_QUEUE_SENDING.head_index,
					G_PRIORITY_QUEUE_SENDING.tail_index,G_PRIORITY_QUEUE_SENDING.count,data_in_queue.length);
		}
	}
	return 1;
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
