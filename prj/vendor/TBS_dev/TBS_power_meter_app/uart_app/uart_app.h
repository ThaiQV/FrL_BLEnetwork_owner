/*
 * uart_user.h
 *
 *      Author: hoang
 */

#ifndef UART_APP_H_
#define UART_APP_H_

#ifndef MASTER_CORE
#ifdef POWER_METER_DEVICE

void uart_app_init(void);
void uart_app_loop(void);
void print_uart(const char *fmt, ...);

#endif /* POWER_METER_DEVICE*/
#endif /* MASTER_CORE*/
#endif /* APPS_USER_UART_USER_H_ */
