/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_adv_proc.c
 *Created on		: Jul 9, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "app.h"
#include "stdio.h"
#include "time.h"
#include "fl_adv_repeat.h"
#include "fl_adv_proc.h"
#include "fl_nwk_handler.h"
#include "Freelux_libs/fl_sys_datetime.h"
#include "fl_input_ext.h"
#include "fl_nwk_protocol.h"

//Public Key for the freelux network
unsigned char FL_NWK_PB_KEY[16] = "freeluxnetw0rk25";
const u32 ORIGINAL_TIME_TRUST = 1735689600; //00:00:00 UTC - 1/1/2025
unsigned char FL_NWK_USE_KEY[16]; //this key used to encrypt -> decrypt
volatile u8* FL_NWK_COLLECTION_MODE; //
/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/

#define SEND_TIMEOUT_MS		50 //ms
extern _attribute_data_retention_ volatile fl_timetamp_withstep_t ORIGINAL_MASTER_TIME;

volatile u8 F_SENDING_STATE = 0;

fl_adv_settings_t G_ADV_SETTINGS = {
		.adv_interval_min = ADV_INTERVAL_20MS,
		.adv_interval_max = ADV_INTERVAL_30MS,
		.adv_duration = SEND_TIMEOUT_MS,
		.scan_interval = SCAN_INTERVAL_60MS,
		.scan_window = SCAN_WINDOW_60MS,
		.time_wait_rsp = 10,
		.retry_times = 2,
		//.nwk_chn = {10,11,12}
		};

extern volatile u8  NWK_REPEAT_LEVEL;

/*---------------- Total ADV Rec --------------------------*/
#define IN_DATA_SIZE 		64
fl_pack_t g_data_array[IN_DATA_SIZE];
fl_data_container_t G_DATA_CONTAINER = { .data = g_data_array, .head_index = 0, .tail_index = 0, .mask = IN_DATA_SIZE - 1, .count = 0 };

/*---------------- ADV SEND QUEUE --------------------------*/
#define QUEUE_SENDING_SIZE 		16
fl_pack_t g_sending_array[QUEUE_SENDING_SIZE];
fl_data_container_t G_QUEUE_SENDING = { .data = g_sending_array, .head_index = 0, .tail_index = 0, .mask = QUEUE_SENDING_SIZE - 1, .count = 0 };

fl_hdr_nwk_type_e FL_NWK_HDR[]={NWK_HDR_RECONNECT,NWK_HDR_55,NWK_HDR_F5_INFO,NWK_HDR_F6_SENDMESS,NWK_HDR_ASSIGN,NWK_HDR_HEARTBEAT,NWK_HDR_COLLECT};
#define FL_NWK_HDR_SIZE	(sizeof(FL_NWK_HDR)/sizeof(FL_NWK_HDR[0]))

/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
#ifndef MASTER_CORE
#define SEND_HISTORY_S		1 //second
#endif

/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/
void fl_adv_send(u8* _data, u8 _size, u16 _timeout_ms);
fl_pack_t fl_packet_build(u8 *payload, u8 _len);
s8 fl_adv_IsFromMaster(fl_pack_t data_in_queue);
u8 fl_packet_parse(fl_pack_t _pack, fl_dataframe_format_t *rslt);

u8 IsNWKHDR(u8 _hdr){
	for (u8 var = 0;  var < FL_NWK_HDR_SIZE; ++ var) {
		if(FL_NWK_HDR[var] == _hdr) return var;
	}
	return 0xFF;
}

