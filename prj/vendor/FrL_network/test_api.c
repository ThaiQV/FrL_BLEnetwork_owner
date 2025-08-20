/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: test_api.c
 *Created on		: Aug 19, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#include "tl_common.h"
#include "fl_nwk_api.h"

void _rsp_callback(void *_data){
	u8 *data =  (u8*)_data;
	LOGA(API,"Data:%d\r\n",*data);
}
void _rsp_callback1(void *_data){
	_rsp_callback(_data);
}
void _rsp_callback2(void *_data){
	_rsp_callback(_data);
}
void _rsp_callback3(void *_data){
	_rsp_callback(_data);
}
void _rsp_callback4(void *_data){
	_rsp_callback(_data);
}

int TEST_slave_sendREQ(void) {
	u32 rand_time_send = 0;
	static u32 test_pack_cnt = 0;
	if (blt_soft_timer_find(&TEST_slave_sendREQ) == -1) {
		blt_soft_timer_add(&TEST_slave_sendREQ,rand_time_send * 1000);
	} else {
		rand_time_send = 5000;//RAND(500,4500);
		if (IsJoinedNetwork()) {
			LOGA(APP,"TEST REQ %d ms\r\n",rand_time_send);
			u8 test_u8[4];
			test_pack_cnt++;
			test_u8[0] = U32_BYTE0(test_pack_cnt);
			test_u8[1] = U32_BYTE1(test_pack_cnt);
			test_u8[2] = U32_BYTE2(test_pack_cnt);
			test_u8[3] = U32_BYTE3(test_pack_cnt);
			fl_api_slave_req(NWK_HDR_55,test_u8,SIZEU8(test_u8),&_rsp_callback,1029);
//			test_pack_cnt++;
//			test_u8[0] = U32_BYTE0(test_pack_cnt);
//			test_u8[1] = U32_BYTE1(test_pack_cnt);
//			test_u8[2] = U32_BYTE2(test_pack_cnt);
//			test_u8[3] = U32_BYTE3(test_pack_cnt);
//			fl_api_slave_req(NWK_HDR_55,test_u8,SIZEU8(test_u8),&_rsp_callback1,500);
//			test_pack_cnt++;
//			test_u8[0] = U32_BYTE0(test_pack_cnt);
//			test_u8[1] = U32_BYTE1(test_pack_cnt);
//			test_u8[2] = U32_BYTE2(test_pack_cnt);
//			test_u8[3] = U32_BYTE3(test_pack_cnt);
//			fl_api_slave_req(NWK_HDR_55,test_u8,SIZEU8(test_u8),&_rsp_callback2,300);
//			test_pack_cnt++;
//			test_u8[0] = U32_BYTE0(test_pack_cnt);
//			test_u8[1] = U32_BYTE1(test_pack_cnt);
//			test_u8[2] = U32_BYTE2(test_pack_cnt);
//			test_u8[3] = U32_BYTE3(test_pack_cnt);
//			fl_api_slave_req(NWK_HDR_55,test_u8,SIZEU8(test_u8),&_rsp_callback3,300);
//			test_pack_cnt++;
//			test_u8[0] = U32_BYTE0(test_pack_cnt);
//			test_u8[1] = U32_BYTE1(test_pack_cnt);
//			test_u8[2] = U32_BYTE2(test_pack_cnt);
//			test_u8[3] = U32_BYTE3(test_pack_cnt);
//			fl_api_slave_req(NWK_HDR_55,test_u8,SIZEU8(test_u8),&_rsp_callback4,730);
		}
		return rand_time_send * 1000;
	}
	return 0;
}
