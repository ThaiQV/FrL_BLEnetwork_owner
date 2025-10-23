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

typedef enum{
	STATE_ackECHO=0,
	STATE_ADDED=1,
	STATE_SENT=2
}state_packet_fota_e;

#ifdef MASTER_CORE
/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/


fl_pack_t g_fw_array[FW_DATA_SIZE];
fl_data_container_t G_FW_CONTAINER = { .data = g_fw_array, .head_index = 0, .tail_index = 0, .mask = FW_DATA_SIZE - 1, .count = 0 };



fl_pack_t g_fw_echo_array[FW_ECHO_SIZE];
fl_data_container_t G_ECHO_CONTAINER = { .data = g_fw_echo_array, .head_index = 0, .tail_index = 0, .mask = FW_ECHO_SIZE - 1, .count = 0 };

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
//	//CHECK ackECHO
//	const u8 last_byte_echo = SIZEU8(G_FW_CONTAINER.data[0].data_arr) -1;
//	for (u16 idx = 0; idx <= G_FW_CONTAINER.mask; ++idx) {
//		if(G_FW_CONTAINER.data[idx].data_arr[last_byte_echo] != STATE_ackECHO){
//			return -1;
//		}
//	}
	//broadcast
	u8 broadcast_mac[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	fl_pack_t fw_pack = _fota_fw_packet_build(broadcast_mac,_fw,_len,0);
	if (FL_QUEUE_ADD(&G_FW_CONTAINER,&fw_pack) < 0) {
//		ERR(INF_FILE,"Err FULL <QUEUE ADD FW FOTA>!!\r\n");
		return -1;
	} else {
//		P_PRINTFHEX_A(INF_FILE,fw_pack.data_arr,fw_pack.length,"PUSH(cnt:%d)=>FW[%d]",FL_NWK_FOTA_IsReady(),MAKE_U16(fw_pack.data_arr[1],fw_pack.data_arr[0]));
//		P_INFO("Add FW: %d\r\n",MAKE_U16(fw_pack.data_arr[1],fw_pack.data_arr[0]));
		//
		u16 cur_tail_index = (G_FW_CONTAINER.tail_index - 1) & (G_FW_CONTAINER.mask);
		//Add ack status into the last byte of the G_FW_CONTAINER
		G_FW_CONTAINER.data[cur_tail_index].data_arr[SIZEU8(G_FW_CONTAINER.data[cur_tail_index].data_arr)-1] = STATE_ADDED;
		//P_INFO_HEX(_fw,_len,"FW:");
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
		fl_send_heartbeat();
		fl_pack_t fw_pack = _fota_fw_packet_build(slave_mac,_fw,_len,1);
		if (-1 != fl_adv_sendFIFO_add(fw_pack)) {
			fl_timetamp_withstep_t timetamp_inpack = fl_adv_timetampStepInPack(fw_pack);
			u32 seq_timetamp = fl_rtc_timetamp2milltampStep(timetamp_inpack);
			return fl_queueREQcRSP_add(_slaveID,NWK_HDR_FOTA,seq_timetamp,_fw,_len,&_cb,_timeout_ms,_retry);
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
		u32 rtt;
	} rslt;
	u32 scan_period_ms; //time for getting each slave
	//variable use to broadcast group
	struct{
		u32 timeout1times;
	}var;
}__attribute__((packed)) fl_fota_broadcast_req_t;

fl_fota_broadcast_req_t G_FOTA_BROADCAST_REQ;
int _fota_Broadcast_polling_process(void);

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
		blt_soft_timer_restart(_fota_Broadcast_polling_process,G_FOTA_BROADCAST_REQ.scan_period_ms);
	}
}

int _fota_Broadcast_polling_process(void){
	u8 slave_slot = G_FOTA_BROADCAST_REQ.rslt.sent;
	u8 slaveID = G_FOTA_BROADCAST_REQ.slave_list.sla_info[slave_slot]->slaveID.id_u8;
	if (-1 != fl_wifi2ble_fota_ReqWack(slaveID,G_FOTA_BROADCAST_REQ.payload.data_arr,G_FOTA_BROADCAST_REQ.payload.length,_fota_Broadcast_RSP_cb,100,1)) {
		G_FOTA_BROADCAST_REQ.rslt.sent++;
	}
	else {
		LOGA(INF_FILE,"FOTA Broadcast REQ FAIL (%d/%d)!!\r\n",G_FOTA_BROADCAST_REQ.rslt.sent,G_FOTA_BROADCAST_REQ.rslt.rec);
//		P_PRINTFHEX_A(INF_FILE,G_FOTA_BROADCAST_REQ.rslt.list_rsp,SIZEU8(G_FOTA_BROADCAST_REQ.rslt.list_rsp),"FOTA Broadcast RSP(%d):",SIZEU8(G_FOTA_BROADCAST_REQ.rslt.list_rsp));
		G_FOTA_BROADCAST_REQ.rslt.rspcbk(G_FOTA_BROADCAST_REQ.rslt.list_rsp,SIZEU8(G_FOTA_BROADCAST_REQ.rslt.list_rsp));
	}
	return -1;
}
/***************************************************
 * @brief 		:send req to all network group by group
 *
 * @param[in] 	:...
 *
 * @return	  	:none
 *
 ***************************************************/