static void fl_nwk_encrypt16(unsigned char * key,u8* _data,u8 _size, u8* encrypted){
#define BLOCK_SIZE 16
	if(_size < BLOCK_SIZE){
		ERR(INF,"Encrypt size!!!\r\n");
		return;
	}
	u8 headbytes[BLOCK_SIZE];
	memcpy(headbytes,_data,SIZEU8(headbytes));
	u8 data_buffer[_size];
	memcpy(data_buffer,_data,SIZEU8(data_buffer));
	aes_encrypt(key,headbytes,data_buffer);
	memcpy(encrypted,data_buffer,_size);
#undef BLOCK_SIZE
}
static bool fl_nwk_decrypt16(unsigned char * key,u8* _data,u8 _size, u8* decrypted){
#define BLOCK_SIZE 16
	if(_size < BLOCK_SIZE){
		ERR(INF,"Decrypt size!!!\r\n");
		return false;
	}
	u8 headbytes[BLOCK_SIZE];
	memcpy(headbytes,_data,SIZEU8(headbytes));
	u8 data_buffer[_size];
	memcpy(data_buffer,_data,SIZEU8(data_buffer));
	aes_decrypt(key,headbytes,data_buffer);
	memcpy(decrypted,data_buffer,_size);
/*Checking result decrypt*/
	u32 timetamp_hdr = MAKE_U32(decrypted[3],decrypted[2],decrypted[1],decrypted[0]);
	fl_data_frame_u packet_frame;
	memcpy(packet_frame.bytes,decrypted,SIZEU8(packet_frame.bytes));
	u8 pack_crc = fl_crc8(packet_frame.frame.payload,SIZEU8(packet_frame.frame.payload));
	return (timetamp_hdr>ORIGINAL_TIME_TRUST && IsNWKHDR(decrypted[0])!=0xFF && pack_crc == packet_frame.frame.crc8);
#undef BLOCK_SIZE
}
/***************************************************
 * @brief 		:Generate used key for the network
 * 				**Need to call before encrypt/decrypt function
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
static inline void NWK_MYKEY(void){
	const unsigned char KEY_NULL[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
	u8 key_buffer[NWK_PRIVATE_KEY_SIZE];
#ifdef MASTER_CORE
	extern fl_master_config_t G_MASTER_INFO;
	memcpy(key_buffer,G_MASTER_INFO.nwk.private_key,NWK_PRIVATE_KEY_SIZE);
#else
	extern fl_nodeinnetwork_t G_INFORMATION ;
	memcpy(key_buffer,G_INFORMATION.profile.nwk.private_key,NWK_PRIVATE_KEY_SIZE);
#endif
	if (memcmp(key_buffer,KEY_NULL,SIZEU8(KEY_NULL)) == 0 || *FL_NWK_COLLECTION_MODE == 1) {
		memcpy(FL_NWK_USE_KEY,FL_NWK_PB_KEY,SIZEU8(FL_NWK_PB_KEY));
	} else {
		//build
		memcpy(FL_NWK_USE_KEY,key_buffer,NWK_PRIVATE_KEY_SIZE);
		memcpy(&FL_NWK_USE_KEY[NWK_PRIVATE_KEY_SIZE],FL_NWK_PB_KEY,SIZEU8(FL_NWK_PB_KEY)-NWK_PRIVATE_KEY_SIZE);
	}
}
/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/
static int fl_controller_event_callback(u32 h, u8 *p, int n) {
	if (h & HCI_FLAG_EVENT_BT_STD)		//ble controller hci event
	{
		u8 evtCode = h & 0xff;
		if (evtCode == HCI_EVT_LE_META) {
			u8 subEvt_code = p[0];
			if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT)		// ADV packet
			{
				//after controller is set to scan state, it will report all the adv packet it received by this event
				event_adv_report_t *pa = (event_adv_report_t *) p;
				//P_PRINTFHEX_A(BLE,pa->mac,6,"%s","MAC:");
				//u32 test_adv_cnt = MAKE_U32(pa->data[3],pa->data[2],pa->data[1],pa->data[0]);
				//LOGA(BLE,"ADV rec:%d!!!\r\n",test_adv_cnt);

				fl_pack_t incomming_data;
				incomming_data.length = pa->len + 1; //add rssi byte
				//memcpy(incomming_data.data_arr,pa->data,incomming_data.length);
//				incomming_data.data_arr[0] = pa->data[0];
				//Add decrypt
				NWK_MYKEY();
				if(!fl_nwk_decrypt16(FL_NWK_USE_KEY,pa->data,incomming_data.length,incomming_data.data_arr)) return 0;
#ifdef MASTER_CORE
				//skip from  master
				if (fl_adv_IsFromMaster(incomming_data)) {
					return 0;
				}
#else
				if (!fl_nwk_slave_checkHDR(incomming_data.data_arr[0])) {
					return 0;
				}
//				/* Skip repeate-adv loop */
//				if (FL_QUEUE_FIND(&G_QUEUE_SENDING,&incomming_data,incomming_data.length - 2/*skip rssi + repeat_cnt*/) != -1) {
//					return 0;
//				}
#endif
				if (FL_QUEUE_FIND(&G_DATA_CONTAINER,&incomming_data,incomming_data.length - 1/*skip rssi*/) == -1) {
//					s8 rssi = (s8) pa->data[pa->len];
					if (FL_QUEUE_ADD(&G_DATA_CONTAINER,&incomming_data) < 0) {
						ERR(BLE,"Err <QUEUE ADD>!!\r\n");
					} else {
//						LOGA(APP,"QUEUE ADD (len:%d|RSSI:%d): (%d)%d-%d\r\n",pa->len,rssi,G_DATA_CONTAINER.count,G_DATA_CONTAINER.head_index,
//								G_DATA_CONTAINER.tail_index);
//						P_PRINTFHEX_A(BLE,incomming_data.data_arr,incomming_data.length,"%s(%d):","PACK",incomming_data.length);
					}
				} else {
//					ERR(BLE,"Err <QUEUE ALREADY>!!\r\n");
//					P_PRINTFHEX_A(BLE,incomming_data.data_arr,incomming_data.length,"%s(%d):","PACK",incomming_data.length);
				}
			}
		}
	}
	return 0;
}
/**
 * @brief      callback function of LinkLayer Event "BLT_EV_FLAG_ADV_DURATION_TIMEOUT"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
void fl_durationADV_timeout_proccess(u8 e, u8 *p, int n) {
	//todo: Time sending expired
	F_SENDING_STATE = 0;
	blc_ll_addScanningInAdvState();  //add scan in adv state
	blc_ll_setScanEnable(1,0); //
	bls_ll_setAdvEnable(BLC_ADV_ENABLE);  // adv
//	TICK_GET_PROCESSING_TIME = clock_time();
//	LOGA(BLE,"Duration time:%d\r\n",(clock_time()-TICK_GET_PROCESSING_TIME)/SYSTEM_TIMER_TICK_1MS);
}

/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/

/***************************************************
 * @brief 		: add data payload adv into queue fifo
 *
 * @param[in] 	:packet
 *
 * @return	  	:-1 false otherwise true
 *
 ***************************************************/
