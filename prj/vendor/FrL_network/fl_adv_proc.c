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

/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
#ifdef MASTER_CORE
#define SEND_TIMEOUT_MS		50 //ms
#else
#define SEND_TIMEOUT_MS		50 //ms
extern _attribute_data_retention_ volatile fl_timetamp_withstep_t ORIGINAL_MASTER_TIME;
#endif
_attribute_data_retention_ volatile u8 F_SENDING_STATE = 0;

fl_adv_settings_t G_ADV_SETTINGS = {
		.adv_interval_min = ADV_INTERVAL_20MS,
		.adv_interval_max = ADV_INTERVAL_30MS,
		.adv_duration = SEND_TIMEOUT_MS,
		.scan_interval = SCAN_INTERVAL_60MS,
		.scan_window = SCAN_WINDOW_60MS,
		//.nwk_chn = {10,11,12}
		};

extern volatile u8  REPEAT_LEVEL;

/*---------------- Total ADV Rec --------------------------*/
#define IN_DATA_SIZE 		128
fl_pack_t g_data_array[IN_DATA_SIZE];
fl_data_container_t G_DATA_CONTAINER = { .data = g_data_array, .head_index = 0, .tail_index = 0, .mask = IN_DATA_SIZE - 1, .count = 0 };

/*---------------- ADV SEND QUEUE --------------------------*/
#define QUEUE_SENDING_SIZE 		16
fl_pack_t g_sending_array[QUEUE_SENDING_SIZE];
fl_data_container_t G_QUEUE_SENDING = { .data = g_sending_array, .head_index = 0, .tail_index = 0, .mask = QUEUE_SENDING_SIZE - 1, .count = 0 };
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
				memcpy(incomming_data.data_arr,pa->data,incomming_data.length);

#ifdef MASTER_CORE
				//skip from  master
				if (fl_adv_IsFromMaster(incomming_data)) {
					return 0;
				}
