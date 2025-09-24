/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: TBS_history_proc.c
 *Created on		: Sep 23, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#include "stack/ble/ble.h"
#include "tl_common.h"
#include "TBS_dev_config.h"
#include "../FrL_Network/fl_nwk_handler.h"
#include "../FrL_Network/fl_nwk_api.h"
#include "../FrL_Network/fl_adv_proc.h"

#ifndef MASTER_CORE

#define NUM_HISTORY			20
/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
extern volatile u8 NWK_DEBUG_STT; // it will be assigned into end-point byte (dbg :1bit)
extern volatile u8 NWK_REPEAT_MODE; // 1: level | 0 : non-level
extern volatile u8  NWK_REPEAT_LEVEL;
//
typedef struct {
	u32 timetamp;     // timetamp (32 bits)
	struct {
		//add new index of packet
		u16 index;
		u8 bt_call;
		u8 bt_endcall;
		u8 bt_rst;
		u16 pass_product;
		u16 err_product;
		//add new mode and indx
		u8 mode;
		//previous_status
		u16 pre_pass_product;
		u16 pre_err_product;
		u8 pre_mode;
	//reverse
	//		u8 reverse[7];
	} data;
}__attribute__((packed)) tbs_history_counter_t;

typedef struct {
	u16 indx;
#ifdef COUNTER_DEVICE
	u8 data[SIZEU8(tbs_history_counter_t)];
#endif
#ifdef POWER_METER_DEVICE
	u8 data[SIZEU8(tbs_history_powermeter_t)]
#endif
	u8 status_proc;
}__attribute__((packed)) tbs_history_t;

tbs_history_t G_HISTORY_CONTAINER[NUM_HISTORY];

/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
#define DATA_HISTORY_SIZE 			SIZEU8(G_HISTORY_CONTAINER[0].data)

//EXAMPLE DATABASE
u8 sample_history_database[NUM_HISTORY][DATA_HISTORY_SIZE];


/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/
void TBS_history_createSample(void) {
#ifdef POWER_METER_DEVICE
		for (int i = 0; i < NUM_HISTORY; ++i) {
			tbs_device_powermeter_t record = {
				.timetamp = 1758639600 + i * 60, //
				.type = 0x01,//
				.data = {
					.index = i,
					.frequency = 50,
					.voltage = 220,
					.current1 = 11,
					.current2 = 22,
					.current3 = 33,
					.power1 = 100,
					.power2 = 101,
					.power3 = 102,
					.time1 = 30,
					.time2 = 31,
					.time3 = 32,
					.energy1 = 111111,
					.energy2 = 222222,
					.energy3 = 333333}};

			u8 temp_buffer[34]; // full struct packed
			tbs_pack_powermeter_data(&record,temp_buffer);
			// skip mac
			memcpy(sample_history_database[i],&temp_buffer[6],DATA_HISTORY_SIZE);
	//
	//		P_PRINTFHEX_A(INF_FILE,powermeter_array[i],RECORD_SIZE,"[%d]",i);
	//		tbs_device_powermeter_t received;
	//		tbs_unpack_powermeter_data(&received,temp_buffer);
	//		tbs_power_meter_printf((void*)&received);
		}
#endif
#ifdef COUNTER_DEVICE
	for (int i = 0; i < NUM_HISTORY; ++i) {
		tbs_history_counter_t record = {
				.timetamp = 1758639600 + i * 60, //
				.data = {
						.index=i,
						.bt_call = 2,
						.bt_endcall = 3,
						.bt_rst = 4,
						.pass_product = 5,
						.err_product = 6,
						.mode = 1,
						.pre_pass_product = 7,
						.pre_err_product=8,
						.pre_mode =0
						}
		};
		u8 *dst = sample_history_database[i];
		int pos = 0;
		// Copy timetamp (4 bytes)
		memcpy(&dst[pos],&record.timetamp,sizeof(record.timetamp));
		pos += sizeof(record.timetamp);
		// Copy data struct (15 bytes)
		memcpy(&dst[pos],&record.data,sizeof(record.data));
	}
#endif
	//printf test
	for (u8 var = 0; var < NUM_HISTORY; ++var) {
		P_PRINTFHEX_A(APP,sample_history_database[var],DATA_HISTORY_SIZE,"[%d]",var);
	}
}

