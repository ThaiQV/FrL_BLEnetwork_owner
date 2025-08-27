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
	P_PRINTFHEX_A(INF,data->bytes,SIZEU8(data->bytes),"Raw:");
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
				        .timestamp = 12345678,
						.type = TBS_POWERMETER,
				        .data= {
				        		.frequency = 100,
								.voltage = 300,
								.current1 = 512,
								.current2 = 513,
								.current3 = 514,
								.power1 = 1000,
								.power2 = 1001,
								.power3 = 1002,
								.energy1 = 123456,
								.energy2 = 654321,
								.energy3 = 111111,
		//				        .reserve = 0xABCD
						}
				    };
void test_powermeter(void) {
	u8 buffer[POWER_METER_SIZE];
	memset(buffer,0,POWER_METER_SIZE);
	pack_powermeter_data(&meter, buffer);
//P_PRINTFHEX_A(MCU,buffer,34,"PACK(%d):",SIZEU8(buffer));
	fl_ble_send_wifi(buffer,POWER_METER_PACK_SIZE);
	tbs_device_powermeter_t received;
	unpack_powermeter_data(&received, buffer);
	LOGA(MCU,"MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
			received.mac[0], received.mac[1], received.mac[2],
			received.mac[3], received.mac[4], received.mac[5]);
	LOGA(MCU, "Timestamp: %u\n", received.timestamp);
	LOGA(MCU, "Frequency: %u\n", received.frequency);
	LOGA(MCU, "Voltage: %u\n", received.voltage);
	LOGA(MCU, "Current1: %u\n", received.current1);
	LOGA(MCU, "Current2: %u\n", received.current2);
	LOGA(MCU, "Current3: %u\n", received.current3);
	LOGA(MCU, "Power1: %u\n", received.power1);
	LOGA(MCU, "Power2: %u\n", received.power2);
	LOGA(MCU, "Power3: %u\n", received.power3);
	LOGA(MCU, "Energy1: %u\n", received.energy1);
	LOGA(MCU, "Energy2: %u\n", received.energy2);
	LOGA(MCU, "Energy3: %u\n", received.energy3);
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
	G_POWER_METER. = TBS_POWERMETER;
	G_POWER_METER.timetamp = fl_rtc_get();
	//todo:Init Butt,lcd,7segs,.....
}
void TBS_PowerMeter_Run(void){
	G_COUNTER_DEV.data.timetamp = fl_rtc_get();
	//For testing : randon valid of fields
	G_COUNTER_DEV.data.bt_call = RAND(0,1);
	G_COUNTER_DEV.data.bt_endcall = G_COUNTER_DEV.data.bt_call?0:1;
	G_COUNTER_DEV.data.bt_rst = RAND(0,1);
	G_COUNTER_DEV.data.pass_product = RAND(1,1020);
	G_COUNTER_DEV.data.err_product = RAND(1,500);
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
	TBS_Counter_init();
#endif
#ifdef POWER_METER_DEVICE
	TBS_PowerMeter_init();
#endif
}
void TBS_Device_Run(void){
#ifdef COUNTER_DEVICE
	TBS_Counter_Run();
#endif
#ifdef POWER_METER_DEVICE
	TBS_PowerMeter_Run();
#endif
}
