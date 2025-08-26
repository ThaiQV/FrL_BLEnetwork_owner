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
#include "../TBS_dev/TBS_dev_config.h"

void _rsp_callback(void *_data,void* _data2){
	fl_rsp_container_t *data =  (fl_rsp_container_t*)_data;
	LOGA(API,"Timeout:%d\r\n",data->timeout);
	LOGA(API,"cmdID  :%0X\r\n",data->rsp_check.hdr_cmdid);
	LOGA(API,"SlaveID:%0X\r\n",data->rsp_check.slaveID);
	//rsp data

	if(data->timeout >= 0){
		fl_pack_t *packet = (fl_pack_t *)_data2;
		P_PRINTFHEX_A(API,packet->data_arr,packet->length,"RSP: ");
	}
}
//void _rsp_callback1(void *_data){
//	_rsp_callback(_data);
//}
//void _rsp_callback2(void *_data){
//	_rsp_callback(_data);
//}
//void _rsp_callback3(void *_data){
//	_rsp_callback(_data);
//}
//void _rsp_callback4(void *_data){
//	_rsp_callback(_data);
//}

int TEST_slave_sendREQ(void) {
	u32 rand_time_send = 5000;
	static u32 test_pack_cnt = 0;
	if (blt_soft_timer_find(&TEST_slave_sendREQ) == -1) {
		blt_soft_timer_add(&TEST_slave_sendREQ,rand_time_send * 1000);
	} else {
		rand_time_send = RAND(500,4500);
		if (IsJoinedNetwork() && IsOnline()) {
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
#ifdef COUNTER_DEVICE
u8 TEST_Buttons(fl_exButton_states_e _state, void *_data) {
	u32 *time_tick = (u32*)_data;
	static u32 test_pack_cnt = 0;
	u8 test_u8[4];
	test_pack_cnt++;
	test_u8[0] = U32_BYTE0(test_pack_cnt);
	test_u8[1] = U32_BYTE1(test_pack_cnt);
	test_u8[2] = U32_BYTE2(test_pack_cnt);
	test_u8[3] = U32_BYTE3(test_pack_cnt);
	if (IsJoinedNetwork() && IsOnline()){
		fl_api_slave_req(NWK_HDR_55,test_u8,SIZEU8(test_u8),&_rsp_callback,800);
		LOGA(PERI,"BUTT Calling %s (%d ms):%d\r\n",_state == BUTT_STATE_PRESSnHOLD ? "Press & hold" : "Press & Release",
				(clock_time()-*time_tick)/SYSTEM_TIMER_TICK_1MS,test_pack_cnt);
	}
	//Must to clear status if done
	return BUTT_STATE_NONE;
	//else return _state;
}
u8 TEST_Buttons_RST(fl_exButton_states_e _state, void *_data) {
	u32 *time_tick = (u32*)_data;
	LOGA(USER,"BUTT RST %s (%d ms)\r\n",_state==BUTT_STATE_PRESSnHOLD?"Press & hold":"Press & Release",
			(clock_time()-*time_tick)/SYSTEM_TIMER_TICK_1MS);
	if(_state == BUTT_STATE_PRESSnHOLD){
		ERR(USER,"Factory!!!!!\r\n");
		fl_db_clearAll();
		delay_ms(1000);
		sys_reboot();
	}
	//Must to clear status if done
	return BUTT_STATE_NONE;
	//else return _state;
}
#endif