fl_pack_t tbs_history_create_pack(u8* _data) {
	fl_pack_t packet_built;
	packet_built.length = 0;
	memset(packet_built.data_arr,0,SIZEU8(packet_built.data_arr));
	fl_data_frame_u packet;
	/* parse parameter in the _data */
#ifdef COUNTER_DEVICE
	tbs_history_counter_t *data_dev = (tbs_history_counter_t*)_data;
#else
	tbs_device_powermeter_t *data_dev = (tbs_device_powermeter_t*)_data;
#endif
	packet.frame.hdr = NWK_HDR_A5_HIS;

	packet.frame.timetamp[0] = U32_BYTE0(data_dev->timetamp);
	packet.frame.timetamp[1] = U32_BYTE1(data_dev->timetamp);
	packet.frame.timetamp[2] = U32_BYTE2(data_dev->timetamp);
	packet.frame.timetamp[3] = U32_BYTE3(data_dev->timetamp);
	packet.frame.milltamp = 0;

	packet.frame.slaveID.id_u8 = fl_nwk_mySlaveID();

	//payload
	memset(packet.frame.payload,0xFF,SIZEU8(packet.frame.payload));
	memcpy(packet.frame.payload,&data_dev->data,SIZEU8(data_dev->data));
	//crc
	packet.frame.crc8 = fl_crc8(packet.frame.payload,SIZEU8(packet.frame.payload));
	//endpoint
	packet.frame.endpoint.dbg = NWK_DEBUG_STT;
	packet.frame.endpoint.repeat_cnt = NWK_REPEAT_LEVEL;
	packet.frame.endpoint.rep_settings = NWK_REPEAT_LEVEL;
	packet.frame.endpoint.repeat_mode = NWK_REPEAT_MODE;
	//Create packet from slave
	packet.frame.endpoint.master = FL_FROM_SLAVE;
	//Create adv packet
	packet_built.length=SIZEU8(packet.bytes)-1;
	memcpy(packet_built.data_arr,packet.bytes,packet_built.length);

	return packet_built;
}

static void _CLEAR_G_HISTORY(void) {
	for (u16 i = 0; i < NUM_HISTORY; i++) {
		G_HISTORY_CONTAINER[i].indx = U16_MAX;
		memset(G_HISTORY_CONTAINER[i].data,0,DATA_HISTORY_SIZE);
		G_HISTORY_CONTAINER[i].status_proc = U8_MAX;
	}
}
/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/
s8 TBS_History_Get(u16 _from, u16 _to) {
	_CLEAR_G_HISTORY();
	LOGA(APP,"Get his from:%d->to:%d\r\n",_from,_to);
	for (u16 i = 0; i < NUM_HISTORY && (_from + i) < _to; i++) {
		memset(G_HISTORY_CONTAINER[i].data,0,DATA_HISTORY_SIZE);
		G_HISTORY_CONTAINER[i].indx = _from + i;
		G_HISTORY_CONTAINER[i].status_proc = 0;
		LOGA(APP,"Get his (%d-%d):[%d]%d\r\n",_from,_to,i,G_HISTORY_CONTAINER[i].indx);
	}
	return (_from - _to);
}

void TBS_History_LoadFromFlash(void){
	for (u16 i = 0; i < NUM_HISTORY ; i++) {
		if(G_HISTORY_CONTAINER[i].indx != U16_MAX && G_HISTORY_CONTAINER[i].status_proc == 0){
			//todo: read flash and fill in the G_HISTORY
			//Example:
			memcpy(G_HISTORY_CONTAINER[i].data,sample_history_database[G_HISTORY_CONTAINER[i].indx],DATA_HISTORY_SIZE);
			G_HISTORY_CONTAINER[i].status_proc = 1;
			P_PRINTFHEX_A(APP,G_HISTORY_CONTAINER[i].data,DATA_HISTORY_SIZE,"[%d]HIS:",G_HISTORY_CONTAINER[i].indx);
			fl_adv_sendFIFO_add(tbs_history_create_pack(G_HISTORY_CONTAINER[i].data));
		}
	}
}
/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/

void TBS_History_Init(void){
	//clear G_HISTORY
	_CLEAR_G_HISTORY();
	TBS_history_createSample();
//	for(u8 i=0;i<8;i++){
//		fl_adv_sendFIFO_add(tbs_history_create_pack(G_HISTORY_CONTAINER[i].data));
//	}
}

void TBS_History_Run(void){
//	static u8 add_his_slot = 0;
//	fl_adv_sendFIFO_add(tbs_history_create_pack(G_HISTORY_CONTAINER[add_his_slot].data));
//	add_his_slot++;
//	if(add_his_slot>=NUM_HISTORY){
//		add_his_slot = 0;
//	}
	TBS_History_LoadFromFlash();
}
#endif
