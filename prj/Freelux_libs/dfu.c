#include "dfu.h"

/* Definition */
//#define DFU_DEBUG
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

	if(memcmp(crc128,header->crc128,CRC128_LENGTH) == 0)
	{
		DFU_PRINTF("OTA FW CRC128 is valid\n");
	}
	else
	{
		DFU_PRINTF("OTA FW CRC128 is invalid\n");
		// Erase OTA FW header if the CRC128 is wrong
		FLASH_Erase_Sector(OTA_FW_HEADER);
		return;
	}

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
//		disorder_data(buff);
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
		FLASH_Erase_Sector(OTA_FW_HEADER);
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
	set_dfu_version();

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
			if((HVP(&header_ota_fw) > HVP(&header_current_fw)) || (header_ota_fw.force == OTA_FORCE_UPDATE_VALUE))
			{
				DFU_PRINTF("OTA FW\n");
				// Erase force update
				if(header_ota_fw.force == OTA_FORCE_UPDATE_VALUE)
				{
					header_ota_fw.force = 0x00000000;
					// Erase OTA Header sector before write
					FLASH_Erase_Sector(OTA_FW_HEADER);
					// Write OTA Header
					W25XXX_WR_Block((uint8_t *)&header_ota_fw,OTA_FW_HEADER,sizeof(header_ota_fw));
				}
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
//			// Erase header page before write new header
//			flash_read_mid();
//			flash_unlock_mid146085();
//			flash_erase_sector(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER);
//
//			// Write Application current FW header
//			header_current_fw.major = 0;
//			header_current_fw.minor = 0;
//			header_current_fw.patch = 1;
//			header_current_fw.size  = 0x40000;
//			memcpy(header_current_fw.crc128,crc128,CRC128_LENGTH);
//			flash_write_page(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER, sizeof(header_current_fw), (uint8_t *)&header_current_fw);

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
				ota_header.force = OTA_FORCE_UPDATE_VALUE;
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

///**
//* @brief: set current FW version
//* @param: see below
//*/
//uint8_t set_current_fw_version(uint8_t fw_patch)
//{
//	fw_header_t header_current_fw;
//	flash_read_page(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER, sizeof(fw_header_t), (uint8_t *)&header_current_fw);
//	// Only set version if it is not set
//	if((header_current_fw.major == 0xFF) && (header_current_fw.minor == 0xFF) && (header_current_fw.patch == 0xFF))
//	{
//		// Erase header page before write new header
//		flash_read_mid();
//		flash_unlock_mid146085();
//		flash_erase_sector(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER);
//
//		// Write Application current FW header
//		header_current_fw.major = 0; // Set default = 1 due to only use 1 byte version
//		header_current_fw.minor = 0; // Set default = 1 due to only use 1 byte version
//		header_current_fw.patch = fw_patch;
//		header_current_fw.size  = 0x40000; // Set default = 0x40000
//		flash_write_page(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER, sizeof(header_current_fw), (uint8_t *)&header_current_fw);
//		return 1;
//	}
//	return 0;
//}
//
///**
//* @brief: Get current FW version, due to OTA only send 1 byte version so return only "patch" of version
//* @param: see below
//*/
//uint8_t get_current_fw_version(void)
//{
//	fw_header_t header_current_fw;
//
//	// Read header of current FW in internal-flash
//	flash_read_page(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER, sizeof(fw_header_t), (uint8_t *)&header_current_fw);
//	DFU_PRINTF("Current FW ver: %x.%x.%x\n",header_current_fw.major,header_current_fw.minor,header_current_fw.patch);
//	DFU_PRINTF("Size: %x\n",header_current_fw.size);
//	return header_current_fw.patch;
//}

/**
* @brief: set current FW version
* @param: see below
*/
uint8_t set_current_fw_version(fw_header_t *fw_header)
{
	fw_header_t header_current_fw;
	flash_read_page(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER, sizeof(fw_header_t), (uint8_t *)&header_current_fw);
	// Only set version if it is not set
//	if((header_current_fw.major == 0xFF) && (header_current_fw.minor == 0xFF) && (header_current_fw.patch == 0xFF))
	if((header_current_fw.major != fw_header->major) || (header_current_fw.minor != fw_header->minor) || (header_current_fw.patch != fw_header->patch))
	{
		// Erase header page before write new header
		flash_read_mid();
		flash_unlock_mid146085();
		flash_erase_sector(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER);

		// Write Application current FW header
		header_current_fw.major = fw_header->major; // Set default = 1 due to only use 1 byte version
		header_current_fw.minor = fw_header->minor; // Set default = 1 due to only use 1 byte version
		header_current_fw.patch = fw_header->patch;
		flash_write_page(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER, sizeof(header_current_fw), (uint8_t *)&header_current_fw);
		return 1;
	}
	return 0;
}

/**
* @brief: Get current FW version, due to OTA only send 1 byte version so return only "patch" of version
* @param: see below
*/
void get_current_fw_version(fw_header_t *fw_header)
{
	// Read header of current FW in internal-flash
	flash_read_page(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER, sizeof(fw_header_t), (uint8_t *)fw_header);
	DFU_PRINTF("Current FW ver: %x.%x.%x\n",fw_header->major,fw_header->minor,fw_header->patch);
	DFU_PRINTF("Size: %x\n",fw_header->size);
}

/**
* @brief: set DFU version
* @param: see below
*/
uint8_t set_dfu_version(void)
{
	fw_header_t header_dfu;
	flash_read_page(FLASH_R_BASE_ADDR + DFU_HEADER, sizeof(fw_header_t), (uint8_t *)&header_dfu);
	// Only set version if it is not set
	if(header_dfu.patch != DFU_VERSION)
	{
		// Erase header page before write new header
		flash_read_mid();
		flash_unlock_mid146085();
		flash_erase_sector(FLASH_R_BASE_ADDR + DFU_HEADER);

		// Write Application current FW header
		header_dfu.major = 0; 				// Set default = 1 due to only use 1 byte version
		header_dfu.minor = 0; 				// Set default = 1 due to only use 1 byte version
		header_dfu.patch = DFU_VERSION;
		header_dfu.size  = 0xE000; 			// Set default = 0xE000
		flash_write_page(FLASH_R_BASE_ADDR + DFU_HEADER, sizeof(header_dfu), (uint8_t *)&header_dfu);
		return 1;
	}
	return 0;
}

/**
* @brief: Get DFU version
* @param: see below
*/
uint8_t get_dfu_version(void)
{
	fw_header_t header_dfu;

	// Read header of DFU in internal-flash
	flash_read_page(FLASH_R_BASE_ADDR + DFU_HEADER, sizeof(fw_header_t), (uint8_t *)&header_dfu);
	DFU_PRINTF("Current FW ver: %x.%x.%x\n",header_dfu.major,header_dfu.minor,header_dfu.patch);
	DFU_PRINTF("Size: %x\n",header_dfu.size);
	return header_dfu.patch;
}

/****************************************** For UART DFU ******************************************/

#include "os_queue.h"
#define BOOTLOADER_VERSION "1.0.0"
#define MAGIC_WORDS	"IOTBOOTLOADERUART"

#define	TIMEOUT_MAX					100000
#define	RESET_QUEUE_TIMEOUT_MAX		500000
#define	DFU_UART_BUFF_SIZE			2

#define SEND_BYTE(data)			uart_send_byte(UART1,data)
#define SEND_BUFF(pdata,len)	uart_send(UART1,pdata,len)

queue_t		dfu_uart_queue;
uint8_t 	dfu_uart_queue_fifo[DFU_UART_BUFF_SIZE];
uint8_t 	uart_rx_addr[DFU_UART_BUFF_SIZE];

command_t	cmd;
uint8_t 	enter_bootloader = 0;
uint32_t 	timeout_jump = TIMEOUT_MAX;
uint32_t 	program_address = FLASH_R_BASE_ADDR + APP_IMAGE_ADDR;
uint32_t 	page, len, crc;
uint64_t 	doubleword;
uint32_t	baudrate = 115200;

static void uart1_init(uart_num_e uart_num, uart_tx_pin_e tx_pin, uart_rx_pin_e rx_pin, u32 baudrate)
{
	unsigned short div;
	unsigned char bwpc;

	uart_reset(uart_num);
	uart_set_pin(tx_pin,rx_pin); // uart tx/rx pin set
	uart_cal_div_and_bwpc(baudrate,sys_clk.pclk * 1000 * 1000,&div,&bwpc);
	LOGA(APP,"div: %d, bwpc: %d, pclk: %d\n",div,bwpc,sys_clk.pclk);
	uart_set_rx_timeout(uart_num,bwpc,12,UART_BW_MUL1);
	uart_init(uart_num,div,bwpc,UART_PARITY_NONE,UART_STOP_BIT_ONE);

	uart_clr_irq_mask(uart_num,UART_ERR_IRQ_MASK|UART_RX_IRQ_MASK | UART_TX_IRQ_MASK | UART_TXDONE_MASK | UART_RXDONE_MASK);


	uart_set_tx_dma_config(uart_num,DMA2);
	uart_set_rx_dma_config(uart_num,DMA3);

	uart_clr_tx_done(uart_num);
	uart_set_irq_mask(uart_num,UART_ERR_IRQ_MASK|UART_RX_IRQ_MASK | UART_TXDONE_MASK | UART_RXDONE_MASK);

	core_interrupt_enable();
	plic_interrupt_enable(IRQ18_UART1);

	uart_receive_dma(uart_num,(unsigned char *)uart_rx_addr,sizeof(uart_rx_addr));
}

static void send_cmd_ping(void)
{
	// Send Header
	SEND_BYTE(0x55);
	SEND_BYTE(0xAA);
	// Send Length = 0
	SEND_BYTE(0x00);
	// Send PING CMD
	SEND_BYTE(CMD_PING);
}

/**
* @brief: Initialize UART DFU
* @param: see below
*/
void dfu_uart_init(void)
{
	uart1_init(UART1,UART1_TX_PE0,UART1_RX_PE2,115200);
	queue_create(&dfu_uart_queue, dfu_uart_queue_fifo, sizeof(dfu_uart_queue_fifo));
	send_cmd_ping();
}

/**
* @brief: receive data from interrupt
* @param: see below
*/
void dfu_uart_receive(void)
{
	uint16_t len;

	len = uart_get_dma_rev_data_len(UART1,DMA3);
	uart_receive_dma(UART1,(unsigned char *)uart_rx_addr,sizeof(uart_rx_addr));
	queue_put(&dfu_uart_queue, uart_rx_addr, len);
}

static void wait_for_timeout(void)
{
	if(enter_bootloader == 0)
	{
		if((timeout_jump--) == 0)
		{
//			printf("DFU timeout\n");
			uart_reset(UART1);
			firmware_check();
			enter_bootloader = 1;
		}
	}
}

static uint8_t crc8(uint8_t *pdata, uint16_t len)
{
	uint32_t 	i;
	uint8_t		crc = 0;

	for(i = 0; i < len; i++)
	{
		crc += pdata[i];
	}

	return crc;
}

static void response_ack(cmt_t ack)
{
	// Send Header
	SEND_BYTE(0x55);
	SEND_BYTE(0xAA);
	// Send Length = 0
	SEND_BYTE(0x00);
	// Send ACK CMD
	if(ack == CMD_ACK_OK) SEND_BYTE(CMD_ACK_OK);
	else SEND_BYTE(CMD_ACK_ERROR);
}

/**
* @brief: process UART DFU
* @param: see below
* -----------------------VERSION 1--------------------------
* [ 0x55    0xAA ][       Length        ][  CMD  ][  Data  ]
* [2 Bytes header][1 Byte length of Data][1 Byte ][n Bytes ]
* -----------------------VERSION 2------------------------------------
* [ 0x56    0xAA ][       Length         ][  CMD  ][  Data  ][  CRC  ]
* [2 Bytes header][2 Bytes length of Data][1 Byte ][n Bytes ][1 Byte ]
*/
void dfu_uart_process(void)
{
	uint32_t		len = 0;
//	uint8_t			buff[DFU_UART_BUFF_SIZE];
//	static uint32_t	buff_cnt = 0;
	uint8_t			*fifo;
//	static uint32_t reset_queue_timeout = 0;
//	static uint32_t	wrong_len_count = 0;
	uint8_t			crc = 0;

	len = queue_available_data(&dfu_uart_queue);
	if(len > 0) // minimum of frame length
	{
		fifo = dfu_uart_queue_fifo;
		if((fifo[0] == 0x55) && (fifo[1] == 0xAA))
		{
			if(len > 4)
			{
				memcpy((uint8_t*)&cmd.len,&fifo[2],sizeof(uint16_t));
				if(len >= (cmd.len + 6))
				{
					cmd.cmd		= fifo[4];
					cmd.data	= &fifo[5];
					cmd.crc		= fifo[cmd.len + 5];
					crc = crc8(cmd.data,cmd.len);
//					printf("cmd.len: %d %d crc: %x %x cmd: %d\n",cmd.len,len,crc,cmd.crc,cmd.cmd);
					if( crc == cmd.crc)
					{
						cmd_process();
					}
					else
					{
//						printf("Wrong crc\n");
						response_ack(CMD_ACK_ERROR);
					}
					// Re-init queue to clear queue data
					queue_create(&dfu_uart_queue, dfu_uart_queue_fifo, sizeof(dfu_uart_queue_fifo));
				}
			}
		}
		else
		{
//			printf("Wrong frame: %d\n",len);
			// Re-init queue to clear queue data
			queue_create(&dfu_uart_queue, dfu_uart_queue_fifo, sizeof(dfu_uart_queue_fifo));
		}
	}

	wait_for_timeout();
}

static void send_version(void)
{
	uint8_t length;

	length = sizeof(BOOTLOADER_VERSION) - 1;
	// Send Header
	SEND_BYTE(0x55);
	SEND_BYTE(0xAA);
	// Send Length
	SEND_BYTE(length);
	// Send CMD_BOOTLOADER_VERSION
	SEND_BYTE(CMD_BOOTLOADER_VERSION);
	// Send Version
	SEND_BUFF((unsigned char *)BOOTLOADER_VERSION, length);
}

static void send_read_word(void)
{
	uint32_t *p;
	// Send Header
	SEND_BYTE(0x55);
	SEND_BYTE(0xAA);
	// Send Length
	SEND_BYTE(0x04);
	// Send CMD_BOOTLOADER_VERSION
	SEND_BYTE(CMD_READ_WORD);
	// Send Version
	p = (uint32_t*)program_address;
	SEND_BUFF((unsigned char *)p, sizeof(uint32_t));
}

static void full_erase_application_region(void)
{
	uint32_t i;

	// Erase running region to copy OTA FW
	flash_read_mid();
	flash_unlock_mid146085();
	DFU_PRINTF("Erase Running region\n");
	for(i = 0; i < (APP_IMAGE_SIZE_MAX/APP_PAGE_SIZE); i++)
	{
		flash_erase_sector(FLASH_R_BASE_ADDR + APP_IMAGE_ADDR + i*APP_PAGE_SIZE);
	}
	response_ack(CMD_ACK_OK);
}

static void erase_page(uint32_t page)
{
	if(page > 15) // 16 first pages for boot loader region
	{
		flash_erase_sector(FLASH_R_BASE_ADDR + APP_IMAGE_ADDR + page*APP_PAGE_SIZE);
	}
}

static void program_with_address(void)
{
	flash_write_page(program_address, sizeof(doubleword), (uint8_t *)&doubleword);
}

static void program_array(uint8_t *array, uint16_t len)
{
	flash_write_page(program_address, len, (uint8_t *)array);
	program_address += len;
	response_ack(CMD_ACK_OK);
}

static void verify(uint32_t address, uint32_t len, uint32_t crc)
{
	uint32_t calculate_crc = 0xFFFFFFFF;
	uint32_t *p;

	p = (uint32_t*)address;
	len = len/4; // convert length of  byte to length of word
	for(int i=0;i<len;i++)
	{
		calculate_crc = (calculate_crc ^ p[i]);
	}
	if(calculate_crc == crc)
	{
		response_ack(CMD_ACK_OK);
		// Erase header page of current FW
		flash_erase_sector(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER);
		uart_reset(UART1); // Reset uart1 before jump to application
		jump_to_application();
	}
	else
	{
		response_ack(CMD_ACK_ERROR);
	}
//	printf("calculate_crc: %x - %x\n",calculate_crc,crc);
}

/**
* @brief: process commands
* @param: see below
*/
void cmd_process(void)
{
	switch(cmd.cmd)
	{
		case CMD_IDLE:
			/* [1 Byte CMD] */
			break;
		case CMD_ENTER_BOOTLOADER:
			/* [1 Byte CMD] + [MAGIC WORDS] */
			if(memcmp(cmd.data,MAGIC_WORDS,sizeof(MAGIC_WORDS) - 1) == 0)
			{
				enter_bootloader = 1;
				flash_read_mid();
				flash_unlock_mid146085();
				response_ack(CMD_ACK_OK);
			}
			break;
		case CMD_MCU_RESET:
			/* [1 Byte CMD] */
			sys_reboot();
			break;
		case CMD_ERASE:
			/* [1 Byte CMD] + [4 Bytes Page] */
			memcpy((uint8_t*)&page,(uint8_t*)cmd.data,sizeof(page));
			erase_page(page);
			break;
		case CMD_FULL_ERASE:
			/* [1 Byte CMD]*/
			full_erase_application_region();
			break;
		case CMD_SET_ADDRESS:
			/* [1 Byte CMD] + [4 Byte Address] */
//			memcpy((uint8_t*)&program_address,(uint8_t*)cmd.data,sizeof(program_address));
			program_address = FLASH_R_BASE_ADDR + APP_IMAGE_ADDR; // Set default program address
			break;
		case CMD_PROGRAM_DOUBLEWORD:
			/* [1 Byte CMD] + [8 Byte double word] */
			program_array(cmd.data,8);
			break;
		case CMD_PROGRAM_WITH_ADDRESS:
			/* [1 Byte CMD] + [4 Byte Address] + [4 Byte double word] */
			memcpy((uint8_t*)&program_address,(uint8_t*)cmd.data,sizeof(program_address)); // Get address first
			memcpy((uint8_t*)&doubleword,(uint8_t*)(cmd.data + sizeof(program_address)),sizeof(doubleword)); // Then get doubleword next to address
			program_with_address();
			break;
		case CMD_PROGRAM_ARRAY:
			/* [1 Byte CMD] + [n Byte Data] */
			program_array(cmd.data,cmd.len);
			break;
		case CMD_JUMP_APPLICATION:
			/* [1 Byte CMD] */
			jump_to_application();
			break;
		case CMD_VERIFY:
			/* [1 Byte CMD] + [4 Byte Address] + [4 Byte Length] + [4 Byte CRC] */
//			memcpy((uint8_t*)&program_address,(uint8_t*)cmd.data,sizeof(program_address)); 										// Get address first
			program_address = FLASH_R_BASE_ADDR + APP_IMAGE_ADDR; // Set default program address
			memcpy((uint8_t*)&len,(uint8_t*)(cmd.data + sizeof(program_address)),sizeof(len)); 								// Get length next to address
			memcpy((uint8_t*)&crc,(uint8_t*)(cmd.data + sizeof(program_address) + sizeof(len)),sizeof(crc)); 	// Get CRC next to length
			verify(program_address, len, crc);
			break;
		case CMD_BOOTLOADER_VERSION:
			/* [1 Byte CMD] */
			send_version();
			break;
		case CMD_READ_WORD:
			/* [1 Byte CMD] */
			send_read_word();
			break;
		case CMD_CHANGE_BAUDRATE:
			/* [1 Byte CMD] + [4 Bytes Baud rate] */
			memcpy((uint8_t*)&baudrate,(uint8_t*)cmd.data,sizeof(baudrate)); // Get baud rate
			response_ack(CMD_ACK_OK);
			delay_ms(100);
			uart_reset(UART1);
			uart1_init(UART1,UART1_TX_PE0,UART1_RX_PE2,baudrate);
//			printf("baudrate: %d\n",baudrate);
			break;
		default:
			break;
	}
}
