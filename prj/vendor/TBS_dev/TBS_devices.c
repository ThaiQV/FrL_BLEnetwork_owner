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
#include "../FrL_Network/fl_nwk_api.h"
/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/

void tbs_counter_printf(void* _p){
	tbs_device_counter_t *data = (tbs_device_counter_t*)_p;
	LOGA(INF,"MAC:0x%02X%02X%02X%02X%02X%02X\r\n",data->data.mac[0],data->data.mac[1],data->data.mac[2],
			data->data.mac[3],data->data.mac[4],data->data.mac[5]);
	LOGA(INF,"Timetamp  :%d\r\n",data->data.timetamp);
	LOGA(INF,"Type      :%d\r\n",data->data.type);
	LOGA(INF,"BT_Call   :%d\r\n",data->data.bt_call);
	LOGA(INF,"BT_EndCall:%d\r\n",data->data.bt_endcall);
	LOGA(INF,"BT_Rst    :%d\r\n",data->data.bt_rst);
	LOGA(INF,"BT_Pass   :%d\r\n",data->data.pass_product);
	LOGA(INF,"BT_Err    :%d\r\n",data->data.err_product);
	LOGA(INF,"Mode      :%d\r\n",data->data.mode);
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
												.err_product = 100,
												.mode = 1
												}
									};
//use to store display message
u8 G_COUNTER_LCD[COUNTER_LCD_MESS_MAX][22];
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
void Counter_LCD_Print(void){
	static u32 crc32 = 0;
	u32 crc32_curr = fl_db_crc32((u8*)G_COUNTER_LCD,SIZEU8(G_COUNTER_LCD[0])*COUNTER_LCD_MESS_MAX);
	if (crc32 != crc32_curr) {
		P_INFO("========================\r\n");
		for (u8 i = 0; i < COUNTER_LCD_MESS_MAX; i++) {
			if (G_COUNTER_LCD[i][0] != 0)
				P_INFO("[%d]%s\r\n",i,(char* )G_COUNTER_LCD[i]);
		}
		P_INFO("========================\r\n");
		crc32=crc32_curr;
	}
}

/* TEST CASE  EVENT */
typedef struct {
	u32 lifetime;
	u16 req_num;
	u16 rsp_num;
	u32 rtt;
} test_sendevent_t;

test_sendevent_t TEST_EVENT ={0,0,0};

void TEST_rsp_callback(void *_data,void* _data2){
	fl_rsp_container_t *data =  (fl_rsp_container_t*)_data;
	LOGA(API,"Timeout:%d\r\n",data->timeout);
	LOGA(API,"cmdID  :%0X\r\n",data->rsp_check.hdr_cmdid);
	LOGA(API,"SlaveID:%0X\r\n",data->rsp_check.slaveID);
	//rsp data
	if(data->timeout > 0){
		fl_pack_t *packet = (fl_pack_t *)_data2;
		P_PRINTFHEX_A(API,packet->data_arr,packet->length,"RSP: ");
//		P_INFO("Master RSP:%c%c\r\n",packet->data_arr[7],packet->data_arr[8]);
		TEST_EVENT.rsp_num++;
	}else{
//		P_INFO("Master RSP: NON-RSP \r\n");
	}
	u32 lifetime = (fl_rtc_get() - TEST_EVENT.lifetime);
	P_INFO("==============================\r\n");
	P_INFO("* LifeTime:%dh%dm%ds\r\n",lifetime / 3600,(lifetime % 3600) / 60,lifetime % 60);
	P_INFO("* RTT     :%d ms\r\n",data->timeout>0?((clock_time()-TEST_EVENT.rtt)/SYSTEM_TIMER_TICK_1MS):0);
	P_INFO("* REQ/RSP :%d/%d\r\n",TEST_EVENT.req_num,TEST_EVENT.rsp_num);
	P_INFO("* LOSS    :%d\r\n",abs(TEST_EVENT.req_num-TEST_EVENT.rsp_num));
	P_INFO("==============================\r\n");
}

int TEST_Counter_Event(void){
	extern u8 GETINFO_FLAG_EVENTTEST; // for testing
	u32 period = RAND(1,30);
	if (IsJoinedNetwork() && IsOnline() && GETINFO_FLAG_EVENTTEST ==1) {
		G_COUNTER_DEV.data.bt_call = RAND(0,1);
		G_COUNTER_DEV.data.bt_endcall = G_COUNTER_DEV.data.bt_call ? 0 : 1;
		G_COUNTER_DEV.data.bt_rst = G_COUNTER_DEV.data.bt_call ? 0 : RAND(0,1);
		fl_api_slave_req(NWK_HDR_55,G_COUNTER_DEV.bytes,SIZEU8(G_COUNTER_DEV.bytes),&TEST_rsp_callback,0,1);
		TEST_EVENT.rtt = clock_time();
		TEST_EVENT.req_num++;
		P_INFO("TEST EVNET after:%d s\r\n",period);
	}
	return period*1000*1000;
}
/* END TEST*/

void TBS_Counter_init(void){
	memcpy(G_COUNTER_DEV.data.mac,blc_ll_get_macAddrPublic(),SIZEU8(G_COUNTER_DEV.data.mac));
	G_COUNTER_DEV.data.type = TBS_COUNTER;
	G_COUNTER_DEV.data.timetamp = fl_rtc_get();
	//passing lcd message
	for (u8 var = 0; var < COUNTER_LCD_MESS_MAX; ++var) {
		memset(G_COUNTER_LCD[var],0,SIZEU8(G_COUNTER_LCD[var]));
	}
	//todo:Init Butt,lcd,7segs,.....
	//TEST_EVENT.lifetime = fl_rtc_get();
	//blt_soft_timer_add(&TEST_Counter_Event,5000*1000);
}
void TBS_Counter_Run(void){
	G_COUNTER_DEV.data.timetamp = fl_rtc_get();
	//For testing : randon valid of fields
//	G_COUNTER_DEV.data.bt_call = RAND(0,1);
//	G_COUNTER_DEV.data.bt_endcall = G_COUNTER_DEV.data.bt_call?0:1;
//	G_COUNTER_DEV.data.bt_rst = RAND(0,1);
//	G_COUNTER_DEV.data.pass_product = RAND(1,1020);
//	G_COUNTER_DEV.data.err_product = RAND(1,500);
//	G_COUNTER_DEV.data.mode = 1;
	Counter_LCD_Print();
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
