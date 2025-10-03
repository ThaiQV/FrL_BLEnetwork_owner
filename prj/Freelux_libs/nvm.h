#ifndef _NVM_H_
#define _NVM_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "SPI_FLASH.h"

/*-----Definition-----*/
#define NVM_ERASED_KEY_ID				0x0000
#define NVM_UNUSED_KEY_ID				0xFFFF

/* For NVM */
#define NVM_BASE_ADDRESS				EX_FLASH_NVM_ADDRESS
#define NVM_PAGE_SIZE					4096
#define NVM_MAX_SIZE					8192

/* For NVM KEY ID FOR DEVICE WEEKLY STORAGE */
#define STORAGE_MAP						0x0100

#define OTA_FW_HEADER_KEY				0x0200

/*-----Structures-----*/
/*-----Structures-----*/
typedef enum
{
	NVM_OK = 0,
	NVM_ERROR,
	NVM_FULL,
	NVM_NO_RECORD,
	NVM_WRONG_KEY_ID,
}nvm_status_t;

typedef struct
{
	uint16_t	key_id;
	uint16_t	len;
}header_t;

typedef struct
{
	header_t	header;
	uint8_t		*data;
}record_t;


void nvm_init(void);
void nvm_erase(void);
void nvm_write(uint32_t addr, uint8_t *p_data, uint32_t len);
void nvm_read(uint32_t addr, uint8_t *p_data, uint32_t len);
nvm_status_t nvm_record_init(void);
nvm_status_t nvm_record_read(uint16_t key_id, uint8_t *p_data, uint32_t len);
nvm_status_t nvm_record_write(uint16_t key_id, uint8_t *p_data, uint32_t len);

/*-----Prototypes-----*/

#endif
