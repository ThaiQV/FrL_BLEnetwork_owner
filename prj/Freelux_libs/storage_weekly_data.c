#include "storage_weekly_data.h"

/*------------Definitions------------*/
#define	STORAGE_DEBUG		0
#if STORAGE_DEBUG
#define STORAGE_LOG(...)	LOGA(DRV,__VA_ARGS__)
#else
#define STORAGE_LOG(...)
#endif

static bool check_sector_available(uint32_t sector);

/* Variables */
uint32_t timestamp_start = 0;
uint32_t timeslot_current = 0;
uint32_t timeslot_unspecified = 0;
uint8_t  timeslot_map[TIMESLOT_MAP_LENGTH];

/* Functions */

/**
* @brief: Storage initialize
* @param: see below
* @retval: None
*/
void storage_init(void)
{
	nvm_status_t ret;
	uint32_t i;

	// Init non-volatile memory
	nvm_init();
	// Load timestamp start
	ret = nvm_record_read(STORAGE_TIMESTAMP_START,(uint8_t*)&timestamp_start,sizeof(timestamp_start));
	if(ret == NVM_NO_RECORD)
	{
		storage_set_timestamp_start(0);
	}
	STORAGE_LOG("timestamp_start: %d\n",timestamp_start);
	// Load timeslot current
	ret = nvm_record_read(STORAGE_TIMESLOT_CURRENT,(uint8_t*)&timeslot_current,sizeof(timeslot_current));
	if(ret == NVM_NO_RECORD)
	{
		storage_set_timeslot_current(0);
	}
	STORAGE_LOG("timeslot_current: %d\n",timeslot_current);
	// Load timeslot unspecified
	ret = nvm_record_read(STORAGE_TIMESLOT_UNSPECIFIED,(uint8_t*)&timeslot_unspecified,sizeof(timeslot_unspecified));
	if(ret == NVM_NO_RECORD)
	{
		storage_set_timeslot_unspecified(0);
	}
	STORAGE_LOG("timeslot_unspecified: %d\n",timeslot_unspecified);
	// Load timeslot map
	ret = nvm_record_read(STORAGE_TIMESLOT_MAP,(uint8_t*)timeslot_map,sizeof(timeslot_map));
	if(ret == NVM_NO_RECORD)
	{
		// If there is no record of timeslot map, erase the whole device storage
		for(i = 0; i < (DEVICE_STORAGE_SIZE/DEF_UDISK_SECTOR_SIZE);i++)
		{
			FLASH_Erase_Sector(EX_FLASH_DEVICE_STORAGE_ADDRESS + i*DEF_UDISK_SECTOR_SIZE);
		}
		memset(timeslot_map,0x00,sizeof(timeslot_map));
		nvm_record_write(STORAGE_TIMESLOT_MAP,(uint8_t*)timeslot_map,sizeof(timeslot_map));
	}

	PRINTF("timeslot_map: ");
	for(i=0;i<sizeof(timeslot_map);i++)
	{
		PRINTF("%x ",timeslot_map[i]);
	}
	PRINTF("\n");
}

/**
* @brief: Storage deinitialize
* @param: see below
* @retval: None
*/
void storage_clean(void)
{
	uint32_t i;

	// Reset timestamp start
	storage_set_timestamp_start(0);
	// Reset timeslot current
	timeslot_current = 0;
	nvm_record_write(STORAGE_TIMESLOT_CURRENT,(uint8_t*)&timeslot_current,sizeof(timeslot_current));
	// Erase device storage area
	for(i = 0; i < (DEVICE_STORAGE_SIZE/DEF_UDISK_SECTOR_SIZE);i++)
	{
		FLASH_Erase_Sector(EX_FLASH_DEVICE_STORAGE_ADDRESS + i*DEF_UDISK_SECTOR_SIZE);
	}
	// Reset timestamp map
	memset(timeslot_map,0x00,sizeof(timeslot_map));
	nvm_record_write(STORAGE_TIMESLOT_MAP,(uint8_t*)timeslot_map,sizeof(timeslot_map));
}

