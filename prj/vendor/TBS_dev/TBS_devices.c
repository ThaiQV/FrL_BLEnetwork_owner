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

#define TBS_DEVICE_STORE_INTERVAL 		2*1010*1001 //5s
#define TBS_PACKET_INDEX_MAX			12288
#include "TBS_dev_app/user_lib.h"
/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
void tbs_counter_printf(type_debug_t _plog_type,void* _p){
	tbs_device_counter_t *data = (tbs_device_counter_t*)_p;
	LOGA(_plog_type,"COUNTER STRUCT SIZE :%d/%d\r\n",SIZEU8(tbs_device_counter_t),SIZEU8(data->data));
	LOGA(_plog_type,"MAC:0x%02X%02X%02X%02X%02X%02X\r\n",data->mac[0],data->mac[1],data->mac[2],data->mac[3],data->mac[4],data->mac[5]);
	LOGA(_plog_type,"Timetamp    :%d\r\n",data->timetamp);
	LOGA(_plog_type,"Type        :%d\r\n",data->type);
	LOGA(_plog_type,"Index       :%d\r\n",data->data.index);
	LOGA(_plog_type,"BT_Call     :%d\r\n",data->data.bt_call);
	LOGA(_plog_type,"BT_EndCall  :%d\r\n",data->data.bt_endcall);
	LOGA(_plog_type,"BT_Rst      :%d\r\n",data->data.bt_rst);
	LOGA(_plog_type,"BT_Pass     :%d\r\n",data->data.pass_product);
	LOGA(_plog_type,"BT_Err      :%d\r\n",data->data.err_product);
	LOGA(_plog_type,"Mode        :%d\r\n",data->data.mode);
	LOGA(_plog_type,"pre_pass    :%d\r\n",data->data.pre_pass_product);
	LOGA(_plog_type,"pre_err     :%d\r\n",data->data.pre_err_product);
	LOGA(_plog_type,"pre_mode    :%d\r\n",data->data.pre_mode);
	LOGA(_plog_type,"pre_timetamp:%d\r\n",data->data.pre_timetamp);
}

void tbs_power_meter_printf(type_debug_t _plog_type,void* _p) {
	tbs_device_powermeter_t* dev = (tbs_device_powermeter_t*)_p;
	LOGA(_plog_type,"POWERMETER STRUCT SIZE :%d/%d\r\n",SIZEU8(tbs_device_powermeter_t),SIZEU8(dev->data));
	LOGA(_plog_type,"MAC       :0x%02X%02X%02X%02X%02X%02X\r\n",dev->mac[0],dev->mac[1],dev->mac[2],dev->mac[3],dev->mac[4],dev->mac[5]);
	LOGA(_plog_type,"Timetamp  :%d\r\n",dev->timetamp);
	LOGA(_plog_type,"Type      :%d\r\n",dev->type);
	LOGA(_plog_type,"Index     :%d\r\n",dev->data.index);
	LOGA(_plog_type,"Frequency :%u\r\n",dev->data.frequency);
	LOGA(_plog_type,"Voltage   :%u\r\n",dev->data.voltage);
	LOGA(_plog_type,"Current1  :%u\r\n",dev->data.current1);
	LOGA(_plog_type,"Current2  :%u\r\n",dev->data.current2);
	LOGA(_plog_type,"Current3  :%u\r\n",dev->data.current3);
	LOGA(_plog_type,"Power1    :%u\r\n",dev->data.power1);
	LOGA(_plog_type,"Power2    :%u\r\n",dev->data.power2);
	LOGA(_plog_type,"Power3    :%u\r\n",dev->data.power3);
	LOGA(_plog_type,"Time1     :%u\r\n",dev->data.time1);
	LOGA(_plog_type,"Time2     :%u\r\n",dev->data.time2);
	LOGA(_plog_type,"Time3     :%u\r\n",dev->data.time3);
	LOGA(_plog_type,"Energy1   :%u\r\n",dev->data.energy1);
	LOGA(_plog_type,"Energy2   :%u\r\n",dev->data.energy2);
	LOGA(_plog_type,"Energy3   :%u\r\n",dev->data.energy3);
}

#ifndef MASTER_CORE

