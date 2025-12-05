/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_nwk_handler.c
 *Created on		: Aug 19, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#include "tl_common.h"
#include "fl_nwk_handler.h"
#include "fl_nwk_protocol.h"
#include "fl_adv_proc.h"

/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/

fl_node_data_t G_NODELIST_TABLE[MAX_NODES];
#define NODELIST_TABLE_SIZE  (sizeof(G_NODELIST_TABLE)/sizeof(G_NODELIST_TABLE[0]))

fl_pack_t g_nodelist_array[NODELIST_SENDING_SIZE];
fl_data_container_t NODELIST_TABLE_SENDING =
		{ .data = g_nodelist_array, .head_index = 0, .tail_index = 0, .mask = NODELIST_SENDING_SIZE - 1, .count = 0 };


volatile fl_timetamp_withstep_t WIFI_ORIGINAL_GETALL;

void fl_queue_REQnRSP_TimeoutInit(void);
int fl_queue_REQnRSP_TimeoutStart(void);
/***************************************************
 * @brief 		: API for user send req and receive rsp
 *
 * @param[in] 	:_cmdid: ID HDR of the req
 * 				 _data : pointer to data
 * 				 _len  : size of data
 * 				 _cb   : function callback when receipt rsp from the master
 * 				 _timeout_ms: timeout for waiting rsp (expired to callback fnc with NULL)
 *
 * @return	  	:-1: fail otherwise success
 *
 ***************************************************/
fl_rsp_container_t G_QUEUE_REQ_CALL_RSP[QUEUE_RSP_SLOT_MAX];

void _queue_REQcRSP_clear(fl_rsp_container_t *_mem){
	_mem->timeout = 0;
	_mem->timeout_set=0;
	_mem->retry = 0;
	_mem->rsp_cb = 0;
	_mem->rsp_check.seqTimetamp = 0;
	_mem->rsp_check.hdr_cmdid = 0;
	_mem->rsp_check.slaveID = 0xFF;
	memset(_mem->req_payload.payload,0,SIZEU8(_mem->req_payload.payload));
	_mem->req_payload.len = 0;
}

void fl_queueREQcRSP_clear(fl_rsp_container_t *_in){
	u8 indx = 0;
	for (indx = 0; indx < QUEUE_RSP_SLOT_MAX; ++indx) {
		_queue_REQcRSP_clear(&_in[indx]);
	}
}

u8 fl_queueREQcRSP_sort(void){
	u8 indx = 0;
	fl_rsp_container_t swap[QUEUE_RSP_SLOT_MAX];
	fl_queueREQcRSP_clear(swap);
	//copy to buffer
	for (indx = 0; indx < QUEUE_RSP_SLOT_MAX; ++indx) {
		swap[indx] = G_QUEUE_REQ_CALL_RSP[indx];
	}
	//remove empty slot
	fl_queueREQcRSP_clear(G_QUEUE_REQ_CALL_RSP);
	u8 avai_slot = 0;
	for (indx = 0; indx < QUEUE_RSP_SLOT_MAX; ++indx) {
		if(swap[indx].rsp_cb != 0){
			G_QUEUE_REQ_CALL_RSP[avai_slot] = swap[indx];
			avai_slot++;
		}
	}
	//
	return (avai_slot >= QUEUE_RSP_SLOT_MAX?avai_slot=QUEUE_RSP_SLOT_MAX:avai_slot );
}

s8 fl_queueREQcRSP_find(fl_rsp_callback_fnc *_cb,u32 _SeqTimetamp/*,u32 _timeout_ms*/, u8 *o_avaislot){
	u8 indx = 0;
	*o_avaislot=fl_queueREQcRSP_sort();
	for (indx = 0; indx < QUEUE_RSP_SLOT_MAX; ++indx) {
		if(*G_QUEUE_REQ_CALL_RSP[indx].rsp_cb == *_cb /* && G_QUEUE_REQ_CALL_RSP[indx].timeout_set == _timeout_ms*1000*/
				&& G_QUEUE_REQ_CALL_RSP[indx].rsp_check.seqTimetamp == _SeqTimetamp)
		{
			*o_avaislot = 0xFF;
			return indx;
		}
	}
	return -1;
}

