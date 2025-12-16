/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_wifi2ble_fota.c
 *Created on		: Oct 13, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/


/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
#include "tl_common.h"
#include "fl_nwk_protocol.h"
#include "fl_adv_proc.h"
#include "fl_nwk_handler.h"
#include "fl_ble_wifi.h"
#include "fl_wifi2ble_fota.h"

/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
extern fl_adv_settings_t G_ADV_SETTINGS;
//
typedef struct {
	u16 slot_avaible;
	u16 slot_sent;
	struct {
		u8 timeout_exit;
	} settings;
	struct {
		s16 push_return;
	} runtime;
}__attribute__((packed)) fl_wifi2ble_fota_runtime_t;

/*---------------- FW ADV SEND QUEUE --------------------------*/
fl_pack_t g_fw_sending_array[FOTA_SIZE];
fl_data_container_t G_FW_QUEUE_SENDING = { .data = g_fw_sending_array, .head_index = 0, .tail_index = 0, .mask = FOTA_SIZE - 1, .count = 0 };

#define FOTA_FW_QUEUE_SIZE			(G_FW_QUEUE_SENDING.mask + 1)
#define FOTA_RETRY_MAX				(2)

u8 F_EXTITFOTA_TIME = 60;//s

/*------------------ MAIN STRUCT -------------------------------*/
fl_wifi2ble_fota_runtime_t G_FOTA = {
									.slot_avaible = 0,
									.slot_sent = 0,
									.settings = { .timeout_exit = 60 },
									.runtime = { .push_return = FOTA_EXIT_VALUE }
									};

/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/
#define IsFOTA_Run() 			{if(G_FOTA.runtime.push_return == FOTA_EXIT_VALUE)return G_FOTA.runtime.push_return;}

u16 FL_NWK_FOTA_IsReady(void){
	return G_FW_QUEUE_SENDING.count;
}
/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/
#ifdef MASTER_CORE
#define IsFOTA_Run() 			{if(G_FOTA.runtime.push_return == FOTA_EXIT_VALUE)return G_FOTA.runtime.push_return;}

int _fota_timeout_expired(void){
	ERR(FILE,"FOTA TIMEOUT-Expired %d s\r\n",F_EXTITFOTA_TIME);
	G_FOTA.runtime.push_return = FOTA_EXIT_VALUE;
	return -1;
}

/***************************************************
 * @brief 		: generate fota fw packer for the adv sending
 *
 * @param[in] 	:mac: slave (0xFF,0xFF,0xFF,0xFF,0xFF,0xFF : Broadcast)
 * 				_data: fw to upload
 * 				_size: size of _data
 * 				_ack : get rsp from the slave (0:non-ack /1: ack)
 *
 * @return	  	: pack with the fl_pack_t type
 *
 ***************************************************/
