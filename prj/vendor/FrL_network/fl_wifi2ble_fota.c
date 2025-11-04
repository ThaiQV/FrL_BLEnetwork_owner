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

#ifdef MASTER_CORE
/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
extern fl_adv_settings_t G_ADV_SETTINGS;

typedef enum{
	FOTA_NONE = 0,
	FOTA_READY,
	FOTA_SENDING,
	FOTA_EXPIRED,
}fl_fota_status_e;

typedef struct {
	u32 idx_addr;
	fl_pack_t fw; //max size of the each upload
	u8 status_proc;
}__attribute__((packed)) fl_fota_container_t;

fl_fota_container_t G_FOTA_FW[FOTA_SIZE]; // => maximum size for each sending
u16 F_FOTA_FW_FULL = 0;
/*---------------- FW ADV SEND QUEUE --------------------------*/
fl_pack_t g_fw_sending_array[FOTA_SIZE];
fl_data_container_t G_FW_QUEUE_SENDING = { .data = g_fw_sending_array, .head_index = 0, .tail_index = 0, .mask = FOTA_SIZE - 1, .count = 0 };

#define FOTA_CONTAINER_SIZE			(sizeof(G_FOTA_FW)/sizeof(G_FOTA_FW[0]))
#define FOTA_FW_EXPIRED				32 //num of packs mistake echo message
/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/
int fl_wifi2ble_fota_ECHO_timedout(void);
int _fota_sending_lastframe(void);

u8 FL_NWK_FOTA_IsReady(void){
	return G_FW_QUEUE_SENDING.count;
}

/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/
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
 *
 * @return	  	: -1: false, otherwise true
 *
 ***************************************************/
