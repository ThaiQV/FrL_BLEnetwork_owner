/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: TCA9555.c
 *Created on		: Aug 20, 2025
 *Author			: Freelux RnD Team
 *Description		: ref https://github.com/RobTillaart/TCA9555/blob/master/TCA9555.cpp#L351
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

TCA9555_t G_TCA9555 = {.my_addr = TCA9555_ADDR_DEFAULT,.error = TCA9555_I2C_ERROR};

#define _error 		G_TCA9555.error
#define _address 	G_TCA9555.my_addr

/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/

bool _writeRegister(uint8_t reg, uint8_t value) {
	u8 data[2] = {reg,value};
	u8 rslt = i2c_write(_address,data,2);
	if (rslt != I2C_OK) {
		_error = TCA9555_I2C_ERROR;
		return false;
	}
	_error = TCA9555_OK;
	return true;
}

uint8_t _readRegister(uint8_t reg) {
	u8 data=0;
	int rv = i2c_write(_address,&reg,1);
	if (rv != I2C_OK) {
		_error = TCA9555_I2C_ERROR;
		return rv;
	} else {
		_error = (i2c_read(_address,&data,1)==I2C_OK?TCA9555_OK:TCA9555_I2C_ERROR);
	}
	return data;
}

/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/
uint8_t TCA95xx_getAddress(void)
{
  return _address;
}
bool TCA95xx_isConnected(void) {
	_readRegister(0);
	return (_error == TCA9555_OK);
}

//////////////////////////////////////////////////////////
//
//  1 PIN INTERFACE
//
//  pin = 0..15
//  mode = INPUT, OUTPUT
bool TCA95xx_pinMode1(uint8_t pin, uint8_t mode) {
	if (pin > 15) {
		_error = TCA9555_PIN_ERROR;
		return false;
	}
	if ((mode != INPUT_MODE) && (mode != OUTPUT_MODE)) {
		_error = TCA9555_VALUE_ERROR;
		return false;
	}
	uint8_t CONFREG = TCA9555_CONFIGURATION_PORT_0;
	if (pin > 7) {
		CONFREG = TCA9555_CONFIGURATION_PORT_1;
		pin -= 8;
	}
	uint8_t val = _readRegister(CONFREG);
	uint8_t prevVal = val;
	uint8_t mask = 1 << pin;
	if (mode == INPUT_MODE)
		val |= mask;
	else
		val &= ~mask;
	if (val != prevVal) {
		return _writeRegister(CONFREG,val);
	}
	_error = TCA9555_OK;
	return true;
}

//  pin = 0..15
//  value = LOW_VALUE(0), HIGH_VALUE(not 0)
bool TCA95xx_write1(uint8_t pin, uint8_t value)
{
  if (pin > 15)
  {
    _error = TCA9555_PIN_ERROR;
    return false;
  }
  uint8_t OPR = TCA9555_OUTPUT_PORT_REGISTER_0;
  if (pin > 7)
  {
    OPR = TCA9555_OUTPUT_PORT_REGISTER_1;
    pin -= 8;
  }
  uint8_t val = _readRegister(OPR);
  uint8_t prevVal = val;
  uint8_t mask = 1 << pin;
  if (value) val |= mask;  //  all values <> 0 are HIGH_VALUE.
  else       val &= ~mask;
  if (val != prevVal)
  {
    return _writeRegister(OPR, val);
  }
  _error = TCA9555_OK;
  return true;
}


//  pin = 0..15
uint8_t TCA95xx_read1(uint8_t pin)
{
  if (pin > 15)
  {
    _error = TCA9555_PIN_ERROR;
    return TCA9555_INVALID_READ;
  }
  uint8_t IPR = TCA9555_INPUT_PORT_REGISTER_0;
  if (pin > 7)
  {
    IPR = TCA9555_INPUT_PORT_REGISTER_1;
    pin -= 8;
  }
  uint8_t val = _readRegister(IPR);
  uint8_t mask = 1 << pin;
  _error = TCA9555_OK;
  if (val & mask) return HIGH_VALUE;
  return LOW_VALUE;
}


//  pin = 0..15
//  value = LOW_VALUE(0), HIGH_VALUE ?
bool TCA95xx_setPolarity(uint8_t pin, uint8_t value)
{
  if (pin > 15)
  {
    _error = TCA9555_PIN_ERROR;
    return false;
  }
  if ((value != LOW_VALUE) && (value != HIGH_VALUE))
  {
    _error = TCA9555_VALUE_ERROR;
    return false;
  }
  uint8_t POLREG = TCA9555_POLARITY_REGISTER_0;
  if (pin > 7)
  {
    POLREG = TCA9555_POLARITY_REGISTER_1;
    pin -= 8;
  }
  uint8_t val = _readRegister(POLREG);
  uint8_t prevVal = val;
  uint8_t mask = 1 << pin;
  if (value == HIGH_VALUE) val |= mask;
  else               val &= ~mask;
  if (val != prevVal)
  {
    return _writeRegister(POLREG, val);
  }
  _error = TCA9555_OK;
  return true;
}


