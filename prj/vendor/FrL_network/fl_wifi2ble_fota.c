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
#ifdef MASTER_CORE
/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
#define FW_DATA_SIZE 		32
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

u8 fl_wifi2ble_fota_push(u8 *_fw, u8 _len) {
	fl_pack_t fw_pack;
	fw_pack.length = _len;
	memset(fw_pack.data_arr,0,sizeof(fw_pack.data_arr));
	memcpy(fw_pack.data_arr,_fw,_len);
	if (FL_QUEUE_ADD(&G_FW_CONTAINER,&fw_pack) < 0) {
		ERR(INF_FILE,"Err FULL <QUEUE ADD FW FOTA>!!\r\n");
		return -1;
	} else {
		P_PRINTFHEX_A(INF_FILE,fw_pack.data_arr,fw_pack.length,"PUSH(cnt:%d)=>FW[%d]",fw_pack.data_arr[0],FL_NWK_FOTA_IsReady());
		return G_FW_CONTAINER.tail_index;
	}
	return -1;
}

/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/
fl_pack_t _fw_packet_build(u8* _fw){
#define CONTANST_FW_SIZE	(2+26+1) //ordinal (2bytes) + fw (26bytes)+ crc (1bytes)
	fl_pack_t pack;
	memset(pack.data_arr,0,sizeof(pack.data_arr));
	pack.data_arr[0] = NWK_HDR_FOTA;
	memcpy(pack.data_arr+1,_fw,CONTANST_FW_SIZE);
	pack.length = 31;

	P_PRINTFHEX_A(INF_FILE,pack.data_arr,sizeof(pack.data_arr),"FW SEND(%d):",pack.length);
	return pack;
}

/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/

void fl_wifi2ble_fota_init(void){
	LOG_P(INF_FILE,"FOTA Initilization!!!\r\n");
}

void fl_wifi2ble_fota_run(void) {
	extern fl_adv_settings_t G_ADV_SETTINGS;
	extern volatile u8 F_SENDING_STATE;
	fl_pack_t fw_in_queue;
	if (!F_SENDING_STATE) {
		if (FL_QUEUE_GET(&G_FW_CONTAINER,&fw_in_queue)) {
//			u16 ordinal = MAKE_U16(fw_in_queue.data_arr[1],fw_in_queue.data_arr[0]);
			//P_PRINTFHEX_A(INF_FILE,fw_in_queue.data_arr+2,fw_in_queue.length -2,"GET(cnt:%d)=>FW[%d]",ordinal,FL_NWK_FOTA_IsReady());
			F_SENDING_STATE = 1;
			fl_pack_t fw_pack_build = _fw_packet_build(fw_in_queue.data_arr);

			fl_adv_send(fw_pack_build.data_arr,fw_pack_build.length,G_ADV_SETTINGS.adv_duration);
		}
	}
}
#endif
