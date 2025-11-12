#include "dfu.h"

/* Definition */
#define DFU_DEBUG	0
#ifdef DFU_DEBUG
#define DFU_PRINTF(...)	LOGA(APP,__VA_ARGS__);
#else
#define DFU_PRINTF(...)
#endif

#define HEADER_EMPTY	0xFFFFFF
#define HVP(header) 	header_version_parse(header)

/* Variables */

uint8_t crc128[CRC128_LENGTH];

uint8_t  ota_map[16];
/* Functions */

/**
* @brief: jump to application region
* @param: see below
* @retval: None
*/
void jump_to_application(void)
{
	DFU_PRINTF("Jump to Application\n");
	core_interrupt_disable();
	JUMP_TO_APP();
}

/**
* @brief: check FW in running region is valid or not
* @param: see below
* @retval: state of FW
*/
uint8_t check_valid_current_fw(void)
{
	u8 buf[64];

	flash_read_page(FLASH_R_BASE_ADDR + APP_IMAGE_ADDR + FLASH_TLNK_FLAG_OFFSET, 1, buf);
	if(buf[0] == FW_START_UP_FLAG)
	{
		return 1;
	}
	return 0;
}

/**
* @brief: check FW in OTA region is valid or not
* @param: see below
* @retval: state of FW
*/
uint8_t check_valid_ota_fw(uint32_t address)
{
	u8 buf[64];

	W25XXX_Read((uint8_t *)buf,address + FLASH_TLNK_FLAG_OFFSET,sizeof(buf));
	if(buf[0] == FW_START_UP_FLAG)
	{
		return 1;
	}
	return 0;
}

/**
* @brief: init crc128 IV
* @param: see below
* @retval: None
*/
void crc128_init(void)
{
	memset(crc128,0xFF,sizeof(crc128));
}

/**
* @brief: calculate crc128 for check sum of OTA FW
* @param: see below
* @retval: None
*/
void crc128_calculate(uint8_t *pdata)
{
	uint8_t i;

	for(i = 0; i < CRC128_LENGTH; i++)
	{
		crc128[i] = (crc128[i] ^ pdata[i]);
	}
}

/**
* @brief: parse header version for FW
* @param: see below
* @retval: header version number
*/
uint32_t header_version_parse(fw_header_t *header)
{
	return ((uint32_t)(header->major << 16) + (uint32_t)(header->minor << 8) + (uint32_t)(header->patch));
}

/**
*@brief	disorder for OTA data
*@param see below
*/
void disorder_data(uint8_t *p)
{
	uint8_t	disorder[CRC128_LENGTH] = {0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA};
	uint8_t	i;

	for(i = 0; i < CRC128_LENGTH; i++)
	{
		p[i] = (p[i] ^ disorder[i]);
	}
}
/**
* @brief: copy fw from OTA region to running region
* @param: see below
* @retval: None
*/
void fw_copy(fw_header_t *header, uint32_t fw_addr)
{
	uint32_t 	i,j;
	uint32_t 	size;
	uint32_t 	remain;
	uint8_t		buff[1024];
	uint8_t		buff_ex[32];
	uint8_t		packet[22];
	fw_header_t	header_app = {0};

	DFU_PRINTF("Copy FW: ver: %d.%d.%d - size: %d\n",header->major,header->minor,header->patch,header->size);

	// Check CRC128 of OTA FW in external flash before load into internal flash
	crc128_init();
	size = 0;
	while(size < header->size)
	{
		W25XXX_Read((uint8_t *)buff,fw_addr + size,CRC128_LENGTH);
		disorder_data(buff);
		crc128_calculate(buff);
		size += CRC128_LENGTH;
	}

//	DFU_PRINTF("Header CRC128: ");
//	for(i = 0; i < CRC128_LENGTH; i++)
//	{
//		printf("%x ",header->crc128[i]);
//	}
//	printf("\n");
//	DFU_PRINTF("Calculate CRC128: ");
//	for(i = 0; i < CRC128_LENGTH; i++)
//	{
//		printf("%x ",crc128[i]);
//	}
//	printf("\n");

	if(memcmp(crc128,header->crc128,CRC128_LENGTH) == 0)
	{
		DFU_PRINTF("OTA FW CRC128 is valid\n");
	}
	else
	{
		DFU_PRINTF("OTA FW CRC128 is invalid\n");
		return;
	}

//	// Compare FW in flash vs ex-flash
//	crc128_init();
//	size = 0;
//	while(size < header->size)
//	{
//		flash_read_page(FLASH_R_BASE_ADDR + APP_IMAGE_ADDR + size, CRC128_LENGTH, (uint8_t *)buff);
//		crc128_calculate(buff);
//		size += CRC128_LENGTH;
//	}
//
//	DFU_PRINTF("FW Calculate CRC128: ");
//	for(i = 0; i < CRC128_LENGTH; i++)
//	{
//		printf("%x ",crc128[i]);
//	}
//	printf("\n");
//
//	size = 0;
//	while(size < header->size)
//	{
//		flash_read_page(FLASH_R_BASE_ADDR + APP_IMAGE_ADDR + size, CRC128_LENGTH, (uint8_t *)buff);
//		W25XXX_Read((uint8_t *)buff_ex,fw_addr + size,CRC128_LENGTH);
//
//		disorder_data(buff_ex);
//
//		if(memcmp(buff_ex,buff,CRC128_LENGTH) != 0)
//		{
//			DFU_PRINTF("Wrong compare: %x\n",size);
//			for(i = 0; i < CRC128_LENGTH; i++)
//			{
//				printf("%x ",buff[i]);
//			}
//			printf("\n");
//			for(i = 0; i < CRC128_LENGTH; i++)
//			{
//				printf("%x ",buff_ex[i]);
//			}
//			printf("\n");
//
////			W25XXX_WR_Block((uint8_t*)buff,fw_addr + size,OTA_PACKET_LENGTH);
//		}
//		size += CRC128_LENGTH;
//	}


	// Erase running region to copy OTA FW
	flash_read_mid();
	flash_unlock_mid146085();
	DFU_PRINTF("Erase Running region\n");
	for(i = 0; i < (APP_IMAGE_SIZE_MAX/APP_PAGE_SIZE); i++)
	{
		flash_erase_sector(FLASH_R_BASE_ADDR + APP_IMAGE_ADDR + i*APP_PAGE_SIZE);
	}
	// Copy OTA FW into running region
	size = header->size/sizeof(buff);
	for(i = 0; i < size; i++)
	{
		W25XXX_Read((uint8_t *)buff,fw_addr + i*sizeof(buff),sizeof(buff));
		for(j = 0; j < (sizeof(buff)/CRC128_LENGTH); j++)
		{
			disorder_data((uint8_t*)&buff[j*CRC128_LENGTH]);
		}
		flash_write_page(FLASH_R_BASE_ADDR + APP_IMAGE_ADDR + i*sizeof(buff), sizeof(buff), (uint8_t *)buff);
		DFU_PRINTF("Copy: %d%%\n",(((i+1)*100)/size));
	}

	remain = header->size%sizeof(buff);
	if(remain > 0)
	{
		W25XXX_Read((uint8_t *)buff,fw_addr + size*sizeof(buff),sizeof(buff));
		for(j = 0; j < (sizeof(buff)/CRC128_LENGTH); j++)
		{
			disorder_data((uint8_t*)&buff[j*CRC128_LENGTH]);
		}
		flash_write_page(FLASH_R_BASE_ADDR + APP_IMAGE_ADDR + size*sizeof(buff), sizeof(buff), (uint8_t *)buff);
		DFU_PRINTF("Copy remain: %d bytes\n",size);
	}

	// Verify new FW
	size = 0;
	crc128_init();
	while(size < header->size)
	{
		flash_read_page(FLASH_R_BASE_ADDR + APP_IMAGE_ADDR + size, CRC128_LENGTH, (uint8_t *)buff);
		disorder_data(buff);
		crc128_calculate(buff);
		if((size%0x1000) == 0)
		{
			DFU_PRINTF("Verify: %d%%\n",((size*100)/header->size));
		}
		size += CRC128_LENGTH;
	}
	DFU_PRINTF("Verify: 100%%\n");
	// Check CRC128
	if(memcmp(crc128,header->crc128,CRC128_LENGTH) == 0)
	{
		DFU_PRINTF("CRC128 is correct\n");
		// Write Application FW header
		header_app.major = header->major;
		header_app.minor = header->minor;
		header_app.patch = header->patch;
		header_app.size  = header->size;
		memcpy(header_app.crc128,crc128,CRC128_LENGTH);
		// Erase header page before write new header
		flash_erase_sector(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER);
		flash_write_page(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER, sizeof(header_app), (uint8_t *)&header_app);

		jump_to_application();
	}
	else
	{
//		 Erase OTA FW header if the CRC128 is wrong
//		FLASH_Erase_Sector(OTA_FW_HEADER);
		DFU_PRINTF("CRC128 is incorrect\n");
	}
}

/**
* @brief: Erase the region in ex-flash
* @param: see below
* @retval: None
*/
void ex_flash_region_erase(uint32_t region)
{
	uint32_t 	i;

	// Erase OTA region in ex-flash
	if(region == OTA_FW_ADDRESS)
	{
		DFU_PRINTF("Erase OTA region\n");
		for(i = 0; i < ((EX_FLASH_NVM_ADDRESS - EX_FLASH_OTA_FW_ADDRESS)/DEF_UDISK_SECTOR_SIZE); i++)
		{
			FLASH_Erase_Sector(EX_FLASH_OTA_FW_ADDRESS +  i*DEF_UDISK_SECTOR_SIZE);
		}
	}
	else if(region == ORIGINAL_FW_ADDRESS)
	{
		DFU_PRINTF("Erase ORIGINAL region\n");
		for(i = 0; i < ((EX_FLASH_OTA_FW_ADDRESS - EX_FLASH_ORIGINAL_FW_ADDRESS)/DEF_UDISK_SECTOR_SIZE); i++)
		{
			FLASH_Erase_Sector(EX_FLASH_ORIGINAL_FW_ADDRESS +  i*DEF_UDISK_SECTOR_SIZE);
		}
	}
	else
	{
		return;
	}
}
/**
* @brief: copy current FW into OTA region
* @param: see below
* @retval: None
*/
void put_fw_into_ex_flash(uint32_t fw_addr)
{
	uint32_t 	i,j;
	uint8_t		buff[1024];
	fw_header_t	header = {0};
	uint32_t	header_addr;
//	uint32_t	image_size = 0x18000;//APP_IMAGE_SIZE_MAX;
	uint32_t	image_size = 0x40000;//APP_IMAGE_SIZE_MAX;

	// Erase OTA region in ex-flash
	if(fw_addr == OTA_FW_ADDRESS)
	{
		DFU_PRINTF("Copy FW into OTA region\n");
		for(i = 0; i < ((EX_FLASH_NVM_ADDRESS - EX_FLASH_OTA_FW_ADDRESS)/DEF_UDISK_SECTOR_SIZE); i++)
		{
			FLASH_Erase_Sector(EX_FLASH_OTA_FW_ADDRESS +  i*DEF_UDISK_SECTOR_SIZE);
		}
		header_addr = OTA_FW_HEADER;
	}
	else if(fw_addr == ORIGINAL_FW_ADDRESS)
	{
		DFU_PRINTF("Copy FW into ORIGINAL region\n");
		for(i = 0; i < ((EX_FLASH_OTA_FW_ADDRESS - EX_FLASH_ORIGINAL_FW_ADDRESS)/DEF_UDISK_SECTOR_SIZE); i++)
		{
			FLASH_Erase_Sector(EX_FLASH_ORIGINAL_FW_ADDRESS +  i*DEF_UDISK_SECTOR_SIZE);
		}
		header_addr = ORIGINAL_FW_HEADER;
	}
	else
	{
		return;
	}

	// Copy FW into select region
	flash_read_mid();
	flash_unlock_mid146085();
	crc128_init();
	for(i = 0; i < (image_size/sizeof(buff)); i++)
	{
		flash_read_page(FLASH_R_BASE_ADDR + APP_IMAGE_ADDR + i*sizeof(buff), sizeof(buff), (uint8_t *)buff);
		// calculate crc128
		for(j = 0; j < (sizeof(buff)/CRC128_LENGTH); j++)
		{
			crc128_calculate(&buff[j*CRC128_LENGTH]);
			disorder_data((uint8_t*)&buff[j*CRC128_LENGTH]);
		}
		W25XXX_WR_Block((uint8_t *)buff,fw_addr + i*sizeof(buff),sizeof(buff));
		DFU_PRINTF("Copy: %d%%\n",(((i+1)*100)/(image_size/sizeof(buff))));
	}

	DFU_PRINTF("FW CRC128: ");
	for(i = 0; i < CRC128_LENGTH; i++)
	{
		printf("%x ",crc128[i]);
	}
	printf("\n");

	// Write OTA header
	header.major = 0;
	header.minor = 0;
	header.patch = 1;
	header.size  = image_size;
	memcpy(header.crc128,crc128,CRC128_LENGTH);

	W25XXX_WR_Block((uint8_t *)&header,header_addr,sizeof(header));
}


/**
* @brief: Check and compare FW in running region and OTA region
* @param: see below
* @retval: None
*/
void firmware_check(void)
{
	// Check valid FW
	fw_header_t header_current_fw;
	fw_header_t header_ota_fw;
	fw_header_t header_original_fw;

	// Init ex-flash
	FLASH_Port_Init();

//	ex_flash_region_erase(ORIGINAL_FW_ADDRESS);
//	ex_flash_region_erase(OTA_FW_ADDRESS);

	// Read header of OTA FW in ex-flash
	W25XXX_Read((uint8_t *)&header_ota_fw,OTA_FW_HEADER,sizeof(fw_header_t));
	DFU_PRINTF("OTA FW Ver: %x.%x.%x\n",header_ota_fw.major,header_ota_fw.minor,header_ota_fw.patch);
	DFU_PRINTF("Size: %x\n",header_ota_fw.size);

	// Read header of original FW in ex-flash
	W25XXX_Read((uint8_t *)&header_original_fw,ORIGINAL_FW_HEADER,sizeof(fw_header_t));
	DFU_PRINTF("Original FW ver: %x.%x.%x\n",header_original_fw.major,header_original_fw.minor,header_original_fw.patch);
	DFU_PRINTF("Size: %x\n",header_original_fw.size);

	// Read header of current FW in internal-flash
	flash_read_page(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER, sizeof(fw_header_t), (uint8_t *)&header_current_fw);
	DFU_PRINTF("Current FW ver: %x.%x.%x\n",header_current_fw.major,header_current_fw.minor,header_current_fw.patch);
	DFU_PRINTF("Size: %x\n",header_current_fw.size);

	// Check original FW
	if(HVP(&header_original_fw) == HEADER_EMPTY) // No original FW
	{
		if(check_valid_current_fw() == 1)
		{
			// Put current FW into Original FW region
			put_fw_into_ex_flash(ORIGINAL_FW_ADDRESS);
		}
	}

	// Check OTA FW
	if(HVP(&header_ota_fw) != HEADER_EMPTY)
	{
		if(HVP(&header_current_fw) != HEADER_EMPTY)
		{
//			if(HVP(&header_ota_fw) > HVP(&header_current_fw))
			{
				DFU_PRINTF("OTA FW\n");
				// Copy FW from OTA FW to current FW
				fw_copy(&header_ota_fw,OTA_FW_ADDRESS);
			}
		}
	}

	// Check current FW
	if(HVP(&header_current_fw) == HEADER_EMPTY)
	{
		// First time start or no current FW
		if(check_valid_current_fw() == 1)
		{
			// Erase header page before write new header
			flash_read_mid();
			flash_unlock_mid146085();
			flash_erase_sector(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER);

			// Write Application current FW header
			header_current_fw.major = 0;
			header_current_fw.minor = 0;
			header_current_fw.patch = 1;
			header_current_fw.size  = 0x40000;
			memcpy(header_current_fw.crc128,crc128,CRC128_LENGTH);
			flash_write_page(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER, sizeof(header_current_fw), (uint8_t *)&header_current_fw);

			jump_to_application();
		}
		else
		{
			if(HVP(&header_ota_fw) != HEADER_EMPTY)
			{
				DFU_PRINTF("OTA FW\n");
				fw_copy(&header_ota_fw,OTA_FW_ADDRESS);
			}
			else
			{
				if(HVP(&header_original_fw) != HEADER_EMPTY) // No current FW and OTA FW
				{
					DFU_PRINTF("ORIGINAL FW\n");
					fw_copy(&header_original_fw,ORIGINAL_FW_ADDRESS);
				}
			}
		}
	}
	else
	{
		jump_to_application();
	}

//	put_fw_into_ex_flash(OTA_FW_ADDRESS);
}


/******************************************OTA Region******************************************/
/**
* @brief: init for ota
* @param: see below
*/
void ota_init(void)
{
	nvm_status_t ret;
	int i;

	nvm_init();
	// Load OTA memory map
	ret = nvm_record_read(OTA_MEMORY_MAP,(uint8_t*)ota_map,sizeof(ota_map));
	if(ret == NVM_NO_RECORD)
	{
		memset(ota_map,0x00,sizeof(ota_map));
		nvm_record_write(OTA_MEMORY_MAP,(uint8_t*)ota_map,sizeof(ota_map));
	}

	printf("OTA MAP: ");
	for(i=0;i<sizeof(ota_map);i++)
	{
		printf("%x ",ota_map[i]);
	}
	printf("\n");
}

/**
* @brief: check crc of ota packet
* @param: see below
*/
uint8_t ota_packet_crc(uint8_t *pdata)
{
	uint8_t crc = 0;
	uint8_t	i = 0;

	for(i = 0; i < 22; i++)
	{
		crc = (crc + pdata[i]);
	}

	return crc;
}

/**
* @brief: Put data into OTA region
* @param: see below
* -------------------------------------------------------------------------
* |                           Packet BEGIN/END                            |
* -------------------------------------------------------------------------
* | Packet type | Device type | Version |      Size      |   Signature    |
* -------------------------------------------------------------------------
* |    1 byte   |    1 byte   | 1 byte  |     3 bytes    |    16 bytes    |
* -------------------------------------------------------------------------
* -------------------------------------------------------------------------
* |                      22B OTA Packet DATA                              |
* -------------------------------------------------------------------------
* | Packet type | Device type | Version | Memory address |      Data      |
* -------------------------------------------------------------------------
* |    1 byte   |    1 byte   | 1 byte  |     3 bytes    |    16 bytes    |
* -------------------------------------------------------------------------
* @retval: ota_ret_t
*/
ota_ret_t ota_fw_put(uint8_t *pdata, uint8_t crc)
{
	ota_packet_type_t	packet_type;
	ota_device_type_t 	device_type;
	uint8_t 			version;
	uint32_t 			memory_addr;
	ota_fw_header_t		packet_header;
	fw_header_t			ota_header;
	uint32_t			i,j;
	uint32_t			sector;
	uint8_t				slot;

	packet_type = pdata[0];

	if(ota_packet_crc(pdata) == crc)
	{
		if(packet_type == OTA_PACKET_BEGIN)
		{
			packet_header.state = OTA_FW_STATE_EMPTY;
			packet_header.type = pdata[1];
			packet_header.version = pdata[2];
			packet_header.size = (uint32_t)pdata[3] + (uint32_t)(pdata[4] << 8) + (uint32_t)(pdata[5] << 16);
			memcpy((uint8_t*)packet_header.signature,(uint8_t*)&pdata[6],OTA_PACKET_LENGTH);
			ota_packet_header_set(&packet_header);
			DFU_PRINTF("OTA Begin\n");
			return OTA_RET_OK;
		}
		else if(packet_type == OTA_PACKET_END)
		{
			ota_packet_header_get(&packet_header);
			if((packet_header.state == OTA_FW_STATE_EMPTY) || (packet_header.state == OTA_FW_STATE_WRITING))
			{
				packet_header.state = OTA_FW_STATE_COMPLETE;
				packet_header.type = pdata[1];
				packet_header.version = pdata[2];
				packet_header.size = (uint32_t)pdata[3] + (uint32_t)(pdata[4] << 8) + (uint32_t)(pdata[5] << 16);
				memcpy((uint8_t*)packet_header.signature,(uint8_t*)&pdata[6],OTA_PACKET_LENGTH);
				ota_packet_header_set(&packet_header);

				// Write OTA header
				ota_header.major = 0;
				ota_header.minor = 0;
				ota_header.patch = packet_header.version;
				ota_header.size  = packet_header.size;
				memcpy(ota_header.crc128,(uint8_t*)packet_header.signature,OTA_PACKET_LENGTH);

				DFU_PRINTF("Signature: ");
				for(i = 0; i < CRC128_LENGTH; i++)
				{
					printf("%x ",ota_header.crc128[i]);
				}
				printf("\n");

				// Erase OTA Header sector before write
				FLASH_Erase_Sector(OTA_FW_HEADER);
				// Write OTA Header
				W25XXX_WR_Block((uint8_t *)&ota_header,OTA_FW_HEADER,sizeof(ota_header));
				// Write OTA Map
				memset(ota_map,0xFF,sizeof(ota_map));
				nvm_record_write(OTA_MEMORY_MAP,(uint8_t*)ota_map,sizeof(ota_map));

				DFU_PRINTF("OTA End\n");
				return OTA_RET_OK;
			}
		}
		else if(packet_type == OTA_PACKET_DATA)
		{
			device_type = pdata[1];
			version		= pdata[2];
			memory_addr	= (uint32_t)pdata[3] + (uint32_t)(pdata[4] << 8) + (uint32_t)(pdata[5] << 16);

			ota_packet_header_get(&packet_header);
			if((packet_header.state == OTA_FW_STATE_EMPTY) || (packet_header.state == OTA_FW_STATE_WRITING))
			{
				if((device_type == packet_header.type) && (version == packet_header.version))
				{
					sector = memory_addr/0x1000;
					// Check sector is available to write
					for(i = 0; i < sizeof(ota_map); i++)
					{
						slot = ota_map[i];
						for(j = 0; j < 8; j++)
						{
							if(sector == (j + i*8))
							{
								if(((0x01 << j) & slot) != 0)
								{
									// clear sector position in ota_map and erase sector
									ota_map[i] &= (~(0x01 << j));
									FLASH_Erase_Sector(OTA_FW_ADDRESS +  sector*DEF_UDISK_SECTOR_SIZE);
									DFU_PRINTF("Erase sector: %d\n",sector);
								}
							}
						}
					}
					memory_addr = OTA_FW_ADDRESS + memory_addr;
					if(ota_packet_crc(pdata) != crc)
					{
						return OTA_RET_ERROR;
					}
					W25XXX_WR_Block((uint8_t*)&pdata[6],memory_addr,OTA_PACKET_LENGTH);
	//				DFU_PRINTF("Write addr: %x\n",memory_addr);
					return OTA_RET_OK;
				}
			}
		}
	}
	return OTA_RET_ERROR;
}

ota_ret_t ota_packet_header_set(ota_fw_header_t *header)
{
	nvm_status_t nvm_ret;

	nvm_ret = nvm_record_write(OTA_FW_HEADER_KEY,(uint8_t*)header,sizeof(ota_fw_header_t));
	if(nvm_ret == NVM_OK)
	{
		return OTA_RET_OK;
	}
	return OTA_RET_ERROR;
}

ota_ret_t ota_packet_header_get(ota_fw_header_t *header)
{
	nvm_status_t nvm_ret;

	nvm_ret = nvm_record_read(OTA_FW_HEADER_KEY,(uint8_t*)header,sizeof(ota_fw_header_t));
	if(nvm_ret == NVM_OK)
	{
		return OTA_RET_OK;
	}
	return OTA_RET_ERROR;
}

/**
* @brief: set current FW verison
* @param: see below
*/
uint8_t set_current_fw_version(uint8_t fw_patch)
{
	fw_header_t header_current_fw;

	flash_read_page(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER, sizeof(fw_header_t), (uint8_t *)&header_current_fw);

	// Only set version if it is not set
	if((header_current_fw.major == 0xFF) && (header_current_fw.minor == 0xFF) && (header_current_fw.patch == 0xFF))
	{
		// Erase header page before write new header
		flash_read_mid();
		flash_unlock_mid146085();
		flash_erase_sector(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER);

		// Write Application current FW header
		header_current_fw.major = 0; // Set default = 1 due to only use 1 byte version
		header_current_fw.minor = 0; // Set default = 1 due to only use 1 byte version
		header_current_fw.patch = fw_patch;
		header_current_fw.size  = 0x40000; // Set default = 0x40000
		flash_write_page(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER, sizeof(header_current_fw), (uint8_t *)&header_current_fw);
	}
}

/**
* @brief: Get current FW version, due to OTA only send 1 byte version so return only "patch" of version
* @param: see below
*/
uint8_t get_current_fw_version(void)
{
	fw_header_t header_current_fw;

	// Read header of current FW in internal-flash
	flash_read_page(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER, sizeof(fw_header_t), (uint8_t *)&header_current_fw);
	DFU_PRINTF("Current FW ver: %x.%x.%x\n",header_current_fw.major,header_current_fw.minor,header_current_fw.patch);
	DFU_PRINTF("Size: %x\n",header_current_fw.size);
	return header_current_fw.patch;
}


void test_ota(void)
{
	uint32_t 	i,j;
	uint8_t		buff[1024];
	uint8_t		packet[22];
	uint32_t	address;
	uint32_t	image_size = 0x40000;//APP_IMAGE_SIZE_MAX;
//	uint32_t	image_size = 0x1000;//APP_IMAGE_SIZE_MAX;
	ota_ret_t	ret;
	uint8_t		crc = 0;


	// put packet begin
	packet[0] = OTA_PACKET_BEGIN; 			// packet begin
	packet[1] = 0;							// device type
	packet[2] = 2;							// version
	packet[3] = (uint8_t)image_size;		// FW size
	packet[4] = (uint8_t)(image_size>>8);	// FW size
	packet[5] = (uint8_t)(image_size>>16);	// FW size
	ret = ota_fw_put(packet,crc);
	DFU_PRINTF("Begin: %d\n",ret);

	flash_read_mid();
	flash_unlock_mid146085();
	crc128_init();

	// put packet data
	packet[0] = OTA_PACKET_DATA; 			// packet data
	packet[1] = 0;							// device type
	packet[2] = 2;							// version
	address = 0;
	for(i = 0; i < (image_size/sizeof(buff)); i++)
	{
		flash_read_page(FLASH_R_BASE_ADDR + APP_IMAGE_ADDR + i*sizeof(buff), sizeof(buff), (uint8_t *)buff);
		for(j = 0; j < (sizeof(buff)/OTA_PACKET_LENGTH); j++)
		{
			packet[3] = (uint8_t)address;		// address
			packet[4] = (uint8_t)(address>>8);	// address
			packet[5] = (uint8_t)(address>>16);	// address
			memcpy(&packet[6],&buff[j*OTA_PACKET_LENGTH],OTA_PACKET_LENGTH);
			ret = ota_fw_put(packet,crc);
			address += OTA_PACKET_LENGTH;
		}
		// calculate crc128
		for(j = 0; j < (sizeof(buff)/CRC128_LENGTH); j++)
		{
			crc128_calculate(&buff[j*CRC128_LENGTH]);
		}
		DFU_PRINTF("Copy: %d%% - %d\n",(((i+1)*100)/(image_size/sizeof(buff))),ret);
	}

	// put packet begin
	packet[0] = OTA_PACKET_END; 			// packet begin
	packet[1] = 0;							// device type
	packet[2] = 2;							// version
	packet[3] = (uint8_t)image_size;		// FW size
	packet[4] = (uint8_t)(image_size>>8);	// FW size
	packet[5] = (uint8_t)(image_size>>16);	// FW size
	memcpy(&packet[6],crc128,OTA_PACKET_LENGTH);
	ret = ota_fw_put(packet,crc);
	DFU_PRINTF("End: %d\n",ret);
}