//  pin = 0..15
uint8_t TCA95xx_getPolarity(uint8_t pin)
{
  if (pin > 15)
  {
    _error = TCA9555_PIN_ERROR;
    return false;
  }
  uint8_t POLREG = TCA9555_POLARITY_REGISTER_0;
  if (pin > 7) POLREG = TCA9555_POLARITY_REGISTER_1;
  _error = TCA9555_OK;
  uint8_t mask = _readRegister(POLREG);
  return (mask >> pin) == 0x01;
}


//////////////////////////////////////////////////////////
//
//  8 PIN INTERFACE
//
//  port = 0..1
bool TCA95xx_pinMode8(uint8_t port, uint8_t mask)
{
  if (port > 1)
  {
    _error = TCA9555_PORT_ERROR;
    return false;
  }
  _error = TCA9555_OK;
  if (port == 0) return _writeRegister(TCA9555_CONFIGURATION_PORT_0, mask);
  return _writeRegister(TCA9555_CONFIGURATION_PORT_1, mask);
}


//  port = 0..1
//  mask = 0x00..0xFF
bool TCA95xx_write8(uint8_t port, uint8_t mask)
{
  if (port > 1)
  {
    _error = TCA9555_PORT_ERROR;
    return false;
  }
  _error = TCA9555_OK;
  if (port == 0) return _writeRegister(TCA9555_OUTPUT_PORT_REGISTER_0, mask);
  return _writeRegister(TCA9555_OUTPUT_PORT_REGISTER_1, mask);
}


//  port = 0..1
int TCA95xx_read8(uint8_t port)
{
  if (port > 1)
  {
    _error = TCA9555_PORT_ERROR;
    return TCA9555_INVALID_READ;
  }
  _error = TCA9555_OK;
  if (port == 0) return _readRegister(TCA9555_INPUT_PORT_REGISTER_0);
  return _readRegister(TCA9555_INPUT_PORT_REGISTER_1);
}


//  port = 0..1
//  mask = 0x00..0xFF
bool TCA95xx_setPolarity8(uint8_t port, uint8_t mask)
{
  if (port > 1)
  {
    _error = TCA9555_PORT_ERROR;
    return false;
  }
  _error = TCA9555_OK;
  if (port == 0) return _writeRegister(TCA9555_POLARITY_REGISTER_0, mask);
  return _writeRegister(TCA9555_POLARITY_REGISTER_1, mask);
}


//  port = 0..1
uint8_t TCA95xx_getPolarity8(uint8_t port)
{
  if (port > 1)
  {
    _error = TCA9555_PORT_ERROR;
    return 0;
  }
  _error = TCA9555_OK;
  if (port == 0) return _readRegister(TCA9555_POLARITY_REGISTER_0);
  return _readRegister(TCA9555_POLARITY_REGISTER_1);
}


//////////////////////////////////////////////////////////
//
// 16 PIN INTERFACE
//
//  mask = 0x0000..0xFFFF
bool TCA95xx_pinMode16(uint16_t mask)
{
  bool b = true;
  b &= TCA95xx_pinMode8(0, mask & 0xFF);
  b &= TCA95xx_pinMode8(1, mask >> 8);
  return b;
}


//  mask = 0x0000..0xFFFF
bool TCA95xx_write16(uint16_t mask)
{
  bool b = true;
  b &= TCA95xx_write8(0, mask & 0xFF);
  b &= TCA95xx_write8(1, mask >> 8);
  return b;
}


uint16_t TCA95xx_read16()
{
  uint16_t rv = 0;
  rv |= (TCA95xx_read8(1) << 8);
  rv |= TCA95xx_read8(0);
  return rv;
}


//  mask = 0x0000..0xFFFF
bool TCA95xx_setPolarity16(uint16_t mask)
{
  bool b = true;
  b &= TCA95xx_setPolarity8(0, mask & 0xFF);
  b &= TCA95xx_setPolarity8(1, mask >> 8);
  return b;
}


uint8_t TCA95xx_getPolarity16()
{
  uint16_t rv = 0;
  rv |= (TCA95xx_getPolarity8(1) << 8);
  rv |= TCA95xx_getPolarity8(0);
  return rv;
}

/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/
bool TCA95xx_begin(u8 myid, uint16_t mask_mode_hex) {
	_address = (myid<<1);
	if ((_address < (0x20<<1)) || (_address > (0x27<<1)) || !TCA95xx_isConnected())
		return false;
	TCA95xx_pinMode16(mask_mode_hex);

//	if (mode == OUTPUT_MODE) {
//		TCA95xx_pinMode16(mask);
//		TCA95xx_write16(0x0000);
//	} else {
//		TCA95xx_pinMode16(mask);
//	}
	return true;
}