fl_pack_t _fota_fw_packet_build(u8* _slave_mac,u8* _data, u8 _len,bool _ack){
	/**************************************************************************/
	/* | HDR | Timetamp | Mill_time | SlaveID | payload | crc8_payload | Ep | */
	/* | 1B  |   4Bs    |    1B     |    1B   |   22Bs  |   	1B	   | 1B | -> .master = FL_FROM_MASTER_ACK / FL_FROM_MASTER */
	/**************************************************************************/
	fl_pack_t rslt = {.length = 0};
	s8 slaveID = fl_master_Node_find(_slave_mac);
	if (slaveID == -1 && !IS_MAC_INVALID(_slave_mac,0xFF)) {
		ERR(FILE,"SlaveID NOT FOUND!!\r\n");
		return rslt;
	}
	else{
		if(IS_MAC_INVALID(_slave_mac,0xFF)){
			slaveID = 0xFF; //broadcast to network
		}
	}
	//generate seqtimetamp
	fl_timetamp_withstep_t timetampStep = fl_rtc_getWithMilliStep();
	fl_data_frame_u req_pack;
	/*Create common packet */
	req_pack.frame.hdr = NWK_HDR_FOTA;
	req_pack.frame.timetamp[0] = U32_BYTE0(timetampStep.timetamp);
	req_pack.frame.timetamp[1] = U32_BYTE1(timetampStep.timetamp);
	req_pack.frame.timetamp[2] = U32_BYTE2(timetampStep.timetamp);
	req_pack.frame.timetamp[3] = U32_BYTE3(timetampStep.timetamp);
	//Add new mill-step
	req_pack.frame.milltamp = timetampStep.milstep;

	req_pack.frame.slaveID = slaveID;
	//Create payload
	memset(req_pack.frame.payload,0x0,SIZEU8(req_pack.frame.payload));
	memcpy(req_pack.frame.payload,_data,_len);
	//crc
	req_pack.frame.crc8 = fl_crc8(req_pack.frame.payload,SIZEU8(req_pack.frame.payload));

	//create endpoint => always set below
	req_pack.frame.endpoint.dbg = 0;
	req_pack.frame.endpoint.repeat_cnt = 3;
	req_pack.frame.endpoint.rep_settings = 3;
	req_pack.frame.endpoint.repeat_mode = 0;
	//Create packet from slave
	req_pack.frame.endpoint.master = _ack==0?FL_FROM_MASTER:FL_FROM_MASTER_ACK;

	//copy to resutl data struct
	rslt.length = SIZEU8(req_pack.bytes) - 1; //skip rssi
	memcpy(rslt.data_arr,req_pack.bytes,rslt.length );
//	LOGA(FILE,"Send %02X REQ to Slave %d:%d/%d\r\n",req_pack.frame.hdr,slaveID,timetampStep.timetamp,timetampStep.milstep);
	P_PRINTFHEX_A(INF_FILE,rslt.data_arr,rslt.length,"REQ %X(%s)->%d(0x%02X):",
				req_pack.frame.hdr,req_pack.frame.endpoint.master==FL_FROM_MASTER_ACK?"ack":"non-ack",slaveID,slaveID);

	return rslt;
}
/***************************************************
 * @brief 		: push fw payload into the sending queues
 * 				 *this is the Broadcast cmd use to send the all network
 *
 * @param[in] 	:_fw : fw frame
 * 				 _len: size of the _fw
 * 				 _pack_type: begin = 0,end =2,data = 1
 *
 * @return	  	: -1: false, 0x7FFF
 *
 ***************************************************/
s16 fl_wifi2ble_fota_fwpush(u8 *_fw, u8 _len,fl_fota_pack_type_e _pack_type) {
	IsFOTA_Run();
	//broadcast
	u8 broadcast_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	fl_pack_t fw_pack = _fota_fw_packet_build(broadcast_mac,_fw,_len,0);
	u16 head = G_FW_QUEUE_SENDING.head_index;
	//add new step -> filter duplicate ble sending
	for (u8 indx = 0; indx < G_FW_QUEUE_SENDING.mask + 1; indx++) {
		if (plog_IndexOf(G_FW_QUEUE_SENDING.data[indx].data_arr, _fw,_len,G_FW_QUEUE_SENDING.data[indx].length)!= -1) {
//			ERR(MCU,"FOTA Duplicate.....\r\n");
			return -1;
		}
	}
	///===end filter
	if (G_FW_QUEUE_SENDING.count < FOTA_FW_QUEUE_SIZE) {
		do {
			if (G_FW_QUEUE_SENDING.data[head].length == 0) {
				G_FW_QUEUE_SENDING.data[head] = fw_pack;
				G_FW_QUEUE_SENDING.data[head].data_arr[FOTA_TYPEPACK_POSITION] = _pack_type;
				G_FW_QUEUE_SENDING.data[head].data_arr[FOTA_RETRY_POSITION] = 0;
				memset(&G_FW_QUEUE_SENDING.data[head].data_arr[FOTA_MAC_INCOM_POSITION],0xFF,FOTA_MAC_INCOM_SIZE);
				if (G_FW_QUEUE_SENDING.count < FOTA_FW_QUEUE_SIZE)
					G_FW_QUEUE_SENDING.count++;
				return head;
			}
			head = (head + 1) & G_FW_QUEUE_SENDING.mask;
		} while (head!= G_FW_QUEUE_SENDING.head_index);
	}
	return G_FOTA.runtime.push_return;
}

