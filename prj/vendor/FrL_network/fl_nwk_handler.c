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

s8 fl_queueREQcRSP_find(fl_rsp_callback_fnc *_cb,u32 _timeout_ms, u8 *o_avaislot){
	u8 indx = 0;
	*o_avaislot=fl_queueREQcRSP_sort();
	for (indx = 0; indx < QUEUE_RSP_SLOT_MAX; ++indx) {
		if(*G_QUEUE_REQ_CALL_RSP[indx].rsp_cb == *_cb && G_QUEUE_REQ_CALL_RSP[indx].timeout == (s32)_timeout_ms)
		{
			*o_avaislot = 0xFF;
			return indx;
		}
	}
	return -1;
}

s8 fl_queueREQcRSP_add(u8 slaveid,u8 cmdid,u8* _payloadreq,u8 _len,fl_rsp_callback_fnc *_cb, u32 _timeout_ms,u8 _retry){
	u8 avai_slot= 0xFF;
	if(fl_queueREQcRSP_find(_cb,_timeout_ms,&avai_slot) == -1 && avai_slot < QUEUE_RSP_SLOT_MAX){
		G_QUEUE_REQ_CALL_RSP[avai_slot].timeout = (s32)_timeout_ms;
		G_QUEUE_REQ_CALL_RSP[avai_slot].timeout_set = (s32)_timeout_ms;
		G_QUEUE_REQ_CALL_RSP[avai_slot].rsp_cb = *_cb;
		G_QUEUE_REQ_CALL_RSP[avai_slot].rsp_check.hdr_cmdid = cmdid;
		G_QUEUE_REQ_CALL_RSP[avai_slot].rsp_check.slaveID = slaveid;
		G_QUEUE_REQ_CALL_RSP[avai_slot].retry = _retry;
		G_QUEUE_REQ_CALL_RSP[avai_slot].req_payload.len = _len;
		memcpy(G_QUEUE_REQ_CALL_RSP[avai_slot].req_payload.payload,_payloadreq,_len);
		return avai_slot;
	}
	ERR(API,"queueREQcRSP Add (%d) %d/%d ms \r\n",avai_slot,(u32)_cb,_timeout_ms);
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
	if(blt_soft_timer_find(&fl_queue_REQnRSP_TimeoutStart)==-1){
		LOGA(INF,"REQcRSP initialization (%d ms)!!\r\n",QUEUQ_REQcRSP_INTERVAL);
		blt_soft_timer_add(&fl_queue_REQnRSP_TimeoutStart,QUEUQ_REQcRSP_INTERVAL);
		fl_queueREQcRSP_clear(G_QUEUE_REQ_CALL_RSP);
	}else{
		u8 avai_slot = fl_queueREQcRSP_sort();
		for(u8 i =0;i < avai_slot;i++){
			//check timeout
			if(G_QUEUE_REQ_CALL_RSP[i].timeout > 0 && G_QUEUE_REQ_CALL_RSP[i].rsp_cb != 0){
				G_QUEUE_REQ_CALL_RSP[i].timeout -= (s32)QUEUQ_REQcRSP_INTERVAL;
				if(G_QUEUE_REQ_CALL_RSP[i].timeout <= 0){
					fl_rsp_container_t REQ_BUF = G_QUEUE_REQ_CALL_RSP[i];
					//clear event
					_queue_REQcRSP_clear(&G_QUEUE_REQ_CALL_RSP[i]);
					//Excute retry
					if (REQ_BUF.retry > 0) {
						REQ_BUF.retry--;
#ifdef MASTER_CORE
						u8 mac[6];
						if (fl_master_SlaveMAC_get(REQ_BUF.rsp_check.slaveID,mac) != -1) {
							if (-1 != fl_api_master_req(mac,REQ_BUF.rsp_check.hdr_cmdid,REQ_BUF.req_payload.payload,REQ_BUF.req_payload.len,
											REQ_BUF.rsp_cb,(u32) REQ_BUF.timeout_set / 1000,REQ_BUF.retry)) {
								LOGA(API,"Retry;%d\r\n",REQ_BUF.retry);
								continue;
							}
						}
#else
						if(-1!=fl_api_slave_req(REQ_BUF.rsp_check.hdr_cmdid,REQ_BUF.req_payload.payload,REQ_BUF.req_payload.len,REQ_BUF.rsp_cb,
										(u32)REQ_BUF.timeout_set/1000,REQ_BUF.retry)) {
							LOGA(API,"Retry;%d\r\n",REQ_BUF.retry);
							continue;
						}
#endif
					}
					LOGA(API,"%d/%d TIMEOUT!!! \r\n",G_QUEUE_REQ_CALL_RSP[i].rsp_check.hdr_cmdid,G_QUEUE_REQ_CALL_RSP[i].rsp_check.slaveID);
					REQ_BUF.rsp_cb((void*) &REQ_BUF,0); //timeout
				}
			}
		}
	}
	return 0;
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
void fl_queue_REQcRSP_ScanRec(fl_pack_t _pack)
{
#else
void fl_queue_REQcRSP_ScanRec(fl_pack_t _pack,void *_id)
{
	fl_nodeinnetwork_t *_myID = (fl_nodeinnetwork_t*)_id;
#endif
	extern u8 fl_packet_parse(fl_pack_t _pack, fl_dataframe_format_t *rslt);
	fl_data_frame_u packet;
	if (!fl_packet_parse(_pack,&packet.frame)) {
		ERR(API,"Packet parse fail!!!\r\n");
		return;
	}else{
#ifndef MASTER_CORE
		if(_myID->slaveID.id_u8 != packet.frame.slaveID.id_u8 || _myID->slaveID.id_u8==0xFF){//not me
			return;
		}
		//Synchronize debug log
		NWK_DEBUG_STT = packet.frame.endpoint.dbg;
		DEBUG_TURN(NWK_DEBUG_STT);
		_myID->active = true;
#endif
		u8 avai_slot = fl_queueREQcRSP_sort();
		for(u8 i =0;i < avai_slot;i++){
			if (G_QUEUE_REQ_CALL_RSP[i].rsp_check.hdr_cmdid != 0 && G_QUEUE_REQ_CALL_RSP[i].rsp_check.slaveID != 0xFF) {
				if (G_QUEUE_REQ_CALL_RSP[i].rsp_check.hdr_cmdid == packet.frame.hdr
					&& G_QUEUE_REQ_CALL_RSP[i].rsp_check.slaveID == packet.frame.slaveID.id_u8) {
					LOGA(API,"RSP:%d/%d\r\n",packet.frame.hdr,packet.frame.slaveID.id_u8);
					G_QUEUE_REQ_CALL_RSP[i].rsp_cb((void*)&G_QUEUE_REQ_CALL_RSP[i],(void*)&_pack); //timeout
					//clear event
					_queue_REQcRSP_clear(&G_QUEUE_REQ_CALL_RSP[i]);
				}
			}
			//else return;
		}
	}
}

/*======================================================================*/


/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/


/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/



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
