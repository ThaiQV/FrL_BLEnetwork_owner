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

#define FW_DATA_SIZE 		1024

fl_pack_t g_fw_array[FW_DATA_SIZE];
fl_data_container_t G_FW_CONTAINER = { .data = g_fw_array, .head_index = 0, .tail_index = 0, .mask = FW_DATA_SIZE - 1, .count = 0 };

/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/
u8 FL_NWK_FOTA_IsReady(void){
	return G_FW_CONTAINER.count;
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
	req_pack.frame.endpoint.repeat_cnt = 2;
	req_pack.frame.endpoint.rep_settings = 2;
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
	if (FL_QUEUE_ADD(&G_FW_CONTAINER,&fw_pack) < 0) {
		ERR(INF_FILE,"Err FULL <QUEUE ADD FW FOTA>!!\r\n");
		return -1;
	} else {
//		P_PRINTFHEX_A(INF_FILE,fw_pack.data_arr,fw_pack.length,"PUSH(cnt:%d)=>FW[%d]",FL_NWK_FOTA_IsReady(),MAKE_U16(fw_pack.data_arr[1],fw_pack.data_arr[0]));
//		P_INFO("Add FW: %d\r\n",MAKE_U16(fw_pack.data_arr[1],fw_pack.data_arr[0]));
		return G_FW_CONTAINER.tail_index;
	}
	return -1;
}
/***************************************************
 * @brief 		:create and register req and listen rsp from the slaves
 *
 * @param[in] 	:_slaveID: id of the slave
 * 				 _fw: fw payload
 * 				 _len: size of _fw
 * 				 _cb: function callback when had rsp from the slaves or timed out
 * 				 _timeout_ms: time of the waiting rsp (ms)
 * 				 _retry: num of resend if timed out
 *
 * @return	  	:-1: false, otherwise true
 *
 ***************************************************/
s8 fl_wifi2ble_fota_ReqWack(u8 _slaveID,u8* _fw,u8 _len,fl_rsp_callback_fnc _cb, u32 _timeout_ms,u8 _retry){
	if(_cb == 0){
		ERR(INF_FILE,"Fota ReqWACk must to Callback Fnc!!\r\n");
		return -1;
	}
	if(_slaveID == 0xFF){
		ERR(INF_FILE,"Not support the broadcast ACK !!\r\n");
		return -1;
	}
	u8 slave_mac[6];
	if (-1 != fl_master_SlaveMAC_get(_slaveID,slave_mac)) {
		fl_pack_t fw_pack = _fota_fw_packet_build(slave_mac,_fw,_len,1);
		if (fw_pack.length > 5) {
			int fl_send_heartbeat();

			if (FL_QUEUE_ADD(&G_FW_CONTAINER,&fw_pack) < 0) {
				ERR(INF_FILE,"Err FULL <QUEUE ADD FW FOTA>!!\r\n");
				return -1;
			} else {
//			P_PRINTFHEX_A(INF_FILE,fw_pack.data_arr,fw_pack.length,"PUSH(cnt:%d)=>FW[%d]",FL_NWK_FOTA_IsReady(),
//					MAKE_U16(fw_pack.data_arr[1],fw_pack.data_arr[0]));
				//		P_INFO("Add FW: %d\r\n",MAKE_U16(fw_pack.data_arr[1],fw_pack.data_arr[0]));
				fl_timetamp_withstep_t timetamp_inpack = fl_adv_timetampStepInPack(fw_pack);
				u32 seq_timetamp = fl_rtc_timetamp2milltampStep(timetamp_inpack);
				return fl_queueREQcRSP_add(_slaveID,NWK_HDR_FOTA,seq_timetamp,_fw,_len,&_cb,_timeout_ms,_retry);
			}
		}
	}
	ERR(INF_FILE,"FOTA Req <Err>!!\r\n");
	return -1;
}
/*************************************************************************************************************************************************
 *    BROADCAST REQ PROCESSOR
 *************************************************************************************************************************************************/