/***************************************************
 * @brief 		:Run step-by-step upload fw to slave
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/

int fl_wifi2ble_fota_system_end(u8 *_payload_end, u8 _len) {
	int rslt = -1;
	//if (G_FOTA.slot_sent == G_FW_QUEUE_SENDING.count && G_FOTA.slot_avaible == 0)
	if (G_FW_QUEUE_SENDING.count ==0){
		rslt = fl_wifi2ble_fota_fwpush(_payload_end,_len,FOTA_PACKET_END);
	}
	//todo: get rsp of the slave and recheck missing packet
//	P_INFO_HEX(_payload_end,_len,"%d|-> FOTA END:",rslt);
	return rslt;
}

int fl_wifi2ble_fota_system_begin(u8 *_payload_start,u8 _len){
	G_FOTA.runtime.push_return = -1;
	fl_wifi2ble_fota_ContainerClear();
	int rslt = fl_wifi2ble_fota_fwpush(_payload_start,_len,FOTA_PACKET_BEGIN);
	//todo: get rsp of the slave if wifi need
	return rslt;
}

int fl_wifi2ble_fota_system_data(u8 *_payload,u8 _len){
	return fl_wifi2ble_fota_fwpush(_payload,_len,FOTA_PACKET_DATA);
}

#else
/***************************************************
 * @brief 		: push fw payload into the sending queues
 * 				 *this is the Broadcast cmd use to send the all network
 * @param[in] 	:_fw : fw frame
 * 				 _len: size of the _fw
 * @return	  	: -1: false, otherwise true
 *
 ***************************************************/
s16 fl_wifi2ble_fota_fwpush(fl_pack_t *fw_pack, fl_fota_pack_type_e _pack_type) {
	s16 indx_4full= -1;
	if(_pack_type==FOTA_PACKET_BEGIN){
		fl_wifi2ble_fota_ContainerClear();
//		fl_slave_fota_RECclear();
	}
	u16 head = G_FW_QUEUE_SENDING.head_index;
	do{
		if (G_FW_QUEUE_SENDING.data[head].length == 0) {
			G_FW_QUEUE_SENDING.data[head].length = fw_pack->length - 1;
			memset(G_FW_QUEUE_SENDING.data[head].data_arr,0,SIZEU8(G_FW_QUEUE_SENDING.data[head].data_arr));
			memcpy(G_FW_QUEUE_SENDING.data[head].data_arr,fw_pack->data_arr,G_FW_QUEUE_SENDING.data[head].length);
			G_FW_QUEUE_SENDING.data[head].data_arr[FOTA_TYPEPACK_POSITION] = _pack_type;
			G_FW_QUEUE_SENDING.data[head].data_arr[FOTA_RETRY_POSITION] = 0;
			//Add mac_incomming
			memcpy(&G_FW_QUEUE_SENDING.data[head].data_arr[FOTA_MAC_INCOM_POSITION],&fw_pack->data_arr[FOTA_MAC_INCOM_POSITION],FOTA_MAC_INCOM_SIZE);
//			P_INFO_HEX(G_FW_QUEUE_SENDING.data[head].data_arr,SIZEU8(G_FW_QUEUE_SENDING.data[head].data_arr),"FOTA-GET(%d):",G_FW_QUEUE_SENDING.data[head].length);
			if (G_FW_QUEUE_SENDING.count < FOTA_FW_QUEUE_SIZE)
				G_FW_QUEUE_SENDING.count++;
			return head;
		}
//		else {
//			if (indx_4full == -1)
//				indx_4full = (head + 1) & G_FW_QUEUE_SENDING.mask;
//		}
		head=(head + 1) & G_FW_QUEUE_SENDING.mask;
	} while (head != G_FW_QUEUE_SENDING.head_index);
	//FULL -> add to mostRetry index
//	indx_4full = (head + 1) & G_FW_QUEUE_SENDING.mask;
//	G_FW_QUEUE_SENDING.data[indx_4full].length = fw_pack->length - 1;
//	memset(G_FW_QUEUE_SENDING.data[indx_4full].data_arr,0,SIZEU8(G_FW_QUEUE_SENDING.data[indx_4full].data_arr));
//	memcpy(G_FW_QUEUE_SENDING.data[indx_4full].data_arr,fw_pack->data_arr,G_FW_QUEUE_SENDING.data[indx_4full].length);
//	G_FW_QUEUE_SENDING.data[indx_4full].data_arr[FOTA_TYPEPACK_POSITION] = _pack_type;
//	G_FW_QUEUE_SENDING.data[indx_4full].data_arr[FOTA_RETRY_POSITION] = 0;
//	//Add mac_incomming
//	memcpy(&G_FW_QUEUE_SENDING.data[indx_4full].data_arr[FOTA_MAC_INCOM_POSITION],&fw_pack->data_arr[FOTA_MAC_INCOM_POSITION],6);
	return indx_4full;
}
#endif