/**
* @brief: Set timestamp start
* @param: see below
* @retval: None
*/
void storage_set_timestamp_start(uint32_t timestamp)
{
	timestamp_start = timestamp;
	nvm_record_write(STORAGE_TIMESTAMP_START,(uint8_t*)&timestamp_start,sizeof(timestamp_start));
}

/**
* @brief: Get timestamp start
* @param: see below
* @retval: see below
*/
uint32_t storage_get_timestamp_start(void)
{
	nvm_record_read(STORAGE_TIMESTAMP_START,(uint8_t*)&timestamp_start,sizeof(timestamp_start));
	return timestamp_start;
}

/**
* @brief: Set timeslot_current
* @param: see below
* @retval: None
*/
void storage_set_timeslot_current(uint32_t timeslot)
{
	timeslot_current = timeslot;
	nvm_record_write(STORAGE_TIMESLOT_CURRENT,(uint8_t*)&timeslot_current,sizeof(timeslot_current));
}

/**
* @brief: Set timeslot_unspecified
* @param: see below
* @retval: None
*/
void storage_set_timeslot_unspecified(uint32_t timeslot)
{
	timeslot_unspecified = timeslot;
	nvm_record_write(STORAGE_TIMESLOT_UNSPECIFIED,(uint8_t*)&timeslot_unspecified,sizeof(timeslot_unspecified));
}
/**
* @brief: storage time slot map
* @param: see below
* @retval: see below
*/
static bool check_sector_available(uint32_t sector)
{
	uint32_t i,j;
	uint8_t	 slot = 0;

	for(i = 0; i < (TIMESLOT_MAP_LENGTH); i++)
	{
		slot = timeslot_map[i];
		for(j = 0; j < 8; j++)
		{
			if(sector == (j + i*8))
			{
				if(((0x01 << j) & slot) == 0)
				{
//					STORAGE_LOG("check_sector_available: %d\n",1);
					return 1;
				}
				else
				{
//					STORAGE_LOG("check_sector_available: %d\n",0);
					return 0;
				}
			}
		}
	}
	return 0;
}

/**
* @brief: Check the timeslot map. If a timeslot is already written in a corresponding sector, erase that sector
* @param: see below
* @retval: see below
*/
storage_ret_t storage_timeslot_map_check(uint32_t timeslot, uint32_t len)
{
	uint32_t sector = 0;
	uint32_t remain = 0;
	uint8_t	 next_sector_erase = 0;

	sector = ((timeslot*len)/DEF_UDISK_SECTOR_SIZE);
	// Remain bytes in this sector can be used to store new value
	remain = ((timeslot*len)%DEF_UDISK_SECTOR_SIZE);
	// Check whether the data need next sector to store
	if((remain + len) > DEF_UDISK_SECTOR_SIZE)
	{
		next_sector_erase = 1;
	}
	if(check_sector_available(sector) == 0) // if sector is written
	{
		// Erase the sector has this timeslot if it has full written already
		FLASH_Erase_Sector(EX_FLASH_DEVICE_STORAGE_ADDRESS + sector*DEF_UDISK_SECTOR_SIZE);
		timeslot_map[(sector/8)] &= (~(0x01 << (sector%8)));
		STORAGE_LOG("FLASH_Erase_Sector 1: %d\n",sector);
	}
	if(next_sector_erase == 1)
	{
		// Check next sector
		sector++;
		if(check_sector_available(sector) == 0)
		{
			FLASH_Erase_Sector(EX_FLASH_DEVICE_STORAGE_ADDRESS + sector*DEF_UDISK_SECTOR_SIZE);
			timeslot_map[(sector/8)] &= (~(0x01 << (sector%8)));
			STORAGE_LOG("FLASH_Erase_Sector 2: %d\n",sector);
		}
	}
	return STORAGE_RET_OK;
}

