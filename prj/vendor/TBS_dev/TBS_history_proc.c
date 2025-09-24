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
#ifndef MASTER_CORE
#ifdef COUNTER_DEVICE
#define DATA_HISTORY_SIZE	20 // 4 (timetamp) + 1 (type) + 15 (data)
#endif
#ifdef POWER_METER_DEVICE
#define DATA_HISTORY_SIZE	28 // 4 (timestamp) + 1 (type) + 2 (index) + 21 (packed data)
#endif
#define NUM_HISTORY			20
/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
typedef struct {
	u16 indx;
	u8 data[DATA_HISTORY_SIZE];
	u8 status_proc;
}__attribute__((packed)) tbs_history_t;

tbs_history_t G_HISTORY_CONTAINER[NUM_HISTORY];

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

void TBS_history_createSample(void) {
	u8 his_array[NUM_HISTORY][DATA_HISTORY_SIZE];
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
			memcpy(his_array[i],&temp_buffer[6],DATA_HISTORY_SIZE);
	//
	//		P_PRINTFHEX_A(INF_FILE,powermeter_array[i],RECORD_SIZE,"[%d]",i);
	//		tbs_device_powermeter_t received;
	//		tbs_unpack_powermeter_data(&received,temp_buffer);
	//		tbs_power_meter_printf((void*)&received);
		}
#endif
#ifdef COUNTER_DEVICE
	for (int i = 0; i < NUM_HISTORY; ++i) {
		tbs_device_counter_t record = {
				.timetamp = 1758639600 + i * 60, //
				.type = 0x00, //
				.data = {
						.index = i,
						.bt_call = 1,
						.bt_endcall = 0,
						.bt_rst = 0,
						.pass_product = 100 + i,
						.err_product = 5 + i,
						.mode = 1,
						.pre_pass_product = 90 + i,
						.pre_err_product = 3 + i,
						.pre_mode = 0 } };

		u8 *dst = his_array[i];
		int pos = 0;
		// Copy timetamp (4 bytes)
		memcpy(&dst[pos],&record.timetamp,sizeof(record.timetamp));
		pos += sizeof(record.timetamp);
		// Copy type (1 byte)
		dst[pos++] = record.type;
		// Copy data struct (15 bytes)
		memcpy(&dst[pos],&record.data,sizeof(record.data));
	}
#endif
	//store into global struct
	for (u8 var = 0; var < NUM_HISTORY; ++var) {
		G_HISTORY_CONTAINER[var].indx =  var;
		memcpy(G_HISTORY_CONTAINER[var].data,his_array[var],DATA_HISTORY_SIZE);
		P_PRINTFHEX_A(INF_FILE,G_HISTORY_CONTAINER[var].data,DATA_HISTORY_SIZE,"[%d]",var);
	}
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
#endif
