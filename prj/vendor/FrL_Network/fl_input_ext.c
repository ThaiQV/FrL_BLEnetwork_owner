/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_input_ext.c
 *Created on		: Jul 15, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#include "tl_common.h"
#include "fl_input_ext.h"
#include "string.h"
#include <stdint.h>
#include <stdio.h>
#include "fl_nwk_protocol.h"
#include "../Freelux_libs/fl_ble_wifi_protocol.h"

/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
#define FL_IO_READ(x)		gpio_read(x)
#define FL_IO_WRITE(x,y)	gpio_write(x,y)
typedef struct {
	struct {
		gpio_pin_e signal;
	} led;
	struct {
		gpio_pin_e collect;
	} sw;
#ifdef MASTER_CORE
	struct {
		uart_num_e uart_num;
		uart_tx_pin_e tx_pin;
		uart_rx_pin_e rx_pin;
		u32 baudrate;
		dma_chn_e dma_tx_chn;
		dma_chn_e dma_rx_chn;
	} serial;
#endif
}__attribute__((packed)) fl_input_external_t;

volatile fl_input_external_t G_INPUT_EXT;

#ifdef MASTER_CORE

#define FL_RXFIFO_SIZE		72
#define FL_RXFIFO_NUM		2
#define FL_TXFIFO_SIZE		72
#define FL_TXFIFO_NUM		2

#define UART_DATA_LEN    	(FL_TXFIFO_SIZE - 2)   // data max 252

typedef struct {
	unsigned int len; // data max 252
	unsigned char data[UART_DATA_LEN];
} fl_uart_data_t;

_attribute_data_retention_ u8 fl_rx_fifo_b[FL_RXFIFO_SIZE * FL_RXFIFO_NUM] = { 0 };
_attribute_data_retention_ my_fifo_t fl_rx_fifo = { FL_RXFIFO_SIZE, FL_RXFIFO_NUM, 0, 0, fl_rx_fifo_b, };

_attribute_data_retention_ u8 fl_tx_fifo_b[FL_TXFIFO_SIZE * FL_TXFIFO_NUM] = { 0 };
_attribute_data_retention_ my_fifo_t fl_tx_fifo = { FL_TXFIFO_SIZE, FL_TXFIFO_NUM, 0, 0, fl_tx_fifo_b, };

volatile _attribute_data_retention_ unsigned char FLAG_uart_dma_send = 0;

fl_uart_data_t FL_TXDATA; //T_txdata_buf

#endif
/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/

/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/
#ifdef MASTER_CORE
/////////////////////////////////////blc_register_hci_handler for spp////////////////////////////
static void fl_serial_AddLenIn1st(u8 *parr, u8 _size) {
	u8 arr_bkp[UART_DATA_LEN];
	if (_size > UART_DATA_LEN)
		ERR(DRV,"Over Data!!!\r\n");
	memset(arr_bkp,0,sizeof(arr_bkp));
	arr_bkp[0] = _size;
	memcpy(arr_bkp + 1,parr,_size);

	memset(parr,0,_size + 1);
	memcpy(parr,arr_bkp,_size + 1);
}
/**
 * @brief		this function is used to process rx uart data.
 * @param[in]	none
 * @return      0 is ok
 */
static int rx_from_uart_cb(void) //UART data send to Master,we will handler the data as CMD or DATA
{
	if (my_fifo_get(&fl_rx_fifo) == 0) {
		return 0;
	}
	u8* p = my_fifo_get(&fl_rx_fifo);
//	fl_serial_send(p,(unsigned int) p[0]+1);
	u8 data_verify[UART_DATA_LEN];
	memset(data_verify,0,sizeof(data_verify));
	memcpy(data_verify,p+1,p[0]);
	PLOG_Parser_Cmd(data_verify);
	my_fifo_pop(&fl_rx_fifo);
	return 0;
}
/**
 * @brief		this function is used to process tx uart data.
 * @param[in]	none
 * @return      0 is ok
 */
static int tx_to_uart_cb(void) {
	u8 *p = my_fifo_get(&fl_tx_fifo);
	if (p && !FLAG_uart_dma_send) {
		FL_TXDATA.len = (unsigned int) p[0];
		memcpy(&FL_TXDATA.data,p,FL_TXDATA.len);
		LOGA(DRV,"lenData:%d\r\n",FL_TXDATA.len);
		P_PRINTFHEX_A(DRV,FL_TXDATA.data,FL_TXDATA.len,"%s(%d):","Tx",FL_TXDATA.len);
		if (uart_send_dma(G_INPUT_EXT.serial.uart_num,(u8 *) (&FL_TXDATA.data),FL_TXDATA.len)) {
			my_fifo_pop(&fl_tx_fifo);
			FLAG_uart_dma_send = 1;
		}
	}
	return 0;
}

/**
 * @brief		this function is used to process tx uart data.
 * @param[in]
 * @param[in]
 * @return      0 is ok
 */
int fl_serial_send(u8* _data, u8 _len) {
	u8 *p = my_fifo_wptr(&fl_tx_fifo);
	if (!p) {
		return -1;
	}
	memcpy(p,_data,_len);
	my_fifo_next(&fl_tx_fifo);
	return 0;
}