int fl_adv_sendFIFO_add(fl_pack_t _pack) {
	u8 master_send_skip_LSB = 0;
#ifdef MASTER_CORE
	master_send_skip_LSB = 1;
#endif
	if (FL_QUEUE_FIND(&G_QUEUE_SENDING,&_pack,_pack.length - master_send_skip_LSB /* - 1 skip LSB (master)*/) == -1) {
		if (FL_QUEUE_ADD(&G_QUEUE_SENDING,&_pack) < 0) {
			ERR(BLE,"Err FULL <QUEUE ADD ADV SENDING>!!\r\n");
			return -1;
		} else {
//			P_PRINTFHEX_A(BLE,_pack.data_arr,_pack.length,"%s(%d):","QUEUE ADV",_pack.length);
			LOGA(BLE,"QUEUE SEND ADD: %d/%d (cnt:%d)\r\n",G_QUEUE_SENDING.head_index,G_QUEUE_SENDING.tail_index,G_QUEUE_SENDING.count);
			return G_QUEUE_SENDING.tail_index;
		}
	}
	ERR(BLE,"Err <QUEUE ALREADY ADV SENDING>!!\r\n");
	return -1;
}
#ifdef MASTER_CORE

void fl_master_adv_createRSPCommon(void) {
	u8 SlaveID[22-5-2]; //22 bytes paloay - 4 bytes timetamps - delta timetamps
	u32 timetamp_com[22-5-2]; // for testing list
	u8 numSlave=0;
	u8 timetamp_min_u8[SIZEU8(fl_timetamp_withstep_t)];
	u32 timetamp_min = 0;
	u32 timetamp_max = 0;
	u32 origin_pack = fl_rtc_timetamp2milltampStep(ORIGINAL_MASTER_TIME);
	fl_data_frame_u check_rspcom;

	fl_pack_t *p_55RSP[SIZEU8(SlaveID)];
	for (u8 var = 0; var < G_QUEUE_SENDING.mask + 1; ++var) {
		fl_timetamp_withstep_t timetamp_inpack = fl_adv_timetampStepInPack(G_QUEUE_SENDING.data[var]);
		u32 inpack = fl_rtc_timetamp2milltampStep(timetamp_inpack);
		memset(check_rspcom.bytes,0,SIZEU8(check_rspcom.bytes));
		memcpy(check_rspcom.bytes,G_QUEUE_SENDING.data[var].data_arr,G_QUEUE_SENDING.data[var].length);
		if (inpack >= origin_pack && check_rspcom.frame.endpoint.master == FL_FROM_MASTER
				&& check_rspcom.frame.hdr == NWK_HDR_55 && check_rspcom.frame.slaveID.id_u8 != 0xFF) {

			p_55RSP[numSlave] = &G_QUEUE_SENDING.data[var];
			SlaveID[numSlave] = check_rspcom.frame.slaveID.id_u8;
			timetamp_com[numSlave] = inpack;
			if(timetamp_min==0){
				timetamp_min=inpack;
				timetamp_max = inpack;
				memcpy(timetamp_min_u8,check_rspcom.frame.payload,SIZEU8(fl_timetamp_withstep_t));
			}else{
				if(inpack<=timetamp_min){
					timetamp_min = inpack;
					memcpy(timetamp_min_u8,check_rspcom.frame.timetamp,SIZEU8(fl_timetamp_withstep_t));
				}
				if(inpack>timetamp_max){
					timetamp_max = inpack;
				}
			}
			if(numSlave < SIZEU8(SlaveID))numSlave++;
			else break;
		}
	}
	if (numSlave < 2)
		return;
	//For testing
	LOGA(APP,"ORIGIN:%d\r\n",ORIGINAL_MASTER_TIME.timetamp);
	P_PRINTFHEX_A(APP,SlaveID,numSlave,"SlaveID(%d):",numSlave);
	LOGA(APP,"TimeTamp_min  :%d\r\n",timetamp_min);
	u16 delta = (timetamp_max > timetamp_min) ? (u16) (timetamp_max - timetamp_min) : 0;
	LOGA(APP,"TimeTamp_max  :%d\r\n",timetamp_max);
	LOGA(APP,"TimeTamp_delta:%d\r\n",delta);
	for (u8 i = 0; i < numSlave; i++) {
		LOGA(APP,"TimeTamp:%d\r\n",timetamp_com[i]);
	}
	//Clear RSP55 old and update G_SENDING_QUEUE.count
	for(u8 i = 0;i<numSlave;i++){
		memset(p_55RSP[i]->data_arr,0,SIZEU8(p_55RSP[i]->data_arr));
		p_55RSP[i]->length = 0;
	}
	fl_pack_t RSP_55_Com = fl_master_packet_RSP_55Com_build(SlaveID,numSlave,timetamp_min_u8,delta);
	P_PRINTFHEX_A(APP,RSP_55_Com.data_arr,RSP_55_Com.length,"RSP_55_Com(%d):",numSlave);
	fl_adv_sendFIFO_add(RSP_55_Com);
//	u8 origin[SIZEU8(fl_timetamp_withstep_t)];
//	origin[0] = U32_BYTE0(ORIGINAL_MASTER_TIME.timetamp);
//	origin[1] = U32_BYTE1(ORIGINAL_MASTER_TIME.timetamp);
//	origin[2] = U32_BYTE2(ORIGINAL_MASTER_TIME.timetamp);
//	origin[3] = U32_BYTE3(ORIGINAL_MASTER_TIME.timetamp);
//	origin[4] = ORIGINAL_MASTER_TIME.milstep;

}
#endif
/***************************************************
 * @brief 		: run and send adv from the queue
 *
 * @param[in] 	: none
 *
 * @return	  	:
 *
 ***************************************************/
