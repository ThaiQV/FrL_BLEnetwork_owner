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

#define TCA9555_LIB_VERSION               (F("0.4.3"))

#define TCA9555_OK                        0x00
#define TCA9555_PIN_ERROR                 0x81
#define TCA9555_I2C_ERROR                 0x82
#define TCA9555_VALUE_ERROR               0x83
#define TCA9555_PORT_ERROR                0x84

#define TCA9555_INVALID_READ              -100

#define TCA9555_ADDR_DEFAULT  0x20  // Default address

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


#endif /* VENDOR_FRL_NETWORK_PERI_LIBS_TCA9555_H_ */