/*****************************************************************************
 *    SYSTEM FOTA PROCESSOR - END
 *****************************************************************************/
void fl_wifi2ble_fota_ContainerClear(void){
	FL_QUEUE_CLEAR(&G_FW_QUEUE_SENDING,G_FW_QUEUE_SENDING.mask+1);
//	G_FOTA.runtime.push_return=-1;
}

/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/
void fl_wifi2ble_fota_init(void){
	LOG_P(INF_FILE,"FOTA Initilization!!!\r\n");
	fl_wifi2ble_fota_ContainerClear();
	DFU_OTA_INIT();
	//change version
	DFU_OTA_VERISON_SET(66);
#ifdef MASTER_CORE
	//set interval
	fl_wifi2ble_Fota_Interval_set(100);
#endif
}

s16 fl_wifi2ble_fota_recECHO(fl_pack_t _pack_rec,u8* _mac_incom){
	s16 rslt =-1;
#ifdef MASTER_CORE
//	IsFOTA_Run();
	if (G_FW_QUEUE_SENDING.count > 0 )
#endif
	{
//		P_INFO_HEX(_pack_rec.data_arr,SIZEU8(_pack_rec.data_arr),"ECHO:");
		for (s16 head = 0; head < G_FW_QUEUE_SENDING.mask + 1; head++) {
			if (G_FW_QUEUE_SENDING.data[head].length > FOTA_PACK_SIZE_MIN
				&& G_FW_QUEUE_SENDING.data[head].data_arr[FOTA_RETRY_POSITION] > 0 //sent yet
				&& plog_IndexOf(G_FW_QUEUE_SENDING.data[head].data_arr,&_pack_rec.data_arr[FOTA_FW_DATA_POSITION],FOTA_PACK_FW_SIZE,G_FW_QUEUE_SENDING.data[head].length) != -1
				&& plog_IndexOf(G_FW_QUEUE_SENDING.data[head].data_arr,_mac_incom,FOTA_MAC_INCOM_SIZE,SIZEU8(G_FW_QUEUE_SENDING.data[head].data_arr)) == -1)
			{
#ifdef MASTER_CORE
//				P_INFO_HEX(_mac_incom,6,"ECHO MAC:");
//				P_INFO_HEX(G_FW_QUEUE_SENDING.data[head].data_arr,SIZEU8(G_FW_QUEUE_SENDING.data[head].data_arr),"ECHO:");
				fl_fota_pack_type_e type_pack = G_FW_QUEUE_SENDING.data[head].data_arr[FOTA_TYPEPACK_POSITION];
#endif
				G_FW_QUEUE_SENDING.data[head].length = 0;
				memset(G_FW_QUEUE_SENDING.data[head].data_arr,0,SIZEU8(G_FW_QUEUE_SENDING.data[head].data_arr));
				if (G_FW_QUEUE_SENDING.count > 0)
					G_FW_QUEUE_SENDING.count--;
#ifdef MASTER_CORE
				if (G_FW_QUEUE_SENDING.count == 0 && type_pack == FOTA_PACKET_END) {
					blt_soft_timer_delete(_fota_timeout_expired);
					P_INFO("FOTA Done!!!\r\n");
					fl_wifi2ble_fota_ContainerClear();
					G_FOTA.runtime.push_return = FOTA_EXIT_VALUE;
					break;
				}
#else
//				if (type_packtype_pack == FOTA_PACKET_END) {
//					fl_wifi2ble_fota_ContainerClear();
//					fl_slave_fota_RECclear();
//				}
#endif
				rslt = head;
			}
			//P_INFO("head(cnt:%d):%d/%d\r\n",G_FW_QUEUE_SENDING.count,head,tail);
		}
	}
	return rslt;
}

