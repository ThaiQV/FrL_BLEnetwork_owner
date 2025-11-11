/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: TBS_dev_config.h
 *Created on		: Aug 22, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#ifndef VENDOR_FRL_NETWORK_TBS_DEV_CONFIG_H_
#define VENDOR_FRL_NETWORK_TBS_DEV_CONFIG_H_

typedef enum{
	TBS_COUNTER = 0x00,
	TBS_POWERMETER=0x01
}__attribute__((__packed__)) tbs_dev_type_e;

#ifndef MASTER_CORE
//#define HW_SAMPLE_TEST
//#define BLOCK_MASTER
#define COUNTER_DEVICE
#define COUNTER_LCD_MESS_MAX		10
#ifndef COUNTER_DEVICE
#define POWER_METER_DEVICE
void TBS_PowerMeter_RESETbyMaster(u8 _ch1,u8 _ch2,u8 _ch3);
void TBS_PwMeter_SetThreshod(u16 _chn1,u16 _chn2,u16 _chn3);
#endif
#endif
void tbs_counter_printf(type_debug_t _plog_type,void* _p);
void tbs_power_meter_printf(type_debug_t _plog_type,void* _p);
void TBS_Device_Init(void);
void TBS_Device_Run(void);
void TBS_Device_Index_manage(void);
void TBS_history_createSample(void);
void TBS_History_StoreToFlash(u8* _data_struct);
void TBS_History_ClearAll(void);
s8 TBS_History_Get(u16 _from, u16 _to) ;
void TBS_History_Init(void);
void TBS_History_Proc(void);
#endif /* VENDOR_FRL_NETWORK_TBS_DEV_CONFIG_H_ */