void fl_adv_sendFIFO_run(void) {
	fl_pack_t data_in_queue;
//	static u8 check_hb_slot = 0;
//	u16 cur_slot = 0;
	if (!F_SENDING_STATE) {

#ifdef MASTER_CORE
		fl_master_adv_createRSPCommon();
#endif
		u8 loop_check = 0;
		while (FL_QUEUE_GET_LOOP(&G_QUEUE_SENDING,&data_in_queue)) {
			fl_timetamp_withstep_t  timetamp_inpack = fl_adv_timetampStepInPack(data_in_queue);
			u32 inpack = fl_rtc_timetamp2milltampStep(timetamp_inpack);
			u32 origin_pack = fl_rtc_timetamp2milltampStep(ORIGINAL_MASTER_TIME);
			loop_check++;
			if ( inpack < origin_pack)
			{
				if (loop_check <= G_QUEUE_SENDING.mask)
					continue;
				else{
//					ERR(INF,"NULL ADV SENDING!! \r\n");
					return;
				}
			}
			F_SENDING_STATE = 1;

			fl_adv_send(data_in_queue.data_arr,data_in_queue.length,G_ADV_SETTINGS.adv_duration);
#ifdef MASTER_CORE
			//for testing
//			u16 cur_slot = ((G_QUEUE_SENDING.head_index - 1) &G_QUEUE_SENDING.mask);
			fl_data_frame_u check_heartbeat;
			memcpy(check_heartbeat.bytes,data_in_queue.data_arr,data_in_queue.length);
			if (check_heartbeat.frame.hdr == NWK_HDR_HEARTBEAT) {
//				check_hb_slot = cur_slot;
				u32 origin = fl_rtc_timetamp2milltampStep(ORIGINAL_MASTER_TIME);
				u32 new_origin = fl_rtc_timetamp2milltampStep(timetamp_inpack);
				if (origin < new_origin) {
					LOGA(APP,"Master Synchronzation Timetamp:%d(%d)\r\n",ORIGINAL_MASTER_TIME.timetamp,ORIGINAL_MASTER_TIME.milstep);
				}
				//TODO: IMPORTANT SYNCHRONIZATION TIMESTAMP
				fl_master_SYNC_ORIGINAL_TIMETAMP(timetamp_inpack);
			}
#endif
//			if (cur_slot != check_hb_slot) {
//				LOGA(APP,"slot of adv in SENDING QUEUE :%d|%d\r\n",cur_slot , check_hb_slot);
//			}
			return;
		}
	}
}