/***************************************************
 * @brief 		:send broadcast REQ to network and listen rsp of them
 *
 * @param[in] 	:_fw: fw payload
 * 				 _len: size of _fw
 * 				 _cb: function callback when had rsp from the slaves or timed out
 * 				 _timeout_ms: time of the waiting rsp (ms)
 * 				 _retry: num of resend if timed out
 *
 * @return	  	:-1: false, otherwise true
 *
 ***************************************************/

typedef struct {
	fl_pack_t payload;
	struct {
		u8 total;
		fl_nodeinnetwork_t *sla_info[MAX_NODES];
	} slave_list;
	struct {
		u8 sent;
		u8 rec;
		u8 list_rsp[30];
		fota_broadcast_rsp_cbk rspcbk;
	} rslt;
	u32 period_ms; //time for getting each slave
}__attribute__((packed)) fl_fota_broadcast_req_t;

fl_fota_broadcast_req_t G_FOTA_BROADCAST_REQ;
int _fota_Broadcast_process(void);

void _fota_Broadcast_RSP_cb(void* _data, void* _data2){
	fl_rsp_container_t *data =  (fl_rsp_container_t*)_data;
	LOGA(INF_FILE,"Timeout:%d\r\n",data->timeout);
	LOGA(INF_FILE,"cmdID  :%0X\r\n",data->rsp_check.hdr_cmdid);
	LOGA(INF_FILE,"SlaveID:%0X\r\n",data->rsp_check.slaveID);
	//rsp data
	if(data->timeout > 0){
		G_FOTA_BROADCAST_REQ.rslt.rec++;
		LOGA(INF_FILE,"Slave %d(0x%02X) reponsed \r\n",data->rsp_check.slaveID,data->rsp_check.slaveID);
		G_FOTA_BROADCAST_REQ.rslt.list_rsp[data->rsp_check.slaveID / 8] |= (1 << (data->rsp_check.slaveID  % 8));
	}
	else
	{
		//toto: someting timed-out
	}
	if(G_FOTA_BROADCAST_REQ.rslt.sent >= G_FOTA_BROADCAST_REQ.slave_list.total){
		LOGA(INF_FILE,"FOTA Broadcast REQ done (%d/%d)!!\r\n",G_FOTA_BROADCAST_REQ.rslt.sent,G_FOTA_BROADCAST_REQ.rslt.rec);
//		P_PRINTFHEX_A(INF_FILE,G_FOTA_BROADCAST_REQ.rslt.list_rsp,SIZEU8(G_FOTA_BROADCAST_REQ.rslt.list_rsp),"FOTA Broadcast RSP(%d):",SIZEU8(G_FOTA_BROADCAST_REQ.rslt.list_rsp));
		G_FOTA_BROADCAST_REQ.rslt.rspcbk(G_FOTA_BROADCAST_REQ.rslt.list_rsp,SIZEU8(G_FOTA_BROADCAST_REQ.rslt.list_rsp));
	}
	else
	{
		blt_soft_timer_restart(_fota_Broadcast_process,G_FOTA_BROADCAST_REQ.period_ms);
	}
}

