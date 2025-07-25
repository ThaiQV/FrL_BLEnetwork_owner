/********************************************************************************************************
 * @file     app_config.h
 *
 * @brief    This is the header file for BLE SDK
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

#pragma once

/////////////////// FEATURE SELECT /////////////////////////////////
/**
 *  @brief  Feature select in bLE Sample project
 */
#define BLE_APP_PM_ENABLE					0
#define PM_DEEPSLEEP_RETENTION_ENABLE		0
#define TEST_CONN_CURRENT_ENABLE            0 //test connection current, disable UI to have a pure power
#define APP_SECURITY_ENABLE      			0
#define APP_DIRECT_ADV_ENABLE				0
#define BLE_OTA_SERVER_ENABLE				1
#define BATT_CHECK_ENABLE					0
#define	BLT_SOFTWARE_TIMER_ENABLE			1
/**
 *  @brief  flash firmware check
 */
#define FLASH_FIRMWARE_CHECK_ENABLE			0

/**
 *  @brief  firmware signature check
 */
#define FIRMWARES_SIGNATURE_ENABLE			0

/**
 *  @brief  DEBUG  Configuration
 */

#define UART_PRINT_DEBUG_ENABLE  			1
#ifdef UART_PRINT_DEBUG_ENABLE
#define PRINT_DEBUG_PRIORITY_IRQ			1 //highest priority for debugging
#endif
#define DEBUG_GPIO_ENABLE					0
#define JTAG_DEBUG_ENABLE					0

/**
 *  @brief  UI Configuration
 */
#define UI_LED_ENABLE          	 			0
#define UI_BUTTON_ENABLE					0
#if (TEST_CONN_CURRENT_ENABLE) //test current, disable keyboard
#define	UI_KEYBOARD_ENABLE			0
#else
#define	UI_KEYBOARD_ENABLE			0
#endif

//SAMPLE SELECT EVK BOARD
#if (UI_KEYBOARD_ENABLE)   // if test pure power, kyeScan GPIO setting all disabled
//---------------  KeyMatrix PB2/PB3/PB4/PB5 -----------------------------
#define	MATRIX_ROW_PULL					PM_PIN_PULLDOWN_100K
#define	MATRIX_COL_PULL					PM_PIN_PULLUP_10K

#define	KB_LINE_HIGH_VALID				0   //dirve pin output 0 when keyscan, scanpin read 0 is valid

#define			CR_VOL_UP				0xf0  ////
#define			CR_VOL_DN				0xf1

/**
 *  @brief  Normal keyboard map
 */
#define		KB_MAP_NORMAL	{	{CR_VOL_DN,		VK_1},	 \
											{CR_VOL_UP,		VK_2}, }

//////////////////// KEY CONFIG (EVK board) ///////////////////////////
#define  KB_DRIVE_PINS  {GPIO_PC2, GPIO_PC0}
#define  KB_SCAN_PINS   {GPIO_PC3, GPIO_PC1}

//drive pin as gpio
#define	PC2_FUNC				AS_GPIO
#define	PC0_FUNC				AS_GPIO

//drive pin need 100K pulldown
#define	PULL_WAKEUP_SRC_PC2		MATRIX_ROW_PULL
#define	PULL_WAKEUP_SRC_PC0		MATRIX_ROW_PULL

//drive pin open input to read gpio wakeup level
#define PC2_INPUT_ENABLE		1
#define PC0_INPUT_ENABLE		1

//scan pin as gpio
#define	PC3_FUNC				AS_GPIO
#define	PC1_FUNC				AS_GPIO

//scan  pin need 10K pullup
#define	PULL_WAKEUP_SRC_PC3		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PC1		MATRIX_COL_PULL

//scan pin open input to read gpio level
#define PC3_INPUT_ENABLE		1
#define PC1_INPUT_ENABLE		1

#if (UI_LED_ENABLE)
/**
 *  @brief  Definition gpio for led
 */
#define	GPIO_LED_WHITE			GPIO_PB6
#define	GPIO_LED_GREEN			GPIO_PB5
#define	GPIO_LED_BLUE			GPIO_PB4
#define GPIO_LED_RED			GPIO_PB7
#define LED_ON_LEVAL 			1 		//gpio output high voltage to turn on led

#define PB7_FUNC				AS_GPIO
#define PB6_FUNC				AS_GPIO
#define PB5_FUNC				AS_GPIO
#define PB4_FUNC				AS_GPIO

#define	PB7_OUTPUT_ENABLE		1
#define	PB6_OUTPUT_ENABLE		1
#define PB5_OUTPUT_ENABLE		1
#define	PB4_OUTPUT_ENABLE		1
#endif