/**
 * @brief      Setting and sending ADV packets
 * @param	   none
 * @return     none
 */
void fl_adv_send(u8* _data, u8 _size, u16 _timeout_ms) {
//	while (blc_ll_getCurrentState() == BLS_LINK_STATE_SCAN && blc_ll_getCurrentState() == BLS_LINK_STATE_ADV) {
//	};
	if (_data && _size >= 1) {
		rf_set_power_level_index(MY_RF_POWER_INDEX);
		u8 mac[6];
		own_addr_type_t app_own_address_type = OWN_ADDRESS_PUBLIC;
		memcpy(mac,(app_own_address_type == OWN_ADDRESS_PUBLIC) ? blc_ll_get_macAddrPublic() : blc_ll_get_macAddrRandom(),6);
		u8 status = bls_ll_setAdvParam(G_ADV_SETTINGS.adv_interval_min,G_ADV_SETTINGS.adv_interval_max,ADV_TYPE_SCANNABLE_UNDIRECTED,
				app_own_address_type,0,NULL,BLT_ENABLE_ADV_ALL,ADV_FP_NONE);
		if (status != BLE_SUCCESS) {
			ERR(BLE,"Set ADV param is FAIL !!!\r\n")
			while (1);
		}  //debug: adv setting err
		/*Encryt data*/
		u8 encrypted[_size];
		memset(encrypted,0,SIZEU8(encrypted));
		NWK_MYKEY();
		fl_nwk_encrypt16(FL_NWK_USE_KEY,_data,_size,encrypted);
		bls_ll_setAdvData(encrypted,_size);
		bls_ll_setAdvDuration(_timeout_ms * 1000,1); // ms->us
		bls_app_registerEventCallback(BLT_EV_FLAG_ADV_DURATION_TIMEOUT,&fl_durationADV_timeout_proccess);

		bls_ll_setAdvEnable(BLC_ADV_ENABLE);
	} else {
		ERR(APP,"Err: ADV send!\r\n");
	}
}
/***************************************************
 * @brief 		:soft-timer callback for the sending
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
int TEST_ADVPeriod_softtimer_cbk(void) {
	static u32 tbl_advData = 0;
	tbl_advData++;
	u8 adv_arr[] = { U32_BYTE0(tbl_advData), U32_BYTE1(tbl_advData), U32_BYTE2(tbl_advData), U32_BYTE3(tbl_advData) };
	//remove callback timer
//	blt_soft_timer_delete(&TEST_ADVPeriod_softtimer_cbk);
	fl_pack_t sending_data;
	sending_data = fl_packet_build(adv_arr,SIZEU8(adv_arr));
	fl_adv_sendFIFO_add(sending_data);
	return 0;
}
/***************************************************
 * @brief 		: TEST sending
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
void fl_adv_sendtest(void) {
	u32 period = 2000 * 1000;
	LOGA(APP,"FL ADV testing (%d)!!\r\n",period);
	blt_soft_timer_add(&TEST_ADVPeriod_softtimer_cbk,period);
}
/***************************************************
 * @brief 		:init periodic timer adv
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
void fl_adv_init(void) {
	fl_db_init();
#ifdef MASTER_CORE
	extern fl_master_config_t G_MASTER_INFO;
//	fl_input_external_init();
	//fl_adv_sendtest();
	fl_nwk_master_init();

	G_ADV_SETTINGS.nwk_chn.chn1 = &G_MASTER_INFO.nwk.chn[0];
	G_ADV_SETTINGS.nwk_chn.chn2 = &G_MASTER_INFO.nwk.chn[1];
	G_ADV_SETTINGS.nwk_chn.chn3 = &G_MASTER_INFO.nwk.chn[2];
	extern volatile u8 MASTER_INSTALL_STATE;
	FL_NWK_COLLECTION_MODE = &MASTER_INSTALL_STATE;
#else
	extern fl_nodeinnetwork_t G_INFORMATION;
	fl_nwk_slave_init();
	fl_repeater_init();

	G_ADV_SETTINGS.nwk_chn.chn1 = &G_INFORMATION.profile.nwk.chn[0];
	G_ADV_SETTINGS.nwk_chn.chn2 = &G_INFORMATION.profile.nwk.chn[1];
	G_ADV_SETTINGS.nwk_chn.chn3 = &G_INFORMATION.profile.nwk.chn[2];
	FL_NWK_COLLECTION_MODE = &G_INFORMATION.profile.run_stt.join_nwk;
#endif
	// Init REQ call RSP
	fl_queue_REQnRSP_TimeoutInit();
	//
	rf_set_power_level_index(MY_RF_POWER_INDEX);
	blc_ll_setAdvCustomedChannel(*G_ADV_SETTINGS.nwk_chn.chn1,*G_ADV_SETTINGS.nwk_chn.chn2,*G_ADV_SETTINGS.nwk_chn.chn3);
	fl_adv_scanner_init();
#ifdef MASTER_CORE
	//Start network
	fl_nwk_protocol_InitnRun();
#endif
}
/***************************************************
 * @brief 		:init collection channel (0,1,2)
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
void fl_adv_collection_channel_init(void){
//	while ((blc_ll_getCurrentState() == BLS_LINK_STATE_SCAN && blc_ll_getCurrentState() == BLS_LINK_STATE_ADV)) {
//	};
//	while(F_SENDING_STATE){bls_ll_setAdvEnable(BLC_ADV_DISABLE); }; //adv disable

	bls_ll_setAdvEnable(BLC_ADV_DISABLE);  //adv disable
	rf_set_power_level_index(MY_RF_POWER_INDEX);
	//clear all G_SENDING
	FL_QUEUE_CLEAR(&G_QUEUE_SENDING,QUEUE_SENDING_SIZE);
	blc_ll_setAdvCustomedChannel(0,1,2);

	blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT);
	blc_hci_registerControllerEventHandler(fl_controller_event_callback);
	//report all adv
	blc_ll_setScanParameter(SCAN_TYPE_ACTIVE,G_ADV_SETTINGS.scan_interval,G_ADV_SETTINGS.scan_window,OWN_ADDRESS_PUBLIC,SCAN_FP_ALLOW_ADV_ANY);
	blc_ll_addScanningInAdvState();  //add scan in adv state
	blc_ll_setScanEnable(1,0);
	bls_ll_setAdvEnable(BLC_ADV_ENABLE);  //adv enable
	FL_QUEUE_CLEAR(&G_DATA_CONTAINER,IN_DATA_SIZE);

	P_INFO("Collection Init(%d): %d| %d| %d\r\n",bls_ll_setAdvEnable(BLC_ADV_ENABLE),*G_ADV_SETTINGS.nwk_chn.chn1,*G_ADV_SETTINGS.nwk_chn.chn2,*G_ADV_SETTINGS.nwk_chn.chn3);  //adv enable

}
/***************************************************
 * @brief 		:de-init collection channel (0,1,2) => rollback nwk channel
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
void fl_adv_collection_channel_deinit(void){
//	bls_ll_setAdvEnable(BLC_ADV_DISABLE);
//	while (blc_ll_getCurrentState() == BLS_LINK_STATE_SCAN && blc_ll_getCurrentState() == BLS_LINK_STATE_ADV) {
//	};
//	while(F_SENDING_STATE){bls_ll_setAdvEnable(BLC_ADV_DISABLE); }; //adv disable
	bls_ll_setAdvEnable(BLC_ADV_DISABLE);  //adv disable
	rf_set_power_level_index(MY_RF_POWER_INDEX);
	FL_QUEUE_CLEAR(&G_QUEUE_SENDING,QUEUE_SENDING_SIZE);

	blc_ll_setAdvCustomedChannel(*G_ADV_SETTINGS.nwk_chn.chn1,*G_ADV_SETTINGS.nwk_chn.chn2,*G_ADV_SETTINGS.nwk_chn.chn3);

	blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT);
	blc_hci_registerControllerEventHandler(fl_controller_event_callback);
	//report all adv
	blc_ll_setScanParameter(SCAN_TYPE_ACTIVE,G_ADV_SETTINGS.scan_interval,G_ADV_SETTINGS.scan_window,OWN_ADDRESS_PUBLIC,SCAN_FP_ALLOW_ADV_ANY);
	blc_ll_addScanningInAdvState();  //add scan in adv state
	blc_ll_setScanEnable(1,0);
	bls_ll_setAdvEnable(BLC_ADV_ENABLE);  //adv enable
	FL_QUEUE_CLEAR(&G_DATA_CONTAINER,IN_DATA_SIZE);
	P_INFO("Collection Deinit(%d):%d |%d |%d\r\n",bls_ll_setAdvEnable(BLC_ADV_ENABLE),*G_ADV_SETTINGS.nwk_chn.chn1,*G_ADV_SETTINGS.nwk_chn.chn2,*G_ADV_SETTINGS.nwk_chn.chn3)
}

void fl_adv_setting_update(void) {
	blc_ll_setScanParameter(SCAN_TYPE_ACTIVE,G_ADV_SETTINGS.scan_interval,G_ADV_SETTINGS.scan_window,OWN_ADDRESS_PUBLIC,SCAN_FP_ALLOW_ADV_ANY);
}
/***************************************************
 * @brief 		:init adv scanner
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
void fl_adv_scanner_init(void) {
	LOG_P(ZIG,"FL scanner initialization !\r\n");
	blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT);
	blc_hci_registerControllerEventHandler(fl_controller_event_callback);
	//report all adv
	blc_ll_setScanParameter(SCAN_TYPE_ACTIVE,G_ADV_SETTINGS.scan_interval,G_ADV_SETTINGS.scan_window,OWN_ADDRESS_PUBLIC,SCAN_FP_ALLOW_ADV_ANY);
	blc_ll_addScanningInAdvState();  //add scan in adv state
	blc_ll_setScanEnable(1,0);
	bls_ll_setAdvEnable(BLC_ADV_ENABLE);  //adv enable
	FL_QUEUE_CLEAR(&G_DATA_CONTAINER,IN_DATA_SIZE);
}
/***************************************************
 * @brief 		: parse data array to data fields
 *
 * @param[in] 	: pointer pack
 *
 * @return	  	: 1 if success, 0 otherwise
 *
 ***************************************************/
