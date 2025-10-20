#ifndef STORAGE_WEEKLY_DATA_H
#define STORAGE_WEEKLY_DATA_H

#include "nvm.h"

/* For storage */
#define DEVICE_STORAGE_SIZE		0x80000
#define SECOND_PER_WEEK			604800
#define MAP_LENGTH				16

/***** Structures *****/

/* For storage */
typedef enum
{
	STORAGE_RET_OK = 0,
	STORAGE_RET_ERROR
}storage_ret_t;


/***** Prototypes *****/
/* For storage */
void storage_init(void);
void storage_clean(void);
bool check_sector_available(uint32_t sector);
storage_ret_t storage_map_check(uint16_t index, uint32_t len);
void storage_timeslot_map_fill_status(uint16_t index, uint32_t len);
void storage_timeslot_map_set(uint16_t index, uint32_t len);
storage_ret_t storage_put_data(uint8_t *pdata,uint32_t pdata_len);
storage_ret_t storage_get_data(uint8_t *pdata,uint32_t len);

#endif
