#ifndef __DFU_H__
#define __DFU_H__

#include "tl_common.h"
#include "SPI_FLASH.h"

/* B91 Memory map
 *
 *     0x20080000 |------------------|
 *                |     User DATA    |
 *     0x20060000 |------------------|
 *                |     Firmware     |
 *     0x20010000 |------------------|
 *                |     Firmware     |
 *                |      Header      |
 *     0x2000F000 |------------------|
 *                |       Boot       |
 *                |      Loader      |
 *     0x20000000 |------------------|
 * */

/* Definition */

#define FLASH_R_BASE_ADDR   			0x20000000
#define APP_IMAGE_HEADER				0xF000
#define APP_IMAGE_ADDR					0x10000
#define APP_IMAGE_SIZE_MAX				0x50000
#define APP_PAGE_SIZE					0x1000
#define FLASH_TLNK_FLAG_OFFSET			32
#define FW_START_UP_FLAG				0x4B
#define DFU_BUTTON						GPIO_PE4

// CRC128
#define CRC128_LENGTH					16
// OTA FW header
#define OTA_FW_HEADER					EX_FLASH_OTA_FW_ADDRESS
#define OTA_FW_HEADER_SIZE_MAX			0x1000
#define OTA_FW_ADDRESS					(OTA_FW_HEADER + OTA_FW_HEADER_SIZE_MAX)

#define ORIGINAL_FW_HEADER				EX_FLASH_ORIGINAL_FW_ADDRESS
#define ORIGINAL_FW_HEADER_SIZE_MAX		0x1000
#define ORIGINAL_FW_ADDRESS				(ORIGINAL_FW_HEADER + ORIGINAL_FW_HEADER_SIZE_MAX)

#define JUMP_TO_APP()					((void(*)(void))(FLASH_R_BASE_ADDR + APP_IMAGE_ADDR))()

/* Structures */

typedef struct
{
//	uint32_t 	version;
	uint8_t 	major;
	uint8_t 	minor;
	uint8_t 	patch;
	uint32_t 	size;
	uint8_t		crc128[CRC128_LENGTH];
}fw_header_t;

/* Prototypes */

void jump_to_application(void);
uint8_t check_valid_current_fw(void);
uint8_t check_valid_ota_fw(void);
void crc128_init(void);
void crc128_calculate(uint8_t *pdata);
uint32_t header_version_parse(fw_header_t *header);
void fw_copy(fw_header_t *header, uint32_t fw_addr);
void put_fw_into_ex_flash(uint32_t fw_addr);
void firmware_check(void);

#endif //__DFU_H__
