/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_ext_adv_proc.c
 *Created on		: Oct 8, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/


#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "app.h"
#include "fl_nwk_protocol.h"

/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/

#define	APP_ADV_SETS_NUMBER						1		// Number of Supported Advertising Sets
#define APP_MAX_LENGTH_ADV_DATA					1024	// Maximum Advertising Data Length,   (if legacy ADV, max length 31 bytes is enough)
#define APP_MAX_LENGTH_SCAN_RESPONSE_DATA		31		// Maximum Scan Response Data Length, (if legacy ADV, max length 31 bytes is enough)

_attribute_data_retention_ u8 app_adv_set_param[ADV_SET_PARAM_LENGTH * APP_ADV_SETS_NUMBER];

_attribute_data_retention_ u8 app_primary_adv_pkt[MAX_LENGTH_PRIMARY_ADV_PKT * APP_ADV_SETS_NUMBER];

_attribute_data_retention_ u8 app_secondary_adv_pkt[MAX_LENGTH_SECOND_ADV_PKT * APP_ADV_SETS_NUMBER];

_attribute_data_retention_ u8 app_advData[APP_MAX_LENGTH_ADV_DATA * APP_ADV_SETS_NUMBER];

_attribute_data_retention_ u8 app_scanRspData[APP_MAX_LENGTH_SCAN_RESPONSE_DATA * APP_ADV_SETS_NUMBER];

fl_adv_settings_t G_EXT_ADV_SETTINGS = {
		.adv_interval_min = ADV_INTERVAL_20MS,
		.adv_interval_max = ADV_INTERVAL_30MS,
		.adv_duration = 60,
		.scan_interval = SCAN_INTERVAL_60MS,
		.scan_window = SCAN_WINDOW_60MS,
		.time_wait_rsp = 10,
		.retry_times = 2,
		};

/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
static int fl_extend_controller_event_callback(u32 h, u8 *p, int n) {

	if (h & HCI_FLAG_EVENT_BT_STD)		//ble controller hci event
	{
		u8 evtCode = h & 0xff;
		if (evtCode == HCI_EVT_LE_META) {
			u8 subEvt_code = p[0];
			if (subEvt_code == HCI_SUB_EVT_LE_EXTENDED_ADVERTISING_REPORT)		// ADV packet
			{
				event_adv_report_t *pa = (event_adv_report_t *) p;
				P_PRINTFHEX_A(BLE,pa->mac,6,"MAC:");
				P_PRINTFHEX_A(BLE,pa->data,pa->len,"EXT-PACK(%d):",pa->len);
			}
		}
	}
	return 0;
}
/**
 * @brief      callback function of LinkLayer Event "BLT_EV_FLAG_ADV_DURATION_TIMEOUT"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
void fl_duration_EXT_ADV_timeout_proccess(u8 e, u8 *p, int n){
	blc_ll_setExtAdvEnable_1(BLC_ADV_ENABLE,1,ADV_HANDLE0,100,0);//duration = 100*10 = 1s
	LOGA(BLE,"EXT-ADV Timed-out duration!!!\r\n");
}
/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/
void fl_ext_adv_init(void) {
	LOG_P(PERI,"EXT-ADV Init!!!\r\n");
	blc_ll_initExtendedAdvertising_module(app_adv_set_param,app_primary_adv_pkt,APP_ADV_SETS_NUMBER);
	blc_ll_initExtSecondaryAdvPacketBuffer(app_secondary_adv_pkt,MAX_LENGTH_SECOND_ADV_PKT);
	blc_ll_initExtAdvDataBuffer(app_advData,APP_MAX_LENGTH_ADV_DATA);
//	blc_ll_initExtScanRspDataBuffer(app_scanRspData,APP_MAX_LENGTH_SCAN_RESPONSE_DATA);
//	blc_ll_setAdvCustomedChannel(G_EXT_ADV_SETTINGS.nwk_chn.chn1,G_EXT_ADV_SETTINGS.nwk_chn.chn2,G_EXT_ADV_SETTINGS.nwk_chn.chn3);
//	blc_ll_setAdvCustomedChannel(10,11,12);
	blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_EXTENDED_ADVERTISING_REPORT);
	blc_hci_registerControllerEventHandler(fl_extend_controller_event_callback);
	//report all adv

	blc_ll_setScanParameter(SCAN_TYPE_ACTIVE,G_EXT_ADV_SETTINGS.scan_interval,G_EXT_ADV_SETTINGS.scan_window,OWN_ADDRESS_PUBLIC,SCAN_FP_ALLOW_ADV_ANY);
	blc_ll_addScanningInAdvState();  //add scan in adv state
	blc_ll_setScanEnable(1,0);
//	bls_ll_setAdvEnable(BLC_ADV_ENABLE);  //adv enable

}

void fl_ext_adv_send(void) {
	LOG_P(PERI,"EXT-ADV Sending!!!!\r\n");
	blc_ll_setExtAdvParam(ADV_HANDLE0,ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED,G_EXT_ADV_SETTINGS.adv_interval_min,
			G_EXT_ADV_SETTINGS.adv_interval_max,
			BLT_ENABLE_ADV_ALL,OWN_ADDRESS_PUBLIC,BLE_ADDR_PUBLIC,NULL,ADV_FP_NONE,TX_POWER_8dBm,BLE_PHY_1M,0,BLE_PHY_1M,ADV_SID_0,0);

	u8 testAdvData[1024];
	for (int i = 0; i < 1024; i++) {
		testAdvData[i] = i;
	}
	blc_ll_setExtAdvData(ADV_HANDLE0,DATA_OPER_FIRST,DATA_FRAGM_ALLOWED,251,testAdvData);
	blc_ll_setExtAdvData(ADV_HANDLE0,DATA_OPER_INTER,DATA_FRAGM_ALLOWED,251,testAdvData + 251);
	blc_ll_setExtAdvData(ADV_HANDLE0,DATA_OPER_INTER,DATA_FRAGM_ALLOWED,251,testAdvData + 502);
	blc_ll_setExtAdvData(ADV_HANDLE0,DATA_OPER_INTER,DATA_FRAGM_ALLOWED,251,testAdvData + 753);
	blc_ll_setExtAdvData(ADV_HANDLE0,DATA_OPER_LAST,DATA_FRAGM_ALLOWED,6,testAdvData + 1004);

	blc_ll_setExtAdvEnable_1(BLC_ADV_ENABLE,1,ADV_HANDLE0,100,0);//duration = 100*10 = 1s

	//set rf power index, user must set it after every suspend wakeup, cause relative setting will be reset in suspend
	rf_set_power_level_index(MY_RF_POWER_INDEX);

	blc_ll_init2MPhyCodedPhy_feature();

	bls_app_registerEventCallback(BLT_EV_FLAG_ADV_DURATION_TIMEOUT,&fl_duration_EXT_ADV_timeout_proccess);

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
