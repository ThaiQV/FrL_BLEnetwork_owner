/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_ble_wifi.h
 *Created on		: Aug 20, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#ifndef VENDOR_FRL_NETWORK_FL_BLE_WIFI_H_
#define VENDOR_FRL_NETWORK_FL_BLE_WIFI_H_

typedef enum{
	W2B_STOP_NWK=1,
	W2B_START_NWK,
	//todo something
}fl_wifi2ble_exc_e;

void fl_ble_wifi_proc(u8* _pdata) ;
void fl_ble2wifi_HISTORY_SEND(u8* mac,u8* timetamp,u8* _data);
void fl_ble2wifi_EVENT_SEND(u8* _slave_mac);
void fl_wifi2ble_Excute(fl_wifi2ble_exc_e cmd);

#endif /* VENDOR_FRL_NETWORK_FL_BLE_WIFI_H_ */