#ifdef MASTER_CORE
s16 fl_wifi2ble_fota_proc(void) {
	IsFOTA_Run();
	G_FOTA.settings.timeout_exit = F_EXTITFOTA_TIME;
	G_FOTA.slot_sent=0;
	G_FOTA.slot_avaible=0;
	for (u16 indx = 0; indx < FOTA_FW_QUEUE_SIZE && G_FW_QUEUE_SENDING.count > 0; ++indx) {
		/* Refesh G_FOTA */
		if (G_FW_QUEUE_SENDING.data[indx].length > FOTA_PACK_SIZE_MIN) {
			if (G_FW_QUEUE_SENDING.data[indx].data_arr[FOTA_RETRY_POSITION] > 0) {
				G_FOTA.slot_sent++;
			} else {
				G_FOTA.slot_avaible++;
			}
		}
	}
	//exit processing if don't receive echo
	if (G_FOTA.slot_sent == G_FW_QUEUE_SENDING.count && G_FOTA.slot_avaible == 0) {
		if (blt_soft_timer_find(_fota_timeout_expired) == -1) {
//			P_INFO("FOTA Waiting Echo runtime(%d s)!!!\r\n",G_FOTA.settings.timeout_exit);
			blt_soft_timer_restart(_fota_timeout_expired,G_FOTA.settings.timeout_exit * 999 * 1010);
		}
	} else {
		if (G_FOTA.slot_sent < G_FW_QUEUE_SENDING.count && G_FOTA.slot_avaible > 0) {
			if (blt_soft_timer_find(_fota_timeout_expired) != -1) {
//				P_INFO("FOTA Echo Receipt !! (%d/%d|cnt:%d)\r\n",G_FOTA.slot_sent,G_FOTA.slot_avaible,G_FW_QUEUE_SENDING.count);
				blt_soft_timer_delete(_fota_timeout_expired);
			}
		}
	}
	/*-------------------*/
	return 0;
}
#endif