#else
				if (!fl_nwk_slave_checkHDR(pa->data[0])) {
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
#ifndef MASTER_CORE
/***************************************************
 * @brief 		:Align QUEUE SENDING
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
bool _align_QUEUE_SENDING(fl_pack_t _pack){
	typedef struct {
		fl_pack_t two_pack[2];
		u8 origin_indx;
		u8 loop_times;
	}manage_sending_loop_t;

	static manage_sending_loop_t man_loop={.origin_indx = 0xFF,.loop_times = 1, .two_pack = {{.length = 0},{.length =0}}};
	//update pack_arr
	man_loop.two_pack[0] = man_loop.two_pack[1];
	man_loop.two_pack[1] = _pack;
	//find orginal index
	u32  timetamp_in_pack = fl_rtc_timetamp2milltampStep(fl_adv_timetampStepInPack(man_loop.two_pack[1]));
	u32  timetamp_in_pre_pack = fl_rtc_timetamp2milltampStep(fl_adv_timetampStepInPack(man_loop.two_pack[0]));

	if(timetamp_in_pack && timetamp_in_pre_pack){ //  parse success
		//check master original of the previous with currently
		if(timetamp_in_pack > timetamp_in_pre_pack && timetamp_in_pack >= fl_rtc_timetamp2milltampStep(ORIGINAL_MASTER_TIME)){
//			man_loop.origin_indx = idx_inQUEUE;
			man_loop.loop_times = (man_loop.loop_times > REPEAT_LEVEL) ? 1 : man_loop.loop_times + 1;
		}
		//Check loop_times to skip packet with ttl < loop_times
		fl_dataframe_format_t data_parsed;
		man_loop.two_pack[1].length+=1;
		u8 rslt_parser=fl_packet_parse(man_loop.two_pack[1],&data_parsed);
//		LOGA(BLE,"LoopTimes:%d|(%d)TTL:%d\r\n",man_loop.loop_times,rslt_parser,data_parsed.endpoint.repeat_cnt);
//		LOGA(BLE,"[0].len:%d | [1].len:%d\r\n",man_loop.two_pack[0].length,man_loop.two_pack[1].length);
		if (rslt_parser) {
//			LOGA(BLE,"LoopTimes:%d|(%d)TTL:%d\r\n",man_loop.loop_times,rslt_parser,data_parsed.endpoint.repeat_cnt);
			return (data_parsed.endpoint.repeat_cnt <= man_loop.loop_times);
		}
	}
	else{
		if (_pack.length != 0) {
			man_loop.two_pack[0] = _pack;
			man_loop.two_pack[1] = _pack;
		}
	}
	return false;
}
#endif

/***************************************************
 * @brief 		: add data payload adv into queue fifo
 *
 * @param[in] 	:packet
 *
 * @return	  	:-1 false otherwise true
 *
 ***************************************************/
int fl_adv_sendFIFO_add(fl_pack_t _pack) {
	if (FL_QUEUE_FIND(&G_QUEUE_SENDING,&_pack,_pack.length /* - 1 skip rssi*/) == -1) {
		if (FL_QUEUE_ADD(&G_QUEUE_SENDING,&_pack) < 0) {
			ERR(BLE,"Err <QUEUE ADD ADV SENDING>!!\r\n");
			return -1;
		} else {
//			P_PRINTFHEX_A(BLE,_pack.data_arr,_pack.length,"%s(%d):","QUEUE ADV",_pack.length);
			return G_QUEUE_SENDING.tail_index;
		}
	}
	ERR(BLE,"Err <QUEUE ALREADY ADV SENDING>!!\r\n");
	return -1;
}
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
	if (!F_SENDING_STATE) {

#ifdef MASTER_CORE
		if (FL_QUEUE_GET(&G_QUEUE_SENDING,&data_in_queue)) {
#else
		while (FL_QUEUE_GET_LOOP(&G_QUEUE_SENDING,&data_in_queue)) {

			fl_timetamp_withstep_t  timetamp_inpack = fl_adv_timetampStepInPack(data_in_queue);
			u32 inpack = fl_rtc_timetamp2milltampStep(timetamp_inpack);
			u32 origin_pack = fl_rtc_timetamp2milltampStep(ORIGINAL_MASTER_TIME);
			if ( inpack < origin_pack){
//				LOGA(APP,"timetamp:%d | %d \r\n",inpack,origin_pack);
//				P_PRINTFHEX_A(APP,data_in_queue.data_arr,data_in_queue.length,"[%d]TTL(%d):",G_QUEUE_SENDING.head_index,
//						data_in_queue.data_arr[data_in_queue.length - 1] & 0x03);
				return;
			}
			//For debuging
//			P_PRINTFHEX_A(BLE,data_in_queue.data_arr,data_in_queue.length,"[%d]TTL(%d):",G_QUEUE_SENDING.head_index,
//					data_in_queue.data_arr[data_in_queue.length - 1] & 0x03);

#endif
//			LOGA(APP,"ADV FIFO(%d):%d/%d\r\n",G_QUEUE_SENDING.count,G_QUEUE_SENDING.head_index,G_QUEUE_SENDING.tail_index);
			F_SENDING_STATE = 1;
			fl_adv_send(data_in_queue.data_arr,data_in_queue.length,G_ADV_SETTINGS.adv_duration);
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
	while (blc_ll_getCurrentState() == BLS_LINK_STATE_SCAN && blc_ll_getCurrentState() == BLS_LINK_STATE_ADV) {
	};
	if (_data && _size >= 2) {
//		LOG_P(APP,"Sending..... \r\n");
		rf_set_power_level_index(MY_RF_POWER_INDEX);
//		bls_ll_setAdvEnable(BLC_ADV_ENABLE);
		u8 mac[6];
		own_addr_type_t app_own_address_type = OWN_ADDRESS_PUBLIC;
		memcpy(mac,(app_own_address_type == OWN_ADDRESS_PUBLIC) ? blc_ll_get_macAddrPublic() : blc_ll_get_macAddrRandom(),6);
		u8 status = bls_ll_setAdvParam(G_ADV_SETTINGS.adv_interval_min,G_ADV_SETTINGS.adv_interval_max,ADV_TYPE_SCANNABLE_UNDIRECTED,
				app_own_address_type,0,NULL,BLT_ENABLE_ADV_ALL,ADV_FP_NONE);
		if (status != BLE_SUCCESS) {
			ERR(BLE,"Set ADV param is FAIL !!!\r\n")
			while (1)
				;
		}  //debug: adv setting err
		bls_ll_setAdvData(_data,_size);
		bls_ll_setAdvDuration(_timeout_ms * 1000,1); // ms->us
		bls_app_registerEventCallback(BLT_EV_FLAG_ADV_DURATION_TIMEOUT,&fl_durationADV_timeout_proccess);
//		TICK_GET_PROCESSING_TIME = clock_time();
//		LOGA(BLE,"Scan time:%d\r\n",(clock_time()-TICK_GET_PROCESSING_TIME)/SYSTEM_TIMER_TICK_1MS);
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
	//fl_adv_sendtest();
	fl_nwk_master_init();
	fl_input_external_init();

	G_ADV_SETTINGS.nwk_chn.chn1 = &G_MASTER_INFO.nwk.chn[0];
	G_ADV_SETTINGS.nwk_chn.chn2 = &G_MASTER_INFO.nwk.chn[1];
	G_ADV_SETTINGS.nwk_chn.chn3 = &G_MASTER_INFO.nwk.chn[2];

#else
	//fl_input_external_init();
	extern fl_nodeinnetwork_t G_INFORMATION;
	fl_nwk_slave_init();
	fl_repeater_init();

	G_ADV_SETTINGS.nwk_chn.chn1 = &G_INFORMATION.profile.nwk.chn[0];
	G_ADV_SETTINGS.nwk_chn.chn2 = &G_INFORMATION.profile.nwk.chn[1];
	G_ADV_SETTINGS.nwk_chn.chn3 = &G_INFORMATION.profile.nwk.chn[2];
#endif
	// Init REQ call RSP
	fl_queue_REQnRSP_TimeoutStart();
	//
	rf_set_power_level_index(MY_RF_POWER_INDEX);
	blc_ll_setAdvCustomedChannel(*G_ADV_SETTINGS.nwk_chn.chn1,*G_ADV_SETTINGS.nwk_chn.chn2,*G_ADV_SETTINGS.nwk_chn.chn3);
	fl_adv_scanner_init();
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
	packet.frame.endpoint.repeat_cnt = REPEAT_LEVEL;

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
				fl_repeat_run(&data_in_queue);
			}
			//Todo: Handle FORM MASTER REQ
			if (fl_adv_IsFromMaster(data_in_queue)) {
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
