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

#define FLASH_R_BASE_ADDR   		0x20000000
#define APP_IMAGE_HEADER			0xF000
#define APP_IMAGE_ADDR				0x10000
#define APP_IMAGE_SIZE_MAX			0x50000
#define APP_PAGE_SIZE				0x1000
#define FLASH_TLNK_FLAG_OFFSET		32
#define FW_START_UP_FLAG			0x4B
#define DFU_BUTTON					GPIO_PE4

// OTA FW header
#define OTA_FW_HEADER				0x00080000
#define OTA_FW_HEADER_SIZE_MAX		0x1000
#define OTA_FW_ADDRESS				(0x00080000 + OTA_FW_HEADER_SIZE_MAX)

#define JUMP_TO_APP()				((void(*)(void))(FLASH_R_BASE_ADDR + APP_IMAGE_ADDR))()

/* Structures */

typedef struct
{
	uint32_t 	version;
	uint32_t 	size;
	uint8_t		crc128[16];
}fw_header_t;

/* Prototypes */

void jump_to_application(void);
uint8_t check_valid_current_fw(void);
void firmware_header_check(void);

#endif //__DFU_H__
