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

/******************************************************************************/
/******************************************************************************/
/***                                External-libs		                     **/
/******************************************************************************/
/******************************************************************************/
#include "../Freelux_libs/storage_weekly_data.h"

#define tbs_history_flash_init					storage_init
#define tbs_history_store						storage_put_data
#define tbs_history_load						storage_get_data
#define tbs_history_cleanAll					storage_clean

/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
#define NUM_HISTORY								512

extern volatile u8 NWK_DEBUG_STT; // it will be assigned into end-point byte (dbg :1bit)
extern volatile u8 NWK_REPEAT_MODE; // 1: level | 0 : non-level
extern volatile u8  NWK_REPEAT_LEVEL;
//
typedef struct {
	u32 timetamp;     // timetamp (32 bits)
	u8 type;
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
		u32 pre_timetamp;
	//reverse
	//		u8 reverse[7];
	} data;
}__attribute__((packed)) tbs_history_counter_t;
//
typedef struct {
	u32 timetamp;     	// timetamp (32 bits)
	// Measurement fields (bit-level precision noted)
	u8 type;
	struct {
		u16 index;          // 16 bits
		u8 frequency;     	// 7 bits
		u16 voltage;        // 9 bits
		u16 current1;       // 10 bits
		u16 current2;       // 10 bits
		u16 current3;       // 10 bits
        u8 fac_power1_current_type1;			// 7 bits + 1 bit
        u8 fac_power2_current_type2;			// 7 bits+ 1 bit
        u8 fac_power3_current_type3;			// 7 bits + 1 bit
        u8 time1;          // 6 bits
        u8 time2;          // 6 bits
        u8 time3;          // 6 bits
        u32 energy1;       // 24 bits
        u32 energy2;       // 24 bits
        u32 energy3;       // 24 bits
	} data;
}__attribute__((packed)) tbs_history_powermeter_t;

typedef struct {
	u16 indx;
#ifdef COUNTER_DEVICE
	u8 data[SIZEU8(tbs_history_counter_t)];
#endif
#ifdef POWER_METER_DEVICE
	u8 data[SIZEU8(tbs_history_powermeter_t)];
#endif
	u8 status_proc;
}__attribute__((packed)) tbs_history_t;

tbs_history_t G_HISTORY_CONTAINER[NUM_HISTORY];

/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
#define DATA_HISTORY_SIZE 			22//SIZEU8(G_HISTORY_CONTAINER[0].data)

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
					.mac = {0, 0, 0, 0, 0, 0},
					.timetamp = 1758639600 + i * 60,
					.type = TBS_POWERMETER,
					.data= {
							.index = i,
							.frequency = 50,
							.voltage = 220,
							.current1 = 11,
							.current2 = 22,
							.current3 = 33,
							.fac_power1 = 220,
							.fac_power2 = 221,
							.fac_power3 = 222,
							.time1 = 51,
							.time2 = 52,
							.time3 = 53,
							.energy1 = 111111,
							.energy2 = 222222,
							.energy3 = 333333,
											}};
			memcpy(record.mac,blc_ll_get_macAddrPublic(),SIZEU8(record.mac));
			u8 temp_buffer[SIZEU8(tbs_history_powermeter_t)]; // full struct packed
			tbs_pack_powermeter_data(&record,temp_buffer);

			// skip mac -> store timetamp
			memcpy(&sample_history_database[i][0],&temp_buffer[6],4);
			// skip type -> store data
			memcpy(&sample_history_database[i][4],&temp_buffer[6+4+1],SIZEU8(record.data));
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
						.pre_mode =0,
						.pre_timetamp =0,
						.pre_timetamp = 1758639600 + ((i!=0)?i-1:0)*60
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
	static u8 next_req=0;
	/* parse parameter in the _data */
#ifdef COUNTER_DEVICE
	tbs_history_counter_t *data_dev = (tbs_history_counter_t*)_data;
	//payload
	memset(packet.frame.payload,0xFF,SIZEU8(packet.frame.payload));
	memcpy(packet.frame.payload,&data_dev->data,SIZEU8(data_dev->data));
	packet.frame.timetamp[0] = U32_BYTE0(data_dev->timetamp);
	packet.frame.timetamp[1] = U32_BYTE1(data_dev->timetamp);
	packet.frame.timetamp[2] = U32_BYTE2(data_dev->timetamp);
	packet.frame.timetamp[3] = U32_BYTE3(data_dev->timetamp);
#else
//	tbs_history_powermeter_t *data_dev = (tbs_history_powermeter_t*)_data;
	tbs_device_powermeter_t data_dev;
	memcpy(data_dev.mac,fl_nwk_mySlaveMac,SIZEU8(data_dev.mac));
	memcpy((u8*)&data_dev + SIZEU8(data_dev.mac),_data,SIZEU8(tbs_device_powermeter_t)-SIZEU8(data_dev.mac));
	P_PRINTFHEX_A(INF,data_dev,SIZEU8(tbs_device_powermeter_t)-SIZEU8(data_dev.mac),"HIS PACK:");
	u8 packed_data[POWER_METER_BITSIZE];
	tbs_pack_powermeter_data(&data_dev,packed_data);
	u8 indx_data = SIZEU8(data_dev.type) + SIZEU8(data_dev.mac) + SIZEU8(data_dev.timetamp);
	//payload
	memset(packet.frame.payload,0xFF,SIZEU8(packet.frame.payload));
	memcpy(packet.frame.payload,&packed_data[indx_data],SIZEU8(packet.frame.payload));
	packet.frame.timetamp[0] = U32_BYTE0(data_dev.timetamp);
	packet.frame.timetamp[1] = U32_BYTE1(data_dev.timetamp);
	packet.frame.timetamp[2] = U32_BYTE2(data_dev.timetamp);
	packet.frame.timetamp[3] = U32_BYTE3(data_dev.timetamp);