void fl_input_serial_rec(void)
{
	u8* w = fl_rx_fifo.p + (fl_rx_fifo.wptr & (fl_rx_fifo.num - 1)) * fl_rx_fifo.size;
	u32 data_len = uart_get_dma_rev_data_len(G_INPUT_EXT.serial.uart_num,G_INPUT_EXT.serial.dma_rx_chn);
	LOGA(DRV,"DMA Len:%d\r\n",data_len);
	fl_serial_AddLenIn1st(w,(u8)data_len);
	uart_clr_irq_status(G_INPUT_EXT.serial.uart_num,UART_CLR_RX);
	if (w[0] != 0)
	{
		my_fifo_next(&fl_rx_fifo);
		u8* p = fl_rx_fifo.p + (fl_rx_fifo.wptr & (fl_rx_fifo.num - 1)) * fl_rx_fifo.size;
		uart_receive_dma(G_INPUT_EXT.serial.uart_num,(unsigned char *) p,(unsigned int) fl_rx_fifo.size);
	}

//	u8* p = my_fifo_get(&fl_rx_fifo);
//	ble_wifi_protocol_put_queue(&p[1],*p);
}

void fl_input_serial_init(uart_num_e uart_num, uart_tx_pin_e tx_pin, uart_rx_pin_e rx_pin, u32 baudrate) {
	unsigned short div;
	unsigned char bwpc;

	G_INPUT_EXT.serial.uart_num = uart_num;
	G_INPUT_EXT.serial.tx_pin = tx_pin;
	G_INPUT_EXT.serial.rx_pin = rx_pin;
	G_INPUT_EXT.serial.baudrate = baudrate;
	G_INPUT_EXT.serial.dma_rx_chn = DMA2;
	G_INPUT_EXT.serial.dma_tx_chn = DMA3;

	uart_reset(G_INPUT_EXT.serial.uart_num);
	uart_set_pin(G_INPUT_EXT.serial.tx_pin,G_INPUT_EXT.serial.rx_pin); // uart tx/rx pin set
	uart_cal_div_and_bwpc(G_INPUT_EXT.serial.baudrate,sys_clk.pclk * 1000 * 1000,&div,&bwpc);
	uart_set_rx_timeout(G_INPUT_EXT.serial.uart_num,bwpc,12,UART_BW_MUL1);
	uart_init(G_INPUT_EXT.serial.uart_num,div,bwpc,UART_PARITY_NONE,UART_STOP_BIT_ONE);

	uart_clr_irq_mask(G_INPUT_EXT.serial.uart_num,UART_RX_IRQ_MASK | UART_TX_IRQ_MASK | UART_TXDONE_MASK | UART_RXDONE_MASK);
	core_interrupt_enable();

	uart_set_tx_dma_config(G_INPUT_EXT.serial.uart_num,G_INPUT_EXT.serial.dma_tx_chn);
	uart_set_rx_dma_config(G_INPUT_EXT.serial.uart_num,G_INPUT_EXT.serial.dma_rx_chn);

	uart_clr_tx_done(G_INPUT_EXT.serial.uart_num);
	uart_set_irq_mask(G_INPUT_EXT.serial.uart_num,UART_RXDONE_MASK);
	uart_set_irq_mask(G_INPUT_EXT.serial.uart_num,UART_TXDONE_MASK);

	irq_source_e irq_src;
	if (G_INPUT_EXT.serial.uart_num == UART0)
		irq_src = IRQ19_UART0;
	if (G_INPUT_EXT.serial.uart_num == UART1)
		irq_src = IRQ18_UART1;
	plic_interrupt_enable(irq_src);

	u8 *uart_rx_addr = (fl_rx_fifo_b + (fl_rx_fifo.wptr & (fl_rx_fifo.num - 1)) * fl_rx_fifo.size);
	uart_receive_dma(G_INPUT_EXT.serial.uart_num,(unsigned char *) uart_rx_addr,(unsigned int) fl_rx_fifo.size);

	extern void blc_register_hci_handler(void *prx, void *ptx);
	blc_register_hci_handler(rx_from_uart_cb,tx_to_uart_cb);				//customized uart handler

	//passing excution function
	PLOG_RegisterCbk(_Passing_CmdLine);
}

#endif
/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/

/***************************************************
 * @brief 		:Read input status
 *
 * @param[in] 	:none
 *
 * @return	  	:
 ***************************************************/
void fl_input_collection_node_handle(blt_timer_callback_t _fnc, u16 _timeout_ms) {
	static u8 previous_stt = 0;
	u8 state = FL_IO_READ(G_INPUT_EXT.sw.collect);
	if (previous_stt != state)
		delay_ms(100);
	state = FL_IO_READ(G_INPUT_EXT.sw.collect);
	if (previous_stt != state) {
		LOGA(USER,"Collection mode:%d\r\n",state);
		FL_IO_WRITE(G_INPUT_EXT.sw.collect,state);
		if (state)
			blt_soft_timer_add(_fnc,_timeout_ms * 1000); //
		else
			blt_soft_timer_delete(_fnc);
		previous_stt = state;
	}
}
/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/
void fl_input_external_init(void) {

	G_INPUT_EXT.sw.collect = GPIO_PB1;
	gpio_function_en(G_INPUT_EXT.sw.collect);
	gpio_set_output(G_INPUT_EXT.sw.collect,0); 		//disable output
	gpio_set_input(G_INPUT_EXT.sw.collect,1); 		//enable input
	gpio_set_up_down_res(G_INPUT_EXT.sw.collect,GPIO_PIN_PULLUP_10K);

	G_INPUT_EXT.led.signal = GPIO_PB4;
	gpio_function_en(G_INPUT_EXT.led.signal);
	gpio_set_output(G_INPUT_EXT.led.signal,1);
	gpio_set_input(G_INPUT_EXT.led.signal,0);
	gpio_set_up_down_res(G_INPUT_EXT.led.signal,GPIO_PIN_PULLUP_10K);

}
