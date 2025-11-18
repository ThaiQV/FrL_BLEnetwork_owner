/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: test_fota.c
 *Created on		: Oct 14, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#include "tl_common.h"
#include "fl_wifi2ble_fota.h"
#include "fl_ble_wifi.h"
#include "../Freelux_libs/dfu.h"
#ifdef MASTER_CORE
/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/


/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
u32 VIRTUAL_FW_INDX =0;
u32 VIRTUAL_FW_ADDR = 0;
u32 VIRTUAL_FW_SIZE = 0x2400;//APP_IMAGE_SIZE_MAX
u32 VIRTUAL_FW_NUMOFSENDING = 0;
u8 VIRTUAL_FW_VERSION = 0;
/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/
int _send_fw(void){
	const u32 address_offset =0;
	uint32_t	image_size = VIRTUAL_FW_SIZE;		//APP_IMAGE_SIZE_MAX;
	uint8_t		packet[22];
	uint8_t		buff[OTA_PACKET_LENGTH];
	packet[0] = OTA_PACKET_DATA; 			// packet data
	packet[1] = 0;							// device type
	packet[2] = VIRTUAL_FW_VERSION;				// version
	s16 rslt_send_fota=-1;
//	VIRTUAL_FW_ADDR = 0;
	//for (i = 0; i < (image_size / sizeof(buff)); i++)
	if(VIRTUAL_FW_INDX<(image_size / sizeof(buff)))
	{
		flash_read_page(address_offset+ FLASH_R_BASE_ADDR + VIRTUAL_FW_INDX * sizeof(buff),sizeof(buff),(uint8_t *) buff);
		packet[3] = (uint8_t) VIRTUAL_FW_ADDR;		// address
		packet[4] = (uint8_t) (VIRTUAL_FW_ADDR >> 8);	// address
		packet[5] = (uint8_t) (VIRTUAL_FW_ADDR >> 16);	// address
		memcpy(&packet[6],buff,OTA_PACKET_LENGTH);
		rslt_send_fota = fl_wifi2ble_fota_system_data(packet,sizeof(packet));
		if(-1 == rslt_send_fota)
		{
			return 0;
		}
		else if(rslt_send_fota==FOTA_EXIT_VALUE){
			ERR(APP,"FOTA SUSPEND!!!\r\n");
			return -1;
		}
		VIRTUAL_FW_NUMOFSENDING++;
		VIRTUAL_FW_ADDR += OTA_PACKET_LENGTH;
		// calculate crc128
		for (u8 j = 0; j < (sizeof(buff) / CRC128_LENGTH); j++) {
			crc128_calculate(&buff[j * CRC128_LENGTH]);
		}
//		DFU_PRINTF("Copy: %d%%\n",(((i + 1) * 100) / (image_size / sizeof(buff))));
		VIRTUAL_FW_INDX++;

		P_INFO("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		P_INFO("Downloading:%d/%d",VIRTUAL_FW_INDX*OTA_PACKET_LENGTH,VIRTUAL_FW_SIZE);
//		P_INFO("\rDownloading: %6d / %6d\r", VIRTUAL_FW_INDX * OTA_PACKET_LENGTH, VIRTUAL_FW_SIZE);
		return 0;
	}
	//END Packet fw
	// put packet end
	packet[0] = OTA_PACKET_END; 			// packet begin
	packet[1] = 0;							// device type
	packet[2] = VIRTUAL_FW_VERSION;			// version
	packet[3] = (uint8_t)image_size;		// FW size
	packet[4] = (uint8_t)(image_size>>8);	// FW size
	packet[5] = (uint8_t)(image_size>>16);	// FW size
	memcpy(&packet[6],DFU_OTA_CRC128_GET(),OTA_PACKET_LENGTH);
	///
	if(-1==fl_wifi2ble_fota_system_end(&packet[0],sizeof(packet))) return 0;
	VIRTUAL_FW_NUMOFSENDING+=1;
	P_INFO("FOTA num(v.%d) : %d\r\n",VIRTUAL_FW_VERSION,VIRTUAL_FW_NUMOFSENDING);
	return -1;
}

/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/
void TEST_virtual_fw(u32 _fwsize ) {
	blt_soft_timer_delete(_send_fw);
	crc128_init();
	if (blt_soft_timer_find(_send_fw) == -1) {
		VIRTUAL_FW_INDX=0;
		VIRTUAL_FW_ADDR = 0;
		VIRTUAL_FW_NUMOFSENDING = 1;
		VIRTUAL_FW_VERSION++;
		fl_wifi2ble_fota_ContainerClear();
		if (_fwsize > 0) {
			VIRTUAL_FW_SIZE = _fwsize;
		}
		uint32_t	image_size = VIRTUAL_FW_SIZE;	//APP_IMAGE_SIZE_MAX;
		uint8_t		packet[22];
		//	// put packet begin
		packet[0] = OTA_PACKET_BEGIN; 			// packet begin
		packet[1] = 0;							// device type
		packet[2] = 2;							// version
		packet[3] = (uint8_t)image_size;		// FW size
		packet[4] = (uint8_t)(image_size>>8);	// FW size
		packet[5] = (uint8_t)(image_size>>16);	// FW size
		fl_wifi2ble_fota_system_begin(&packet[0],sizeof(packet));

		blt_soft_timer_add(&_send_fw,100 * 1004);
	}
}

/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/
#endif
