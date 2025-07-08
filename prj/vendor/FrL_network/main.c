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

/**
 * @brief		BLE SDK RF interrupt handler.
 * @param[in]	none
 * @return      none
 */
_attribute_ram_code_
void rf_irq_handler(void)
{
	DBG_CHN10_HIGH;

	irq_blt_sdk_handler ();

	DBG_CHN10_LOW;
}

/**
 * @brief		BLE SDK System timer interrupt handler.
 * @param[in]	none
 * @return      none
 */
_attribute_ram_code_
void stimer_irq_handler(void)
{
	DBG_CHN9_HIGH;

	irq_blt_sdk_handler ();

	DBG_CHN9_LOW;
}

/**
 * @brief		BLE SDK UART0 interrupt handler.
 * @param[in]	none
 * @return      none
 */
void uart0_irq_handler(void)
{
#if UART_DMA_USE
	extern void uart0_recieve_irq(void);
	uart0_recieve_irq();
#else
	if(uart_get_irq_status(UART0, UART_RXBUF_IRQ_STATUS))
	{
		extern void uart0_recieve_irq(void);

		uart0_recieve_irq();
	}
#endif
}

/**
 * @brief		This is main function
 * @param[in]	none
 * @return      none
 */
_attribute_ram_code_ int main (void)   //must on ramcode
{
	DBG_CHN0_LOW;
	blc_pm_select_internal_32k_crystal();

	sys_init(DCDC_1P4_DCDC_1P8,VBAT_MAX_VALUE_GREATER_THAN_3V6);

	/* detect if MCU is wake_up from deep retention mode */
	int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup();  //MCU deep retention wakeUp

	CCLK_16M_HCLK_16M_PCLK_16M;

	rf_drv_ble_init();

	gpio_init(!deepRetWakeUp);

	if(!deepRetWakeUp){//read flash size
		blc_readFlashSize_autoConfigCustomFlashSector();

		#if (FLASH_FIRMWARE_CHECK_ENABLE)
		blt_firmware_completeness_check
		#endif

		#if FIRMWARES_SIGNATURE_ENABLE
			blt_firmware_signature_check();
		#endif
	}

	blc_app_loadCustomizedParameters();  //load customized freq_offset cap value

	if( deepRetWakeUp ){ //MCU wake_up from deepSleep retention mode
#if (PM_DEEPSLEEP_RETENTION_ENABLE)
		user_init_deepRetn ();
#endif
	}
	else{ //MCU power_on or wake_up from deepSleep mode
		user_init_normal();
	}
	irq_enable();
	while (1) {
		main_loop ();
	}

	return 0;
}
