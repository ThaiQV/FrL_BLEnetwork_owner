/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: TCA9555.c
 *Created on		: Aug 20, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/


#include "tl_common.h"
#include "TCA9555.h"
/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
//Telink B91 => define i2c functions
#define i2c_write		i2c_master_write
#define i2c_read		i2c_master_read
#define i2c_write_read	i2c_master_write_read

#define I2C_OK			1
#define I2C_ERR			0

/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
//  REGISTERS P23-24
#define TCA9555_INPUT_PORT_REGISTER_0     0x00    //  read()
#define TCA9555_INPUT_PORT_REGISTER_1     0x01
#define TCA9555_OUTPUT_PORT_REGISTER_0    0x02    //  write()
#define TCA9555_OUTPUT_PORT_REGISTER_1    0x03
#define TCA9555_POLARITY_REGISTER_0       0x04    //  get/setPolarity()
#define TCA9555_POLARITY_REGISTER_1       0x05
#define TCA9555_CONFIGURATION_PORT_0      0x06    //  pinMode()
#define TCA9555_CONFIGURATION_PORT_1      0x07

typedef struct {
	uint8_t my_addr;
	uint8_t error;
}__attribute__((packed)) TCA9555_t;

TCA9555_t G_TCA9555 = {.my_addr = TCA9555_ADDR_DEFAULT};

#define _error 		G_TCA9555.error
#define _address 	G_TCA9555.my_addr

/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/
//bool _writeRegister(uint8_t reg, uint8_t value) {
//	u8 rslt = i2c_write(_address,reg,value);
//	if (_wire->endTransmission() != 0) {
//		_error = TCA9555_I2C_ERROR;
//		return false;
//	}
//	_error = TCA9555_OK;
//	return true;
//}
//
//
//uint8_t _readRegister(uint8_t reg)
//{
//
//  int rv = 	i2c_master_read(reg,tmp,2);
//  if (rv != I2C_OK)
//  {
//    _error = TCA9555_I2C_ERROR;
//    return rv;
//  }
//  else
//  {
//    _error = TCA9555_OK;
//  }
//  _wire->requestFrom(_address, (uint8_t)1);
//  return _wire->read();
//}


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
