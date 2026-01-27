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

#ifdef POWER_METER_DEVICE
#include "TBS_power_meter_app/power_meter_app.h"
#endif

#define TBS_DEVICE_STORE_INTERVAL 		2*1010*1001 //5s
#define TBS_PACKET_INDEX_MAX			12288
#include "TBS_dev_app/user_lib.h"

#define COUNTER_LCD_REMOVE_DISPLAY		ct_remove_nwwk
#define COUNTER_LCD_PRESS_DISPLAY		ct_add_bt_print

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
	LOGA(_plog_type,"Current1  :%u (%s)\r\n",dev->data.current1 ,((dev->data.power1 >> 7)& 0x1) > 0?"A":"mA");
	LOGA(_plog_type,"Current2  :%u (%s)\r\n",dev->data.current2 ,((dev->data.power2 >> 7)& 0x1) > 0?"A":"mA");
	LOGA(_plog_type,"Current3  :%u (%s)\r\n",dev->data.current3 ,((dev->data.power3 >> 7)& 0x1) > 0?"A":"mA");
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

u8 G_COUNTER_LCD[COUNTER_LCD_MESS_MAX][LCD_MESSAGE_SIZE];
#define G_TBS_DEVICE		G_COUNTER_DEV
#endif
#ifdef POWER_METER_DEVICE

#define LED_SIGNNAL_PIN_INIT(pin)				do{													\
												gpio_function_en(pin);								\
												gpio_set_output(pin, 1);							\
												gpio_set_up_down_res(pin, GPIO_PIN_PULLUP_1M);		\
												gpio_set_high_level(pin);							\
											}while(0)
#define LED_SIGNAL_ONOFF(pin,x)				{(x!=1)?gpio_set_high_level(pin):gpio_set_low_level(pin);}

#define LED_PAIR_PIN_INIT()					LED_SIGNNAL_PIN_INIT(GPIO_PA6)
#define LED_PAIR_ONOFF(on)					LED_SIGNAL_ONOFF(GPIO_PA6,on)

#define LED_NETWORK_PIN_INIT()				LED_SIGNNAL_PIN_INIT(GPIO_PA5)
#define LED_NETWORK_ONOFF(on)				LED_SIGNAL_ONOFF(GPIO_PA5,on)

#define BUTTON_PIN_INIT(pin)				do{														\
												gpio_function_en(pin);								\
												gpio_set_output(pin, 0);							\
												gpio_set_input(pin, 1);								\
											}while(0)

#define BUTTON_CONFIG_INIT()				BUTTON_PIN_INIT(GPIO_PB0)
#define BUTTON_CONFIG_STATE					gpio_read(GPIO_PB0)

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
u16 G_POWER_METER_PARAMETER[4];
#define PW_SAMPLE_PERIOD			G_POWER_METER_PARAMETER[3]

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
void Counter_LCD_RemoveDisplay(void){
	COUNTER_LCD_REMOVE_DISPLAY();
}
void Counter_LCD_PEU_Display(u8 _row,char* _mess){
	COUNTER_LCD_PRESS_DISPLAY(_mess,_row,BT_PEU_ID);
}
void Counter_LCD_PED_Display(u8 _row,char* _mess){
	COUNTER_LCD_PRESS_DISPLAY(_mess,_row,BT_PED_ID);
}
void Counter_LCD_PPD_Display(u8 _row,char* _mess){
	COUNTER_LCD_PRESS_DISPLAY(_mess,_row,BT_PPD_ID);

}
void Counter_LCD_ENDCALL_Display(u8 _row,char* _mess){
	COUNTER_LCD_PRESS_DISPLAY(_mess,_row,BT_ENDCALL_ID);
}
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
//	extern u8 GETINFO_FLAG_EVENTTEST; // for testing
	u32 period = RAND(1,60);