/**
* @brief: write status of timeslot map sector is written if the sector is fully written
* @param: see below
* @retval: see below
*/
void storage_timeslot_map_fill_status(uint32_t timeslot, uint32_t len)
{
	uint32_t sector = 0;

	sector = ((timeslot*len)/DEF_UDISK_SECTOR_SIZE);
	// Set status of previous sector is written
	if(sector > 0)
	{
		timeslot_map[(sector/8)] |= (0x01 << ((sector%8) - 1));
//		STORAGE_LOG("storage_timeslot_map_fill_status: %d - %d\n",sector,timeslot_map[(sector/8)]);
	}
	else if(sector == 0) // Set status of the last sector is written
	{
		timeslot_map[(TIMESLOT_MAP_LENGTH - 1)] |= (0x01 << (7));
//		STORAGE_LOG("storage_timeslot_map_fill_status: %d - %d\n",sector,timeslot_map[(TIMESLOT_MAP_LENGTH - 1)]);
	}
	nvm_record_write(STORAGE_TIMESLOT_MAP,(uint8_t*)timeslot_map,sizeof(timeslot_map));
}

/**
* @brief: Set timeslot to map
* @param: see below
* @retval: see below
*/
void storage_timeslot_map_set(uint32_t timeslot, uint32_t len)
{
	uint32_t sector = 0;

	sector = ((timeslot*len)/DEF_UDISK_SECTOR_SIZE);
	timeslot_map[(sector/8)] |= (0x01 << ((sector%8)));

	STORAGE_LOG("storage_timeslot_map_set: %d - %d\n",sector,timeslot_map[(sector/8)]);
}

/**
* @brief: put data into storage
* @param:
* timestamp: timestamp of the pdata
* pdata: data structure
* len: length of data structure <= 52(= 512K/10080 minute)
* @retval: storage_ret_t
*/
storage_ret_t storage_put_data(uint32_t timestamp, uint8_t *pdata,uint32_t len)
{
	uint32_t	address;
	uint32_t	timeslot;
	uint32_t	sector = 0;

	if(timestamp >= timestamp_start)
	{
		if(timestamp >= (timestamp_start + SECOND_PER_WEEK))
		{
			storage_timeslot_map_set(timeslot_current,len);
			storage_set_timestamp_start(timestamp);
			storage_set_timeslot_current(0);
			STORAGE_LOG("Reset all\n");
		}

		if(len <= 52)
		{
			timeslot = ((timestamp - timestamp_start)/60);
			if((timeslot > timeslot_current) || (timeslot_current == 0))
			{
				storage_set_timeslot_current(timeslot);
				// Check the timeslot to erase corresponding sector
				storage_timeslot_map_check(timeslot,len);
				// Write to flash
				address = EX_FLASH_DEVICE_STORAGE_ADDRESS + timeslot*len;
				W25XXX_WR_Block(pdata,address,len);
				// Write the status of sector to timeslot map
				storage_timeslot_map_fill_status(timeslot,len);
				return STORAGE_RET_OK;
			}
		}
	}
	else // Put data into unspecified region
	{
		// Erase page if timeslot_unspecified is in the first position of the page
		if(((timeslot_unspecified*len) % DEF_UDISK_SECTOR_SIZE) < len)
		{
			sector = ((timeslot_unspecified*len)/DEF_UDISK_SECTOR_SIZE);
			FLASH_Erase_Sector(EX_FLASH_DEVICE_STORAGE_ADDRESS + sector*DEF_UDISK_SECTOR_SIZE);
		}
		// Storage data into unspecified region
		address = EX_FLASH_DEVICE_UNSTORAGE_ADDRESS + timeslot_unspecified*len;
		W25XXX_WR_Block(pdata,address,len);
		timeslot_unspecified++;
		storage_set_timeslot_unspecified(timeslot_unspecified);
		if((timeslot_unspecified*len) > DEVICE_STORAGE_SIZE)
		{
			storage_set_timeslot_unspecified(0);
		}
	}
	return STORAGE_RET_ERROR;
}

