#ifndef STORAGE_WEEKLY_DATA_H
#define STORAGE_WEEKLY_DATA_H

#include "nvm.h"

/* For storage */
#define DEVICE_STORAGE_SIZE		0x80000
#define SECOND_PER_WEEK			604800
#define TIMESLOT_MAP_LENGTH		16
/* For OTA */
#define OTA_PACKET_LENGTH		16

/***** Structures *****/

/* For storage */
typedef enum
{
	STORAGE_RET_OK = 0,
	STORAGE_RET_ERROR
}storage_ret_t;

/* For OTA */
typedef enum
{
	OTA_RET_OK = 0,
	OTA_RET_ERROR
}ota_ret_t;

typedef enum
{
	OTA_DEVICE_GATEWAY = 0,
	OTA_DEVICE_PRODUCT_COUNTER,
	OTA_DEVICE_POWER_METER
}ota_device_type_t;

typedef enum
{
	OTA_FW_STATE_EMPTY = 0,
	OTA_FW_STATE_COMPLETE,
	OTA_FW_STATE_WRITING,
	OTA_FW_STATE_ERROR
}ota_fw_state_t;

typedef struct
{
	ota_fw_state_t		state;
	ota_device_type_t 	type;
	uint8_t				version;
	uint32_t			size;
	uint8_t				signature[OTA_PACKET_LENGTH];
}ota_fw_header_t;

/***** Prototypes *****/
/* For storage */
void storage_init(void);
void storage_clean(void);
void storage_set_timestamp_start(uint32_t timestamp);
uint32_t storage_get_timestamp_start(void);
void storage_set_timeslot_current(uint32_t timeslot);
void storage_set_timeslot_unspecified(uint32_t timeslot);
storage_ret_t storage_timeslot_map_check(uint32_t timeslot, uint32_t len);
void storage_timeslot_map_fill_status(uint32_t timeslot, uint32_t len);
void storage_timeslot_map_set(uint32_t timeslot, uint32_t len);
storage_ret_t storage_put_data(uint32_t timestamp, uint8_t *pdata,uint32_t len);
storage_ret_t storage_get_data(uint32_t timestamp, uint8_t *pdata,uint32_t len);
storage_ret_t storage_get_data_unspecified(uint32_t index, uint8_t *pdata,uint32_t len);
/* For OTA */
ota_ret_t ota_fw_put(uint8_t *pdata);
ota_ret_t ota_fw_header_set(ota_fw_header_t *header);
ota_ret_t ota_fw_header_get(ota_fw_header_t *header);
#endif