s16 fl_wifi2ble_fota_run(void) {
	extern volatile u8 F_SENDING_STATE;
	fl_pack_t his_data_in_queue;
	if (!F_SENDING_STATE) {
#ifdef MASTER_CORE
		IsFOTA_Run();
#endif
		u16 head = G_FW_QUEUE_SENDING.head_index;
		//for (u16 sample_scan = 0; sample_scan < FOTA_FW_QUEUE_SIZE && G_FW_QUEUE_SENDING.count>0 && !F_SENDING_STATE; sample_scan++)
		while(!F_SENDING_STATE && G_FW_QUEUE_SENDING.count>0){
			his_data_in_queue = G_FW_QUEUE_SENDING.data[G_FW_QUEUE_SENDING.head_index];
			if (his_data_in_queue.length > FOTA_PACK_SIZE_MIN) { //minimun size of the fota
				//FOR SLAVE
				//if (his_data_in_queue.data_arr[FOTA_RETRY_POSITION] % 2 == 0)
				{
//					P_INFO_HEX(his_data_in_queue.data_arr,SIZEU8(his_data_in_queue.data_arr),"FOTA-SEND(%d):",his_data_in_queue.length);
					fl_adv_send(his_data_in_queue.data_arr,his_data_in_queue.length,G_ADV_SETTINGS.adv_duration);
				}
				G_FW_QUEUE_SENDING.data[G_FW_QUEUE_SENDING.head_index].data_arr[FOTA_RETRY_POSITION]++;
//				// Clear if over-timeout
				if (G_FW_QUEUE_SENDING.data[G_FW_QUEUE_SENDING.head_index].data_arr[FOTA_RETRY_POSITION] > FOTA_RETRY_MAX) {
					G_FW_QUEUE_SENDING.data[G_FW_QUEUE_SENDING.head_index].length = 0;
					memset(G_FW_QUEUE_SENDING.data[G_FW_QUEUE_SENDING.head_index].data_arr,0,SIZEU8(G_FW_QUEUE_SENDING.data[G_FW_QUEUE_SENDING.head_index].data_arr));
					if (G_FW_QUEUE_SENDING.count > 0) {
						G_FW_QUEUE_SENDING.count--;
					}
#ifdef MASTER_CORE
					if (G_FW_QUEUE_SENDING.count == 0 && G_FW_QUEUE_SENDING.data[G_FW_QUEUE_SENDING.head_index].data_arr[FOTA_TYPEPACK_POSITION] == FOTA_PACKET_END) {
						blt_soft_timer_delete(_fota_timeout_expired);
						P_INFO("FOTA Done!!!\r\n");
						fl_wifi2ble_fota_ContainerClear();
						G_FOTA.runtime.push_return = FOTA_EXIT_VALUE;
					}
#endif
				}
			}
			G_FW_QUEUE_SENDING.head_index = (G_FW_QUEUE_SENDING.head_index + 1) & G_FW_QUEUE_SENDING.mask;
			if(head == G_FW_QUEUE_SENDING.head_index) break;
		}
//#else
//		IsFOTA_Run();
//		his_data_in_queue = G_FW_QUEUE_SENDING.data[G_FW_QUEUE_SENDING.head_index];
//		if (G_FW_QUEUE_SENDING.count > 0 && his_data_in_queue.length > FOTA_PACK_SIZE_MIN) {
//			//if (his_data_in_queue.data_arr[FOTA_RETRY_POSITION]% FOTA_RETRY_MAX == 0)
//			{
//				P_INFO_HEX(his_data_in_queue.data_arr,his_data_in_queue.length,"[%d/%d]FOTA(retry:%d):",G_FW_QUEUE_SENDING.head_index,G_FW_QUEUE_SENDING.count,his_data_in_queue.data_arr[FOTA_RETRY_POSITION]);
//				fl_adv_send(his_data_in_queue.data_arr,his_data_in_queue.length,G_ADV_SETTINGS.adv_duration);
//			}
//			//update num of retry
//			G_FW_QUEUE_SENDING.data[G_FW_QUEUE_SENDING.head_index].data_arr[FOTA_RETRY_POSITION]++;
//			// retry processor
//			if (G_FW_QUEUE_SENDING.data[G_FW_QUEUE_SENDING.head_index].data_arr[FOTA_RETRY_POSITION] > FOTA_RETRY_MAX) {
////				G_FW_QUEUE_SENDING.data[indx].data_arr[FOTA_MILSTEP_POSITION] += RAND_INT(-50,50);
//				memset(G_FW_QUEUE_SENDING.data[G_FW_QUEUE_SENDING.head_index].data_arr,0,
//						SIZEU8(G_FW_QUEUE_SENDING.data[G_FW_QUEUE_SENDING.head_index].data_arr));
//				G_FW_QUEUE_SENDING.data[G_FW_QUEUE_SENDING.head_index].length=0;
//				if (G_FW_QUEUE_SENDING.count > 0) {
//					G_FW_QUEUE_SENDING.count--;
//				}
//			}
//		}
//		G_FW_QUEUE_SENDING.head_index = (G_FW_QUEUE_SENDING.head_index + 1) & G_FW_QUEUE_SENDING.mask;
//#endif
	}
	return 0;
}

