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

fl_pack_t g_fw_array[FW_DATA_SIZE];
fl_data_container_t G_FW_CONTAINER = { .data = g_fw_array, .head_index = 0, .tail_index = 0, .mask = FW_DATA_SIZE - 1, .count = 0 };

//fl_pack_t g_echo_array[FW_ECHO_SIZE];
//fl_data_container_t G_ECHO_CONTAINER = { .data = g_echo_array, .head_index = 0, .tail_index = 0, .mask = FW_ECHO_SIZE - 1, .count = 0 };

typedef struct {
	fl_pack_t payload[FW_ECHO_SIZE];
	u16 count;
}__attribute__((packed)) fl_scan_echo_t;

#define ECHO_TIMEOUT_REC			(16*G_ADV_SETTINGS.adv_duration)

fl_scan_echo_t G_ECHO_CONTAINER;

/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/
int fl_wifi2ble_fota_ECHO_timedout(void);

u8 FL_NWK_FOTA_IsReady(void){
	return G_FW_CONTAINER.count;
}

void _print_scanECHO(fl_scan_echo_t *_pcon) {
	for (u16 var = 0; var < G_ECHO_CONTAINER.count; ++var) {
		P_INFO_HEX(G_ECHO_CONTAINER.payload[var].data_arr,G_ECHO_CONTAINER.payload[var].length,"[%d/%d](len:%d):",var,G_ECHO_CONTAINER.count,
				G_ECHO_CONTAINER.payload[var].length);
	}
}

