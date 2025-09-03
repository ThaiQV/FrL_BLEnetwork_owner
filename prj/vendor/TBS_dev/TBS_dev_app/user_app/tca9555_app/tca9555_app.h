/*
 * tca9555_app.h
 *
 *      Author: hoang
 */

#ifndef TCA9555_APP_H_
#define TCA9555_APP_H_

/*
 * i2c and tca9555
 */
/* I2C Clock */
#define I2C_CLOCK					1000000//400K

/* I2C slave ID */
#define I2C_SLAVE_ID				0x20
#define I2C_SLAVE_ADDR				0x20<<1//0x8800
#define I2C_SLAVE_ADDR_LEN			3//2

#define DBG_DATA_LEN				16

#define I2C_PIN_SDA			I2C_GPIO_SDA_E2
#define I2C_PIN_SCL			I2C_GPIO_SCL_E0

#define I2C_CLOCK_SOURCE			(sys_clk.pclk * 1000 * 1000)

void user_tca9555_app_init(void);

#endif /* VENDOR_USER_USER_APP_TCA9555_APP_TCA9555_APP_H_ */