/**
* @brief: Get data from storage
* @param:
* timestamp: timestamp of the pdata
* pdata: data structure
* len: length of data structure <= 52(= 512K/10080 minute)
* @retval: storage_ret_t
*/
storage_ret_t storage_get_data(uint32_t timestamp, uint8_t *pdata,uint32_t len)
{
	uint32_t	address;
	uint32_t	timeslot;

	if(len <= 52)
	{
		timeslot = ((timestamp - timestamp_start)/60);
		// Read from flash
		address = EX_FLASH_DEVICE_STORAGE_ADDRESS + timeslot*len;
		W25XXX_Read(pdata,address,len);
		return STORAGE_RET_OK;
	}
	return STORAGE_RET_ERROR;
}

/**
* @brief: Get data unspecified from storage
* @param: see below
* index: determine position from timeslot_unspecified. (timeslot = timeslot_unspecified - index - 1)
* @retval: storage_ret_t
*/
storage_ret_t storage_get_data_unspecified(uint32_t index, uint8_t *pdata,uint32_t len)
{
	uint32_t	address;

	if(len <= 52)
	{
		// Read from flash
		address = EX_FLASH_DEVICE_UNSTORAGE_ADDRESS + (timeslot_unspecified - index - 1)*len;
		W25XXX_Read(pdata,address,len);
		return STORAGE_RET_OK;
	}
	return STORAGE_RET_ERROR;
}

/******************************************OTA Region******************************************/
/*
 *     Page 128  |------------------|
 *               |       OTA        |
 *               |     Firmware     |
 *     Page 1    |------------------|
 *               |     Firmware     |
 *               |      Header      |
 *     Page 0    |------------------|
 * */

/**
* @brief: Put data into OTA region
* @param: see below
* -----------------------------------------------------
* | Device type | Version | Memory address |   Data   |
* -----------------------------------------------------
* |    1 byte   | 1 byte  |     3 bytes    | 16 bytes |
* -----------------------------------------------------
* @retval: ota_ret_t
*/
ota_ret_t ota_fw_put(uint8_t *pdata)
{
	ota_device_type_t 	device_type;
	uint8_t 			version;
	uint32_t 			memory_addr;
	ota_fw_header_t		header;

	device_type = pdata[0];
	version		= pdata[1];
	memory_addr	= (uint32_t)pdata[2] + (uint32_t)(pdata[3] << 8) + (uint32_t)(pdata[4] << 16);

	ota_fw_header_get(&header);
	if((header.state == OTA_FW_STATE_EMPTY) || (header.state == OTA_FW_STATE_WRITING))
	{
		if((device_type == header.type) && (version == header.version))
		{
			memory_addr = EX_FLASH_OTA_FW_ADDRESS + memory_addr;
			W25XXX_WR_Block((uint8_t*)&pdata[5],memory_addr,OTA_PACKET_LENGTH);
			return OTA_RET_OK;
		}
	}
	return OTA_RET_ERROR;
}

ota_ret_t ota_fw_header_set(ota_fw_header_t *header)
{
	nvm_status_t nvm_ret;

	nvm_ret = nvm_record_write(OTA_FW_HEADER_KEY,(uint8_t*)header,sizeof(ota_fw_header_t));
	if(nvm_ret == NVM_OK)
	{
		return OTA_RET_OK;
	}
	return OTA_RET_ERROR;
}

ota_ret_t ota_fw_header_get(ota_fw_header_t *header)
{
	nvm_status_t nvm_ret;

	nvm_ret = nvm_record_read(OTA_FW_HEADER_KEY,(uint8_t*)header,sizeof(ota_fw_header_t));
	if(nvm_ret == NVM_OK)
	{
		return OTA_RET_OK;
	}
	return OTA_RET_ERROR;
}



