#ifndef __DFU_H__
#define __DFU_H__

#include "tl_common.h"
#include "SPI_FLASH.h"
#include "nvm.h"

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
	/* For bootloader */
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

	/* For OTA */
	#define OTA_PACKET_LENGTH		16

/* Structures */
	/* For bootloader */
	typedef struct
	{
	//	uint32_t 	version;
		uint8_t 	major;
		uint8_t 	minor;
		uint8_t 	patch;
		uint32_t 	size;
		uint8_t		crc128[CRC128_LENGTH];
	}fw_header_t;

	/* For OTA */
	typedef enum
	{
		OTA_RET_OK = 0,
		OTA_RET_ERROR
	}ota_ret_t;

	typedef enum
	{
		OTA_PACKET_BEGIN = 0,
		OTA_PACKET_DATA,
		OTA_PACKET_END
	}ota_packet_type_t;

	typedef enum
	{
		OTA_DEVICE_GATEWAY = 0,
		OTA_DEVICE_PRODUCT_COUNTER,
		OTA_DEVICE_POWER_METER
	}ota_device_type_t;

	typedef enum
	{
		OTA_FW_STATE_EMPTY = 0,
		OTA_FW_STATE_COMPLETE,
		OTA_FW_STATE_WRITING,
		OTA_FW_STATE_ERROR
	}ota_fw_state_t;

	typedef struct
	{
		ota_fw_state_t		state;
		ota_device_type_t 	type;
		uint8_t				version;
		uint32_t			size;
		uint8_t				signature[OTA_PACKET_LENGTH];
	}ota_fw_header_t;

/* Prototypes */
	/* For bootloader */
	void jump_to_application(void);
	uint8_t check_valid_current_fw(void);
	uint8_t check_valid_ota_fw(uint32_t address);
	void crc128_init(void);
	void crc128_calculate(uint8_t *pdata);
	uint32_t header_version_parse(fw_header_t *header);
	void fw_copy(fw_header_t *header, uint32_t fw_addr);
	void ex_flash_region_erase(uint32_t region);
	void put_fw_into_ex_flash(uint32_t fw_addr);
	void firmware_check(void);

	/* For OTA */
	void ota_init(void);
	uint8_t ota_packet_crc(uint8_t *pdata);
	ota_ret_t ota_fw_put(uint8_t *pdata, uint8_t crc);
	ota_ret_t ota_packet_header_set(ota_fw_header_t *header);
	ota_ret_t ota_packet_header_get(ota_fw_header_t *header);
	void test_ota(void);
#endif //__DFU_H__