//	if (IsOnline() && GETINFO_FLAG_EVENTTEST ==1) {
//		if (TEST_EVENT.req_num == 0) {
//			G_COUNTER_DEV.data.pre_err_product = G_COUNTER_DEV.data.index;
//		}
//		//
//		G_COUNTER_DEV.data.bt_call = RAND(0,1);
//		G_COUNTER_DEV.data.bt_endcall = G_COUNTER_DEV.data.bt_call ? 0 : 1;
//		G_COUNTER_DEV.data.bt_rst = 0;
//		fl_api_slave_req(NWK_HDR_55,(u8*)&G_COUNTER_DEV.data,SIZEU8(G_COUNTER_DEV.data),&TEST_rsp_callback,0,1);
//		TEST_EVENT.rtt = clock_time();
//		TEST_EVENT.req_num++;
//		G_COUNTER_DEV.data.pass_product = TEST_EVENT.req_num;
//		P_INFO("TEST EVNET after:%d s\r\n",period);
//	}
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
void TBS_PowerMeter_RMS_Read(void);

void TBS_PowerMeter_Button_Exc(void){
#define PRESS_VALUE  			0
#define RELEASE_VALUE 			1
#define DEBOUCE_FILTER			30 //ms
#define FAST_PRESSnRELEASE		150 //ms
#define FACTORY_REBOOTnHOLD		5*1000 //s
#define PAIRING_HOLD			5*1000 //s
	// Flag Reboot
	static bool rst_flag = true;
	//get frequency callback
	static u32 lasttick = 0;
	u32 deltaT = (clock_time()-lasttick)/SYSTEM_TIMER_TICK_1MS;
	if (deltaT < DEBOUCE_FILTER) {
		//return;
	}
	lasttick = clock_time();
	// process timing button
	static u32 press_time = 0; //ms
	if(BUTTON_CONFIG_STATE == PRESS_VALUE){
		press_time += deltaT;
		//Excute features
		if (rst_flag && press_time >= FACTORY_REBOOTnHOLD) {
			ERR(APP,"Factory default......(%d,%d)\r\n",press_time,rst_flag);
			fl_db_clearAll();
			TBS_History_ClearAll();
			sys_reboot();
		} else {
			if (press_time >= PAIRING_HOLD) {
				if (!IsPairing()) {
					ERR(APP,"Pairing......(%d,%d)\r\n",press_time,rst_flag);
					fl_nwk_slave_nwkclear();
				}
				press_time = 0;
			}
		}
	}
	else if (BUTTON_CONFIG_STATE == RELEASE_VALUE) {
		//Clear Reboot flag
		rst_flag = false;
		//short press and release
		if (press_time < PAIRING_HOLD && press_time > FAST_PRESSnRELEASE) {
			//ERR(APP,"Fast Press(%d ms)...\r\n",press_time);
			TBS_PowerMeter_RMS_Read();
			//TEST
			extern void pmt_update_data_to_rp(void);
			pmt_update_data_to_rp();
			u8 _payload[SIZEU8(tbs_device_powermeter_t)];
			tbs_device_powermeter_t *pwmeter_data = (tbs_device_powermeter_t*) &G_POWER_METER;
			tbs_pack_powermeter_data(pwmeter_data,_payload);
			u8 indx_data = SIZEU8(pwmeter_data->type) + SIZEU8(pwmeter_data->mac) + SIZEU8(pwmeter_data->timetamp);
			if (IsJoinedNetwork()) {
				fl_api_slave_req(NWK_HDR_55,&_payload[indx_data],SIZEU8(pwmeter_data->data),0,0,1);
			}
		}
		//Reset time
		press_time=0;
	}

#undef PRESS_VALUE
#undef RELEASE_VALUE
#undef DEBOUCE_SCAN
#undef PAIRING_HOLD
#undef FACTORY_REBOOTnHOLD
}

void TBS_PowerMeter_RMS_Read(void) {

	float calib_U, calib_I, calib_P;
	for (u8 chn = 1; chn < 4; chn++) {
		pmt_getcalib(chn,&calib_U,&calib_I,&calib_P);
		P_INFO("[%d]Freq:%.2f,Urms(%.3f):%.3f,Irms(%.3f):%.3f,Prms(%.3f):%.3f\r\n",chn,pmt_read_F(chn),calib_U,pmt_read_U(chn),calib_I,pmt_read_I(chn),calib_P,pmt_read_P(chn));
	}
	P_INFO("==================================\r\n");
}