#ifdef COUNTER_DEVICE
tbs_device_counter_t G_COUNTER_DEV = {  .timetamp = 0,
										.type = TBS_COUNTER,
										.data = {
												.index=0,
												.bt_call = 0,
												.bt_endcall = 0,
												.bt_rst = 0,
												.pass_product = 0,
												.err_product = 0,
												.mode = 0,
												.pre_pass_product = 0,
												.pre_err_product=0,
												.pre_mode =0,
												.pre_timetamp =0
												}
									};

#define G_TBS_DEVICE		G_COUNTER_DEV

u8 G_COUNTER_LCD[COUNTER_LCD_MESS_MAX][LCD_MESSAGE_SIZE];
#define G_TBS_DEVICE		G_COUNTER_DEV
#endif
#ifdef POWER_METER_DEVICE
tbs_device_powermeter_t G_POWER_METER = {
				        .mac = {0, 0, 0, 0, 0, 0},
				        .timetamp = 0,
						.type = TBS_POWERMETER,
				        .data= {
				        		.index = 102,
				        		.frequency = 50,
								.voltage = 220,
								.current1 = 11,
								.current2 = 22,
								.current3 = 33,
								.power1 = 220,
								.power2 = 221,
								.power3 = 222,
								.time1 = 51,
								.time2 = 52,
								.time3 = 53,
								.energy1 = 111111,
								.energy2 = 222222,
								.energy3 = 333333,
						}
				    };

#define G_TBS_DEVICE		G_POWER_METER
//use to store setting parameter
u16 G_POWER_METER_PARAMETER[3];

void test_powermeter(void) {
	u8 buffer[POWER_METER_BITSIZE];
	memset(buffer,0,POWER_METER_BITSIZE);
	tbs_pack_powermeter_data(&G_POWER_METER, buffer);
//P_PRINTFHEX_A(MCU,buffer,34,"PACK(%d):",SIZEU8(buffer));
	tbs_device_powermeter_t received;
	tbs_unpack_powermeter_data(&received, buffer);
	tbs_power_meter_printf(APP,(void*)&received);
}
#endif

/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
#ifdef COUNTER_DEVICE
void Counter_LCD_MessageStore(void){
	static u32 crc32 = 0;
	u32 crc32_curr = fl_db_crc32((u8*)G_COUNTER_LCD,SIZEU8(G_COUNTER_LCD[0])*COUNTER_LCD_MESS_MAX);
	if (crc32 != crc32_curr) {
		LOGA(PERI,"========================\r\n");
		for (u8 i = 0; i < COUNTER_LCD_MESS_MAX; i++) {
			if (G_COUNTER_LCD[i][0] != 0xFF)
				LOGA(PERI,"0x%02X[%d]%s\r\n",G_COUNTER_LCD[i][LCD_MESSAGE_SIZE-1],i,(char* )G_COUNTER_LCD[i]);
		}
		LOGA(PERI,"========================\r\n");
		crc32=crc32_curr;
		//Store message
		fl_db_slavesettings_save((u8*)G_COUNTER_LCD,SIZEU8(G_COUNTER_LCD[0])*COUNTER_LCD_MESS_MAX);
	}
}

