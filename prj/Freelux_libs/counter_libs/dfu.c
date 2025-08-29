#include "dfu.h"

void jump_to_application(void)
{
	core_interrupt_disable();
	JUMP_TO_APP();
}

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

void calculate_crc128(uint8_t *iv, uint8_t *crc128)
{
	uint8_t i;

	for(i = 0; i < 16; i++)
	{
		iv[i] = (iv[i] ^ crc128[i]);
	}
}

void firmware_header_check(void)
{
//	flash_read_mid();
//	flash_unlock_mid146085();
//	flash_erase_sector(APP_IMAGE_ADDR + i*APP_PAGE_SIZE);
//
//	flash_read_page(FLASH_R_BASE_ADDR + APP_IMAGE_ADDR + FLASH_TLNK_FLAG_OFFSET, 1, buf);
//	flash_write_page(APP_IMAGE_ADDR + program_index, len, pdata);


	// Check valid F
	fw_header_t header_current_fw;
	fw_header_t header_ota_fw;
	fw_header_t	header = {0};

	// Init ex-flash
	FLASH_Port_Init();
	// Read header of OTA FW in ex-flash
	W25XXX_Read((uint8_t *)&header_ota_fw,OTA_FW_HEADER,sizeof(fw_header_t));
	LOGA(APP,"Version EX: %x\n",header_ota_fw.version);
	LOGA(APP,"Size EX: %x\n",header_ota_fw.size);
	// Read header of current FW in internal-flash
	flash_read_page(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER, sizeof(fw_header_t), (uint8_t *)&header_current_fw);
	LOGA(APP,"Version IN: %x\n",header_current_fw.version);
	LOGA(APP,"Size IN: %x\n",header_current_fw.size);


	if(header_current_fw.version == 0xFFFFFFFF)
	{
		// First time start or no current FW
		if(check_valid_current_fw() == 1)
		{
			// Write the original FW with version
			flash_read_mid();
			flash_unlock_mid146085();
			flash_write_page(FLASH_R_BASE_ADDR + APP_IMAGE_HEADER, sizeof(fw_header_t), (uint8_t *)&header);
		}
	}

	if(header_ota_fw.version != 0xFFFFFFFF)
	{
		if(header_current_fw.version != 0xFFFFFFFF)
		{
			if(header_ota_fw.version > header_current_fw.version)
			{
				// Copy FW from OTA FW to current FW
			}
		}
		else
		{
			// Copy FW from OTA FW to current FW
		}
	}
}