void TBS_PowerMeter_Upload2Master(void){
	extern void pmt_update_data_to_rp(void);
	TBS_PowerMeter_RMS_Read();
	LOG_P(APP,"Upload data to server....\r\n");
	pmt_update_data_to_rp();
}
void TBS_PowerMeter_TimerIRQ_handler(void) {
	//todo
//	gpio_toggle(GPIO_PA6);
//	gpio_toggle(GPIO_PA5);
	/*Clear......**/
	if (timer_get_irq_status(TMR_STA_TMR0)) {
		timer_clr_irq_status(TMR_STA_TMR0);
		timer_set_cap_tick(TIMER0,timer0_get_tick() + PW_SAMPLE_PERIOD* SYSTEM_TIMER_TICK_1MS);
	}
}

void TBS_PowerMeter_TimerIRQ_Init(u8 _period_ms) {
	if (PW_SAMPLE_PERIOD!= _period_ms) {
		PW_SAMPLE_PERIOD=_period_ms;
		P_INFO("Re-SamplePeriod:%d ms\r\n",PW_SAMPLE_PERIOD);
	}
	timer_set_mode(TIMER0,TIMER_MODE_SYSCLK);
	timer_set_init_tick(TIMER0,clock_time());
	P_INFO("TIMER INIT (%d)\r\n",timer0_get_tick());
	timer_set_cap_tick(TIMER0, timer0_get_tick() + PW_SAMPLE_PERIOD*SYSTEM_TIMER_TICK_1MS);
	plic_set_priority(IRQ4_TIMER0, IRQ_PRI_LEV3);
	plic_interrupt_enable(IRQ4_TIMER0);
	timer_start(TIMER0);
	core_interrupt_enable();
//tesst
	gpio_function_en(GPIO_PA6);
	gpio_set_output(GPIO_PA6,1);	//enable output
	gpio_set_input(GPIO_PA6,0);		//disable input
	gpio_set_level(GPIO_PA6,1);

	gpio_function_en(GPIO_PA5);
	gpio_set_output(GPIO_PA5,1);	//enable output
	gpio_set_input(GPIO_PA5,0);		//disable input
	gpio_set_level(GPIO_PA5,1);
}

void TBS_PowerMeter_init(void){
	//init db settings
	fl_db_slavesettings_init();
	//Load settings
	u8 settings[6];
	memcpy(settings,fl_db_slavesettings_load().setting_arr,SIZEU8(settings));
	G_POWER_METER_PARAMETER[0] = MAKE_U16(settings[1],settings[0]);
	G_POWER_METER_PARAMETER[1] = MAKE_U16(settings[3],settings[2]);
	G_POWER_METER_PARAMETER[2] = MAKE_U16(settings[5],settings[4]);

	P_INFO("Threshold channel:%d-%d-%d\r\n",G_POWER_METER_PARAMETER[0],	G_POWER_METER_PARAMETER[1],	G_POWER_METER_PARAMETER[2]);
	P_INFO("SamplePeriod:%d ms\r\n",PW_SAMPLE_PERIOD);
	memcpy(G_POWER_METER.mac,blc_ll_get_macAddrPublic(),SIZEU8(G_POWER_METER.mac));
	G_POWER_METER.type = TBS_POWERMETER;
	G_POWER_METER.timetamp= fl_rtc_get();
//	test_powermeter();
	//todo:Init Butt,lcd,7segs,.....
	for(int i= 0; i < 3; i++)
	{
		G_POWER_METER_PARAMETER[i] = (G_POWER_METER_PARAMETER[i] == 0) ? 5: G_POWER_METER_PARAMETER[i];
	}
	// fl_tbs_data_t tbs_load = fl_db_tbsprofile_load();
	// memcpy(G_POWER_METER_PARAMETER, &tbs_load.data[40], sizeof(G_POWER_METER_PARAMETER));
//	printf("G_POWER_METER_PARAMETER0: %d\n", G_POWER_METER_PARAMETER[0]);
//	printf("G_POWER_METER_PARAMETER1: %d\n", G_POWER_METER_PARAMETER[1]);
//	printf("G_POWER_METER_PARAMETER2: %d\n", G_POWER_METER_PARAMETER[2]);
//	printf("G_POWER_METER_PARAMETER3: %d\n", G_POWER_METER_PARAMETER[3]);

	power_meter_app_init();

	///Init LED SIGNAL & BUTTONS Excution
	LED_PAIR_PIN_INIT();
	LED_NETWORK_PIN_INIT();
	//Button config
	BUTTON_CONFIG_INIT();
	// TBS_PowerMeter_TimerIRQ_Init(100);
}
void TBS_PowerMeter_Run(void){
	memcpy(G_POWER_METER.mac,blc_ll_get_macAddrPublic(),SIZEU8(G_POWER_METER.mac));
	G_POWER_METER.timetamp = fl_rtc_get();
	//button excution
	TBS_PowerMeter_Button_Exc();
	//power meter app
	power_meter_app_loop();
	//status network
	LED_NETWORK_ONOFF(IsOnline());
	//status pairing mode
	LED_PAIR_ONOFF(IsPairing());
}
#endif
/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/