#elif (UI_BUTTON_ENABLE)
//SAMPLE SELECT DONGLE BOARD
#undef  PM_DEEPSLEEP_RETENTION_ENABLE
#define PM_DEEPSLEEP_RETENTION_ENABLE				0    //dongle demo no need deepSleepRetention
//---------------  Button -------------------------------
/**
 *  @brief  Definition gpio for button detection
 */
#define	SW1_GPIO				GPIO_PB2
#define	SW2_GPIO				GPIO_PB3
#define PB2_FUNC				AS_GPIO
#define PB3_FUNC				AS_GPIO
#define PB2_INPUT_ENABLE		1
#define PB3_INPUT_ENABLE		1
#define PULL_WAKEUP_SRC_PB2     PM_PIN_PULLUP_10K
#define PULL_WAKEUP_SRC_PB3     PM_PIN_PULLUP_10K

#if (UI_LED_ENABLE)
/**
 *  @brief  Definition gpio for led
 */
//---------------  LED ----------------------------------
#define	GPIO_LED_RED			GPIO_PB4
#define	GPIO_LED_WHITE			GPIO_PB1
#define	GPIO_LED_GREEN			GPIO_PB0
#define	GPIO_LED_BLUE			GPIO_PB7
#define	GPIO_LED_YELLOW			GPIO_PB5

#define PB4_FUNC				AS_GPIO
#define PB1_FUNC				AS_GPIO
#define PB0_FUNC				AS_GPIO
#define PB7_FUNC				AS_GPIO
#define PB5_FUNC				AS_GPIO

#define	PB4_OUTPUT_ENABLE		1
#define	PB1_OUTPUT_ENABLE		1
#define PB0_OUTPUT_ENABLE		1
#define	PB7_OUTPUT_ENABLE		1
#define	PB5_OUTPUT_ENABLE		1

#define LED_ON_LEVAL 			1 		//gpio output high voltage to turn on led
#endif
#endif

/////////////////// DEEP SAVE FLG //////////////////////////////////
#define USED_DEEP_ANA_REG                   DEEP_ANA_REG1 //u8,can save 8 bit info when deep
#define	LOW_BATT_FLG					    BIT(0) //if 1: low battery
#define CONN_DEEP_FLG	                    BIT(1) //if 1: conn deep, 0: adv deep
#define IR_MODE_DEEP_FLG	 				BIT(2) //if 1: IR mode, 0: BLE mode
#define LOW_BATT_SUSPEND_FLG				BIT(3) //if 1 : low battery, < 1.8v

#if (BATT_CHECK_ENABLE)
#define VBAT_CHANNEL_EN						0

#if VBAT_CHANNEL_EN
/**		The battery voltage sample range is 1.8~3.5V    **/
#else
/** 	if the battery voltage > 3.6V, should take some external voltage divider	**/
#define GPIO_BAT_DETECT				GPIO_PB0
#define PB0_FUNC						AS_GPIO
#define PB0_INPUT_ENABLE				0
#define PB0_OUTPUT_ENABLE				0
#define PB0_DATA_OUT					0
#define ADC_INPUT_PIN_CHN				ADC_GPIO_PB0
#endif
#endif

#if (JTAG_DEBUG_ENABLE)//2-wire jtag mode

#define PE4_FUNC	AS_TDI  //JTAG 4-WIRE FUNCTION
#define PE5_FUNC	AS_TDO  //JTAG 4-WIRE FUNCTION
#define PE6_FUNC	AS_TMS  //JTAG 4-WIRE FUNCTION
#define PE7_FUNC	AS_TCK  //JTAG 4-WIRE FUNCTION

#define PE4_INPUT_ENABLE	1 //JTAG 4-WIRE FUNCTION
#define PE5_INPUT_ENABLE	1 //JTAG 4-WIRE FUNCTION
#define PE6_INPUT_ENABLE	1 //JTAG 4-WIRE FUNCTION
#define PE7_INPUT_ENABLE	1 //JTAG 4-WIRE FUNCTION

#define PULL_WAKEUP_SRC_PE4	GPIO_PIN_PULLUP_10K //JTAG 4-WIRE FUNCTION
#define PULL_WAKEUP_SRC_PE5	GPIO_PIN_PULLUP_10K //JTAG 4-WIRE FUNCTION
#define PULL_WAKEUP_SRC_PE6	GPIO_PIN_PULLUP_10K //JTAG 4-WIRE FUNCTION
#define PULL_WAKEUP_SRC_PE7	GPIO_PIN_PULLDOWN_100K //JTAG 4-WIRE FUNCTION

#endif

