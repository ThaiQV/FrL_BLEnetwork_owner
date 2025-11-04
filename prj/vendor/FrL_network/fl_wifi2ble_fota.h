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

#include "../Freelux_libs/dfu.h"
#define DFU_OTA_INIT					ota_init
#define DFU_OTA_CRC128_INIT				crc128_init
#define DFU_OTA_CRC128_GET				ota_crc128_get
#define DFU_OTA_CRC128_CAL				crc128_calculate
#define DFU_OTA_FW_PUT					ota_fw_put

#ifdef MASTER_CORE
//typedef enum{
//	BroadCast_POLLING =0,
//	BroadCast_GROUP=1
//}fl_fota_broadcast_mode_e;

//typedef void (*fota_broadcast_rsp_cbk)(u8*, u8);
u8 FL_NWK_FOTA_IsReady(void);
s16 fl_wifi2ble_fota_fwpush(u32 _indx_addr,u8 *_fw, u8 _len);
//s8 fl_wifi2ble_fota_Broadcast_REQwACK(u8* _fw, u8 _len,fota_broadcast_rsp_cbk _fncbk,fl_fota_broadcast_mode_e _mode );
void fl_wifi2ble_fota_ContainerClear(void);
void fl_wifi2ble_fota_init(void);
void fl_wifi2ble_fota_run(void);
int fl_wifi2ble_fota_system_end(u8 *_payload_end,u8 _len);
int fl_wifi2ble_fota_system_start(u8 *_payload_start,u8 _len);
void fl_wifi2ble_fota_recECHO(fl_pack_t _pack_rec);
#endif

#endif /* VENDOR_FRL_NETWORK_FL_WIFI2BLE_FOTA_H_ */