s16 _ScanECHO_sort(fl_scan_echo_t *_pcon){
	fl_scan_echo_t buff;
	memcpy((u8*)&buff,_pcon,SIZEU8(fl_scan_echo_t));
	memset(_pcon,0,SIZEU8(fl_scan_echo_t));
	buff.count=0;
	for(u16 i =0;i<(sizeof(buff.payload)/sizeof(fl_pack_t));i++){
		if(buff.payload[i].length > 0){
			_pcon->payload[buff.count] = buff.payload[i];
			_pcon->count++;
			buff.count++;
		}
	}
	return (buff.count >= (sizeof(buff.payload)/sizeof(fl_pack_t)))?-1:buff.count;
}
s16 _ScanECHO_find(fl_scan_echo_t *_pcon, fl_pack_t _pack) {
//	for (u16 i = 0; i < _pcon->count); i++) {
//		if(mem){
//
//		}
//	}
	return -1;
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

	req_pack.frame.slaveID.id_u8 = slaveID;
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
s16 fl_wifi2ble_fota_fwpush(u8 *_fw, u8 _len) {
	//broadcast
	u8 broadcast_mac[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	fl_pack_t fw_pack = _fota_fw_packet_build(broadcast_mac,_fw,_len,0);

	//sort echo scan
	_print_scanECHO(&G_ECHO_CONTAINER);
	u16 numofecho = _ScanECHO_sort(&G_ECHO_CONTAINER);
	_print_scanECHO(&G_ECHO_CONTAINER);

	if (FL_QUEUE_ADD(&G_FW_CONTAINER,&fw_pack) < 0) {
		//		ERR(INF_FILE,"Err FULL <QUEUE ADD FW FOTA>!!\r\n");
		return -1;
	} else {
		//P_INFO_HEX(_fw,_len,"FW:");
		if(numofecho!=-1)G_ECHO_CONTAINER.payload[numofecho] = fw_pack;
		return G_FW_CONTAINER.tail_index;
	}

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
//int fl_wifi2ble_fota_system_processor(void){
//
//}

int fl_wifi2ble_fota_system_end(u8 *_payload_end,u8 _len){
//	P_INFO_HEX(_payload_end,_len,"|-> FOTA END PACKET:");
	int rslt =  fl_wifi2ble_fota_fwpush(_payload_end,_len);
	//todo: get rsp of the slave and recheck missing packet

	return rslt;
}

int fl_wifi2ble_fota_system_start(u8 *_payload_start,u8 _len){
//	P_INFO_HEX(_payload_start,_len,"|-> FOTA START PACKET:");
	int rslt = fl_wifi2ble_fota_fwpush(_payload_start,_len);
	//todo: get rsp of the slave if wifi need
	return rslt;
}

/*************************************************************************************************************************************************
 *    SYSTEM FOTA PROCESSOR - END
 *************************************************************************************************************************************************/
void fl_wifi2ble_fota_ContainerClear(void){
	FL_QUEUE_CLEAR(&G_FW_CONTAINER,G_FW_CONTAINER.mask+1);
	memset((u8*)&G_ECHO_CONTAINER,0,SIZEU8(fl_scan_echo_t));
}
/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/

int fl_wifi2ble_fota_ECHO_timedout(void){

	return -1;
}

void fl_wifi2ble_fota_init(void){
	LOG_P(INF_FILE,"FOTA Initilization!!!\r\n");
	fl_wifi2ble_fota_ContainerClear();
//	DFU_OTA_INIT();
//	blt_soft_timer_add(fl_wifi2ble_fota_ECHO_timedout,G_ADV_SETTINGS.adv_duration*987);//ms
}

void fl_wifi2ble_fota_recECHO(fl_pack_t _pack_rec){
	static u32 count_echo=0;
	s16 cur_index =-1;
	if (_pack_rec.length > 5 && (cur_index = FL_QUEUE_FIND(&G_FW_CONTAINER,&_pack_rec,_pack_rec.length - 2)) != -1) {

//		fl_scan_echo_t *fw_echo = (fl_scan_echo_t *)G_ECHO_CONTAINER.data[cur_index].data_arr;
//		P_INFO("Timeout index ECHO(len:%d,time:%d):%d\r\n",G_ECHO_CONTAINER.data[cur_index].length,fw_echo->timeout,cur_index);
//
//		G_ECHO_CONTAINER.data[cur_index].length = 0; //clear
//		memset(G_ECHO_CONTAINER.data[cur_index].data_arr,0,SIZEU8(G_ECHO_CONTAINER.data[cur_index].data_arr));
		//G_ECHO_CONTAINER.count = (G_ECHO_CONTAINER.count>0)?G_ECHO_CONTAINER.count-1:0;

		//Begin packet -> start fota
		u8 BEGIN_FOTA[3] = { 0x00, 0, 0x02 };
		u8 END_FOTA[3]={0x02,0,0x02};
		if (-1 != plog_IndexOf(&_pack_rec.data_arr[5],BEGIN_FOTA,SIZEU8(BEGIN_FOTA),5)) {
			P_INFO("============ BEGIN FOTA !! \r\n");
			count_echo++;
		}
		//end packet -> finish fota
		else if(-1 != plog_IndexOf(&_pack_rec.data_arr[5],END_FOTA,SIZEU8(END_FOTA),5)){
			P_INFO("============ END FOTA (cnt:%d) !! \r\n",count_echo);
			count_echo=0;
			fl_wifi2ble_fota_ContainerClear();
		}else{
			if(count_echo)count_echo++;
		}
	}
}

void fl_wifi2ble_fota_run(void) {
	extern fl_adv_settings_t G_ADV_SETTINGS;
	extern volatile u8 F_SENDING_STATE;
	fl_pack_t fw_in_queue;
	if (!F_SENDING_STATE) {
		if (FL_QUEUE_GET(&G_FW_CONTAINER,&fw_in_queue)){
			P_PRINTFHEX_A(INF_FILE,fw_in_queue.data_arr,fw_in_queue.length,"FOTA SEND(%d):",fw_in_queue.data_arr[SIZEU8(fw_in_queue.data_arr)-1]);
			P_INFO("SEND FOTA(cnt:%d)%d/%d\r\n",G_FW_CONTAINER.count,G_FW_CONTAINER.head_index,G_FW_CONTAINER.tail_index);
			fl_adv_send(fw_in_queue.data_arr,fw_in_queue.length,G_ADV_SETTINGS.adv_duration);
			//Add ECHO listening and checking
//			FL_QUEUE_ADD(&G_ECHO_CONTAINER,&fw_in_queue);
//			if(FL_QUEUE_ISFULL(&G_ECHO_CONTAINER)){
//				P_INFO("Start timer wait echo!!!\r\n");
//				blt_soft_timer_restart(fl_wifi2ble_fota_ECHO_timedout,10*G_ADV_SETTINGS.adv_duration*987);//ms
//			}
		}
		else{
			//FL_QUEUE_CLEAR(&G_FW_CONTAINER,G_FW_CONTAINER.mask+1);
		}
	}

}
#endif