void Counter_LCD_MessageCheck_FlagNew(void){
	for (u8 var = 0; var < COUNTER_LCD_MESS_MAX; ++var) {
		tbs_counter_lcd_t *mess_lcd = (tbs_counter_lcd_t *)&G_COUNTER_LCD[var][0];
		if(mess_lcd->f_new == 1){
			P_INFO("[%d]%s\r\n",var,(char* )mess_lcd->mess);
			mess_lcd->f_new=0;
		}
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
		G_COUNTER_DEV.data.pre_pass_product = TEST_EVENT.rsp_num;
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
	u32 period = RAND(1,60);
	if (IsOnline() && GETINFO_FLAG_EVENTTEST ==1) {
		if (TEST_EVENT.req_num == 0) {
			G_COUNTER_DEV.data.pre_err_product = G_COUNTER_DEV.data.index;
		}
		//
		G_COUNTER_DEV.data.bt_call = RAND(0,1);
		G_COUNTER_DEV.data.bt_endcall = G_COUNTER_DEV.data.bt_call ? 0 : 1;
		G_COUNTER_DEV.data.bt_rst = 0;
		fl_api_slave_req(NWK_HDR_55,(u8*)&G_COUNTER_DEV.data,SIZEU8(G_COUNTER_DEV.data),&TEST_rsp_callback,0,1);
		TEST_EVENT.rtt = clock_time();
		TEST_EVENT.req_num++;
		G_COUNTER_DEV.data.pass_product = TEST_EVENT.req_num;
		P_INFO("TEST EVNET after:%d s\r\n",period);
	}
	return period*1000*1000;
}
/* END TEST*/

void TBS_Counter_init(void){
	memcpy(G_COUNTER_DEV.mac,blc_ll_get_macAddrPublic(),SIZEU8(G_COUNTER_DEV.mac));
	G_COUNTER_DEV.type = TBS_COUNTER;
	G_COUNTER_DEV.timetamp = fl_rtc_get();
	//passing lcd message
	memcpy(G_COUNTER_LCD,fl_db_slavesettings_init().setting_arr,SIZEU8(G_COUNTER_LCD[0]) * COUNTER_LCD_MESS_MAX);
//	//for debuging
//	LOG_P(PERI,"========================\r\n");
//	for (u8 i = 0; i < COUNTER_LCD_MESS_MAX; i++) {
//		if (G_COUNTER_LCD[i][0] != 0xFF){
////			sprintf(str_mess,"[%d]%s",i,G_COUNTER_LCD[i]);
//			P_INFO("[%d]%s-%d\r\n",i,(char* )G_COUNTER_LCD[i],G_COUNTER_LCD[i][LCD_MESSAGE_SIZE-1]);
//		}
//	}
//	LOG_P(PERI,"========================\r\n");
#ifndef HW_SAMPLE_TEST
	//todo:Init Butt,lcd,7segs,.....
	user_app_init();
#endif
	//TEst
	TEST_EVENT.lifetime = fl_rtc_get();
	blt_soft_timer_add(&TEST_Counter_Event,5000*1000);

}

void TBS_Counter_Run(void){
	memcpy(G_COUNTER_DEV.mac,blc_ll_get_macAddrPublic(),SIZEU8(G_COUNTER_DEV.mac));
	G_COUNTER_DEV.timetamp = fl_rtc_get();
	Counter_LCD_MessageStore();
//	Counter_LCD_MessageCheck_FlagNew();
	//todo: TBS_Device_App
#ifndef HW_SAMPLE_TEST
	user_app_loop();
#endif
}
#endif
#ifdef POWER_METER_DEVICE
void TBS_PowerMeter_init(void){
	//init db settings
	fl_db_slavesettings_init();
	//Load settings
	u8 settings[6];
	memcpy(settings,fl_db_slavesettings_load().setting_arr,SIZEU8(settings));
	G_POWER_METER_PARAMETER[0] = MAKE_U16(settings[1],settings[0]);
	G_POWER_METER_PARAMETER[1] = MAKE_U16(settings[3],settings[2]);
	G_POWER_METER_PARAMETER[2] = MAKE_U16(settings[5],settings[4]);

	LOGA(PERI,"Threshold channel:%d-%d-%d\r\n",G_POWER_METER_PARAMETER[0],	G_POWER_METER_PARAMETER[1],	G_POWER_METER_PARAMETER[2]);

//	memcpy(G_POWER_METER.mac,blc_ll_get_macAddrPublic(),SIZEU8(G_POWER_METER.mac));
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

void TBS_Device_Flash_Init_n_Reload(void){
	LOGA(FLA,"TBS_Device flash init and reload  !! \r\n");
	fl_db_userdata_t userdata = fl_db_slaveuserdata_init();
	memcpy((u8*)&G_TBS_DEVICE.timetamp,userdata.payload,SIZEU8(G_TBS_DEVICE)-6);
}

/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/
#ifdef POWER_METER_DEVICE

void TBS_PowerMeter_RESETbyMaster(u8 _ch1,u8 _ch2,u8 _ch3){
	LOGA(PERI,"Master RESET PWMeter channel:%d-%d-%d\r\n",_ch1,_ch2,_ch3);
	//todo: RESET pwmeter struct
}

void TBS_PwMeter_SetThreshod(u16 _chn1,u16 _chn2,u16 _chn3){
	LOGA(PERI,"Master SET Threshold channel:%d-%d-%d\r\n",_chn1,_chn2,_chn3);
	G_POWER_METER_PARAMETER[0]=_chn1;
	G_POWER_METER_PARAMETER[1]=_chn2;
	G_POWER_METER_PARAMETER[2]=_chn3;
	//Store settings
	fl_db_slavesettings_save((u8*)G_POWER_METER_PARAMETER,SIZEU8(G_POWER_METER_PARAMETER));
	//For testing
//	u8 settings[6];
//	memcpy(settings,fl_db_slavesettings_load().setting_arr,SIZEU8(settings));
//	G_POWER_METER_PARAMETER[0] = MAKE_U16(settings[1],settings[0]);
//	G_POWER_METER_PARAMETER[1] = MAKE_U16(settings[3],settings[2]);
//	G_POWER_METER_PARAMETER[2] = MAKE_U16(settings[5],settings[4]);
//
//	ERR(PERI,"Threshold channel:%d-%d-%d\r\n",G_POWER_METER_PARAMETER[0],	G_POWER_METER_PARAMETER[1],	G_POWER_METER_PARAMETER[2]);

}

#endif
/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/
int TBS_Device_Store_run(void) {
	char *dev_str;
	u8 data_size=0;
#ifdef COUNTER_DEVICE
	u8 data[SIZEU8(tbs_device_counter_t)];
	dev_str = "Counter";
	memcpy(data,(u8*)&G_COUNTER_DEV,SIZEU8(data));
#else
	dev_str = "PWMeter";
	u8 data[SIZEU8(tbs_device_powermeter_t)];
	memcpy(data,(u8*)&G_POWER_METER,SIZEU8(data));
#endif
	data_size = SIZEU8(data) - 6;//skip mac
	static u32 crc_check_change = 0;
	u32 crc = fl_db_crc32(data+6+4,data_size); //skip mac + timestamp
	if (crc_check_change != crc) {
		LOGA(FLA,"%s store currently data !!\r\n",dev_str);
		if(G_TBS_DEVICE.type == TBS_COUNTER) tbs_counter_printf(FLA,(void*)&data);
		else tbs_power_meter_printf(FLA,(void*)&data);
		fl_db_slaveuserdata_save(data+6,data_size);
		crc_check_change = crc;
	}
	return TBS_DEVICE_STORE_INTERVAL;
}

void TBS_Device_Index_manage(void) {
//	ERR(FLA,"0x%02X callback (indx:%d)!!\r\n",_cmdID,G_TBS_DEVICE.data.index);
#ifndef HW_SAMPLE_TEST
	u16 CHECK_ERR=0;
	CHECK_ERR = G_TBS_DEVICE.data.index;
	P_INFO("Before:%d\r\n",G_TBS_DEVICE.data.index);
	//todo:store to flash
	TBS_History_StoreToFlash((u8*) &G_TBS_DEVICE);
	P_INFO("After:%d\r\n",G_TBS_DEVICE.data.index);
#endif
	G_TBS_DEVICE.data.index++;
	TBS_Device_Store_run();
	P_INFO("Current:%d\r\n",G_TBS_DEVICE.data.index);
	if(CHECK_ERR == G_TBS_DEVICE.data.index){
		ERR(PERI,"ERR Index.....\r\n");
	}
	if (G_TBS_DEVICE.data.index >= TBS_PACKET_INDEX_MAX) {
		G_TBS_DEVICE.data.index = 0;
	}
	//ERR(FLA,"0x%02X callback (indx:%d)!!\r\n",_cmdID,G_TBS_DEVICE.data.index);
}

void TBS_Device_Init(void){
	TBS_Device_Flash_Init_n_Reload();
#ifdef COUNTER_DEVICE
	TBS_Counter_init();
#endif
#ifdef POWER_METER_DEVICE
	TBS_PowerMeter_init();
#endif
	if(G_TBS_DEVICE.type == TBS_COUNTER) tbs_counter_printf(FLA,(void*)&G_TBS_DEVICE);
	else tbs_power_meter_printf(FLA,(void*)&G_TBS_DEVICE);
#ifndef HW_SAMPLE_TEST
	//History init
	TBS_History_Init();
#endif
	blt_soft_timer_add(TBS_Device_Store_run,TBS_DEVICE_STORE_INTERVAL);
}

void TBS_Device_Run(void){
#ifdef COUNTER_DEVICE
	TBS_Counter_Run();
#endif
#ifdef POWER_METER_DEVICE
	TBS_PowerMeter_Run();
#endif
#ifndef HW_SAMPLE_TEST
	TBS_History_Proc();
#endif
}
#endif
