/*
 * uart_user.h
 *
 *      Author: hoang
 */

#ifndef UART_APP_H_
#define UART_APP_H_


void uart_app_init(void);
void uart_app_loop(void);
void print_uart(const char *fmt, ...);

#endif /* APPS_USER_UART_USER_H_ */
