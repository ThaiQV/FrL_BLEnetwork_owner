/********************************** (C) COPYRIGHT *******************************
 * File Name          : SPI_FLAH.h
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2022/11/24
 * Description        : SPI FLASHChip operation file
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#ifndef __SPI_FLASH_H
#define __SPI_FLASH_H

#ifdef __cplusplus
 extern "C" {
#endif 

/******************************************************************************/
/* header files */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "tl_common.h"
/******************************************************************************/
/* SPI Pins */
#define PIN_CS		GPIO_PB0 //GPIO_PE7 GPIO_PB0
#define PIN_WP		GPIO_PB1
#define PIN_MISO	GPIO_PB2
#define PIN_MOSI	GPIO_PB3
#define PIN_CLK		GPIO_PB4

#define PIN_SET(pin)	gpio_set_high_level(pin)
#define PIN_CLEAR(pin)	gpio_set_low_level(pin)
#define PIN_READ(pin)	gpio_get_level(pin)

#define STORAGE_MEDIUM                 MEDIUM_SPI_FLASH

#if (STORAGE_MEDIUM == MEDIUM_SPI_FLASH)
    #define DEF_CFG_DISK_SEC_SIZE      4096                                                /* Disk sector size */
    #define DEF_FLASH_SECTOR_SIZE      4096                                                /* Flash sector size */
    #define DEF_UDISK_SECTOR_SIZE      DEF_CFG_DISK_SEC_SIZE                               /* UDisk sector size */
#elif (STORAGE_MEDIUM == MEDIUM_INTERAL_FLASH)
    #define DEF_CFG_DISK_SEC_SIZE      512                                                 /* Disk sector size */
    #define DEF_FLASH_SECTOR_SIZE      512                                                 /* Flash sector size */
    #define DEF_UDISK_SECTOR_SIZE      DEF_CFG_DISK_SEC_SIZE                               /* UDisk sector size */
#endif

/******************************************************************************/
/* SPI Serial Flash OPERATION INSTRUCTIONS */
#define CMD_FLASH_READ             0x03                                         /* Read Memory at 25 MHz */
#define CMD_FLASH_SECTOR_ERASE     0x20                                         /* Erase 4 KByte of memory array */
#define CMD_FLASH_BYTE_PROG        0x02                                         /* To Program One Data Byte */
#define CMD_FLASH_RDSR             0x05                                         /* Read-Status-Register */
#define CMD_FLASH_EWSR             0x50                                         /* Enable-Write-Status-Register */
#define CMD_FLASH_WREN             0x06                                         /* Write-Enable */
#define CMD_FLASH_WRDI             0x04                                         /* Write-Disable */
#define CMD_FLASH_JEDEC_ID         0x9F                                         /* JEDEC ID read */

/******************************************************************************/
#define DEF_DUMMY_BYTE             0xFF

/******************************************************************************/
/* FLASH Parameter Definition */
#define SPI_FLASH_SectorSize       4096
#define SPI_FLASH_PageSize         256
#define SPI_FLASH_PerWritePageSize 256

/******************************************************************************/
/* SPI FLASH Type */
#define DEF_TYPE_W25XXX            0                                            /* W25XXX */

/******************************************************************************/
/* SPI FLASH Type Define */

/* W25XXX*/
#define W25X10_FLASH_ID            0xEF3011                                     /* 1M bit */
#define W25X20_FLASH_ID            0xEF3012                                     /* 2M bit */
#define W25X40_FLASH_ID            0xEF3013                                     /* 4M bit */
#define W25X80_FLASH_ID            0xEF4014                                     /* 8M bit */
#define W25Q16_FLASH_ID1           0xEF3015                                     /* 16M bit */
#define W25Q16_FLASH_ID2           0xEF4015                                     /* 16M bit */
#define W25Q32_FLASH_ID1           0xEF4016                                     /* 32M bit */
#define W25Q32_FLASH_ID2           0xEF6016                                     /* 32M bit */
#define W25Q64_FLASH_ID1           0xEF4017                                     /* 64M bit */
#define W25Q64_FLASH_ID2           0xEF6017                                     /* 64M bit */
#define W25Q128_FLASH_ID1          0xEF4018                                     /* 128M bit */
#define W25Q128_FLASH_ID2          0xEF6018                                     /* 128M bit */
#define W25Q256_FLASH_ID1          0xEF4019                                     /* 256M bit */
#define W25Q256_FLASH_ID2          0xEF6019                                     /* 256M bit */

/******************************************************************************/
/* SPI Serial Flash MEMORY MAP
 *
 *     0x00400000 |------------------|
 *                |   1.5MB Reverse  |
 *     0x00280000 |------------------|
 *                |Device unspecified|
 *                |   data storage   |
 *     0x00200000 |------------------|
 *                |   Device weekly  |
 *                |   data storage   |
 *     0x00180000 |------------------|
 *                |       NVM        |
 *     0x00100000 |------------------|
 *                |       OTA        |
 *                |     Firmware     |
 *     0x00080000 |------------------|
 *                |     Original     |
 *                |     Firmware     |
 *     0x00000000 |------------------|
 * */

#define EX_FLASH_ORIGINAL_FW_ADDRESS		0x00000000
#define EX_FLASH_OTA_FW_ADDRESS				0x00080000
#define EX_FLASH_NVM_ADDRESS				0x00100000
#define EX_FLASH_DEVICE_STORAGE_ADDRESS		0x00180000
#define EX_FLASH_DEVICE_UNSTORAGE_ADDRESS	0x00200000

/******************************************************************************/
/* Variable Definition */
extern volatile uint8_t  Flash_Type;                                            /* FLASH chip: 0: W25XXXseries   */
extern volatile uint32_t Flash_ID;                                              /* FLASH ID */
extern volatile uint32_t Flash_Sector_Count;                                    /* FLASH sector number */
extern volatile uint16_t Flash_Sector_Size;                                     /* FLASH sector size */

/******************************************************************************/
/* external functions */
void __delay_us(unsigned int us);
unsigned char writeSPIByte(unsigned char transmit);
int writeSPIWord(unsigned short int setting);
int readSPIWord(void);
unsigned char readSPIByte(void);

extern void FLASH_Port_Init(void);
extern uint8_t SPI_FLASH_SendByte(uint8_t byte);
extern uint8_t SPI_FLASH_ReadByte(void);
extern uint32_t FLASH_ReadID(void);
extern void FLASH_WriteEnable(void);
extern void FLASH_WriteDisable(void);
extern uint8_t FLASH_ReadStatusReg(void);
extern void FLASH_IC_Check(void);
extern void FLASH_Erase_Sector(uint32_t address);
extern void FLASH_RD_Block_Start(uint32_t address);
extern void FLASH_RD_Block(uint8_t *pbuf, uint32_t len);
extern void FLASH_RD_Block_End(void);
void W25XXX_Read(uint8_t *pbuf, uint32_t address, uint32_t len);
extern void W25XXX_WR_Block(uint8_t *pbuf, uint32_t address, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif
