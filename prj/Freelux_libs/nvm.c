/**
  * @file       nvm.c
  * @author     LVD
  * @date       01/10/24
  * @version    V1.0.0
  * @brief     	API for nvm
  */
#include "nvm.h"

/*------------Definitions------------*/
#define	OS_NVM_DEBUG		0
#if OS_NVM_DEBUG
#define OS_NVM_LOG(...)	LOGA(DRV,__VA_ARGS__)
#else
#define OS_NVM_LOG(...)
#endif

/* Variables */
uint32_t nvm_current_pos = 0;
/* Functions */

/**
* @brief: non-volatile memory initiation
* @param: see below
* @retval: None
*/
void nvm_init(void)
{
	/* put your init for nvm here */
	/* SPI flash init */
    FLASH_Port_Init();
    /* FLASH ID check */
    FLASH_IC_Check();
    /* Init record */
    nvm_record_init();
}

/**
* @brief: nvm erase
* @param: see below
* @retval: None
*/
void nvm_erase(void)
{
	/* put your erase the whole space of nvm here */
	int i;
	for(i = 0; i <(NVM_MAX_SIZE/NVM_PAGE_SIZE); i++)
	{
		FLASH_Erase_Sector(NVM_BASE_ADDRESS + i*NVM_PAGE_SIZE);
		OS_NVM_LOG("Erase sector: %d\n",i);
	}
}

/**
* @brief: nvm write
* @param: see below
* @retval: None
*/
void nvm_write(uint32_t addr, uint8_t *p_data, uint32_t len)
{
	/* put your write in byte function of nvm here */
	W25XXX_WR_Block((uint8_t*)p_data ,NVM_BASE_ADDRESS + addr, len);
	OS_NVM_LOG("nvm_write\n");
}

/**
* @brief: nvm read
* @param: see below
* @retval: None
*/
void nvm_read(uint32_t addr, uint8_t *p_data, uint32_t len)
{
	/* put your read in byte function of nvm here */
    W25XXX_Read(p_data,NVM_BASE_ADDRESS + addr,len);

	OS_NVM_LOG("nvm_read addr: %d - len: %d\n",addr,len);
}

/**
* @brief: nvm record init
* @param: see below
* @retval: None
*/
nvm_status_t nvm_record_init(void)
{
	uint32_t	current_pos = 0;
	header_t	header = {0};
	
	// Check the first key id
	nvm_read(0,(uint8_t*)&header,sizeof(header));
	if(header.key_id == NVM_UNUSED_KEY_ID)
	{
		nvm_current_pos = 0;
		OS_NVM_LOG("NVM_NO_RECORD\n");
		return NVM_NO_RECORD;
	}
	else
	{
		current_pos = current_pos + sizeof(header) + header.len; // next record pos = current_pos + len of header + len of record
		while(current_pos < NVM_MAX_SIZE)
		{
			nvm_read(current_pos,(uint8_t*)&header,sizeof(header));	// read header(key id and record len)
			if(header.key_id == NVM_UNUSED_KEY_ID)
			{
				nvm_current_pos = current_pos;
				OS_NVM_LOG("nvm_current_pos: %d\n",nvm_current_pos);
				return NVM_OK;
			}
			current_pos = current_pos + sizeof(header) + header.len; // next record pos = current_pos + len of header + len of record
		}
	}
	
	OS_NVM_LOG("NVM_ERROR\n");
	return NVM_ERROR;
}

/**
* @brief: nvm get data of record position
* @param: see below
* @retval: None
*/
static nvm_status_t nvm_record_get_data_pos(uint8_t *p_buffer, uint16_t key_id, uint32_t *return_data_pos)
{
	uint32_t	current_pos = 0;
	uint32_t	return_pos = NVM_MAX_SIZE; // Set return position in end of nvm position
	header_t	header = {0};
	
	// Check read key id
	if(key_id == NVM_UNUSED_KEY_ID)
	{
		OS_NVM_LOG("NVM_WRONG_KEY_ID\n");
		return NVM_WRONG_KEY_ID;
	}
	// Check the first key id
	memcpy(&header,(uint8_t*)(p_buffer + current_pos),sizeof(header));		// read header(key id and record len)
	if(header.key_id == NVM_UNUSED_KEY_ID)
	{
		OS_NVM_LOG("NVM_NO_RECORD\n");
		return NVM_NO_RECORD;
	}
	else
	{
		// Get the last copy record with key_id
		while(current_pos < NVM_MAX_SIZE)
		{
			memcpy(&header,(uint8_t*)(p_buffer + current_pos),sizeof(header));		// read header(key id and record len)
			if(header.key_id == key_id)
			{
				return_pos = current_pos;
				OS_NVM_LOG("return_pos: %d\n",return_pos);
			}
			current_pos = current_pos + sizeof(header) + header.len; 							// next record pos = current_pos + len of header + len of record
		}
		
		if(return_pos < NVM_MAX_SIZE)
		{
			// return return_data_pos
			*return_data_pos = return_pos;
		}
		else
		{
			OS_NVM_LOG("NVM_NO_RECORD\n");
			return NVM_NO_RECORD;
		}
	}
	return NVM_OK;
}

