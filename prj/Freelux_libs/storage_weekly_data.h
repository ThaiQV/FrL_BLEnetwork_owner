#ifndef STORAGE_WEEKLY_DATA_H
#define STORAGE_WEEKLY_DATA_H

#include "nvm.h"

#define DEVICE_STORAGE_SIZE		0x80000
#define SECOND_PER_WEEK			604800
#define TIMESLOT_MAP_LENGTH		16

/* Structures */

typedef enum
{
	STORAGE_RET_OK = 0,
	STORAGE_RET_ERROR
}storage_ret_t;

/* Prototypes */
void storage_init(void);
void storage_clean(void);
void storage_set_timestamp_start(uint32_t timestamp);
uint32_t storage_get_timestamp_start(void);
void storage_set_timeslot_current(uint32_t timeslot);
void storage_set_timeslot_unspecified(uint32_t timeslot);
static bool check_sector_available(uint32_t sector);
storage_ret_t storage_timeslot_map_check(uint32_t timeslot, uint32_t len);
void storage_timeslot_map_fill_status(uint32_t timeslot, uint32_t len);
void storage_timeslot_map_set(uint32_t timeslot, uint32_t len);
storage_ret_t storage_put_data(uint32_t timestamp, uint8_t *pdata,uint32_t len);
storage_ret_t storage_get_data(uint32_t timestamp, uint8_t *pdata,uint32_t len);
storage_ret_t storage_get_data_unspecified(uint32_t index, uint8_t *pdata,uint32_t len);

#endif
