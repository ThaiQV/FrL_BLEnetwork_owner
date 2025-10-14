/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_wifi2ble_fota.h
 *Created on		: Oct 13, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#ifndef VENDOR_FRL_NETWORK_FL_WIFI2BLE_FOTA_H_
#define VENDOR_FRL_NETWORK_FL_WIFI2BLE_FOTA_H_

#ifdef MASTER_CORE
u8 FL_NWK_FOTA_IsReady(void);
u8 fl_wifi2ble_fota_push(u8 *_fw, u8 _len);
void fl_wifi2ble_fota_init(void);
void fl_wifi2ble_fota_run(void);
#endif

#endif /* VENDOR_FRL_NETWORK_FL_WIFI2BLE_FOTA_H_ */
