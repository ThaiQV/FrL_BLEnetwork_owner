#include "storage_weekly_data.h"

/*------------Definitions------------*/
#define	STORAGE_DEBUG		0
#if STORAGE_DEBUG
#define STORAGE_LOG(...)	LOGA(DRV,__VA_ARGS__)
#else
#define STORAGE_LOG(...)
#endif

/* Variables */
uint32_t timestamp_start = 0;
uint32_t timeslot_current = 0;
uint32_t timeslot_unspecified = 0;
uint8_t  map[MAP_LENGTH];

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
	// Load timeslot map
	ret = nvm_record_read(STORAGE_MAP,(uint8_t*)map,sizeof(map));
	if(ret == NVM_NO_RECORD)
	{
		// If there is no record of timeslot map, erase the whole device storage
		for(i = 0; i < (DEVICE_STORAGE_SIZE/DEF_UDISK_SECTOR_SIZE);i++)
		{
			FLASH_Erase_Sector(EX_FLASH_DEVICE_STORAGE_ADDRESS + i*DEF_UDISK_SECTOR_SIZE);
		}
		memset(map,0x00,sizeof(map));
		nvm_record_write(STORAGE_MAP,(uint8_t*)map,sizeof(map));
	}
}

/**
* @brief: Storage deinitialize
* @param: see below
* @retval: None
*/
void storage_clean(void)
{
	uint32_t i;

	// Erase device storage area
	for(i = 0; i < (DEVICE_STORAGE_SIZE/DEF_UDISK_SECTOR_SIZE);i++)
	{
		FLASH_Erase_Sector(EX_FLASH_DEVICE_STORAGE_ADDRESS + i*DEF_UDISK_SECTOR_SIZE);
	}
	// Reset timestamp map
	memset(map,0x00,sizeof(map));
	nvm_record_write(STORAGE_MAP,(uint8_t*)map,sizeof(map));
}

/**
* @brief: storage time slot map
* @param: see below
* @retval: see below
*/
bool check_sector_available(uint32_t sector)
{
	uint32_t i,j;
	uint8_t	 slot = 0;

	for(i = 0; i < (MAP_LENGTH); i++)
	{
		slot = map[i];
		for(j = 0; j < 8; j++)
		{
			if(sector == (j + i*8))
			{
				if(((0x01 << j) & slot) == 0)
				{
					STORAGE_LOG("check_sector_available: %d\n",1);
					return 1;
				}
				else
				{
					STORAGE_LOG("check_sector_available: %d\n",0);
					return 0;
				}
			}
		}
	}
	return 0;
}

/**
* @brief: Check the storage map. If a index is already written in a corresponding sector, erase that sector
* @param: see below
* @retval: see below
*/
storage_ret_t storage_map_check(uint16_t index, uint32_t len)
{
	uint32_t sector = 0;
	uint32_t remain = 0;
	uint8_t	 next_sector_erase = 0;

	sector = ((index*len)/DEF_UDISK_SECTOR_SIZE);
	// Remain bytes in this sector can be used to store new value
	remain = ((index*len)%DEF_UDISK_SECTOR_SIZE);
	// Check whether the data need next sector to store
	if((remain + len) > DEF_UDISK_SECTOR_SIZE)
	{
		next_sector_erase = 1;
	}
	if(check_sector_available(sector) == 0) // if sector is written
	{
		// Erase the sector has this timeslot if it has full written already
		FLASH_Erase_Sector(EX_FLASH_DEVICE_STORAGE_ADDRESS + sector*DEF_UDISK_SECTOR_SIZE);
		map[(sector/8)] &= (~(0x01 << (sector%8)));
		nvm_record_write(STORAGE_MAP,(uint8_t*)map,sizeof(map));
		STORAGE_LOG("FLASH_Erase_Sector 1: %d\n",sector);
	}
	if(next_sector_erase == 1)
	{
		// Check next sector
		sector++;
		if(check_sector_available(sector) == 0)
		{
			FLASH_Erase_Sector(EX_FLASH_DEVICE_STORAGE_ADDRESS + sector*DEF_UDISK_SECTOR_SIZE);
			map[(sector/8)] &= (~(0x01 << (sector%8)));
			nvm_record_write(STORAGE_MAP,(uint8_t*)map,sizeof(map));
			STORAGE_LOG("FLASH_Erase_Sector 2: %d\n",sector);
		}
	}
	return STORAGE_RET_OK;
}

/**
* @brief: write status of map sector is written if the sector is fully written
* @param: see below
* @retval: see below
*/
void storage_map_fill_status(uint32_t index, uint32_t len)
{
	uint32_t sector = 0;

	sector = ((index*len)/DEF_UDISK_SECTOR_SIZE);
	// Set status of previous sector is written
	if(sector > 0)
	{
//		if((map[(sector/8)] && (0x01 << ((sector%8) - 1))) == 0x00)
		if ((map[sector / 8] & (0x01 << (sector % 8))) == 0x00)
		{
			map[(sector/8)] |= (0x01 << ((sector%8) - 1));
			nvm_record_write(STORAGE_MAP,(uint8_t*)map,sizeof(map));
			STORAGE_LOG("storage_map_fill_status: %d - %d\n",sector,map[(sector/8)]);
		}
	}
	else if(sector == 0) // Set status of the last sector is written
	{
		if((map[(MAP_LENGTH - 1)] && (0x01 << (7))) == 0x00)
		{
			map[(MAP_LENGTH - 1)] |= (0x01 << (7));
			nvm_record_write(STORAGE_MAP,(uint8_t*)map,sizeof(map));
			STORAGE_LOG("storage_map_fill_status: %d - %d\n",sector,map[(MAP_LENGTH - 1)]);
		}
	}
}

