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
#include "../Freelux_libs/dfu.h"
#include "../Freelux_libs/storage_weekly_data.h"

/**
 * @brief      uart1 irq code for application
 * @param[in]  none
 * @return     none
 */
void uart1_recieve_irq(void) {
#ifdef MASTER_CORE
	extern unsigned char FLAG_uart_dma_send;
	if (uart_get_irq_status(UART1,UART_TXDONE)) {
		FLAG_uart_dma_send = 0;
		uart_clr_tx_done(UART1);
	}
	if (uart_get_irq_status(UART1,UART_RXDONE)) //A0-SOC can't use RX-DONE status,so this interrupt can noly used in A1-SOC.
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
void uart0_recieve_irq(void) {
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
 * @brief		GPIO interrupt handler.
 * @param[in]	none
 * @return      none
 */
_attribute_ram_code_ void gpio_irq_handler(void){
	LOG_P(APP,"GPIO irq....\r\n");
	gpio_clr_irq_mask(GPIO_IRQ_MASK_GPIO);
	gpio_clr_irq_status(FLD_GPIO_IRQ_CLR);
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

/**
 * @brief		This is main function
 * @param[in]	none
 * @return      none
 */
fl_version_t _bootloader = { 1, 0, 3 };
fl_version_t _fw = { 1, 4, 0 };
fl_version_t _hw = { 0, 0, 0 };

uint8_t buff[20] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
storage_ret_t retval;
_attribute_ram_code_ int main(void)   //must on ramcode
{
	DBG_CHN0_LOW;
	blc_pm_select_internal_32k_crystal();

	sys_init(DCDC_1P4_DCDC_1P8,VBAT_MAX_VALUE_GREATER_THAN_3V6);
	trng_init();
	/* detect if MCU is wake_up from deep retention mode */
	int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup();  //MCU deep retention wakeUp

	CCLK_16M_HCLK_16M_PCLK_16M;

//	rf_drv_ble_init();

	gpio_init(!deepRetWakeUp);
#if (UART_PRINT_DEBUG_ENABLE)
	DEBUG_TX_PIN_INIT()
	;
#endif
//	_fw.patch = get_current_fw_version();
	PLOG_DEVICE_PROFILE(_bootloader,_fw,_hw);

	if (!deepRetWakeUp)  //read flash size
	{
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

	if (deepRetWakeUp)
	{ //MCU wake_up from deepSleep retention mode
		user_init_deepRetn();
	}
	else
	{ //MCU power_on or wake_up from deepSleep mode
//		user_init_normal();

		/* Test code */
		LOG_P(APP,"DFU\n");
		firmware_check();
//		storage_init();

//		ota_init();
//		test_ota();

		irq_enable();
		// wdt init
		wd_set_interval_ms(5000);	// 5s
		wd_start();
		while(1)
		{
			main_loop();
			wd_clear();
		}
		return 0;
	}
}

