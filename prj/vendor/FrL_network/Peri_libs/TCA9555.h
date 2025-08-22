/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: TCA9555.H
 *Created on		: Aug 20, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#ifndef VENDOR_FRL_NETWORK_PERI_LIBS_TCA9555_H_
#define VENDOR_FRL_NETWORK_PERI_LIBS_TCA9555_H_

#define TCA9555_LIB_VERSION               ("0.4.3")

#define TCA9555_OK                        0x00
#define TCA9555_PIN_ERROR                 0x81
#define TCA9555_I2C_ERROR                 0x82
#define TCA9555_VALUE_ERROR               0x83
#define TCA9555_PORT_ERROR                0x84

#define TCA9555_INVALID_READ              -100

#define TCA9555_ADDR_DEFAULT  	  (0x20 << 1)  // Default address :0x40

// Memory map TCA9555
#define TCA9555_REG_INPUT_0       0x00
#define TCA9555_REG_INPUT_1       0x01
#define TCA9555_REG_OUTPUT_0      0x02
#define TCA9555_REG_OUTPUT_1      0x03
#define TCA9555_REG_POLARITY_0    0x04
#define TCA9555_REG_POLARITY_1    0x05
#define TCA9555_REG_CONFIG_0      0x06
#define TCA9555_REG_CONFIG_1      0x07

#if !defined(TCA9555_PIN_NAMES)
#define TCA9555_PIN_NAMES

#define TCA_P00         0
#define TCA_P01         1
#define TCA_P02         2
#define TCA_P03         3
#define TCA_P04         4
#define TCA_P05         5
#define TCA_P06         6
#define TCA_P07         7
#define TCA_P10         8
#define TCA_P11         9
#define TCA_P12         10
#define TCA_P13         11
#define TCA_P14         12
#define TCA_P15         13
#define TCA_P16         14
#define TCA_P17         15

#endif

#define OUTPUT_MODE 					  0x00
#define INPUT_MODE						  0x01
#define HIGH_VALUE						  0x01
#define LOW_VALUE						  0x00

//  mask has only meaning when mode == OUTPUT
bool TCA95xx_begin(u8 myid, uint16_t mask_mode_hex);
bool TCA95xx_isConnected(void);
uint8_t TCA95xx_getAddress(void);

//  1 PIN INTERFACE
//  pin    = 0..15
//  mode  = INPUT, OUTPUT       (INPUT_PULLUP is not supported)
//  value = LOW, HIGH
bool TCA95xx_pinMode1(uint8_t pin, uint8_t mode);
bool TCA95xx_write1(uint8_t pin, uint8_t value);
uint8_t TCA95xx_read1(uint8_t pin);
bool TCA95xx_setPolarity(uint8_t pin, uint8_t value);    //  input pins only.
uint8_t TCA95xx_getPolarity(uint8_t pin);

//  8 PIN INTERFACE
//  port  = 0..1
//  mask  = bit pattern
bool TCA95xx_pinMode8(uint8_t port, uint8_t mask);
bool TCA95xx_write8(uint8_t port, uint8_t mask);
int TCA95xx_read8(uint8_t port);
bool TCA95xx_setPolarity8(uint8_t port, uint8_t value);
uint8_t TCA95xx_getPolarity8(uint8_t port);

//  16 PIN INTERFACE
//  wraps 2x 8 PIN call.
//  opportunistic implementation of functions
//  needs error checking in between calls
//  mask  = bit pattern
bool TCA95xx_pinMode16(uint16_t mask);
bool TCA95xx_write16(uint16_t mask);
uint16_t TCA95xx_read16();
bool TCA95xx_setPolarity16(uint16_t mask);
uint8_t TCA95xx_getPolarity16();

#endif /* VENDOR_FRL_NETWORK_PERI_LIBS_TCA9555_H_ */
