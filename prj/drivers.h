/********************************************************************************************************
 * @file     drivers.h
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

#include "drivers/B91/driver.h"
#include "drivers/B91/ext_driver/driver_ext.h"

#define write_log32(err_code)   write_sram32(0x00014, err_code)

///**
// * TBS project define pinout
// * */
#ifndef MASTER_CORE
#define TBS_COUNTER_DEVICE 1 			//0: Counter, 1: PowerMeter
#if TBS_COUNTER_DEVICE
#define COUNTER_DEVICE
#else
#define POWER_METER_DEVICE
#endif
#else
#endif

//#define TBS_GATEWAY_DEVICE  // uncomment if build for the master

#if MASTER_CORE
#ifndef TBS_GATEWAY_DEVICE
    #error "Please uncomment #define TBS_GATEWAY_DEVICE"
#endif
#else
#ifdef TBS_GATEWAY_DEVICE
    #error "Please comment #define TBS_GATEWAY_DEVICE"
#endif
#endif

//// HSPI common
#define HSPI_CLK						GPIO_PB4
#define HSPI_MISO						GPIO_PB2
#define HSPI_MOSI						GPIO_PB3

#if (defined MASTER_CORE | defined COUNTER_DEVICE | !defined DFU_POWER_METER_DEVICE)
	#define HSPI_CS							GPIO_PB0
	#define HSPI_IO							GPIO_PB1
#else
	#ifdef POWER_METER_DEVICE
		#define HSPI_IO							GPIO_PB1
		#define HSPI_CS							GPIO_PE7
		#define HSPI_CS_POWER_METER_STPM1		GPIO_PE6
		#define HSPI_CS_POWER_METER_STPM2		GPIO_PE5
		#define HSPI_CS_POWER_METER_STPM3		GPIO_PE4
	#endif
#endif