s16 fl_wifi2ble_fota_fwpush(u32 _indx_addr,u8 *_fw, u8 _len) {
	//broadcast
	u8 broadcast_mac[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	fl_pack_t fw_pack = _fota_fw_packet_build(broadcast_mac,_fw,_len,0);
	for (u16 indx = 0; indx < FOTA_CONTAINER_SIZE; ++indx) {
		if(G_FOTA_FW[indx].status_proc == FOTA_NONE){
			G_FOTA_FW[indx].idx_addr = _indx_addr;
			G_FOTA_FW[indx].fw = fw_pack;
			G_FOTA_FW[indx].status_proc = FOTA_READY;
			F_FOTA_FW_FULL=0;
			return indx;
		}
	}
	F_FOTA_FW_FULL=1;
	return -1;
}

/*************************************************************************************************************************************************
 *    SYSTEM FOTA PROCESSOR
 *************************************************************************************************************************************************/

/***************************************************
 * @brief 		:Run step-by-step upload fw to slave
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
int fl_wifi2ble_fota_system_end(u8 *_payload_end,u8 _len){
	int rslt =  fl_wifi2ble_fota_fwpush(0,_payload_end,_len);
	//todo: get rsp of the slave and recheck missing packet
	return rslt;
}

int fl_wifi2ble_fota_system_start(u8 *_payload_start,u8 _len){
//	P_INFO_HEX(_payload_start,_len,"|-> FOTA START PACKET:");
	int rslt = fl_wifi2ble_fota_fwpush(0,_payload_start,_len);
	//todo: get rsp of the slave if wifi need
	return rslt;
}

/*******************************************************************************************************
 *    SYSTEM FOTA PROCESSOR - END
 *******************************************************************************************************/
void fl_wifi2ble_fota_ContainerClear(void){
	FL_QUEUE_CLEAR(&G_FW_QUEUE_SENDING,G_FW_QUEUE_SENDING.mask+1);
	for (u16 var = 0; var < FOTA_CONTAINER_SIZE; ++var) {
		G_FOTA_FW[var].idx_addr = 0;
		memset(G_FOTA_FW[var].fw.data_arr,0,SIZEU8(G_FOTA_FW[var].fw.data_arr));
		G_FOTA_FW[var].fw.length=0;
		G_FOTA_FW[var].status_proc = FOTA_NONE;
		F_FOTA_TIMEOUT = 0;
	}
}
/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/

int fl_wifi2ble_fota_ECHO_timedout(void){
//	if (F_FOTA_TIMEOUT >= FOTA_FW_EXPIRED)  {
//		P_INFO("FOTA retry packets without the echo message (%d) !!!\r\n",FOTA_FW_EXPIRED);
//		for (u16 indx = 0; indx < FOTA_CONTAINER_SIZE; ++indx) {
//			if (G_FOTA_FW[indx].status_proc == FOTA_SENDING) {
//				G_FOTA_FW[indx].status_proc = FOTA_READY; //send againt
//			}
//		}
//		F_FOTA_TIMEOUT = 0;
//	}
	return 0;
}

void fl_wifi2ble_fota_init(void){
	LOG_P(INF_FILE,"FOTA Initilization!!!\r\n");
	fl_wifi2ble_fota_ContainerClear();
//	DFU_OTA_INIT();
}

void fl_wifi2ble_fota_recECHO(fl_pack_t _pack_rec){
//	for (u16 indx = 0; indx < FOTA_CONTAINER_SIZE; ++indx) {
//		if(0 == memcmp(G_FOTA_FW[indx].fw.data_arr,_pack_rec.data_arr,G_FOTA_FW[indx].fw.length)){
//			if (G_FOTA_FW[indx].status_proc == FOTA_SENDING) {
//				F_FOTA_TIMEOUT--;
//				P_INFO("REC F_FOTA_TIMEOUT:%d\r\n",F_FOTA_TIMEOUT);
//				G_FOTA_FW[indx].status_proc = FOTA_NONE;
//				if(F_FOTA_TIMEOUT == 0){
//					blt_soft_timer_delete(_fota_sending_lastframe);
//				}
//			}
//		}
//	}
}

void fl_wifi2ble_fota_proc(void) {
	for (u16 indx = 0; indx < FOTA_CONTAINER_SIZE; ++indx) {
		if (G_FOTA_FW[indx].status_proc == FOTA_READY) {
			if (FL_QUEUE_FIND(&G_FW_QUEUE_SENDING,&G_FOTA_FW[indx].fw,G_FOTA_FW[indx].fw.length - 1) == -1) {
				if (FL_QUEUE_ADD(&G_FW_QUEUE_SENDING,&G_FOTA_FW[indx].fw) < 0) {
					ERR(BLE,"Err FULL <QUEUE ADD FW ADV SENDING>!!\r\n");
					return;
				} else {
					// success
					G_FOTA_FW[indx].status_proc =FOTA_SENDING;
					//exit to step-one-step
					return;
				}
			}
			ERR(BLE,"Err <QUEUE ALREADY FW ADV SENDING>!!\r\n");
		}
	}
}

int _fota_sending_lastframe(void) {
	P_INFO("FOTA Last Frame has sent yet!!\r\n");
//	F_FOTA_TIMEOUT=FOTA_FW_EXPIRED;
//	fl_wifi2ble_fota_ECHO_timedout();
	P_INFO("FOTA retry packets without the echo message (%d) !!!\r\n",F_FOTA_TIMEOUT);
	for (u16 indx = 0; indx < FOTA_CONTAINER_SIZE; ++indx) {
		if (G_FOTA_FW[indx].status_proc == FOTA_SENDING) {
			fl_wifi2ble_fota_fwpush(0,&G_FOTA_FW[indx].fw.data_arr[7],22);
		}
	}
	F_FOTA_TIMEOUT=0;
	return -1;
}

void fl_wifi2ble_fota_run(void) {
	extern volatile u8 F_SENDING_STATE;
	fl_pack_t his_data_in_queue;
	if (!F_SENDING_STATE) {
		if (FL_QUEUE_GET(&G_FW_QUEUE_SENDING,&his_data_in_queue)) {
			fl_adv_send(his_data_in_queue.data_arr,his_data_in_queue.length,G_ADV_SETTINGS.adv_duration);
			P_PRINTFHEX_A(APP,his_data_in_queue.data_arr,his_data_in_queue.length,"[%d-%d/%d]FOTA(%d):",G_FW_QUEUE_SENDING.head_index,
					G_FW_QUEUE_SENDING.tail_index,G_FW_QUEUE_SENDING.count,his_data_in_queue.length);
//			F_FOTA_TIMEOUT++;
//			P_INFO("F_FOTA_TIMEOUT:%d\r\n",F_FOTA_TIMEOUT);
			//register timed-out checking expired packets
//			if (blt_soft_timer_find(fl_wifi2ble_fota_ECHO_timedout) == -1) {
//				blt_soft_timer_restart(fl_wifi2ble_fota_ECHO_timedout,FOTA_FW_EXPIRED * G_ADV_SETTINGS.adv_duration * 987); //ms
//			}
//			if(blt_soft_timer_find(&_fota_sending_lastframe)==-1 && F_FOTA_TIMEOUT > 0)
//			{
//				blt_soft_timer_restart(&_fota_sending_lastframe,5000*1000);
//			}
		}
		else{
			if(F_FOTA_FW_FULL){
				P_INFO("FOTA FW Full->check echo rec\r\n");

			}
		}
	}
}
#endif