/////////////////////////////////////// PRINT DEBUG INFO ///////////////////////////////////////
#if (UART_PRINT_DEBUG_ENABLE)
//the baud rate should not bigger than 115200 when MCU clock is 16M)
//the baud rate should not bigger than 1000000 when MCU clock is 24M)
#define PRINT_BAUD_RATE             		115200
#define DEBUG_INFO_TX_PIN           		GPIO_PA7
#define PULL_WAKEUP_SRC_PA7         		PM_PIN_PULLUP_10K
#define PA7_OUTPUT_ENABLE         			1
#define PA7_DATA_OUT                     	1 //must

#define DEBUG_TX_PIN_INIT()					do{	\
												gpio_function_en(DEBUG_INFO_TX_PIN);								\
												gpio_set_output(DEBUG_INFO_TX_PIN, 1);								\
												gpio_set_up_down_res(DEBUG_INFO_TX_PIN, GPIO_PIN_PULLUP_1M);		\
												gpio_set_high_level(DEBUG_INFO_TX_PIN);								\
											}while(0)
#endif

/**
 *  @brief  Definition for gpio debug
 */
#if(DEBUG_GPIO_ENABLE)

#define GPIO_CHN0							GPIO_PE1
#define GPIO_CHN1							GPIO_PE2
#define GPIO_CHN2							GPIO_PA0
#define GPIO_CHN3							GPIO_PA4
#define GPIO_CHN4							GPIO_PA3
#define GPIO_CHN5							GPIO_PB0
#define GPIO_CHN6							GPIO_PB2
#define GPIO_CHN7							GPIO_PE0

#define GPIO_CHN8							GPIO_PA2
#define GPIO_CHN9							GPIO_PA1
#define GPIO_CHN10							GPIO_PB1
#define GPIO_CHN11							GPIO_PB3

#define GPIO_CHN12							GPIO_PC7
#define GPIO_CHN13							GPIO_PC6
#define GPIO_CHN14							GPIO_PC5
#define GPIO_CHN15							GPIO_PC4

#define PE1_OUTPUT_ENABLE					1
#define PE2_OUTPUT_ENABLE					1
#define PA0_OUTPUT_ENABLE					1
#define PA4_OUTPUT_ENABLE					1
#define PA3_OUTPUT_ENABLE					1
#define PB0_OUTPUT_ENABLE					1
#define PB2_OUTPUT_ENABLE					1
#define PE0_OUTPUT_ENABLE					1

#define PA2_OUTPUT_ENABLE					1
#define PA1_OUTPUT_ENABLE					1
#define PB1_OUTPUT_ENABLE					1
#define PB3_OUTPUT_ENABLE					1
#define PC7_OUTPUT_ENABLE					1
#define PC6_OUTPUT_ENABLE					1
#define PC5_OUTPUT_ENABLE					1
#define PC4_OUTPUT_ENABLE					1