/**
* @brief: nvm read record
* @param: see below
* @retval: None
*/
nvm_status_t nvm_record_read(uint16_t key_id, uint8_t *p_data, uint32_t len)
{	
	uint32_t	current_pos = 0;
	uint32_t	return_pos = NVM_MAX_SIZE; // Set return position in end of nvm position
	header_t	header = {0};
	
	// Check read key id
	if(key_id == NVM_UNUSED_KEY_ID)
	{
		OS_NVM_LOG("NVM_WRONG_KEY_ID\n");
		return NVM_WRONG_KEY_ID;
	}
	// Check the first key id
	nvm_read(0,(uint8_t*)&header.key_id,sizeof(header.key_id));
	if(header.key_id == NVM_UNUSED_KEY_ID)
	{
		OS_NVM_LOG("NVM_NO_RECORD\n");
		return NVM_NO_RECORD;
	}
	else
	{
		// Get the last copy record with key_id
		while(current_pos < NVM_MAX_SIZE)
		{
			nvm_read(current_pos,(uint8_t*)&header,sizeof(header));	// read header(key id and record len)
			if(header.key_id == key_id)
			{
				return_pos = current_pos;
			}
			current_pos = current_pos + sizeof(header) + header.len; // next record pos = current_pos + len of header + len of record
		}
		
		if(return_pos < NVM_MAX_SIZE)
		{
			// Read record
			nvm_read(return_pos + sizeof(header),p_data,len);
		}
		else
		{
			OS_NVM_LOG("NVM_NO_RECORD: Key: %d\n",key_id);
			return NVM_NO_RECORD;
		}
	}
	return NVM_OK;
}

/**
* @brief: nvm reroll
* @param: see below
* @retval: None
*/
static nvm_status_t nvm_reroll(uint16_t current_key_id)
{
	uint8_t		buffer[NVM_MAX_SIZE] = {0xFF};
	uint16_t	key_array[NVM_MAX_SIZE/sizeof(header_t)];
	uint16_t	key_index = 0;
	uint32_t	current_pos = 0;
	uint32_t	i = 0;
	header_t	header = {0};
	uint32_t	record_data_pos = 0;
	uint16_t	record_data_len = 0;
	uint8_t		record_data_len_low;
	uint8_t		record_data_len_high;
	
	OS_NVM_LOG("nvm_reroll\n");
	
	// copy all nvm to buffer
	nvm_read(0,buffer,NVM_MAX_SIZE);
	
//	for(i=0;i<NVM_MAX_SIZE;i++)
//	{
//		OS_NVM_LOG("%d ",buffer[i]);
//	}
//	OS_NVM_LOG("\n");
	
	// Erase all nvm space
	nvm_erase();
	nvm_current_pos = 0;
	// Get array of key id
	while(current_pos < NVM_MAX_SIZE)
	{
		memcpy(&header,&buffer[current_pos],sizeof(header)); 					// read header
		current_pos = current_pos + sizeof(header) + header.len; 			// next record pos = current_pos + len of header + len of record
		for(i = 0; i < key_index; i++)
		{
			if(header.key_id == key_array[i])
			{
				// if key id is available
				break;
			}
		}
		
		if(header.key_id == NVM_UNUSED_KEY_ID) // check with invalid key id
		{
			break;
		}
		
		if(i >= key_index) // a new key
		{
			// store new key id to array
			if(header.key_id != current_key_id) // Don't store current_key_id, it will be store last after reroll 
			{
				key_array[key_index++] = header.key_id;
			}
		}
	}
	// Write the last copy of records to nvm
	for(i = 0; i < key_index; i++)
	{
		nvm_record_get_data_pos(buffer,key_array[i],&record_data_pos);
		record_data_len_low = *(buffer + record_data_pos + sizeof(header.key_id));
		record_data_len_high = *(buffer + record_data_pos + sizeof(header.key_id) + 1);
		record_data_len = (uint16_t)((record_data_len_high << 8) + record_data_len_low);
		OS_NVM_LOG("key: %d - record_data_len: %d\n",key_array[i],record_data_len);
		nvm_write(nvm_current_pos,(uint8_t*)(buffer + record_data_pos),sizeof(header_t) + record_data_len);			// write record
		nvm_current_pos += sizeof(header_t) + record_data_len;																								// increase nvm_current_pos
	}
	
	return NVM_OK;
}

/**
* @brief: nvm record write
* @param: see below
* @retval: None
* @des:
*	[---------------------NVM Space---------------------]
*	[---[Record][Record][Record]...[NVM Remain bytes]---]
*	[				         ^						    ]
*	[						 |						    ]
*	[				 [NVM current pos]		 			]
*/
nvm_status_t nvm_record_write(uint16_t key_id, uint8_t *p_data, uint32_t len)
{
	uint32_t			record_len;
	header_t			header;
	nvm_status_t	retval;
	
	OS_NVM_LOG("NVM_WRITE: key: %d - len: %d - pos: %d\n",key_id,len,nvm_current_pos);
	// Set header
	header.key_id = key_id;
	header.len = len;
	// Calculate lenght of a record
	record_len = len + sizeof(header_t);

	// Check NVM is not overflow
	if(record_len <= (NVM_MAX_SIZE - nvm_current_pos))
	{
		nvm_write(nvm_current_pos,(uint8_t*)&header,sizeof(header_t));	// Write header
		nvm_current_pos += sizeof(header_t);														// increase nvm_current_pos
		nvm_write(nvm_current_pos,(uint8_t*)p_data,len);								// Write data
		nvm_current_pos += len;																					// increase nvm_current_pos
	}
	else
	{
		retval = nvm_reroll(key_id); // reroll nvm to filter duplicated records
		if(retval == NVM_ERROR)
		{
			return NVM_ERROR;
		}
		// store new record if not overflow
		if(record_len <= (NVM_MAX_SIZE - nvm_current_pos))
		{
			nvm_write(nvm_current_pos,(uint8_t*)&header,sizeof(header_t));	// Write header
			nvm_current_pos += sizeof(header_t);														// increase nvm_current_pos
			nvm_write(nvm_current_pos,(uint8_t*)p_data,len);								// Write data
			nvm_current_pos += len;	
		}
		else
		{
			// No space to store new record
			OS_NVM_LOG("NVM_FULL\n");
			return NVM_FULL;
		}
	}

	return NVM_OK;
}