u8 fl_packet_parse(fl_pack_t _pack, fl_dataframe_format_t *rslt) {
	fl_data_frame_u data_parse;
	if (_pack.length == SIZEU8(data_parse.bytes)) {
		memcpy(data_parse.bytes,_pack.data_arr,_pack.length);
		*rslt = data_parse.frame;
		return 1;
	}
	return 0;
}
/***************************************************
 * @brief 		:check packet rec IsMaster
 *
 * @param[in] 	:none
 *
 * @return	  	:-1 : fail, 0: SLAVE /REPEATER , 1 MASTER
 *
 ***************************************************/
s8 fl_adv_IsFromMaster(fl_pack_t data_in_queue) {
	fl_dataframe_format_t data_parsed;
	if (fl_packet_parse(data_in_queue,&data_parsed)) {
		if(data_parsed.endpoint.master == FL_FROM_MASTER || data_parsed.endpoint.master == FL_FROM_MASTER_ACK){
			return 1;
		}
		else return 0;
	}
	return -1;
}
#ifndef MASTER_CORE
/***************************************************
 * @brief 		:check packet rec From ME (only for slave)
 *
 * @param[in] 	:none
 *
 * @return	  	:
 *
 ***************************************************/
bool fl_adv_IsFromMe(fl_pack_t data_in_queue) {
	extern fl_nodeinnetwork_t G_INFORMATION;
	fl_dataframe_format_t data_parsed;
	if (fl_packet_parse(data_in_queue,&data_parsed)) {
		if((data_parsed.endpoint.master == FL_FROM_SLAVE || data_parsed.endpoint.master == FL_FROM_SLAVE_ACK) && data_parsed.slaveID.id_u8 == G_INFORMATION.slaveID.id_u8
				&& G_INFORMATION.slaveID.id_u8 != 0xFF){
			return true;
		}
	}
	return false;
}
bool fl_adv_MasterToMe(fl_pack_t data_in_queue) {
	extern fl_nodeinnetwork_t G_INFORMATION;
	fl_dataframe_format_t data_parsed;
	if (fl_packet_parse(data_in_queue,&data_parsed)) {
		if((data_parsed.endpoint.master == FL_FROM_MASTER|| data_parsed.endpoint.master == FL_FROM_MASTER_ACK)
				&& (
						(data_parsed.slaveID.id_u8 == G_INFORMATION.slaveID.id_u8 && G_INFORMATION.slaveID.id_u8 != 0xFF)
						|| (data_parsed.slaveID.id_u8 == 0xFF ||  G_INFORMATION.slaveID.id_u8 == 0xFF)
					)
			){
			return true;
		}
	}
	return false;
}
#endif
/***************************************************
 * @brief 		: build packet adv via the freelux protocol
 *
 * @param[in] 	: none
 *
 * @return	  	: 1 if success, 0 otherwise
 *
 ***************************************************/
