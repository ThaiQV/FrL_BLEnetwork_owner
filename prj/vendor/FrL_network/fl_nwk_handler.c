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

/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/

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

s8 fl_queueREQcRSP_add(u8 slaveid,u8 cmdid,u32 _SeqTimetamp,u8* _payloadreq,u8 _len,fl_rsp_callback_fnc *_cb, u32 _timeout_ms,u8 _retry){
	extern fl_adv_settings_t G_ADV_SETTINGS;
	u8 avai_slot= 0xFF;
	if(fl_queueREQcRSP_find(_cb,_SeqTimetamp,&avai_slot) == -1 && avai_slot < QUEUE_RSP_SLOT_MAX){
		G_QUEUE_REQ_CALL_RSP[avai_slot].timeout = (_timeout_ms!=0?(_timeout_ms + 8*G_ADV_SETTINGS.adv_duration):16*G_ADV_SETTINGS.adv_duration)*1000;
		G_QUEUE_REQ_CALL_RSP[avai_slot].timeout_set = G_QUEUE_REQ_CALL_RSP[avai_slot].timeout;
		G_QUEUE_REQ_CALL_RSP[avai_slot].rsp_cb = *_cb;
		G_QUEUE_REQ_CALL_RSP[avai_slot].rsp_check.seqTimetamp = _SeqTimetamp;
		G_QUEUE_REQ_CALL_RSP[avai_slot].rsp_check.hdr_cmdid = cmdid;
		G_QUEUE_REQ_CALL_RSP[avai_slot].rsp_check.slaveID = slaveid;
		G_QUEUE_REQ_CALL_RSP[avai_slot].retry = _retry;
		G_QUEUE_REQ_CALL_RSP[avai_slot].req_payload.len = _len;
		memcpy(G_QUEUE_REQ_CALL_RSP[avai_slot].req_payload.payload,_payloadreq,_len);
		LOGA(API,"queueREQcRSP Add [%d]SeqTimetamp(%u):%d ms|retry: %d \r\n",avai_slot,_SeqTimetamp,_timeout_ms,_retry);
		return avai_slot;
	}
	ERR(API,"queueREQcRSP Add [%d]SeqTimetamp(%u):%d ms|retry: %d \r\n",avai_slot,_SeqTimetamp,_timeout_ms,_retry);
	return -1;
}
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
	return 0;
}
void fl_queue_REQnRSP_TimeoutInit(void) {
	if (blt_soft_timer_find(&fl_queue_REQnRSP_TimeoutStart) == -1) {
		LOGA(INF,"REQcRSP initialization (%d ms)!!\r\n",QUEUQ_REQcRSP_INTERVAL);
		blt_soft_timer_add(&fl_queue_REQnRSP_TimeoutStart,QUEUQ_REQcRSP_INTERVAL);
		fl_queueREQcRSP_clear(G_QUEUE_REQ_CALL_RSP);
	}
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
		u32 seq_timetamp = 0;
#ifdef MASTER_CORE
		fl_timetamp_withstep_t  timetamp_inpack = fl_adv_timetampStepInPack(_pack);
		seq_timetamp =fl_rtc_timetamp2milltampStep(timetamp_inpack);
#else
		if(_myID->slaveID.id_u8 != packet.frame.slaveID.id_u8 && packet.frame.slaveID.id_u8 !=0xFF){//not me
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
				if (G_QUEUE_REQ_CALL_RSP[i].rsp_check.slaveID == packet.frame.slaveID.id_u8
					&& G_QUEUE_REQ_CALL_RSP[i].rsp_check.seqTimetamp==seq_timetamp){
#else
				u16 timetamp_delta = MAKE_U16(packet.frame.payload[SIZEU8(fl_timetamp_withstep_t)],packet.frame.payload[SIZEU8(fl_timetamp_withstep_t)+1]);
				u8 slaveID_rsp[SIZEU8(packet.frame.payload)-SIZEU8(fl_timetamp_withstep_t)- SIZEU8(timetamp_delta)]; // 22 - 5 - 1
				memset(slaveID_rsp,0xFF,SIZEU8(slaveID_rsp));
				memcpy(slaveID_rsp,&packet.frame.payload[SIZEU8(fl_timetamp_withstep_t)+SIZEU8(timetamp_delta)],SIZEU8(slaveID_rsp));
				u8 my_slaveID_inrspcom = plog_IndexOf(slaveID_rsp,&G_QUEUE_REQ_CALL_RSP[i].rsp_check.slaveID,1,SIZEU8(slaveID_rsp));

				P_PRINTFHEX_A(API,slaveID_rsp,SIZEU8(slaveID_rsp),"SlaveID(%d):",SIZEU8(slaveID_rsp));
				LOGA(API,"REQ Timetmap  :%d\r\n",G_QUEUE_REQ_CALL_RSP[i].rsp_check.seqTimetamp);
				LOGA(API,"TimeTamp_Seq  :%d\r\n",seq_timetamp);
				LOGA(API,"TimeTamp_delta:%d\r\n",timetamp_delta);
				LOGA(API,"MyID:%02X,Found:%d\r\n",G_QUEUE_REQ_CALL_RSP[i].rsp_check.slaveID,my_slaveID_inrspcom);

				if ((G_QUEUE_REQ_CALL_RSP[i].rsp_check.slaveID != 0xFF && packet.frame.slaveID.id_u8 ==0xFF && my_slaveID_inrspcom != -1
						&& G_QUEUE_REQ_CALL_RSP[i].rsp_check.seqTimetamp <= (seq_timetamp + (u32)timetamp_delta)) //Check RSP common
					|| (G_QUEUE_REQ_CALL_RSP[i].rsp_check.slaveID == packet.frame.slaveID.id_u8
							&& G_QUEUE_REQ_CALL_RSP[i].rsp_check.seqTimetamp==seq_timetamp)
					)

				{
					_myID->active = true;
#endif
					rslt = i;
					LOGA(API,"RSP(%d|%d):%d/%d\r\n",G_QUEUE_REQ_CALL_RSP[i].rsp_check.seqTimetamp,seq_timetamp,packet.frame.hdr,packet.frame.slaveID.id_u8);
					G_QUEUE_REQ_CALL_RSP[i].rsp_cb((void*)&G_QUEUE_REQ_CALL_RSP[i],(void*)&_pack); //timeout
					//clear event
					_queue_REQcRSP_clear(&G_QUEUE_REQ_CALL_RSP[i]);
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
fl_hdr_nwk_type_e G_NWK_HDR[] = {NWK_HDR_A5_HIS,NWK_HDR_F6_SENDMESS,NWK_HDR_F7_RSTPWMETER,NWK_HDR_F8_PWMETER_SET,NWK_HDR_F5_INFO, NWK_HDR_COLLECT, NWK_HDR_HEARTBEAT,NWK_HDR_ASSIGN,NWK_HDR_55,NWK_HDR_11_REACTIVE }; // register cmdid RSP
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
