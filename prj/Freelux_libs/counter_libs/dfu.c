#include "dfu.h"

/* Definition */
#define DFU_DEBUG	1
#ifdef DFU_DEBUG
#define DFU_PRINTF(...)	LOGA(APP,__VA_ARGS__);
#else
#define DFU_PRINTF(...)
#endif

#define HEADER_EMPTY	0xFFFFFF
#define HVP(header) 	header_version_parse(header)

/* Variables */

uint8_t crc128[CRC128_LENGTH];

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
uint8_t check_valid_ota_fw(void)
{
	u8 buf[64];

	W25XXX_Read((uint8_t *)buf,OTA_FW_ADDRESS + FLASH_TLNK_FLAG_OFFSET,sizeof(buf));
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
* @brief: copy fw from OTA region to running region
* @param: see below
* @retval: None
*/
void fw_copy(fw_header_t *header, uint32_t fw_addr)
{
	uint32_t 	i,j;
	uint32_t 	size;
	uint8_t		buff[1024];
	fw_header_t	header_app = {0};

	DFU_PRINTF("Copy OTA FW: ver: %d.%d.%d - size: %d\n",header->major,header->minor,header->patch,header->size);
	// Check OTA FW is correct or not
	if(check_valid_ota_fw() == 1)
	{
		DFU_PRINTF("OTA FW is valid\n");
	}
	else
	{
		DFU_PRINTF("OTA FW is invalid\n");
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
		flash_write_page(FLASH_R_BASE_ADDR + APP_IMAGE_ADDR + i*sizeof(buff), sizeof(buff), (uint8_t *)buff);
		DFU_PRINTF("Copy: %d%%\n",(((i+1)*100)/size));
	}
	// Verify new FW
	crc128_init();
	for(i = 0; i < size; i++)
	{
		flash_read_page(FLASH_R_BASE_ADDR + APP_IMAGE_ADDR + i*sizeof(buff), sizeof(buff), (uint8_t *)buff);
		// calculate crc128
		for(j = 0; j < (sizeof(buff)/CRC128_LENGTH); j++)
		{
			crc128_calculate(&buff[j*CRC128_LENGTH]);
		}
		DFU_PRINTF("Verify: %d%%\n",(((i+1)*100)/size));
	}
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
		DFU_PRINTF("CRC128 is incorrect\n");
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
	uint32_t	image_size = 0x18000;//APP_IMAGE_SIZE_MAX;

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
		W25XXX_WR_Block((uint8_t *)buff,fw_addr + i*sizeof(buff),sizeof(buff));
		// calculate crc128
		for(j = 0; j < (sizeof(buff)/CRC128_LENGTH); j++)
		{
			crc128_calculate(&buff[j*CRC128_LENGTH]);
		}
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
	// Check valid F
	fw_header_t header_current_fw;
	fw_header_t header_ota_fw;
	fw_header_t header_original_fw;

	// Init ex-flash
	FLASH_Port_Init();

	// Read header of OTA FW in ex-flash
	W25XXX_Read((uint8_t *)&header_ota_fw,OTA_FW_HEADER,sizeof(fw_header_t));
	DFU_PRINTF("OTA FW Ver: %x.%x.%x\n",header_ota_fw.major,header_ota_fw.minor,header_ota_fw.patch);
	DFU_PRINTF("Size: %x\n",header_ota_fw.size);

	// Read header of ORIGINAL FW in ex-flash
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
			if(HVP(&header_ota_fw) > HVP(&header_current_fw))
			{
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
			jump_to_application();
		}
		else
		{
			if(HVP(&header_ota_fw) != HEADER_EMPTY)
			{
				fw_copy(&header_ota_fw,OTA_FW_ADDRESS);
			}
			else
			{
				if(HVP(&header_original_fw) != HEADER_EMPTY) // No current FW and OTA FW
				{
					fw_copy(&header_ota_fw,ORIGINAL_FW_ADDRESS);
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