s8 fl_queueREQcRSP_add(u8 slaveid,u8 cmdid,u64 _SeqTimetamp,u8* _payloadreq,u8 _len,fl_rsp_callback_fnc *_cb, u32 _timeout_ms,u8 _retry){
	extern fl_adv_settings_t G_ADV_SETTINGS;
	u8 avai_slot= 0xFF;
	if(fl_queueREQcRSP_find(_cb,_SeqTimetamp,&avai_slot) == -1 && avai_slot < QUEUE_RSP_SLOT_MAX){
		G_QUEUE_REQ_CALL_RSP[avai_slot].timeout = ((_timeout_ms!=0?(_timeout_ms + 16*G_ADV_SETTINGS.adv_duration):32*G_ADV_SETTINGS.adv_duration)*1000);
		G_QUEUE_REQ_CALL_RSP[avai_slot].timeout_set = G_QUEUE_REQ_CALL_RSP[avai_slot].timeout;
		G_QUEUE_REQ_CALL_RSP[avai_slot].rsp_cb = *_cb;
		G_QUEUE_REQ_CALL_RSP[avai_slot].rsp_check.seqTimetamp = _SeqTimetamp;
		G_QUEUE_REQ_CALL_RSP[avai_slot].rsp_check.hdr_cmdid = cmdid;
		G_QUEUE_REQ_CALL_RSP[avai_slot].rsp_check.slaveID = slaveid;
		G_QUEUE_REQ_CALL_RSP[avai_slot].retry = _retry;
		G_QUEUE_REQ_CALL_RSP[avai_slot].req_payload.len = _len;
		memcpy(G_QUEUE_REQ_CALL_RSP[avai_slot].req_payload.payload,_payloadreq,_len);
		LOGA(API,"queueREQcRSP Add [%d]SeqTimetamp(%lld):%d ms|retry: %d \r\n",avai_slot,_SeqTimetamp,_timeout_ms,_retry);
		//check fl_queue_REQnRSP_TimeoutStart running
		if (blt_soft_timer_find(&fl_queue_REQnRSP_TimeoutStart) == -1) {
			ERR(INF,"REQcRSP RE-Initialization (%d us)!!\r\n",QUEUQ_REQcRSP_INTERVAL);
			blt_soft_timer_restart(&fl_queue_REQnRSP_TimeoutStart,QUEUQ_REQcRSP_INTERVAL);
		}
//		fl_queue_REQnRSP_OriginTime_set(_SeqTimetamp);
		return avai_slot;
	}
	ERR(API,"queueREQcRSP Add [%d]SeqTimetamp(%lld):%d ms|retry: %d \r\n",avai_slot,_SeqTimetamp,_timeout_ms,_retry);
	return -1;
}
#ifndef MASTER_CORE
void fl_queue_REQnRSP_OriginTime_set(u64 _timestamp_set){
	extern volatile fl_timetamp_withstep_t ORIGINAL_MASTER_TIME;
	u64 origin = _timestamp_set;
	u64 master_curr = fl_rtc_timetamp2milltampStep(ORIGINAL_MASTER_TIME);
	fl_timetamp_withstep_t origin_mil;
	for (u8 indx = 0; indx < QUEUE_RSP_SLOT_MAX; ++indx) {
		if(G_QUEUE_REQ_CALL_RSP[indx].rsp_check.seqTimetamp < origin && G_QUEUE_REQ_CALL_RSP[indx].rsp_check.seqTimetamp != 0
				&&G_QUEUE_REQ_CALL_RSP[indx].rsp_check.seqTimetamp >= master_curr){
			origin = G_QUEUE_REQ_CALL_RSP[indx].rsp_check.seqTimetamp;
			origin_mil = fl_rtc_milltampStep2timetamp(origin);
		}
	}
	if(origin>=master_curr){
		P_INFO("REQnRSP synch ORIGIN_MASTER:%lld\r\n",origin);
		fl_nwk_slave_SYNC_ORIGIN_MASTER(origin_mil.timetamp,origin_mil.milstep);
	}else{
		fl_nwk_slave_SYNC_ORIGIN_MASTER(ORIGINAL_MASTER_TIME.timetamp,ORIGINAL_MASTER_TIME.milstep);
	}
}
#endif
/***************************************************
 * @brief 		:Run checking rsp and timeout
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/

int fl_queue_REQnRSP_TimeoutStart(void){
//	if(blt_soft_timer_find(&fl_queue_REQnRSP_TimeoutStart)==-1){
//		LOGA(INF,"REQcRSP initialization (%d ms)!!\r\n",QUEUQ_REQcRSP_INTERVAL);
//		blt_soft_timer_add(&fl_queue_REQnRSP_TimeoutStart,QUEUQ_REQcRSP_INTERVAL);
//		fl_queueREQcRSP_clear(G_QUEUE_REQ_CALL_RSP);
//	}else
	{
		u8 avai_slot = fl_queueREQcRSP_sort();
		for(u8 i =0;i < avai_slot;i++){
			//check timeout
			if (G_QUEUE_REQ_CALL_RSP[i].timeout >= 0 && G_QUEUE_REQ_CALL_RSP[i].rsp_cb != 0) {
				if (G_QUEUE_REQ_CALL_RSP[i].timeout <= QUEUQ_REQcRSP_INTERVAL) {
					G_QUEUE_REQ_CALL_RSP[i].timeout=0; //Timeout expired
				} else {
					G_QUEUE_REQ_CALL_RSP[i].timeout -= QUEUQ_REQcRSP_INTERVAL;
				}
				if (G_QUEUE_REQ_CALL_RSP[i].timeout <= 0){
					fl_rsp_container_t REQ_BUF = G_QUEUE_REQ_CALL_RSP[i];
//					//clear event
//					_queue_REQcRSP_clear(&G_QUEUE_REQ_CALL_RSP[i]);
					//Excute retry
					if (REQ_BUF.retry > 0) {
						REQ_BUF.retry--;
#ifdef MASTER_CORE
						u8 mac[6];
						if (fl_master_SlaveMAC_get(REQ_BUF.rsp_check.slaveID,mac) != -1) {
//							if (-1!= fl_api_master_req(mac,REQ_BUF.rsp_check.hdr_cmdid,REQ_BUF.req_payload.payload,REQ_BUF.req_payload.len,
//											REQ_BUF.rsp_cb,REQ_BUF.timeout_set / 1000,REQ_BUF.retry)) {
////								LOGA(API,"[%d/%d]SlaveID(%d)->Retry:%d\r\n",i,avai_slot,REQ_BUF.rsp_check.slaveID,REQ_BUF.retry);
//								continue;
//							}
							G_QUEUE_REQ_CALL_RSP[i].timeout = G_QUEUE_REQ_CALL_RSP[i].timeout_set; //refesh timeout for next retry;
							G_QUEUE_REQ_CALL_RSP[i].retry --;
							continue;
						}
#else
//						if(-1!=fl_api_slave_req(REQ_BUF.rsp_check.hdr_cmdid,REQ_BUF.req_payload.payload,REQ_BUF.req_payload.len,
//											REQ_BUF.rsp_cb,REQ_BUF.timeout_set/1000,REQ_BUF.retry)) {
////							LOGA(API,"[%d/%d]SlaveID(%d)->Retry:%d\r\n",i,avai_slot,REQ_BUF.rsp_check.slaveID,REQ_BUF.retry);
//							continue;
//						}
						G_QUEUE_REQ_CALL_RSP[i].timeout = G_QUEUE_REQ_CALL_RSP[i].timeout_set; //refesh timeout for next retry;
						G_QUEUE_REQ_CALL_RSP[i].retry --;
						continue;
#endif
					}
					//clear event
					_queue_REQcRSP_clear(&G_QUEUE_REQ_CALL_RSP[i]);
//					LOGA(API,"%d/%d TIMEOUT!!! \r\n",G_QUEUE_REQ_CALL_RSP[i].rsp_check.hdr_cmdid,G_QUEUE_REQ_CALL_RSP[i].rsp_check.slaveID);
					REQ_BUF.rsp_cb((void*) &REQ_BUF,0); //timeout
				}
			}
		}
	}
	return QUEUQ_REQcRSP_INTERVAL + RAND(1,100);
}
void fl_queue_REQnRSP_TimeoutInit(void) {
	if (blt_soft_timer_find(&fl_queue_REQnRSP_TimeoutStart) == -1) {
		LOGA(INF,"REQcRSP initialization (%d ms)!!\r\n",QUEUQ_REQcRSP_INTERVAL/1000);
		blt_soft_timer_add(&fl_queue_REQnRSP_TimeoutStart,QUEUQ_REQcRSP_INTERVAL);
		fl_queueREQcRSP_clear(G_QUEUE_REQ_CALL_RSP);
	}
#ifndef MASTER_CORE
	//clear buffer nodelist table
	G_NODELIST_TABLE_Clear();
#endif
}
/***************************************************
 * @brief 		:scan pack rec from master
 *
 * @param[in] 	:none
 *
 * @return	  	:pack
 *
 ***************************************************/