int _fota_Broadcast_group_process(void) {
extern fl_adv_settings_t G_ADV_SETTINGS;
#define NUMOFTIMES			8
#define TIMEOUT_1_TIMES 	(G_ADV_SETTINGS.adv_duration*NUMOFTIMES*1000)
	//generate req group fmt
	int next_index = MSB_BIT_SET(G_FOTA_BROADCAST_REQ.rslt.list_rsp,SIZEU8(G_FOTA_BROADCAST_REQ.rslt.list_rsp));
	u8 data_payload[22]; //max size of the payload in the adv packet
	if (next_index < 0) {next_index = 0;}
	else next_index+=1;
	//scan and collect slave has rsped
	G_FOTA_BROADCAST_REQ.rslt.rec=0;
	for (u8 rsp=0;rsp<G_FOTA_BROADCAST_REQ.slave_list.total;rsp++) {
		if(G_FOTA_BROADCAST_REQ.slave_list.sla_info[rsp]->active == true){
			G_FOTA_BROADCAST_REQ.rslt.rec+=1;
		}
	}

	if (G_FOTA_BROADCAST_REQ.var.timeout1times <= TIMEOUT_1_TIMES && next_index > 0) {
		G_FOTA_BROADCAST_REQ.var.timeout1times += G_FOTA_BROADCAST_REQ.scan_period_ms;
		if(G_FOTA_BROADCAST_REQ.rslt.rec == G_FOTA_BROADCAST_REQ.rslt.sent){
			goto NEXT_GET;
		}
	}
	else {
		NEXT_GET:
		if (next_index < G_FOTA_BROADCAST_REQ.slave_list.total) {
			memset(data_payload,0xFF,SIZEU8(data_payload));
			u8 index = 0;
			for (index=0; index < NUMOFTIMES && next_index + index < G_FOTA_BROADCAST_REQ.slave_list.total; ++index) {
				data_payload[index] = G_FOTA_BROADCAST_REQ.slave_list.sla_info[next_index + index]->slaveID.id_u8;

				G_FOTA_BROADCAST_REQ.slave_list.sla_info[next_index + index]->active = false; //clear currently status
				//update location have sent yet
				G_FOTA_BROADCAST_REQ.rslt.list_rsp[(next_index + index) / 8] |= (1 << ((next_index + index) % 8));
				//
			}
			LOGA(INF_FILE,"Currently slot:%d/%d\r\n",next_index,MSB_BIT_SET(G_FOTA_BROADCAST_REQ.rslt.list_rsp,SIZEU8(G_FOTA_BROADCAST_REQ.rslt.list_rsp)))
//			u8 broadcast_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
//			_fota_fw_packet_build(broadcast_mac,data_payload,SIZEU8(data_payload),1);
			fl_master_packet_F5_CreateNSend(data_payload,index);
			G_FOTA_BROADCAST_REQ.rslt.sent+=index;
			G_FOTA_BROADCAST_REQ.var.timeout1times = 0;
		}else{

			LOGA(INF_FILE,"FOTA Broadcast REQ done (%d/%d),RTT:%d ms!!\r\n",G_FOTA_BROADCAST_REQ.rslt.sent,G_FOTA_BROADCAST_REQ.rslt.rec,
					(clock_time()-G_FOTA_BROADCAST_REQ.rslt.rtt)/SYSTEM_TIMER_TICK_1MS);
			return -1;
		}
	}
	return 0;
}