fl_pack_t fl_packet_build(u8 *payload, u8 _len) {
	fl_pack_t pack_built;
	fl_data_frame_u packet;
	memset(packet.bytes,0,SIZEU8(packet.bytes));
	packet.frame.hdr = NWK_HDR_COLLECT; //testing
//	clock_time
	u32 timetamp = fl_rtc_get();
	packet.frame.timetamp[0] = U32_BYTE0(timetamp);
	packet.frame.timetamp[1] = U32_BYTE1(timetamp);
	packet.frame.timetamp[2] = U32_BYTE2(timetamp);
	packet.frame.timetamp[3] = U32_BYTE3(timetamp);
	//u8 data_test[] = {1,2,3,4,5};
	memcpy(packet.frame.payload,payload,_len);
	//for testing repeat level
//	static u8 rep_level = 0;
	packet.frame.endpoint.ep_u8 = 0;
	packet.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;
	pack_built.length = SIZEU8(packet.bytes) - 1; //skip rssi
	memcpy(pack_built.data_arr,packet.bytes,pack_built.length);
	return pack_built;
}
void fl_packet_printf(fl_dataframe_format_t _pack) {
	LOGA(BLE,"HDR:%02X\r\n",_pack.hdr);
//	P_PRINTFHEX_A(BLE,_pack.timetamp,SIZEU8(_pack.timetamp),"%s:","Timetamp");
	u32 time_cvt = MAKE_U32(_pack.timetamp[3],_pack.timetamp[2],_pack.timetamp[1],_pack.timetamp[0]);
	datetime_t datetime_cvt;
	fl_rtc_timestamp_to_datetime(time_cvt,&datetime_cvt);
//	LOGA(BLE,"Timetamp:%d\r\n",(u32 )time_cvt);
	LOGA(BLE,"Datetime:%d/%d/%d -%02d:%02d:%02d\r\n",datetime_cvt.year,datetime_cvt.month,datetime_cvt.day,datetime_cvt.hour,datetime_cvt.minute,
			datetime_cvt.second);
	P_PRINTFHEX_A(BLE,_pack.payload,SIZEU8(_pack.payload),"%s:","Payload");
	LOGA(BLE,"From:%s\r\n",_pack.endpoint.master == 1 ? "MASTER" : "NODE");
	LOGA(BLE,"Repeat_cnt:%d\r\n",_pack.endpoint.repeat_cnt);
	LOGA(BLE,"RSSI:%d\r\n",_pack.rssi);
}

