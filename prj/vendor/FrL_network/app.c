/********************************************************************************************************
 * @file     app.c
 *
 * @brief    This is the source file for BLE SDK
 *
 * @author	 BLE GROUP
 * @date         06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app_config.h"
#include "app.h"
#include "app_ui.h"
//#include "app_att.h"
#include "app_buffer.h"
#include "application/keyboard/keyboard.h"
#include "battery_check.h"

#include "fl_adv_proc.h"
#include "fl_input_ext.h"

#define SYNCHRONIZE_SYSTIME			5000*1000 //5s

_attribute_data_retention_ own_addr_type_t app_own_address_type = OWN_ADDRESS_PUBLIC;

/***************************************************
 * @brief 		: synchronize system time
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
int app_system_time_sync(void) {
	//SYNCHRONIZATION TIME
	datetime_t cur_dt;
	u32 cur_timetamp = fl_rtc_get();
	fl_rtc_timestamp_to_datetime(cur_timetamp,&cur_dt);
	LOGA(APP,"SYSTIME:%02d/%02d/%02d - %02d:%02d:%02d\r\n",cur_dt.year,cur_dt.month,cur_dt.day,cur_dt.hour,cur_dt.minute,cur_dt.second);
	fl_rtc_set(0);
	return 0;
}

/**
 * @brief		user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]	none
 * @return      none
 */
_attribute_no_inline_ void user_init_normal(void) {
	/* random number generator must be initiated here( in the beginning of user_init_nromal).
	 * When deepSleep retention wakeUp, no need initialize again */
	random_generator_init();  //this is must

//////////////////////////// BLE stack Initialization  Begin //////////////////////////////////
	/* for 1M   Flash, flash_sector_mac_address equals to 0xFF000
	 * for 2M   Flash, flash_sector_mac_address equals to 0x1FF000*/
	u8 mac_public[6];
	u8 mac_random_static[6];
	blc_initMacAddress(flash_sector_mac_address,mac_public,mac_random_static);

	P_PRINTFHEX_A(APP,mac_public,6,"%s","MAC:");
#if(BLE_DEVICE_ADDRESS_TYPE == BLE_DEVICE_ADDRESS_PUBLIC)
	app_own_address_type = OWN_ADDRESS_PUBLIC;
#elif(BLE_DEVICE_ADDRESS_TYPE == BLE_DEVICE_ADDRESS_RANDOM_STATIC)
	app_own_address_type = OWN_ADDRESS_RANDOM;
	blc_ll_setRandomAddr(mac_random_static);
#endif

	//////////// Controller Initialization  Begin /////////////////////////
	blc_ll_initBasicMCU();                      //mandatory
	blc_ll_initStandby_module(mac_public);		//mandatory
	blc_ll_initLegacyAdvertising_module(); 		//legacy advertising module: mandatory for BLE slave
//	blc_ll_initConnection_module();				//connection module  mandatory for BLE slave/master
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,

	blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS,ACL_CONN_MAX_TX_OCTETS);

	blc_ll_initAclConnTxFifo(app_acl_txfifo,ACL_TX_FIFO_SIZE,ACL_TX_FIFO_NUM);
	blc_ll_initAclConnRxFifo(app_acl_rxfifo,ACL_RX_FIFO_SIZE,ACL_RX_FIFO_NUM);

	u8 check_status = blc_controller_check_appBufferInitialization();
	if (check_status != BLE_SUCCESS) {
		/* here user should set some log to know which application buffer incorrect */
		write_log32(0x88880000 | check_status);
		while (1)
			;
	}
	//////////// Controller Initialization  End /////////////////////////

	//////////// Host Initialization  Begin /////////////////////////
	/* L2CAP Initialization */
	blc_l2cap_register_handler(blc_l2cap_packet_receive);

	/* SMP Initialization may involve flash write/erase(when one sector stores too much information,
	 *   is about to exceed the sector threshold, this sector must be erased, and all useful information
	 *   should re_stored) , so it must be done after battery check */
	//blc_smp_setParingMethods (LE_Secure_Connection);
#if (APP_SECURITY_ENABLE)
	blc_smp_peripheral_init();

	// Hid device on android7.0/7.1 or later version
	// New paring: send security_request immediately after connection complete
	// reConnect:  send security_request 1000mS after connection complete. If master start paring or encryption before 1000mS timeout, slave do not send security_request.
	blc_smp_configSecurityRequestSending(SecReq_IMM_SEND, SecReq_PEND_SEND, 1000);//if not set, default is:  send "security request" immediately after link layer connection established(regardless of new connection or reconnection)
#else
	blc_smp_setSecurityLevel(No_Security);
#endif
	//////////// Host Initialization  End /////////////////////////

	///////////////////// Power Management initialization///////////////////
	bls_pm_setSuspendMask(SUSPEND_DISABLE);

	//////////////////////////// BLE stack Initialization  End //////////////////////////////////

	///////////////////// freelux adv initialization///////////////////

	////////////////// config adv scan /////////////////////
	fl_adv_init();
	///////////////////// stimer Management initialization///////////////////
	//blc_ll_initPowerManagement_module();
	blt_soft_timer_init();
	///////////////////// TIME SYSTEM initialization///////////////////
	fl_rtc_init();

	blt_soft_timer_add(&app_system_time_sync,SYNCHRONIZE_SYSTIME);
	///////////////////// Serial initialization///////////////////
#ifdef MASTER_CORE
	fl_input_serial_init(UART1,UART1_TX_PE0,UART1_RX_PE2,115200);
#endif
}

/**
 * @brief		user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]	none
 * @return      none
 */
_attribute_ram_code_ void user_init_deepRetn(void) {

}
/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////

/**
 * @brief		This is main_loop function
 * @param[in]	none
 * @return      none
 */
_attribute_no_inline_ void main_loop(void) {
	////////////////////////////////////// BLE entry /////////////////////////////////
	blt_sdk_main_loop();
	////////////////////////////////////// ADV-Period /////////////////////////////////
	fl_adv_run();
	////////////////////////////////////// Soft-timer /////////////////////////////////
	blt_soft_timer_process(MAINLOOP_ENTRY);
}