/**
* @brief: Set timeslot to map
* @param: see below
* @retval: see below
*/
void storage_map_set(uint32_t index, uint32_t len)
{
	uint32_t sector = 0;

	sector = ((index*len)/DEF_UDISK_SECTOR_SIZE);
	map[(sector/8)] |= (0x01 << ((sector%8)));

	STORAGE_LOG("storage_map_set: %d - %d\n",sector,map[(sector/8)]);
}

uint8_t crc8(uint8_t *pdata,uint32_t len)
{
	uint8_t crc = 0;
	uint8_t i;

	for(i = 0; i < len; i++)
	{
		crc = crc + pdata[i];
	}
	return crc;
}

/**
* @brief: put data into storage
* @param:
* timestamp: timestamp of the pdata
* pdata: data structure
* len: length of data structure <= 40(= 512K/12288 slot)
* @retval: storage_ret_t
* [slot 0]: [4B timestamp + 1B type + 2B index + (pdata_len - 4 - 1 - 2)data] + CRC
* [slot 1]: [4B timestamp + 1B type + 2B index + (pdata_len - 4 - 1 - 2)data] + CRC
* ...
* [slot n]: [4B timestamp + 1B type + 2B index + (pdata_len - 4 - 1 - 2)data] + CRC
* len of [slot] = pdata_len + 1
* CRC = crc8(pdata)
*/
storage_ret_t storage_put_data(uint8_t *pdata,uint32_t pdata_len)
{
	uint32_t 		address = 0;
	uint8_t 		crc = 0;
	uint8_t 		read[40];
	uint8_t 		write[40];
	uint8_t			len = 0;
	uint32_t 		sector = 0;
	uint8_t 		sector_data[DEF_UDISK_SECTOR_SIZE];
	uint16_t		pdata_index;

	// Get index from pdata
	memcpy((uint8_t*)&pdata_index,(uint8_t*)&pdata[5],sizeof(pdata_index));
	// Copy pdata to write buffer
	memcpy(&write[len],(uint8_t*)&pdata[0],pdata_len);
	len += pdata_len;
	// Calculate CRC
	crc = crc8(write,len);
	// Put crc next to pdata
	write[len] = crc;
	len += sizeof(crc);
	// Check sector before write flash
	storage_map_fill_status(pdata_index,len);
	storage_map_check(pdata_index,len);
	// Write to flash
	address = EX_FLASH_DEVICE_STORAGE_ADDRESS + pdata_index*len;
	W25XXX_WR_Block(write,address,len);
	W25XXX_Read(read,address,len);
	if(memcmp(read,write,len) == 0) // check write successfully
	{
		return STORAGE_RET_OK;
	}
	else // write fail -> re-write the whole sector
	{
		STORAGE_LOG("re-write\n");
		sector = ((pdata_index*len)/DEF_UDISK_SECTOR_SIZE);
		// read data from sector
		W25XXX_Read(sector_data,EX_FLASH_DEVICE_STORAGE_ADDRESS + sector*DEF_UDISK_SECTOR_SIZE,DEF_UDISK_SECTOR_SIZE);
		// erase this sector
		FLASH_Erase_Sector(EX_FLASH_DEVICE_STORAGE_ADDRESS + sector*DEF_UDISK_SECTOR_SIZE);
		// write new data into sector_data
		memcpy(&sector_data[(pdata_index*len)%DEF_UDISK_SECTOR_SIZE],write,len);
		// write sector_data into erase sector
		W25XXX_WR_Block(sector_data,EX_FLASH_DEVICE_STORAGE_ADDRESS + sector*DEF_UDISK_SECTOR_SIZE,DEF_UDISK_SECTOR_SIZE);

		W25XXX_Read(read,address,len);
		if(memcmp(read,write,len) == 0) // check write successfully
		{
			return STORAGE_RET_OK;
		}
	}
	return STORAGE_RET_ERROR;
}

/**
* @brief: Get data from storage
* @param:
* timestamp: timestamp of the pdata
* pdata: data structure
* len: length of data structure <= 40(= 512K/12288 slot)
* @retval: storage_ret_t
*/
storage_ret_t storage_get_data(uint8_t *pdata,uint32_t len)
{
	uint32_t	address;
	uint16_t	pdata_index;
	uint16_t	read_index;
	uint8_t		crc;
	uint8_t 	read[40];

	if(len <= 40)
	{
		// Get index from pdata
		memcpy((uint8_t*)&pdata_index,(uint8_t*)&pdata[5],sizeof(pdata_index));
		// Read from flash
		address = EX_FLASH_DEVICE_STORAGE_ADDRESS + pdata_index*(len+1); // get pdata + CRC
		W25XXX_Read(read,address,(len+1));
		// Get index from storage
		memcpy((uint8_t*)&read_index,&read[5],sizeof(read_index));
		// Get pdata from read buffer
		memcpy((uint8_t*)pdata,read,len);
		if(read_index == pdata_index) // Check index
		{
			// Check CRC
			crc = crc8(pdata,len);
			if(crc == read[len])
			{
				return STORAGE_RET_OK;
			}
		}
	}
	return STORAGE_RET_ERROR;
}