void TBS_Device_Flash_Init_n_Reload(void) {
	LOGA(FLA,"TBS_Device flash init and reload  !! \r\n");
	fl_db_userdata_t userdata = fl_db_slaveuserdata_init();
	memcpy((u8*) &G_TBS_DEVICE.timetamp,userdata.payload,SIZEU8(G_TBS_DEVICE) - 6);
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
	if(_ch1)pmt_clear_energy(0);
	if(_ch2)pmt_clear_energy(1);
	if(_ch3)pmt_clear_energy(2);
}

void TBS_PwMeter_SetThreshod(u16 _chn1,u16 _chn2,u16 _chn3){
	LOGA(PERI,"Master SET Threshold channel:%d-%d-%d\r\n",_chn1,_chn2,_chn3);
	G_POWER_METER_PARAMETER[0]=_chn1;
	G_POWER_METER_PARAMETER[1]=_chn2;
	G_POWER_METER_PARAMETER[2]=_chn3;
	printf("G_POWER_METER_PARAMETER0: %d\n", G_POWER_METER_PARAMETER[0]);
	printf("G_POWER_METER_PARAMETER1: %d\n", G_POWER_METER_PARAMETER[1]);
	printf("G_POWER_METER_PARAMETER2: %d\n", G_POWER_METER_PARAMETER[2]);
	printf("G_POWER_METER_PARAMETER3: %d\n", G_POWER_METER_PARAMETER[3]);
	fl_tbs_data_t tbs_load = fl_db_tbsprofile_load();
	memcpy(&tbs_load.data[40],  G_POWER_METER_PARAMETER, sizeof(G_POWER_METER_PARAMETER));
	//Store settings
	// fl_db_slavesettings_save((u8*)G_POWER_METER_PARAMETER,SIZEU8(G_POWER_METER_PARAMETER));
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
	u16 CHECK_ERR=0;
	CHECK_ERR = G_TBS_DEVICE.data.index;
#ifndef HW_SAMPLE_TEST
//	P_INFO("Before:%d\r\n",G_TBS_DEVICE.data.index);
	//todo:store to flash
	TBS_History_StoreToFlash((u8*) &G_TBS_DEVICE);
//	P_INFO("After:%d\r\n",G_TBS_DEVICE.data.index);
#endif
	G_TBS_DEVICE.data.index++;
//	P_INFO("Current:%d\r\n",G_TBS_DEVICE.data.index);
	if(CHECK_ERR == G_TBS_DEVICE.data.index){
		ERR(PERI,"ERR Index <Err-%d>\r\n",G_TBS_DEVICE.data.index);
	}
	if (G_TBS_DEVICE.data.index >= TBS_PACKET_INDEX_MAX) {
		G_TBS_DEVICE.data.index = 0;
	}
	TBS_Device_Store_run();
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
#ifndef HW_SAMPLE_TEST
	//History init
	TBS_History_Init();
#endif
	if(G_TBS_DEVICE.type == TBS_COUNTER) tbs_counter_printf(FLA,(void*)&G_TBS_DEVICE);
	else tbs_power_meter_printf(FLA,(void*)&G_TBS_DEVICE);
	blt_soft_timer_add(TBS_Device_Store_run,TBS_DEVICE_STORE_INTERVAL);
}

void TBS_Device_Run(void){
//	gpio_toggle(GPIO_PA5);
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
