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
#include "fl_wifi2ble_fota.h"
#include "../Freelux_libs/SPI_FLASH.h"
#include "../Freelux_libs/nvm.h"
#include "../Freelux_libs/storage_weekly_data.h"

#if(FREERTOS_ENABLE)
#include <FreeRTOS.h>
#include <task.h>
#endif

#define WDT_RST_INTERVAL				5011		//ms

/**
 * @brief      uart1 irq code for application
 * @param[in]  none
 * @return     none
 */
void uart1_recieve_irq(void) {
#ifdef MASTER_CORE
	extern unsigned char FLAG_uart_dma_send;
	unsigned int irq_flags = uart_get_irq_status(UART1,0xFFFFFFFF);
//	char bit_flag[9]; // 8 bit + 1 end
//	for (int i = 7; i >= 0; i--) {
//		bit_flag[7 - i] = ((irq_flags >> i) & 0x1) ? '1' : '0';
//	}
//	bit_flag[8] = '\0'; //
//	ERR(APP,"UART IRQ Flags (0x%02X): %s\r\n",irq_flags & 0xFF,bit_flag);
//
//	if (irq_flags & UART_ERR_IRQ_MASK) {
//		ERR(APP,"UART Error IRQ\r\n");
//	}
//
//	if (irq_flags & UART_RXDONE_MASK) {
//		ERR(APP,"RX DONE\r\n");
//	}
	if (irq_flags & UART_RXBUF_IRQ_STATUS) {
//		ERR(APP,"RX FIFO FULL\r\n");
		fl_input_serial_rec();
		uart_clr_irq_status(UART1, UART_CLR_RX);
		uart_clr_irq_mask(UART1,UART_RXBUF_IRQ_STATUS);
		FLAG_uart_dma_send = 0;
	}
	if (uart_get_irq_status(UART1,UART_TXDONE)) {
		FLAG_uart_dma_send = 0;
		uart_clr_tx_done(UART1);
	}
	if (uart_get_irq_status(UART1,UART_RXDONE)){ //A0-SOC can't use RX-DONE status,so this interrupt can noly used in A1-SOC.
//		ERR(APP,"UART UART_RXDONE  !!\r\n");
		/************************cll rx_irq****************************/
		fl_input_serial_rec();
		uart_clr_irq_status(UART1,UART_CLR_RX);
		//uart_clr_irq_mask(UART1,UART_ERR_IRQ_MASK|UART_RXDONE_MASK|UART_RX_IRQ_MASK);
	}
	if(uart_get_irq_status(UART1,UART_RX_ERR)){
		ERR(APP,"UART FAIL !!\r\n");
		uart_clr_irq_status(UART1,UART_CLR_RX);
		FLAG_uart_dma_send = 0;
		fl_serial_buffer_ClearAll();
	}
#endif
}
#ifdef MASTER_CORE
/**
 * @brief      uart0 irq code for application
 * @param[in]  none
 * @return     none
 */
void uart0_recieve_irq(void) {
//	LOGA(DRV,"UART0 Rec:%c\r\n",uart_read_byte(UART0));
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
 * @brief		GPIO interrupt handler.
 * @param[in]	none
 * @return      none
 */
_attribute_ram_code_ void gpio_irq_handler(void){
//	LOG_P(APP,"GPIO irq....\r\n");
//	gpio_clr_irq_mask(GPIO_IRQ_MASK_GPIO);
//	gpio_clr_irq_status(FLD_GPIO_IRQ_CLR);
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
fl_version_t _bootloader = { 1, 0, 3};
fl_version_t _fw = { 1, 4,36 };
fl_version_t _hw = { 1, 0, 0 };

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
#ifdef MASTER_CORE
//	P_INFO("Startup from FOTA");
	u8 wait=0;
	extern void delay_ms(unsigned int millisec);
	while(wait<10){
		P_INFO(".");
		delay_ms(400);
		wait++;
	}
	P_INFO("ok\r\n");
#endif
//	while(1){
//		delay_ms(2000);
//		ERR(APP,"FOTA ok.....\r\n");
//	}
	//OFF ALL LOG
	PLOG_Stop(ALL);
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
	} else { //MCU power_on or wake_up from deepSleep mode
		user_init_normal();
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
	wd_set_interval_ms(WDT_RST_INTERVAL);
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