/***************************************************
 * @brief 		: send adv report periodic
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
void fl_adv_run(void) {
	//todo: process adv ....
	fl_pack_t data_in_queue;
	if (FL_QUEUE_GET(&G_DATA_CONTAINER,&data_in_queue)) {
//		LOGA(APP,"QUEUE GET : (%d)%d-%d\r\n",G_DATA_CONTAINER.count,G_DATA_CONTAINER.head_index,G_DATA_CONTAINER.tail_index);
		fl_dataframe_format_t data_parsed;
		if (fl_packet_parse(data_in_queue,&data_parsed)) {
//			fl_packet_printf(data_parsed);
#ifdef MASTER_CORE
			fl_nwk_master_run(&data_in_queue); //process reponse from the slaves
#else //SLAVE
			//Todo: Repeat process
			if (fl_adv_IsFromMe(data_in_queue) == false && data_parsed.endpoint.repeat_cnt > 0) {
				//check repeat_mode
				fl_repeat_run(&data_in_queue);
			}
			//Todo: Handle FORM MASTER REQ
			//if (fl_adv_IsFromMaster(data_in_queue))
			if(fl_adv_MasterToMe(data_in_queue))
			{
				fl_nwk_slave_run(&data_in_queue);
			}
#endif
		} else {
			ERR(APP,"ERR <PARSE PACKET>!!\r\n");
			P_PRINTFHEX_A(APP,data_in_queue.data_arr,data_in_queue.length,"%s(%d):","PACK",data_in_queue.length);
		}
	}
#ifdef  MASTER_CORE
	fl_nwk_master_process();
#else
	//Features processor
	fl_nwk_slave_process();
#endif
	/* SEND ADV */
	fl_adv_sendFIFO_run();
}
