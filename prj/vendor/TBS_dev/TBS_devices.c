/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: TBS_devices.c
 *Created on		: Aug 25, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#include "stack/ble/ble.h"
#include "tl_common.h"
#include "TBS_dev_config.h"
#include "../FrL_Network/fl_nwk_handler.h"
#include "TBS_dev_app/user_lib.h"
/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
void tbs_counter_printf(void* _p){
	tbs_device_counter_t *data = (tbs_device_counter_t*)_p;
	LOGA(INF,"MAC:0x%02X%02X%02X%02X%02X%02X\r\n",data->data.mac[0],data->data.mac[1],data->data.mac[2],
			data->data.mac[3],data->data.mac[4],data->data.mac[5]);
	LOGA(INF,"Timetamp:%d\r\n",data->data.timetamp);
	LOGA(INF,"Type:%d\r\n",data->data.type);
	LOGA(INF,"BT_Call:%d\r\n",data->data.bt_call);
	LOGA(INF,"BT_EndCall:%d\r\n",data->data.bt_endcall);
	LOGA(INF,"BT_Rst:%d\r\n",data->data.bt_rst);
	LOGA(INF,"BT_Pass:%d\r\n",data->data.pass_product);
	LOGA(INF,"BT_Err:%d\r\n",data->data.err_product);
//	P_PRINTFHEX_A(INF,data->bytes,SIZEU8(data->bytes),"Raw:");
}

void tbs_power_meter_printf(void* _p) {
	tbs_device_powermeter_t* dev = (tbs_device_powermeter_t*) _p;
	LOGA(INF,"MAC:0x%02X%02X%02X%02X%02X%02X\r\n",dev->mac[0],dev->mac[1],dev->mac[2],dev->mac[3],dev->mac[4],dev->mac[5]);
	LOGA(INF,"Timetamp:%d\r\n",dev->timetamp);
	LOGA(INF,"Type:%d\r\n",dev->type);
	LOGA(INF,"Frequency: %u\n",dev->data.frequency);
	LOGA(INF,"Voltage: %u\n",dev->data.voltage);
	LOGA(INF,"Current1: %u\n",dev->data.current1);
	LOGA(INF,"Current2: %u\n",dev->data.current2);
	LOGA(INF,"Current3: %u\n",dev->data.current3);
	LOGA(INF,"Power1: %u\n",dev->data.power1);
	LOGA(INF,"Power2: %u\n",dev->data.power2);
	LOGA(INF,"Power3: %u\n",dev->data.power3);
	LOGA(INF,"Energy1: %u\n",dev->data.energy1);
	LOGA(INF,"Energy2: %u\n",dev->data.energy2);
	LOGA(INF,"Energy3: %u\n", dev->data.energy3);
//	u8* data_u8 = (u8*)_p;
//	P_PRINTFHEX_A(INF,data_u8,POWER_METER_STRUCT_BYTESIZE,"Raw:");
}

#ifdef COUNTER_DEVICE
tbs_device_counter_t G_COUNTER_DEV = { .data = {
												.timetamp = 0,
												.type = TBS_COUNTER,
												.bt_call = 0,
												.bt_endcall = 0,
												.bt_rst = 0,
												.pass_product = 100,
												.err_product = 5
												}
									};
#endif
#ifdef POWER_METER_DEVICE

tbs_device_powermeter_t G_POWER_METER = {
				        .mac = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01},
				        .timetamp = 12345678,
						.type = TBS_POWERMETER,
				        .data= {
				        		.frequency = 50,
								.voltage = 220,
								.current1 = 11,
								.current2 = 22,
								.current3 = 33,
								.power1 = 242,
								.power2 = 484,
								.power3 = 726,
								.energy1 = 111111,
								.energy2 = 222222,
								.energy3 = 333333,
		//				        .reserve = 0xABCD
						}
				    };
void test_powermeter(void) {
	u8 buffer[POWER_METER_BITSIZE];
	memset(buffer,0,POWER_METER_BITSIZE);
	tbs_pack_powermeter_data(&G_POWER_METER, buffer);
//P_PRINTFHEX_A(MCU,buffer,34,"PACK(%d):",SIZEU8(buffer));
	tbs_device_powermeter_t received;
	tbs_unpack_powermeter_data(&received, buffer);
	tbs_power_meter_printf((void*)&received);
}
#endif
/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
#ifdef COUNTER_DEVICE
void TBS_Counter_init(void){
	memcpy(G_COUNTER_DEV.data.mac,blc_ll_get_macAddrPublic(),SIZEU8(G_COUNTER_DEV.data.mac));
	G_COUNTER_DEV.data.type = TBS_COUNTER;
	G_COUNTER_DEV.data.timetamp = fl_rtc_get();
	//todo:Init Butt,lcd,7segs,.....

}
void TBS_Counter_Run(void){
	G_COUNTER_DEV.data.timetamp = fl_rtc_get();
	//For testing : randon valid of fields
	G_COUNTER_DEV.data.bt_call = RAND(0,1);
	G_COUNTER_DEV.data.bt_endcall = G_COUNTER_DEV.data.bt_call?0:1;
	G_COUNTER_DEV.data.bt_rst = RAND(0,1);
	G_COUNTER_DEV.data.pass_product = RAND(1,1020);
	G_COUNTER_DEV.data.err_product = RAND(1,500);
}
#endif
#ifdef POWER_METER_DEVICE
void TBS_PowerMeter_init(void){
	memcpy(G_POWER_METER.mac,blc_ll_get_macAddrPublic(),SIZEU8(G_POWER_METER.mac));
	G_POWER_METER.type = TBS_POWERMETER;
	G_POWER_METER.timetamp= fl_rtc_get();
	test_powermeter();
	//todo:Init Butt,lcd,7segs,.....

}
void TBS_PowerMeter_Run(void){
	G_POWER_METER.timetamp = fl_rtc_get();
	//For testing : randon valid of fields
//	G_POWER_METER.data.frequency = RAND(0,128);
//	G_POWER_METER.data.voltage = RAND(0,512);
//	G_POWER_METER.data.current1 = RAND(0,1024);
//	G_POWER_METER.data.current2 = RAND(0,1024);
//	G_POWER_METER.data.current3 = RAND(0,1024);
//	G_POWER_METER.data.power1 = RAND(0,16384);
//	G_POWER_METER.data.power2 = RAND(0,16384);
//	G_POWER_METER.data.power3 = RAND(0,16384);
//	G_POWER_METER.data.energy1 = RAND(0,16777216);
//	G_POWER_METER.data.energy2 = RAND(0,16777216);
//	G_POWER_METER.data.energy3 = RAND(0,16777216);
}
#endif
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
void TBS_Device_Init(void){
#ifdef COUNTER_DEVICE
	user_app_init();
#endif
#ifdef POWER_METER_DEVICE
	TBS_PowerMeter_init();
#endif
}
void TBS_Device_Run(void){
#ifdef COUNTER_DEVICE
	user_app_loop();
#endif
#ifdef POWER_METER_DEVICE
	TBS_PowerMeter_Run();
#endif
}
