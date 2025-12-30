/********************************************************************************************************
 * @file    drv_uart.c
 *
 * @brief   This is the source file for drv_uart
 *
 * @author  Zigbee Group
 * @date    2021
 *
 * @par     Copyright (c) 2021, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#include "stdlib.h"
#include "../tl_common.h"
#include "drv_uart.h"
#include "../power_meter_app.h"

#define BUILD_U32(b0, b1, b2, b3) ((unsigned int)((((b3) & 0x000000FF) << 24) + (((b2) & 0x000000FF) << 16) + (((b1) & 0x000000FF) << 8) + ((b0) & 0x000000FF)))
#define UART_RCV_DMA_LEN_FIX()                                                     \
	do                                                                             \
	{                                                                              \
		u32 rcvDataLen = uart_get_dma_rev_data_len(UART_IDX, UART_DMA_CHANNEL_RX); \
		if (rxBuf_uart)                                                            \
		{                                                                          \
			rxBuf_uart[0] = (u8)(rcvDataLen);                                      \
			rxBuf_uart[1] = (u8)(rcvDataLen >> 8);                                 \
			rxBuf_uart[2] = (u8)(rcvDataLen >> 16);                                \
			rxBuf_uart[3] = (u8)(rcvDataLen >> 24);                                \
		}                                                                          \
	} while (0)

// u8 *pUartRxBuf = NULL;
extern u8 rxBuf_uart[128];
static u32 uartRxBufLen = 0;

static u8 uart_dma_send(u8 *pBuf);

drv_uart_t myUartDriver = {
	.status = UART_STA_IDLE,
	.recvCb = NULL,
	.send = uart_dma_send,
};

static u8 *uartDrvTxBuf = NULL;

u8 drv_uart_init(u32 baudrate, u8 *rxBuf, u16 rxBufLen, uart_irq_callback uartRecvCb)
{
	myUartDriver.status = UART_STA_IDLE;
	myUartDriver.recvCb = uartRecvCb;

	if (uartDrvTxBuf)
	{
		uartDrvTxBuf = NULL;
	}
	//	PMT_LOGA("rxBuf: %x\n", rxBuf);
	if ((rxBuf == NULL) || (rxBufLen <= 4))
	{
		return 1;
	}

	u16 div = 0;
	u8 bwpc = 0;

	//	pUartRxBuf = rxBuf;
	uartRxBufLen = rxBufLen - 4;
	//	PMT_LOGA("rxBuf_uart: %x\n", rxBuf_uart);

	uart_reset(UART_IDX);

	uart_cal_div_and_bwpc(baudrate, UART_CLOCK_SOURCE, &div, &bwpc);

	uart_set_rx_timeout(UART_IDX, bwpc, 12, UART_BW_MUL2);

	uart_init(UART_IDX, div, bwpc, UART_PARITY_NONE, UART_STOP_BIT_ONE);

	uart_set_tx_dma_config(UART_IDX, UART_DMA_CHANNEL_TX);
	uart_set_rx_dma_config(UART_IDX, UART_DMA_CHANNEL_RX);

	uart_receive_dma(UART_IDX, rxBuf_uart + 4, uartRxBufLen);
	//	PMT_LOGA("rxBuf_uart: %x\n", rxBuf_uart);

	uart_clr_tx_done(UART_IDX);

	uart_clr_irq_mask(UART_IDX, UART_RX_IRQ_MASK | UART_TX_IRQ_MASK | UART_TXDONE_MASK | UART_RXDONE_MASK);

	uart_set_irq_mask(UART_IDX, UART_RXDONE_MASK | UART_TXDONE_MASK);
	plic_interrupt_enable((UART_IDX == UART0) ? IRQ19_UART0 : IRQ18_UART1);

	return 0;
}

void drv_uart_pin_set(u32 txPin, u32 rxPin)
{
	uart_set_pin(txPin, rxPin);
}

void drv_uart_rx_irq_handler(void)
{
	bool uartRxErr = 0;
	// PMT_LOGA("rxBuf_uart: %x\n", rxBuf_uart);

	if ((uart_get_irq_status(UART_IDX, UART_RX_ERR)))
	{
		uartRxErr = 1;
	}
	else
	{
		/* Fix lost DMA length information, must before clear irq status. */
		UART_RCV_DMA_LEN_FIX();
	}
	uart_clr_irq_status(UART_IDX, UART_CLR_RX);

	/* Need to reconfigure RX DMA. */
	uart_receive_dma(UART_IDX, rxBuf_uart + 4, uartRxBufLen);

	if (uartRxErr)
	{
		return;
	}

	if (myUartDriver.recvCb)
	{
		myUartDriver.recvCb();
	}
}

void drv_uart_tx_irq_handler(void)
{
	uart_clr_tx_done(UART_IDX);

	if (uartDrvTxBuf)
	{
		free(uartDrvTxBuf);
		uartDrvTxBuf = NULL;
	}
	myUartDriver.status = UART_STA_TX_DONE;
}

static u8 uart_dma_send(u8 *pBuf)
{
	u32 len = BUILD_U32(pBuf[0], pBuf[1], pBuf[2], pBuf[3]);

	return uart_send_dma(UART_IDX, pBuf + 4, len);
}

bool uart_tx_done(void)
{
	return ((myUartDriver.status == UART_STA_TX_DONE) ? TRUE : FALSE);
}

bool uart_is_idle(void)
{
	return ((myUartDriver.status == UART_STA_IDLE) ? TRUE : FALSE);
}

u8 drv_uart_tx_start(u8 *data, u32 len)
{
	if (!uart_is_idle())
	{
		while (!uart_tx_done())
			;
	}

	if (!uartDrvTxBuf)
	{
		uartDrvTxBuf = (u8 *)malloc(len + 4);
		if (uartDrvTxBuf)
		{
			myUartDriver.status = UART_STA_TX_DOING;
			uartDrvTxBuf[0] = (u8)(len);
			uartDrvTxBuf[1] = (u8)(len >> 8);
			uartDrvTxBuf[2] = (u8)(len >> 16);
			uartDrvTxBuf[3] = (u8)(len >> 24);
			memcpy(uartDrvTxBuf + 4, data, len);
			if (myUartDriver.send)
			{
				while (!myUartDriver.send(uartDrvTxBuf))
					;
				return 1;
			}
		}
	}
	return 0;
}

void drv_uart_exceptionProcess(void)
{
}
