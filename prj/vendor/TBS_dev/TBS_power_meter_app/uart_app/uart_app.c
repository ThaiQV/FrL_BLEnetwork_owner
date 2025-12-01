/*
 * uart_user.c
 *
 *  Created on: Sep 14, 2023
 *      Author: hoang
 */

#include "uart_app.h"
#include "tl_common.h"
#include "drv_uart.h"
#include <stdarg.h>
#include <stdio.h>
#include "uart_protocol.h"
#include "pmt_data.h"


#define UART_TX_PIN		GPIO_PD6 //GPIO_PE0 //GPIO_PD6
#define UART_RX_PIN		GPIO_PE2


#define UART_PIN_INIT()		do{	\
									drv_uart_pin_set(UART_TX_PIN, UART_RX_PIN);	\
								}while(0)


typedef struct{
	u32 dataLen;
	u8 dataPayload[1];
}uart_rxData_t;

extern u8 *pUartRxBuf;

__attribute__((aligned(4))) u8 moduleTest_uartTxBuf[4] = {0};
__attribute__((aligned(4))) u8 rxBuf_uart[128] = {0};
volatile u8  T_uartPktSentSeqNo = 0;
volatile u32 T_uartPktRecvSeqNo = 0;
volatile u32 T_uartPktRecvLen = 0;
volatile u32 T_uartPktSentExcept = 0;

static void uart_irq_handler_cb(void);

void uart_app_init(void)
{
	irq_disable();
	UART_PIN_INIT();

	drv_uart_init(115200, rxBuf_uart, sizeof(rxBuf_uart)/sizeof(u8), uart_irq_handler_cb);
	irq_enable();
    
}

void uart_app_loop(void)
{
    
    
}

void print_uart(const char *fmt, ...)
{
    char buf[256];        // buffer format
    va_list args;

    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len > 0)
    {
        drv_uart_tx_start((uint8_t *)buf, len);
    }
}

extern uart_driver_t g_uart_driver;

static void uart_irq_handler_cb(void)
{
	uart_rxData_t *rxData = (uart_rxData_t *)rxBuf_uart;
    pmt_protocol_uart_receive(rxData->dataPayload, rxData->dataLen);
}

