/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_adv_repeat.c
 *Created on		: Jul 10, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/
#include "tl_common.h"
#include "fl_adv_repeat.h"
#include "fl_adv_proc.h"
#include "fl_nwk_handler.h"
#include "fl_nwk_protocol.h"

/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
/*---------------- Total Packet Repeat --------------------------*/
#define PACK_REPEAT_SIZE 		16
fl_pack_t g_pack_array[PACK_REPEAT_SIZE];
fl_data_container_t G_REPEAT_CONTAINER = { .data = g_pack_array, .head_index = 0, .tail_index = 0, .mask = PACK_REPEAT_SIZE - 1, .count = 0 };

//fl_repeat_settings_t G_REPEATER_SETTINGS = { .level = REPEAT_LEVEL, .threshold_rssi = REPEAT_RSSI_THRES };

/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/

/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
/***************************************************
 * @brief 		: build packet repeat via the freelux protocol
 *
 * @param[in] 	: none
 *
 * @return	  	: 1 if success, 0 otherwise
 *
 ***************************************************/
fl_pack_t fl_repeat_packet_build(fl_pack_t _pack) {
	fl_pack_t pack_built;
	fl_data_frame_u packet;
	extern u8 fl_packet_parse(fl_pack_t _pack, fl_dataframe_format_t *rslt);
	fl_packet_parse(_pack,&packet.frame);

	packet.frame.endpoint.repeat_cnt += 1;

	pack_built.length = SIZEU8(packet.bytes) - 1; //skip rssi
	memcpy(pack_built.data_arr,packet.bytes,pack_built.length);

	LOGA(ZIG_GP,"Repeat lvl: %d\r\n",packet.frame.endpoint.repeat_cnt);

	return pack_built;
}
/***************************************************
 * @brief 		:filter rssi before repeating
 *
 * @param[in] 	:none
 *
 * @return	  	:true : near otherwise far
 *
 ***************************************************/
bool fl_repeat_filter_rssi(fl_pack_t *_pack) {
//	double envFactor = 2.5;
//	double txPower = 9.1;
	s8 rssi_s8 = _pack->data_arr[_pack->length - 1];
//    double_t exponent = pow(10.0, ((double)(txPower - rssi_s8)) / (10.0 * envFactor));
//    LOGA(ZIG_GP,"Distance:%f cm(%d |%f)\r\n",exponent,rssi_s8,txPower);  //
//    return true;

	//todo something filter
	return (rssi_s8 >= REPEAT_RSSI_THRES);
}
/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/
/***************************************************
 * @brief 		:soft-timer callback for the sending
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
int fl_repeat_send_cbk(void) {
	fl_pack_t data_in_queue;
	if (FL_QUEUE_GET(&G_REPEAT_CONTAINER,&data_in_queue)) {
		//u8 pack_arr[SIZEU8(fl_dataframe_format_t) - 1];
		fl_pack_t packet_built;
		packet_built = fl_repeat_packet_build(data_in_queue);
		//P_PRINTFHEX_A(ZIG_GP,pack_arr,SIZEU8(pack_arr),"%s:","RepeatPack");
//		LOGA(ZIG_GP,"Repeat lvl: %d\r\n",packet_built.data_arr[packet_built.length - 1] & 0x03);

		fl_adv_sendFIFO_add(packet_built);
	}
	return 0;
}
/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/
void fl_repeater_init(void) {
	LOG_P(ZIG_GP,"Repeater init -> ok\r\n");
	FL_QUEUE_CLEAR(&G_REPEAT_CONTAINER,PACK_REPEAT_SIZE);
}
void fl_repeat_run(fl_pack_t *_pack_repeat) {
	//LOGA(ZIG_GP,"REPEAT(%d)-> run\r\n",_pack_repeat->length - 1);
	if (FL_QUEUE_FIND(&G_REPEAT_CONTAINER,_pack_repeat,_pack_repeat->length - 1/*skip rssi */) == -1) { //+ repeat_cnt
		if (FL_QUEUE_ADD(&G_REPEAT_CONTAINER,_pack_repeat) < 0) {
			ERR(ZIG_GP,"Err <QUEUE ADD> - G_REPEAT_CONTAINER!!\r\n");
		} else {
			s8 rssi = _pack_repeat->data_arr[_pack_repeat->length - 1];
			LOGA(ZIG_GP,"QUEUE REPEAT ADD (len:%d|RSSI:%d): (%d)%d-%d\r\n",_pack_repeat->length,rssi,G_REPEAT_CONTAINER.count,
					G_REPEAT_CONTAINER.head_index,G_REPEAT_CONTAINER.tail_index);
			fl_repeat_send_cbk();
		}
	} else {
		LOG_P(ZIG_GP,"Packet has repeated!!!\r\n");
	}
}
