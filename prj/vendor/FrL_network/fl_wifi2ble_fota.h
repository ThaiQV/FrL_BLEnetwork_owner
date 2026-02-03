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

#ifdef OTA_ENABLE
#define DFU_OTA_INIT					ota_init
#define DFU_OTA_CRC128_INIT				crc128_init
#define DFU_OTA_CRC128_GET				ota_crc128_get
#define DFU_OTA_CRC128_CAL				crc128_calculate
#define DFU_OTA_FW_PUT					ota_fw_put
#define DFU_OTA_VERSION_SET(x, y, z)   do {                                            \
														fw_header_t fw_buf;                        \
														fw_buf.major = (uint8_t)(x);           \
														fw_buf.minor = (uint8_t)(y);           \
														fw_buf.patch = (uint8_t)(z);           \
														set_current_fw_version(&fw_buf);           \
													} while(0)
//#define DFU_OTA_VERSION_GET()				0//get_current_fw_version
static inline fw_header_t DFU_OTA_VERSION_GET(void){
														fw_header_t fw_buf;
														get_current_fw_version(&fw_buf);
														return fw_buf;
													}
#else
#define DFU_OTA_INIT()         			((void)0)
#define DFU_OTA_CRC128_INIT   			((void)0)
#define DFU_OTA_CRC128_GET(...) 		(0)
#define DFU_OTA_CRC128_CAL 				(0)
#define DFU_OTA_FW_PUT(...)     		(0)
#define DFU_OTA_VERSION_SET(...)		((void)0)
static inline fw_header_t DFU_OTA_VERSION_GET(void){
														fw_header_t fw_buf;
														return fw_buf;
													}
#endif

#define FOTA_PACK_SIZE_MIN 				16
#define FOTA_PACK_FW_SIZE				21
#define FOTA_RETRY_POSITION				(40-1)//SIZEU8(G_FW_QUEUE_SENDING.data[0].data_arr)
#define FOTA_TYPEPACK_POSITION			(FOTA_RETRY_POSITION-1)
#define FOTA_MAC_INCOM_SIZE				4
#define FOTA_MAC_INCOM_POSITION			(FOTA_TYPEPACK_POSITION-FOTA_MAC_INCOM_SIZE) //mac size is 4 bytes
#define FOTA_MILSTEP_POSITION			5
#define FOTA_FW_DATA_POSITION			(1+4+1+1) //hdr+timestamp[4]+milltamp+slaveID
#define FOTA_EXIT_VALUE					0x7FFF

typedef enum{
	FOTA_DEV_COUNTER = 0,
	FOTA_DEV_PWMETER,
	FOTA_DEV_MASTER,
}fl_fota_dev_type_e;

typedef enum {
	FOTA_PACKET_BEGIN = 0,
	FOTA_PACKET_DATA = 1,
	FOTA_PACKET_END = 2,
} fl_fota_pack_type_e;

//for debuging
typedef struct {
	u32 fw_size;
	u32 body;
	u32 rtt;
	u8 version;
	u8 pack_type;
	u8 fw_type;
	u8 begin;
	u8 end;
} fl_ble2wif_fota_info_t;

u16 FL_NWK_FOTA_IsReady(void);
s16 fl_wifi2ble_fota_find(fl_pack_t *_pack_rec);
void fl_wifi2ble_fota_ContainerClear(void);
void fl_wifi2ble_fota_init(void);
s16 fl_wifi2ble_fota_run(void);
s16 fl_wifi2ble_fota_proc(void);
s16 fl_wifi2ble_fota_recECHO(fl_pack_t _pack_rec,u8* _mac_incom);

#ifdef MASTER_CORE
int fl_wifi2ble_fota_system_end(u8 *_payload_end,u8 _len);
int fl_wifi2ble_fota_system_begin(u8 *_payload_start,u8 _len);
int fl_wifi2ble_fota_system_data(u8 *_payload,u8 _len);
#else
s16 fl_wifi2ble_fota_fwpush(fl_pack_t *fw_pack,fl_fota_pack_type_e _pack_type);
#endif
#endif /* VENDOR_FRL_NETWORK_FL_WIFI2BLE_FOTA_H_ */