s8 fl_wifi2ble_fota_Broadcast_REQwACK(u8* _fw, u8 _len,fota_broadcast_rsp_cbk _fncbk,fl_fota_broadcast_mode_e _mode ) {
	extern fl_slaves_list_t G_NODE_LIST;
	u8 slave_mac[6];
	if (-1 != fl_master_SlaveMAC_get(0xFF,slave_mac) && _fncbk != NULL){
		G_FOTA_BROADCAST_REQ.rslt.rec=0;
		G_FOTA_BROADCAST_REQ.rslt.sent = 0;
		G_FOTA_BROADCAST_REQ.payload.length=0;
		memset(G_FOTA_BROADCAST_REQ.payload.data_arr,0,SIZEU8(G_FOTA_BROADCAST_REQ.payload.data_arr));
		memset(G_FOTA_BROADCAST_REQ.rslt.list_rsp,0,SIZEU8(G_FOTA_BROADCAST_REQ.rslt.list_rsp));
//		memset(G_FOTA_BROADCAST_REQ.slave_list.flag_send,0,SIZEU8(G_FOTA_BROADCAST_REQ.slave_list.flag_send));
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
		//Clear status
		for (u8 var = 0; var < G_NODE_LIST.slot_inused; ++var) {
			G_NODE_LIST.sla_info[var].active = false;
		}
		G_FOTA_BROADCAST_REQ.scan_period_ms = 11*999;
		G_FOTA_BROADCAST_REQ.rslt.rspcbk = _fncbk;
		switch (_mode) {
			case BroadCast_POLLING: {
				blt_soft_timer_restart(&_fota_Broadcast_polling_process,G_FOTA_BROADCAST_REQ.scan_period_ms);
			}
			break;
			case BroadCast_GROUP :{
				G_FOTA_BROADCAST_REQ.rslt.rtt = clock_time();
				blt_soft_timer_restart(&_fota_Broadcast_group_process,G_FOTA_BROADCAST_REQ.scan_period_ms);
			}break;
			default:
			break;
		}
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
	FL_QUEUE_CLEAR(&G_ECHO_CONTAINER,G_ECHO_CONTAINER.mask+1);
}
/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/
void fl_wifi2ble_fota_init(void){
	LOG_P(INF_FILE,"FOTA Initilization!!!\r\n");
	fl_wifi2ble_fota_ContainerClear();
//	DFU_OTA_INIT();
}

void fl_wifi2ble_fota_recECHO(fl_pack_t _pack_rec){
	extern u8 fl_packet_parse(fl_pack_t _pack, fl_dataframe_format_t *rslt);
	fl_pack_t data_in_queue;
	fl_dataframe_format_t packet;
	static u32 count_echo=0;
	static u8 flag_begin_end=0;
	if (!fl_packet_parse(_pack_rec,&packet)) {
		ERR(INF,"Packet parse fail!!!\r\n");
		return;
	}
	if (packet.hdr != NWK_HDR_FOTA) {
		return;
	}
	u8 OTA_BEGIN[3]={0,0,2};
	u8 OTA_END[3]={2,0,2};

	if (-1 == FL_QUEUE_FIND(&G_ECHO_CONTAINER,&_pack_rec,_pack_rec.length - 2)) {
		if (FL_QUEUE_ADD(&G_ECHO_CONTAINER,&_pack_rec) < 0) {
			//ERR(BLE,"Err <QUEUE ADD>!!\r\n");
		} else {
			if (FL_QUEUE_GET(&G_ECHO_CONTAINER,&data_in_queue)) {

				if (fl_packet_parse(data_in_queue,&packet)) {
//					P_INFO_HEX(packet.payload,SIZEU8(packet.payload),"FW:");
					if (plog_IndexOf(packet.payload,OTA_BEGIN,SIZEU8(OTA_BEGIN),SIZEU8(OTA_BEGIN)) != -1) {
						count_echo = 0;
						flag_begin_end=1;
//						FL_QUEUE_CLEAR(&G_ECHO_CONTAINER,G_ECHO_CONTAINER.mask+1);
						P_INFO("============ FOTA BEGIN     ======\r\n");
					} else if (flag_begin_end && plog_IndexOf(packet.payload,OTA_END,SIZEU8(OTA_END),SIZEU8(OTA_END)) != -1) {
						P_INFO("============ FOTA END(%d) ======\r\n",count_echo);
						count_echo = 0;
						flag_begin_end=0;
//						FL_QUEUE_CLEAR(&G_ECHO_CONTAINER,G_ECHO_CONTAINER.mask+1);
					}
					else{
						if(flag_begin_end)count_echo++;
					}
				}
			}
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
//			P_INFO("FOTA(cnt:%d)%d/%d|0x00%02X%02X%02X\r\n",G_FW_CONTAINER.count,G_FW_CONTAINER.head_index,G_FW_CONTAINER.tail_index,
//					fw_in_queue.data_arr[7+3+2],fw_in_queue.data_arr[7+3+1],fw_in_queue.data_arr[7+3+0]);
			fl_adv_send(fw_in_queue.data_arr,fw_in_queue.length,G_ADV_SETTINGS.adv_duration);
		}
		else{
			FL_QUEUE_CLEAR(&G_FW_CONTAINER,G_FW_CONTAINER.mask+1);
		}
	}
}
#endif
