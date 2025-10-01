/*
 * tca9555_app.c
 *
 *      Author: hoang
 */

#ifndef MASTER_CORE
#include "tl_common.h"
#include "vendor/TBS_dev/TBS_dev_config.h"

#ifdef COUNTER_DEVICE

#include "../../user_lib.h"
#include "tca9555_app.h"

void i2c_init(void)
{
	u8 divClock = (u8)( I2C_CLOCK_SOURCE / (4 * I2C_CLOCK));
	i2c_master_init();
	i2c_set_master_clk(divClock);
	i2c_set_pin(I2C_PIN_SDA, I2C_PIN_SCL);
}

uint8_t user_i2c_read(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint16_t length)
{
	i2c_master_write_read(device_addr, &reg_addr, 1,data, length);
//	printf("i2c read:" );
//	for( u8 i = 0; i < length ; i++)
//		{
//			printf("%x ", data[i]);
//		}
//	printf("\n");
	return 0;
}

uint8_t user_i2c_write(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint16_t length)
{
	u8 pdata[length +1];
	pdata[0] = reg_addr;
	memcpy(&pdata[1], data, length);
//	printf("write data: ");
//	for( u8 i = 0; i < length +1 ; i++)
//	{
//		printf("%x ", pdata[i]);
//	}
//	printf("\n");
	i2c_master_write(device_addr, pdata, length+1);
	return 0;
}

void tca9555_delay_ms(uint32_t ms)
{
	delay_ms(ms);
}

tca9555_config_t tca9555_config = {
    .i2c_write = user_i2c_write,
    .i2c_read = user_i2c_read,
    .tca9555_delay_ms = tca9555_delay_ms,
};
tca9555_handle_t tca9555_handle;

void user_tca9555_app_init(void)
{
	i2c_init();
	tca9555_init(&tca9555_handle, &tca9555_config, TCA9555_ADDR_A2_L_A1_L_A0_L);
}
/*****************************************************************/

#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/
