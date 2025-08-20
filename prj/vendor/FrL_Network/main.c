/********************************************************************************************************
 * @file     main.c
 *
 * @brief    This is the source file for BLE SDK
 *
 * @author	 BLE GROUP
 * @date         06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *******************************************************************************************************/

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "app.h"
#include "fl_input_ext.h"
#include "../Freelux_libs/SPI_FLASH.h"
#include "../Freelux_libs/nvm.h"
#include "../Freelux_libs/fl_ble_wifi_protocol.h"
#include "../Freelux_libs/storage_weekly_data.h"

#if(FREERTOS_ENABLE)
#include <FreeRTOS.h>
#include <task.h>
#endif

/**
 * @brief      uart1 irq code for application
 * @param[in]  none
 * @return     none
 */
void uart1_recieve_irq(void)
{
#ifdef MASTER_CORE
	extern unsigned char FLAG_uart_dma_send;
	if(uart_get_irq_status(UART1,UART_TXDONE))
	{
		FLAG_uart_dma_send = 0;
		uart_clr_tx_done(UART1);
	}
	if(uart_get_irq_status(UART1,UART_RXDONE)) //A0-SOC can't use RX-DONE status,so this interrupt can noly used in A1-SOC.
	{
		/************************cll rx_irq****************************/
		fl_input_serial_rec();
		uart_clr_irq_status(UART1,UART_CLR_RX);
	}
#endif
}
#ifdef MASTER_CORE
/**
 * @brief      uart0 irq code for application
 * @param[in]  none
 * @return     none
 */
void uart0_recieve_irq(void)
{
	LOGA(DRV,"UART0 Rec:%c\r\n",uart_read_byte(UART0));
}
#endif
/**
 * @brief		BLE SDK RF interrupt handler.
 * @param[in]	none
 * @return      none
 */
_attribute_ram_code_
void rf_irq_handler(void) {
	DBG_CHN14_HIGH;

	irq_blt_sdk_handler();

	DBG_CHN14_LOW;
}
/**
 * @brief		BLE SDK UART0 interrupt handler.
 * @param[in]	none
 * @return      none
 */
void uart1_irq_handler(void) {
	extern void uart1_recieve_irq(void);
	uart1_recieve_irq();
}
/**
 * @brief		BLE SDK UART1 interrupt handler.
 * @param[in]	none
 * @return      none
 */
void uart0_irq_handler(void) {
//	if (uart_get_irq_status(UART1,UART_RXBUF_IRQ_STATUS)) {
//		extern void uart1_recieve_irq(void);
//		uart1_recieve_irq();
//	}
}
/**
 * @brief		BLE SDK System timer interrupt handler.
 * @param[in]	none
 * @return      none
 */
_attribute_ram_code_
void stimer_irq_handler(void) {
	DBG_CHN15_HIGH;

	irq_blt_sdk_handler();

	DBG_CHN15_LOW;
}

#if (FREERTOS_ENABLE)
#if(!TEST_CONN_CURRENT_ENABLE)
static void led_task(void *pvParameters) {
	reg_gpio_pb_oen &= ~ GPIO_PB7;
	while(1) {
		reg_gpio_pb_out |= GPIO_PB7;
		printf("LED ON;\r\n");
		vTaskDelay(1000);
		printf("LED OFF;\r\n");
		reg_gpio_pb_out &= ~GPIO_PB7;
		vTaskDelay(1000);
	}
}
#endif
void proto_task( void *pvParameters );
#endif

/**
 * @brief		This is main function
 * @param[in]	none
 * @return      none
 */
fl_version_t _bootloader = { 0, 0, 0 };
fl_version_t _fw = { 1, 0, 0 };
fl_version_t _hw = { 0, 0, 0 };

uint8_t	read_msg[64];

_attribute_ram_code_ int main(void)   //must on ramcode
{
	DBG_CHN0_LOW;
	blc_pm_select_internal_32k_crystal();

	sys_init(DCDC_1P4_DCDC_1P8,VBAT_MAX_VALUE_GREATER_THAN_3V6);
	trng_init();
	/* detect if MCU is wake_up from deep retention mode */
	int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup();  //MCU deep retention wakeUp

	CCLK_16M_HCLK_16M_PCLK_16M;

	rf_drv_ble_init();

	gpio_init(!deepRetWakeUp);
#if (UART_PRINT_DEBUG_ENABLE)
	DEBUG_TX_PIN_INIT()
	;
#endif
	PLOG_DEVICE_PROFILE(_bootloader,_fw,_hw);

	if (!deepRetWakeUp) {  //read flash size
#if (BATT_CHECK_ENABLE)
	user_battery_power_check();
#endif

		blc_readFlashSize_autoConfigCustomFlashSector();

#if (FLASH_FIRMWARE_CHECK_ENABLE)
		blt_firmware_completeness_check();
#endif

#if FIRMWARES_SIGNATURE_ENABLE
		blt_firmware_signature_check();
#endif
	}

	/* load customized freq_offset cap value. */
	blc_app_loadCustomizedParameters();

	if (deepRetWakeUp) { //MCU wake_up from deepSleep retention mode
		user_init_deepRetn();
	}
	else
	{ //MCU power_on or wake_up from deepSleep mode
		user_init_normal();

//		delay_ms(3000);
		ble_wifi_protocol_init();
		nvm_init();

		storage_init();
//		storage_clean();
////		nvm_record_write(1,"Hello world!",strlen("Hello world!"));
////		nvm_record_write(2,"Good morning!",strlen("Good morning!"));
////		nvm_record_write(3,"Good afternoon!",strlen("Good afternoon!"));
//	    nvm_record_read(1,read_msg,strlen("Hello world!"));
//		LOGA(DRV,"MSG: %s\n",read_msg);
//		nvm_record_read(2,read_msg,strlen("Good morning!"));
//		LOGA(DRV,"MSG: %s\n",read_msg);
//		nvm_record_read(3,read_msg,strlen("Good afternoon!"));
//		LOGA(DRV,"MSG: %s\n",read_msg);

		uint8_t buff[24];

//		memset(buff,0x06,sizeof(buff));
//		storage_put_data(1756771200 + 604800,buff,sizeof(buff));
//		memset(buff,0x00,sizeof(buff));
//		storage_get_data(1756771200 + 604800,buff,sizeof(buff));
//		LOGA(DRV,"buff: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",buff[0],buff[1],buff[2],buff[3],buff[4],buff[5],
//				buff[6],buff[7],buff[8],buff[9],buff[10],buff[11],
//				buff[12],buff[13],buff[14],buff[15],buff[16],buff[17],
//				buff[18],buff[19],buff[20],buff[21],buff[22],buff[23]);
//
////		memset(buff,0x02,sizeof(buff));
////		storage_put_data(1754956860,buff,sizeof(buff));
////		memset(buff,0x00,sizeof(buff));
//		storage_get_data(1756771260 + 604800,buff,sizeof(buff));
//		LOGA(DRV,"buff: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",buff[0],buff[1],buff[2],buff[3],buff[4],buff[5],
//				buff[6],buff[7],buff[8],buff[9],buff[10],buff[11],
//				buff[12],buff[13],buff[14],buff[15],buff[16],buff[17],
//				buff[18],buff[19],buff[20],buff[21],buff[22],buff[23]);

//		int i;
//		for(i=0;i<400;i++)
//		{
//			memset(buff,2*i+1,sizeof(buff));
//			storage_put_data(1757376000 + 9*604800 + 60*i,buff,sizeof(buff));
//		}
//
//		for(i=0;i<400;i++)
//		{
//			storage_get_data(1757376000 + 9*604800 + 60*i,buff,sizeof(buff));
//			LOGA(DRV,"buff[%d]: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",i,buff[0],buff[1],buff[2],buff[3],buff[4],buff[5],
//					buff[6],buff[7],buff[8],buff[9],buff[10],buff[11],
//					buff[12],buff[13],buff[14],buff[15],buff[16],buff[17],
//					buff[18],buff[19],buff[20],buff[21],buff[22],buff[23]);
//		}
//
//
//		for(i=0;i<200;i++)
//		{
//			memset(buff,i,sizeof(buff));
//			storage_put_data(0,buff,sizeof(buff));
//		}
//
//		for(i=0;i<200;i++)
//		{
//			storage_get_data_unspecified(i,buff,sizeof(buff));
//			LOGA(DRV,"buff[%d]: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",i,buff[0],buff[1],buff[2],buff[3],buff[4],buff[5],
//					buff[6],buff[7],buff[8],buff[9],buff[10],buff[11],
//					buff[12],buff[13],buff[14],buff[15],buff[16],buff[17],
//					buff[18],buff[19],buff[20],buff[21],buff[22],buff[23]);
//		}
//
//		LOGA(DRV,"End Unspecified\n");


#if (FREERTOS_ENABLE)
		extern void blc_ll_set_freertos_en(u8 en);
		blc_ll_set_freertos_en(1);
#endif
	}

#if (FREERTOS_ENABLE)

	extern void vPortRestoreTask();
	if( deepRetWakeUp ) {
		printf("enter restor work.\r\n");
		vPortRestoreTask();

	} else {
#if(!TEST_CONN_CURRENT_ENABLE)
		xTaskCreate( led_task, "tLed", configMINIMAL_STACK_SIZE, (void*)0, (tskIDLE_PRIORITY+1), 0 );
#endif
		xTaskCreate( proto_task, "tProto", 2*configMINIMAL_STACK_SIZE, (void*)0, (tskIDLE_PRIORITY+1), 0 );
		//	xTaskCreate( ui_task, "tUI", configMINIMAL_STACK_SIZE, (void*)0, tskIDLE_PRIORITY + 1, 0 );
		vTaskStartScheduler();
	}
#else
	irq_enable();
	/// wdt init
	wd_set_interval_ms(5000);     // 5s
	wd_start();
	while (1) {
		main_loop();
		wd_clear();
	}
	return 0;
#endif
}

#if (FREERTOS_ENABLE)
//  !!! should notify those tasks that ulTaskNotifyTake long time and should be wakeup every time PM wakeup
void vPortWakeupNotify() {
}
#endif
