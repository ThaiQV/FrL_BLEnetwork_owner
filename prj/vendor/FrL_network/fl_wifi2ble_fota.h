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
typedef void (*fota_broadcast_rsp_cbk)(u8*, u8);
u8 FL_NWK_FOTA_IsReady(void);
s16 fl_wifi2ble_fota_fwpush(u8 *_fw, u8 _len);
s8 fl_wifi2ble_fota_Broadcast_REQwACK(u8* _fw, u8 _len,fota_broadcast_rsp_cbk _fncbk );
void fl_wifi2ble_fota_init(void);
void fl_wifi2ble_fota_run(void);
#endif

#endif /* VENDOR_FRL_NETWORK_FL_WIFI2BLE_FOTA_H_ */