int _fota_Broadcast_process(void){
	u8 slave_slot = G_FOTA_BROADCAST_REQ.rslt.sent;
	u8 slaveID = G_FOTA_BROADCAST_REQ.slave_list.sla_info[slave_slot]->slaveID.id_u8;
	if (-1 != fl_wifi2ble_fota_ReqWack(slaveID,G_FOTA_BROADCAST_REQ.payload.data_arr,G_FOTA_BROADCAST_REQ.payload.length,_fota_Broadcast_RSP_cb,0,1)) {
		G_FOTA_BROADCAST_REQ.rslt.sent++;
	}
	else {
		LOGA(INF_FILE,"FOTA Broadcast REQ FAIL (%d/%d)!!\r\n",G_FOTA_BROADCAST_REQ.rslt.sent,G_FOTA_BROADCAST_REQ.rslt.rec);
//		P_PRINTFHEX_A(INF_FILE,G_FOTA_BROADCAST_REQ.rslt.list_rsp,SIZEU8(G_FOTA_BROADCAST_REQ.rslt.list_rsp),"FOTA Broadcast RSP(%d):",SIZEU8(G_FOTA_BROADCAST_REQ.rslt.list_rsp));
		G_FOTA_BROADCAST_REQ.rslt.rspcbk(G_FOTA_BROADCAST_REQ.rslt.list_rsp,SIZEU8(G_FOTA_BROADCAST_REQ.rslt.list_rsp));
	}
	return -1;
}
s8 fl_wifi2ble_fota_Broadcast_REQwACK(u8* _fw, u8 _len,fota_broadcast_rsp_cbk _fncbk ) {
	extern fl_slaves_list_t G_NODE_LIST;
	u8 slave_mac[6];
	if (-1 != fl_master_SlaveMAC_get(0xFF,slave_mac) && _fncbk != NULL){
		G_FOTA_BROADCAST_REQ.rslt.rec=0;
		G_FOTA_BROADCAST_REQ.rslt.sent = 0;
		G_FOTA_BROADCAST_REQ.payload.length=0;
		memset(G_FOTA_BROADCAST_REQ.payload.data_arr,0,SIZEU8(G_FOTA_BROADCAST_REQ.payload.data_arr));
		memset(G_FOTA_BROADCAST_REQ.rslt.list_rsp,0,SIZEU8(G_FOTA_BROADCAST_REQ.rslt.list_rsp));
		//add payload
		G_FOTA_BROADCAST_REQ.payload.length = _len;
		memcpy(G_FOTA_BROADCAST_REQ.payload.data_arr,_fw,_len);

		// sort online to top list
		G_FOTA_BROADCAST_REQ.slave_list.total = G_NODE_LIST.slot_inused;
		u8 onl_msb_indx = 0;
		u8 off_lsb_indx = G_FOTA_BROADCAST_REQ.slave_list.total - 1;
		for (u8 indx = 0; indx < G_NODE_LIST.slot_inused; ++indx) {
			if (G_NODE_LIST.sla_info[indx].active == true) {
				G_FOTA_BROADCAST_REQ.slave_list.sla_info[onl_msb_indx++] = &G_NODE_LIST.sla_info[indx];
			} else {
				G_FOTA_BROADCAST_REQ.slave_list.sla_info[off_lsb_indx--] = &G_NODE_LIST.sla_info[indx];
			}
		}
		//end sort
		G_FOTA_BROADCAST_REQ.period_ms = 21*999;
		G_FOTA_BROADCAST_REQ.rslt.rspcbk = _fncbk;
		blt_soft_timer_add(&_fota_Broadcast_process,G_FOTA_BROADCAST_REQ.period_ms);
	}
	else{
		ERR(INF_FILE,"Null slave of the network!!!\r\n");
		return -1;
	}
	return 0;
}
/*************************************************************************************************************************************************
 *    BROADCAST REQ PROCESSOR - END
 *************************************************************************************************************************************************/

/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/
void fl_wifi2ble_fota_init(void){
	LOG_P(INF_FILE,"FOTA Initilization!!!\r\n");
	FL_QUEUE_CLEAR(&G_FW_CONTAINER,G_FW_CONTAINER.mask+1);
}

void fl_wifi2ble_fota_run(void) {
	extern fl_adv_settings_t G_ADV_SETTINGS;
	extern volatile u8 F_SENDING_STATE;
	fl_pack_t fw_in_queue;
	if (!F_SENDING_STATE) {
		if (FL_QUEUE_GET(&G_FW_CONTAINER,&fw_in_queue)) {
			F_SENDING_STATE = 1;
			P_PRINTFHEX_A(INF_FILE,fw_in_queue.data_arr,fw_in_queue.length,"FOTA SEND:");
			fl_adv_send(fw_in_queue.data_arr,fw_in_queue.length,G_ADV_SETTINGS.adv_duration);
		}
		else{
			FL_QUEUE_CLEAR(&G_FW_CONTAINER,G_FW_CONTAINER.mask+1);
		}
	}
}
#endif