#define DBG_CHN0_LOW		gpio_write(GPIO_CHN0, 0)
#define DBG_CHN0_HIGH		gpio_write(GPIO_CHN0, 1)
#define DBG_CHN0_TOGGLE		gpio_toggle(GPIO_CHN0)
#define DBG_CHN1_LOW		gpio_write(GPIO_CHN1, 0)
#define DBG_CHN1_HIGH		gpio_write(GPIO_CHN1, 1)
#define DBG_CHN1_TOGGLE		gpio_toggle(GPIO_CHN1)
#define DBG_CHN2_LOW		gpio_write(GPIO_CHN2, 0)
#define DBG_CHN2_HIGH		gpio_write(GPIO_CHN2, 1)
#define DBG_CHN2_TOGGLE		gpio_toggle(GPIO_CHN2)
#define DBG_CHN3_LOW		gpio_write(GPIO_CHN3, 0)
#define DBG_CHN3_HIGH		gpio_write(GPIO_CHN3, 1)
#define DBG_CHN3_TOGGLE		gpio_toggle(GPIO_CHN3)
#define DBG_CHN4_LOW		gpio_write(GPIO_CHN4, 0)
#define DBG_CHN4_HIGH		gpio_write(GPIO_CHN4, 1)
#define DBG_CHN4_TOGGLE		gpio_toggle(GPIO_CHN4)
#define DBG_CHN5_LOW		gpio_write(GPIO_CHN5, 0)
#define DBG_CHN5_HIGH		gpio_write(GPIO_CHN5, 1)
#define DBG_CHN5_TOGGLE		gpio_toggle(GPIO_CHN5)
#define DBG_CHN6_LOW		gpio_write(GPIO_CHN6, 0)
#define DBG_CHN6_HIGH		gpio_write(GPIO_CHN6, 1)
#define DBG_CHN6_TOGGLE		gpio_toggle(GPIO_CHN6)
#define DBG_CHN7_LOW		gpio_write(GPIO_CHN7, 0)
#define DBG_CHN7_HIGH		gpio_write(GPIO_CHN7, 1)
#define DBG_CHN7_TOGGLE		gpio_toggle(GPIO_CHN7)
#define DBG_CHN8_LOW		gpio_write(GPIO_CHN8, 0)
#define DBG_CHN8_HIGH		gpio_write(GPIO_CHN8, 1)
#define DBG_CHN8_TOGGLE		gpio_toggle(GPIO_CHN8)
#define DBG_CHN9_LOW		gpio_write(GPIO_CHN9, 0)
#define DBG_CHN9_HIGH		gpio_write(GPIO_CHN9, 1)
#define DBG_CHN9_TOGGLE		gpio_toggle(GPIO_CHN9)
#define DBG_CHN10_LOW		gpio_write(GPIO_CHN10, 0)
#define DBG_CHN10_HIGH		gpio_write(GPIO_CHN10, 1)
#define DBG_CHN10_TOGGLE	gpio_toggle(GPIO_CHN10)
#define DBG_CHN11_LOW		gpio_write(GPIO_CHN11, 0)
#define DBG_CHN11_HIGH		gpio_write(GPIO_CHN11, 1)
#define DBG_CHN11_TOGGLE	gpio_toggle(GPIO_CHN11)
#define DBG_CHN12_LOW		gpio_write(GPIO_CHN12, 0)
#define DBG_CHN12_HIGH		gpio_write(GPIO_CHN12, 1)
#define DBG_CHN12_TOGGLE	gpio_toggle(GPIO_CHN12)
#define DBG_CHN13_LOW		gpio_write(GPIO_CHN13, 0)
#define DBG_CHN13_HIGH		gpio_write(GPIO_CHN13, 1)
#define DBG_CHN13_TOGGLE	gpio_toggle(GPIO_CHN13)
#define DBG_CHN14_LOW		gpio_write(GPIO_CHN14, 0)
#define DBG_CHN14_HIGH		gpio_write(GPIO_CHN14, 1)
#define DBG_CHN14_TOGGLE	gpio_toggle(GPIO_CHN14)
#define DBG_CHN15_LOW		gpio_write(GPIO_CHN15, 0)
#define DBG_CHN15_HIGH		gpio_write(GPIO_CHN15, 1)
#define DBG_CHN15_TOGGLE	gpio_toggle(GPIO_CHN15)
#else
#define DBG_CHN0_LOW
#define DBG_CHN0_HIGH
#define DBG_CHN0_TOGGLE
#define DBG_CHN1_LOW
#define DBG_CHN1_HIGH
#define DBG_CHN1_TOGGLE
#define DBG_CHN2_LOW
#define DBG_CHN2_HIGH
#define DBG_CHN2_TOGGLE
#define DBG_CHN3_LOW
#define DBG_CHN3_HIGH
#define DBG_CHN3_TOGGLE
#define DBG_CHN4_LOW
#define DBG_CHN4_HIGH
#define DBG_CHN4_TOGGLE
#define DBG_CHN5_LOW
#define DBG_CHN5_HIGH
#define DBG_CHN5_TOGGLE
#define DBG_CHN6_LOW
#define DBG_CHN6_HIGH
#define DBG_CHN6_TOGGLE
#define DBG_CHN7_LOW
#define DBG_CHN7_HIGH
#define DBG_CHN7_TOGGLE
#define DBG_CHN8_LOW
#define DBG_CHN8_HIGH
#define DBG_CHN8_TOGGLE
#define DBG_CHN9_LOW
#define DBG_CHN9_HIGH
#define DBG_CHN9_TOGGLE
#define DBG_CHN10_LOW
#define DBG_CHN10_HIGH
#define DBG_CHN10_TOGGLE
#define DBG_CHN11_LOW
#define DBG_CHN11_HIGH
#define DBG_CHN11_TOGGLE
#define DBG_CHN12_LOW
#define DBG_CHN12_HIGH
#define DBG_CHN12_TOGGLE
#define DBG_CHN13_LOW
#define DBG_CHN13_HIGH
#define DBG_CHN13_TOGGLE
#define DBG_CHN14_LOW
#define DBG_CHN14_HIGH
#define DBG_CHN14_TOGGLE
#define DBG_CHN15_LOW
#define DBG_CHN15_HIGH
#define DBG_CHN15_TOGGLE
#endif  //end of DEBUG_GPIO_ENABLE

#include "vendor/common/default_config.h"