#ifdef MASTER_CORE
s8 fl_queue_REQcRSP_ScanRec(fl_pack_t _pack)
{
#else
s8 fl_queue_REQcRSP_ScanRec(fl_pack_t _pack,void *_id)
{
	fl_nodeinnetwork_t *_myID = (fl_nodeinnetwork_t*)_id;
#endif
	extern u8 fl_packet_parse(fl_pack_t _pack, fl_dataframe_format_t *rslt);
	fl_data_frame_u packet;
	s8 rslt = -1;
	if (!fl_packet_parse(_pack,&packet.frame)) {
		ERR(API,"Packet parse fail!!!\r\n");
		return -1;
	}else{
		u64 seq_timetamp = 0;
#ifdef MASTER_CORE
		fl_timetamp_withstep_t  timetamp_inpack = fl_adv_timetampStepInPack(_pack);
		seq_timetamp =fl_rtc_timetamp2milltampStep(timetamp_inpack);
#else
		if(_myID->slaveID != packet.frame.slaveID && packet.frame.slaveID !=0xFF){//not me
			return -1;
		}
		//get seq_timetamp in rsp
		seq_timetamp = fl_rtc_timetampmillstep_convert(&packet.frame.payload[0]);
		//Synchronize debug log
		NWK_DEBUG_STT = packet.frame.endpoint.dbg;
		DEBUG_TURN(NWK_DEBUG_STT);
#endif
		u8 avai_slot = fl_queueREQcRSP_sort();
		for(u8 i =0;i < avai_slot;i++){
			if (G_QUEUE_REQ_CALL_RSP[i].rsp_check.hdr_cmdid != 0 && G_QUEUE_REQ_CALL_RSP[i].rsp_check.hdr_cmdid == packet.frame.hdr) {
#ifdef MASTER_CORE
				if (G_QUEUE_REQ_CALL_RSP[i].rsp_check.slaveID == packet.frame.slaveID
					&& G_QUEUE_REQ_CALL_RSP[i].rsp_check.seqTimetamp==seq_timetamp){
#else
				u16 timetamp_delta = MAKE_U16(packet.frame.payload[SIZEU8(fl_timetamp_withstep_t)],packet.frame.payload[SIZEU8(fl_timetamp_withstep_t)+1]);
				u8 slaveID_rsp[SIZEU8(packet.frame.payload)-SIZEU8(fl_timetamp_withstep_t)- SIZEU8(timetamp_delta)]; // 22 - 5 - 1
				memset(slaveID_rsp,0xFF,SIZEU8(slaveID_rsp));
				memcpy(slaveID_rsp,&packet.frame.payload[SIZEU8(fl_timetamp_withstep_t)+SIZEU8(timetamp_delta)],SIZEU8(slaveID_rsp));
				u8 my_slaveID_inrspcom = plog_IndexOf(slaveID_rsp,&G_QUEUE_REQ_CALL_RSP[i].rsp_check.slaveID,1,SIZEU8(slaveID_rsp));

				if ((G_QUEUE_REQ_CALL_RSP[i].rsp_check.slaveID != 0xFF && packet.frame.slaveID ==0xFF && my_slaveID_inrspcom != -1
						&& G_QUEUE_REQ_CALL_RSP[i].rsp_check.seqTimetamp <= (seq_timetamp + (u32)timetamp_delta)) //Check RSP common
					|| (G_QUEUE_REQ_CALL_RSP[i].rsp_check.slaveID == packet.frame.slaveID
							&& G_QUEUE_REQ_CALL_RSP[i].rsp_check.seqTimetamp==seq_timetamp)
					)
				{
					_myID->active = true;
					P_PRINTFHEX_A(API,slaveID_rsp,SIZEU8(slaveID_rsp),"SlaveID(%d):",SIZEU8(slaveID_rsp));
					LOGA(API,"REQ Timetmap  :%lld\r\n",G_QUEUE_REQ_CALL_RSP[i].rsp_check.seqTimetamp);
					LOGA(API,"TimeTamp_Seq  :%lld\r\n",seq_timetamp);
					LOGA(API,"TimeTamp_delta:%d\r\n",timetamp_delta);
					LOGA(API,"MyID:%02X,Found:%d\r\n",G_QUEUE_REQ_CALL_RSP[i].rsp_check.slaveID,my_slaveID_inrspcom);
#endif
					rslt = i;
					LOGA(API,"RSP(%lld|%lld):%d/%d\r\n",G_QUEUE_REQ_CALL_RSP[i].rsp_check.seqTimetamp,seq_timetamp,packet.frame.hdr,packet.frame.slaveID);
					G_QUEUE_REQ_CALL_RSP[i].rsp_cb((void*)&G_QUEUE_REQ_CALL_RSP[i],(void*)&_pack); //timeout
					//clear event
					_queue_REQcRSP_clear(&G_QUEUE_REQ_CALL_RSP[i]);
				}
				else{
					LOGA(API,"RSP(%lld|%lld):%02X/%d\r\n",G_QUEUE_REQ_CALL_RSP[i].rsp_check.seqTimetamp,seq_timetamp,packet.frame.hdr,packet.frame.slaveID);
				}
			}
			//else return;
		}
	}
	return rslt;
}

/*======================================================================*/


/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
fl_hdr_nwk_type_e G_NWK_HDR[] = {
		NWK_HDR_NODETALBE_UPDATE,
		NWK_HDR_REMOVE,NWK_HDR_MASTER_CMD,NWK_HDR_FOTA,NWK_HDR_A5_HIS, NWK_HDR_F6_SENDMESS,
		NWK_HDR_F7_RSTPWMETER,NWK_HDR_F8_PWMETER_SET, NWK_HDR_F5_INFO, NWK_HDR_COLLECT, NWK_HDR_HEARTBEAT,
		NWK_HDR_ASSIGN, NWK_HDR_55, NWK_HDR_11_REACTIVE, NWK_HDR_22_PING };// register cmdid RSP
#define NWK_HDR_SIZE (sizeof(G_NWK_HDR)/sizeof(G_NWK_HDR[0]))
/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/

static inline u8 IsNWKHDR(fl_hdr_nwk_type_e cmdid) {
	for (u8 i = 0; i < NWK_HDR_SIZE; i++) {
		if (cmdid == G_NWK_HDR[i]) {
			return 1;
		}
	}
	return 0;
}

/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/

s8 fl_nwk_MemberInNodeTable_find(u8* _mac){
	for (u8 var = 0; var < NODELIST_TABLE_SIZE; ++var) {
		if(0 == memcmp(_mac,G_NODELIST_TABLE[var].mac,SIZEU8(G_NODELIST_TABLE[var].mac))){
			return G_NODELIST_TABLE[var].slaveid;
		}
	}
	return -1;
}
/*
 * Update NODELIST TABLE
 * */

bool FL_NODELIST_TABLE_Updated(void) {
	for (u8 var = 0; var < sizeof(G_NODELIST_TABLE) / sizeof(G_NODELIST_TABLE[0]); ++var) {
		if(G_NODELIST_TABLE[var].slaveid!=0xFF && !IS_MAC_INVALID(G_NODELIST_TABLE[var].mac,0xFF)){
			return true;
		}
	}
	return false;
}

s16 FL_NWK_NODELIST_TABLE_IsReady(void){
	return NODELIST_TABLE_SENDING.count;
}
#ifdef MASTER_CORE
void fl_nwk_generate_table_pack(void) {
	fl_pack_t pack;
	u8 payload_create[28]; // full size adv can be sent
	memset(payload_create,0xFF,SIZEU8(payload_create));
	u8 ptr = 0;
	if(FL_NWK_NODELIST_TABLE_IsReady()>0) return;

	FL_QUEUE_CLEAR(&NODELIST_TABLE_SENDING,NODELIST_TABLE_SENDING.mask+1);
	for (u8 var = 0; var < NODELIST_TABLE_SIZE; ++var) {
		if (G_NODELIST_TABLE[var].slaveid != 0xFF) {
			payload_create[ptr] = G_NODELIST_TABLE[var].slaveid;
			memcpy(&payload_create[++ptr],G_NODELIST_TABLE[var].mac,SIZEU8(G_NODELIST_TABLE[var].mac));
			ptr +=SIZEU8(G_NODELIST_TABLE[var].mac);
			if (ptr >= SIZEU8(payload_create)) {
//				P_INFO_HEX(payload_create,SIZEU8(payload_create),"PACK NODELIST(%d/%d):",var,NODELIST_TABLE_SIZE);
				pack = fl_master_packet_nodelist_table_build(payload_create,SIZEU8(payload_create));
				FL_QUEUE_ADD(&NODELIST_TABLE_SENDING,&pack);
				memset(payload_create,0xFF,SIZEU8(payload_create));
				ptr=0;
			}
			//else ptr += 1; //
		}
	}
	//Last pack
	if(ptr>0){
//		P_INFO_HEX(payload_create,SIZEU8(payload_create),"PACK NODELIST(LSB):");
		pack = fl_master_packet_nodelist_table_build(payload_create,SIZEU8(payload_create));
		FL_QUEUE_ADD(&NODELIST_TABLE_SENDING,&pack);
	}
}
#else
void G_NODELIST_TABLE_Clear(void){
	for (u8 var = 0; var < sizeof(G_NODELIST_TABLE)/sizeof(G_NODELIST_TABLE[0]); ++var) {
		G_NODELIST_TABLE[var].slaveid=0xFF;
		memset(G_NODELIST_TABLE[var].mac,0xFF,SIZEU8(G_NODELIST_TABLE[var].mac));
	}
}
void _nodelist_table_printf(void) {
	static u32 crc32 = 0;
	u32 crc32_cur = fl_db_crc32((u8*) G_NODELIST_TABLE,NODELIST_TABLE_SIZE * sizeof(fl_node_data_t));
	if (crc32_cur != crc32) {
		for (u8 var = 0; var < NODELIST_TABLE_SIZE; var++) {
			if (G_NODELIST_TABLE[var].slaveid != 0xFF && !IS_MAC_INVALID(G_NODELIST_TABLE[var].mac,0xFF)) {
				P_INFO("[%3d]0x%02X%02X%02X%02X%02X%02X\r\n",G_NODELIST_TABLE[var].slaveid,G_NODELIST_TABLE[var].mac[0],G_NODELIST_TABLE[var].mac[1],
						G_NODELIST_TABLE[var].mac[2],G_NODELIST_TABLE[var].mac[3],G_NODELIST_TABLE[var].mac[4],G_NODELIST_TABLE[var].mac[5]);
			}
		}
		crc32 = crc32_cur;
	}
}
void fl_nwk_slave_nodelist_repeat(fl_pack_t *_pack) {
	if (FL_QUEUE_FIND(&NODELIST_TABLE_SENDING,_pack,_pack->length - 2) == -1) {
		FL_QUEUE_ADD(&NODELIST_TABLE_SENDING,_pack);
	}
	//Update my table
	u8 table_arr[28];
	memcpy(table_arr,&_pack->data_arr[1],SIZEU8(table_arr));
	for (u8 var = 0; var < SIZEU8(table_arr); var=var+7) {
		if(table_arr[var] != 0xFF){
			G_NODELIST_TABLE[table_arr[var]].slaveid  = table_arr[var];
			memcpy(G_NODELIST_TABLE[table_arr[var]].mac,&table_arr[var + 1],6);
		}
		//Check myID match myMAC
		if(table_arr[var] == fl_nwk_mySlaveID()){
			//Match SlaveID but incorrect mac => has been removed yet
			if(memcmp(G_NODELIST_TABLE[table_arr[var]].mac,fl_nwk_mySlaveMac(),SIZEU8(G_NODELIST_TABLE[table_arr[var]].mac))){
				ERR(APP,"Removed.....!!!\r\n");
				fl_nwk_slave_nwkclear();
				G_NODELIST_TABLE_Clear();
			}
		}
	}
	_nodelist_table_printf();
}
#endif
void fl_nwk_nodelist_table_run(void) {
	extern void fl_adv_send(u8* _data, u8 _size, u16 _timeout_ms);
	extern fl_adv_settings_t G_ADV_SETTINGS ;
	fl_pack_t pack_in_queue;
	extern volatile u8 F_SENDING_STATE;
	if (!F_SENDING_STATE) {
//		P_INFO("PACK NODELIST:%d\r\n",FL_NWK_NODELIST_TABLE_IsReady());
		if (FL_QUEUE_GET(&NODELIST_TABLE_SENDING,&pack_in_queue) != -1) {
//			P_INFO_HEX(pack_in_queue.data_arr,pack_in_queue.length,"PACK NODELIST(%d):",FL_NWK_NODELIST_TABLE_IsReady());
			fl_adv_send(pack_in_queue.data_arr,pack_in_queue.length,G_ADV_SETTINGS.adv_duration);
		}
	}
}

/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/
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
		if (IsNWKHDR(data_parsed.frame.hdr)) {
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
		if (IsNWKHDR(data_parsed.frame.hdr)) {
			rslt.timetamp = MAKE_U32(data_parsed.frame.timetamp[3],data_parsed.frame.timetamp[2],data_parsed.frame.timetamp[1],data_parsed.frame.timetamp[0]);
			rslt.milstep = data_parsed.frame.milltamp;
			return rslt;
		}
	}
	return rslt;
}
