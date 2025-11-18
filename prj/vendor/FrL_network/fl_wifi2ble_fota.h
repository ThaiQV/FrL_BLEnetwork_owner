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

#define FOTA_EXIT_VALUE					0x7FFF
typedef enum {
	FOTA_PACKET_BEGIN = 0,
	FOTA_PACKET_DATA = 1,
	FOTA_PACKET_END = 2,
} fl_fota_pack_type_e;

u8 FL_NWK_FOTA_IsReady(void);
s16 fl_wifi2ble_fota_find(fl_pack_t *_pack_rec);
void fl_wifi2ble_fota_ContainerClear(void);
void fl_wifi2ble_fota_init(void);
s16 fl_wifi2ble_fota_run(void);
s16 fl_wifi2ble_fota_proc(void);
s16 fl_wifi2ble_fota_recECHO(fl_pack_t _pack_rec);

#ifdef MASTER_CORE
int fl_wifi2ble_fota_system_end(u8 *_payload_end,u8 _len);
int fl_wifi2ble_fota_system_begin(u8 *_payload_start,u8 _len);
int fl_wifi2ble_fota_system_data(u8 *_payload_start,u8 _len);
#else
s16 fl_wifi2ble_fota_fwpush(fl_pack_t *fw_pack,fl_fota_pack_type_e _pack_type);
#endif
#endif /* VENDOR_FRL_NETWORK_FL_WIFI2BLE_FOTA_H_ */