#endif
	packet.frame.hdr = NWK_HDR_A5_HIS;

	packet.frame.milltamp = next_req++;

	packet.frame.slaveID = fl_nwk_mySlaveID();

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
	for (u16 i = 0; i < NUM_HISTORY && (_from + i) <= _to; i++) {
		memset(G_HISTORY_CONTAINER[i].data,0,DATA_HISTORY_SIZE);
		G_HISTORY_CONTAINER[i].indx = _from + i;
		G_HISTORY_CONTAINER[i].status_proc = 0;
		LOGA(APP,"Get his (%d-%d):[%d]%d\r\n",_from,_to,i,G_HISTORY_CONTAINER[i].indx);
	}
	return (_from - _to);
}

void TBS_History_LoadFromFlash(void){
	for (u16 i = 0; i < NUM_HISTORY ; i++)
	{
		if(G_HISTORY_CONTAINER[i].indx != U16_MAX && G_HISTORY_CONTAINER[i].status_proc == 0){
			//todo: read flash and fill in the G_HISTORY
//			memcpy(G_HISTORY_CONTAINER[i].data,sample_history_database[G_HISTORY_CONTAINER[i].indx],DATA_HISTORY_SIZE);

#ifdef COUNTER_DEVICE
	tbs_history_counter_t* his_dev = (tbs_history_counter_t*)G_HISTORY_CONTAINER[i].data;
#endif
#ifdef POWER_METER_DEVICE
	tbs_history_powermeter_t* his_dev = (tbs_history_powermeter_t*)G_HISTORY_CONTAINER[i].data;
#endif
			his_dev->data.index = G_HISTORY_CONTAINER[i].indx;
			G_HISTORY_CONTAINER[i].status_proc = 1;
			if (STORAGE_RET_OK == tbs_history_load(G_HISTORY_CONTAINER[i].data,DATA_HISTORY_SIZE)) {
				P_PRINTFHEX_A(FLA,G_HISTORY_CONTAINER[i].data,DATA_HISTORY_SIZE,"LOAD|[%d]%d:",his_dev->data.index,his_dev->timetamp);
				//Send to Master
				if (fl_adv_sendFIFO_add(tbs_history_create_pack(G_HISTORY_CONTAINER[i].data)) == -1) {
					G_HISTORY_CONTAINER[i].status_proc = 0;
				} else
					G_HISTORY_CONTAINER[i].status_proc = 1;
				//exit to step-one-step
				return;
			}else{
				ERR(FLA,"HIS idx-err:%d\r\n",G_HISTORY_CONTAINER[i].indx);
				P_PRINTFHEX_A(FLA,G_HISTORY_CONTAINER[i].data,DATA_HISTORY_SIZE,"HIS idx-err:");
			}
		}
	}
}
/*
void TBS_History_StoreToFlash(u8* _data_struct){
#ifdef COUNTER_DEVICE
	tbs_history_counter_t his_dev;

	memcpy((u8*)&his_dev,&_data_struct[6],DATA_HISTORY_SIZE);
	P_PRINTFHEX_A(FLA,his_dev,DATA_HISTORY_SIZE,"STORE|[%d]%d:",his_dev.data.index,his_dev.timetamp);
#endif
#ifdef POWER_METER_DEVICE
	u8 his_dev[DATA_HISTORY_SIZE];
	u8 data_packed[POWER_METER_BITSIZE];
	tbs_device_powermeter_t *pwmeter_data = (tbs_device_powermeter_t*) &_data_struct;
	tbs_pack_powermeter_data(pwmeter_data,data_packed);
	u8 indx_data = SIZEU8(pwmeter_data->mac);
	memcpy((u8*)&his_dev,&data_packed[indx_data],DATA_HISTORY_SIZE);
	P_PRINTFHEX_A(FLA,his_dev,DATA_HISTORY_SIZE,"STORE|[%d]%d:",pwmeter_data->data.index,pwmeter_data->timetamp);
#endif
	tbs_history_store((u8*)&his_dev,DATA_HISTORY_SIZE);
}
*/

void TBS_History_StoreToFlash(u8* _data_struct){
#ifdef COUNTER_DEVICE
	tbs_history_counter_t his_dev;
#endif
#ifdef POWER_METER_DEVICE
	tbs_history_powermeter_t his_dev;
#endif

	memcpy((u8*)&his_dev,&_data_struct[6],DATA_HISTORY_SIZE);

	tbs_history_store((u8*)&his_dev,DATA_HISTORY_SIZE);

	P_PRINTFHEX_A(FLA,his_dev,DATA_HISTORY_SIZE,"STORE|[%d]%d:",his_dev.data.index,his_dev.timetamp);
}


void TBS_History_ClearAll(void){
	tbs_history_cleanAll();
}
/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/
void TBS_History_Init(void){
	//FOR TESTING
//	nvm_erase();
//	while(1);
	//clear G_HISTORY
	_CLEAR_G_HISTORY();
//	TBS_history_createSample();
	tbs_history_flash_init();
}

void TBS_History_Proc(void){
	TBS_History_LoadFromFlash();
}
#endif
