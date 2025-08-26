/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: TBS_devices.c
 *Created on		: Aug 25, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/


#include "TBS_dev_config.h"
#include "tl_common.h"
#include "../FrL_Network/fl_nwk_handler.h"
/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
#ifdef COUNTER_DEVICE
fl_device_counter_t G_COUNTER_DEV = { .data = {
												.timetamp = 0,
												.type = 0,
												.bt_call = 0,
												.bt_endcall = 0,
												.bt_rst = 0,
												.pass_product = 100,
												.err_product = 5
												}
									};
#endif
#ifdef POWER_METER
tbs_device_powermeter_t meter = {
				        .mac = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01},
				        .timestamp = 12345678,
				        .frequency = 100,
				        .voltage = 300,
				        .current1 = 512,
				        .current2 = 513,
				        .current3 = 514,
				        .power1 = 1000,
				        .power2 = 1001,
				        .power3 = 1002,
				        .energy1 = 123456,
				        .energy2 = 654321,
				        .energy3 = 111111,
//				        .reserve = 0xABCD
				    };
void test_powermeter(void) {
	u8 buffer[POWER_METER_SIZE];
	memset(buffer,0,POWER_METER_SIZE);
	pack_powermeter_data(&meter, buffer);
//P_PRINTFHEX_A(MCU,buffer,34,"PACK(%d):",SIZEU8(buffer));
	fl_ble_send_wifi(buffer,POWER_METER_PACK_SIZE);
	tbs_device_powermeter_t received;
	unpack_powermeter_data(&received, buffer);
	LOGA(MCU,"MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
			received.mac[0], received.mac[1], received.mac[2],
			received.mac[3], received.mac[4], received.mac[5]);
	LOGA(MCU, "Timestamp: %u\n", received.timestamp);
	LOGA(MCU, "Frequency: %u\n", received.frequency);
	LOGA(MCU, "Voltage: %u\n", received.voltage);
	LOGA(MCU, "Current1: %u\n", received.current1);
	LOGA(MCU, "Current2: %u\n", received.current2);
	LOGA(MCU, "Current3: %u\n", received.current3);
	LOGA(MCU, "Power1: %u\n", received.power1);
	LOGA(MCU, "Power2: %u\n", received.power2);
	LOGA(MCU, "Power3: %u\n", received.power3);
	LOGA(MCU, "Energy1: %u\n", received.energy1);
	LOGA(MCU, "Energy2: %u\n", received.energy2);
	LOGA(MCU, "Energy3: %u\n", received.energy3);
}
#endif
/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/


/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/



/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/



/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/

