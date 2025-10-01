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
#include "gpio.h"
#include "fl_nwk_protocol.h"
#include "fl_ble_wifi.h"
//Lib
#include "../TBS_dev/TBS_dev_config.h"
#include "Peri_libs/TCA9555.h"

/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
#define FL_IO_SCAN_INTERVAL				20 //ms

#define FL_IO_READ(x)					gpio_get_level(x)
#define FL_IO_WRITE(x,y)				gpio_set_level(x,y)

#define PRESSnHOLD_DUTY					FL_IO_SCAN_INTERVAL*50  //ms->s
#define PRESSnRELEASE_DUTY				FL_IO_SCAN_INTERVAL*2  	//ms

typedef bool (*FucRead)(gpio_pin_e);

typedef struct {
	gpio_pin_e pin;
	bool cur_stt;
	u32 t_high;
	u32 t_low;
	fl_gpio_mode_detect_e mode;
	fl_exButton_states_e status;
	FucRead pin_read;
	FncExc exc;
}__attribute__((packed)) fl_exIO_t;

typedef struct {
#ifdef MASTER_CORE
	struct {
		uart_num_e uart_num;
		uart_tx_pin_e tx_pin;
		uart_rx_pin_e rx_pin;
		u32 baudrate;
		dma_chn_e dma_tx_chn;
		dma_chn_e dma_rx_chn;
	}serial;
#else
	struct {
		u8 id;
		gpio_pin_e irq_pin; //irq - not use
		i2c_sda_pin_e sda;
		i2c_scl_pin_e scl;
	} exIO;
	struct {
		gpio_pin_e network;
		gpio_pin_e maneta;
	} led;
#endif

}__attribute__((packed)) fl_input_external_t;

fl_input_external_t G_INPUT_EXT;

fl_exIO_t G_IN_POLLING[10];

#define  IN_POLLING_SIZE		(sizeof(G_IN_POLLING)/sizeof(G_IN_POLLING[0]))

#ifdef MASTER_CORE

typedef struct {
	unsigned int len; // data max 252
	unsigned char data[UART_DATA_LEN];
}fl_uart_data_t;

u8 fl_rx_fifo_b[FL_RXFIFO_SIZE * FL_RXFIFO_NUM] = {0};
my_fifo_t fl_rx_fifo = {FL_RXFIFO_SIZE, FL_RXFIFO_NUM, 0, 0, fl_rx_fifo_b,};

u8 fl_tx_fifo_b[FL_TXFIFO_SIZE * FL_TXFIFO_NUM] = {0};
my_fifo_t fl_tx_fifo = {FL_TXFIFO_SIZE, FL_TXFIFO_NUM, 0, 0, fl_tx_fifo_b,};

volatile unsigned char FLAG_uart_dma_send = 0;

fl_uart_data_t FL_TXDATA; //T_txdata_buf

#endif
/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/

void InitPOLLING(void) {
	for (u8 i = 0; i < IN_POLLING_SIZE; i++) {
		G_IN_POLLING[i].exc = 0;
		G_IN_POLLING[i].pin_read = 0;
	}
}
s8 RegisterPOLLING(fl_exIO_t _io) {
	u8 i = 0;
	for (i = 0; i < IN_POLLING_SIZE; i++) {
		//LOGA(USER,"(%d/%d)%d | %d \r\n",i,IN_POLLING_SIZE,*G_IN_POLLING[i].exc,*_io.exc);
		if ((u32) G_IN_POLLING[i].exc == (u32) _io.exc) {
			break;
		}
		if (G_IN_POLLING[i].exc == 0) {
			G_IN_POLLING[i] = _io;
			G_IN_POLLING[i].cur_stt = _io.pin_read(_io.pin);
			G_IN_POLLING[i].t_high = 0;
			G_IN_POLLING[i].t_low = 0;
			G_IN_POLLING[i].status = BUTT_STATE_NONE;
			G_IN_POLLING[i].pin_read = _io.pin_read;
			return i;
		}
	}
	ERR(USER,"Register POLLING Err!!!\r\n");
	return -1;
}
/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/
#ifdef MASTER_CORE
/////////////////////////////////////blc_register_hci_handler for spp////////////////////////////
static void fl_serial_AddLenIn1st(u8 *parr, u8 _size) {
	u8 arr_bkp[UART_DATA_LEN];
	if (_size > UART_DATA_LEN) {
		ERR(DRV,"Over Data!!!\r\n");
		_size = UART_DATA_LEN;
	}
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
//Add WIFI <-> BLE process cmd
	fl_ble_wifi_proc(p);
//	fl_serial_send(p,(unsigned int) p[0]+1);
	u8 data_verify[FL_RXFIFO_SIZE];
	memset(data_verify,0,sizeof(data_verify));
	memcpy(data_verify,p + 1,p[0]);
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
		memcpy(&FL_TXDATA.data,&p[1],FL_TXDATA.len);
		if (uart_send_dma(G_INPUT_EXT.serial.uart_num,(u8 *) (&FL_TXDATA.data),FL_TXDATA.len)) {
			my_fifo_pop(&fl_tx_fifo);
			FLAG_uart_dma_send = 1;
//			LOGA(DRV,"lenData:%d\r\n",FL_TXDATA.len);
//			P_PRINTFHEX_A(DRV,FL_TXDATA.data,FL_TXDATA.len,"%s(%d):","Tx",FL_TXDATA.len);
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
	if (!p || _len >= UART_DATA_LEN) {
		return -1;
	}
//	u8 data_verify[UART_DATA_LEN];
//	memset(data_verify,0,sizeof(data_verify));
	p[0] = _len;
	memcpy(&p[1],_data,_len);
	my_fifo_next(&fl_tx_fifo);
	return 0;
}

void fl_input_serial_rec(void) {
	u32 data_len = uart_get_dma_rev_data_len(G_INPUT_EXT.serial.uart_num,G_INPUT_EXT.serial.dma_rx_chn);
	if(data_len == 0 || data_len > UART_DATA_LEN){
		ERR(MCU,"OVER DATA!!!\r\n");
//		uart_clr_irq_status(G_INPUT_EXT.serial.uart_num,UART_CLR_RX);
//		uart_reset(G_INPUT_EXT.serial.uart_num);
		u8* cur_addr = fl_rx_fifo.p + (fl_rx_fifo.wptr & (fl_rx_fifo.num - 1)) * fl_rx_fifo.size;
		uart_receive_dma(G_INPUT_EXT.serial.uart_num,(unsigned char *)cur_addr,(unsigned int) fl_rx_fifo.size);
		return;
	}
	u8* w = fl_rx_fifo.p + (fl_rx_fifo.wptr & (fl_rx_fifo.num - 1)) * fl_rx_fifo.size;
	LOGA(DRV,"DMA Len:%d\r\n",data_len);
	fl_serial_AddLenIn1st(w,(u8) data_len);
	uart_clr_irq_status(G_INPUT_EXT.serial.uart_num,UART_CLR_RX);
	if (w[0] != 0) {
		my_fifo_next(&fl_rx_fifo);
		u8* p = fl_rx_fifo.p + (fl_rx_fifo.wptr & (fl_rx_fifo.num - 1)) * fl_rx_fifo.size;
		uart_receive_dma(G_INPUT_EXT.serial.uart_num,(unsigned char *) p,(unsigned int) fl_rx_fifo.size);
	}
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

	uart_clr_irq_mask(G_INPUT_EXT.serial.uart_num,UART_ERR_IRQ_MASK|UART_RX_IRQ_MASK | UART_TX_IRQ_MASK | UART_TXDONE_MASK | UART_RXDONE_MASK);


	uart_set_tx_dma_config(G_INPUT_EXT.serial.uart_num,G_INPUT_EXT.serial.dma_tx_chn);
	uart_set_rx_dma_config(G_INPUT_EXT.serial.uart_num,G_INPUT_EXT.serial.dma_rx_chn);

	uart_clr_tx_done(G_INPUT_EXT.serial.uart_num);
//	uart_set_irq_mask(G_INPUT_EXT.serial.uart_num,UART_RXDONE_MASK);
	uart_set_irq_mask(G_INPUT_EXT.serial.uart_num,UART_ERR_IRQ_MASK|UART_RX_IRQ_MASK | UART_TXDONE_MASK | UART_RXDONE_MASK);

	core_interrupt_enable();
	irq_source_e irq_src;
	if (G_INPUT_EXT.serial.uart_num == UART0)
	irq_src = IRQ19_UART0;
	if (G_INPUT_EXT.serial.uart_num == UART1)
	irq_src = IRQ18_UART1;
	plic_interrupt_enable(irq_src);

	u8 *uart_rx_addr = (fl_rx_fifo_b + (fl_rx_fifo.wptr & (fl_rx_fifo.num - 1)) * fl_rx_fifo.size);
	uart_receive_dma(G_INPUT_EXT.serial.uart_num,(unsigned char *) uart_rx_addr,(unsigned int) fl_rx_fifo.size);

	extern void blc_register_hci_handler(void *prx, void *ptx);
	blc_register_hci_handler(rx_from_uart_cb,tx_to_uart_cb);//customized uart handler

//passing excution function
	PLOG_RegisterCbk(_Passing_CmdLine);

}

#endif
/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/

#ifdef COUNTER_DEVICE

//u8 _button1_excute(fl_exButton_states_e _state, void *_data) {
//	u32 *time_tick = (u32*)_data;
//	LOGA(PERI,"BUTT 1 %s (%d ms)\r\n",_state==BUTT_STATE_PRESSnHOLD?"Press & hold":"Press & Release",
//			(clock_time()-*time_tick)/SYSTEM_TIMER_TICK_1MS);
//	//Must to clear status if done
//	return BUTT_STATE_NONE;
//	//else return _state;
//}

//u8 _button2_excute(fl_exButton_states_e _state, void *_data) {
//	u32 *time_tick = (u32*) _data;
//	LOGA(PERI,"BUTT 2 %s (%d ms)\r\n",_state == BUTT_STATE_PRESSnHOLD ? "Press & hold" : "Press & Release",
//			(clock_time()-*time_tick)/SYSTEM_TIMER_TICK_1MS);
////Must to clear status if done
//	return BUTT_STATE_NONE;
////else return _state;
//}
#endif
/***************************************************
 * @brief 		: polling scan input states (buttons, switch,...)
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
int _scan_external_input(void) {
#define POLLING_READ(indx) G_IN_POLLING[indx].pin_read(G_IN_POLLING[indx].pin)
	s8 indx = 0;
//	static u8 detect_edge  = 0;

	for (indx = 0; indx < IN_POLLING_SIZE && G_IN_POLLING[indx].exc != 0; ++indx) {
		if (POLLING_READ(indx)) {
			G_IN_POLLING[indx].t_high += FL_IO_SCAN_INTERVAL;
		} else {
			G_IN_POLLING[indx].t_low += FL_IO_SCAN_INTERVAL;
		}
		//debug
//		if(POLLING_READ(indx) != detect_edge){
//			detect_edge = POLLING_READ(indx);
//			LOGA(PERI,"t_high = %d,t_low = %d\r\n",G_IN_POLLING[indx].t_high,G_IN_POLLING[indx].t_low);
//		}
		//check action
		if (G_IN_POLLING[indx].mode == DET_LOW) {
			//press n release
			if (G_IN_POLLING[indx].t_low < PRESSnHOLD_DUTY && G_IN_POLLING[indx].t_low >= PRESSnRELEASE_DUTY && POLLING_READ(indx)== 1) {
				if(G_IN_POLLING[indx].status == BUTT_STATE_NONE) {
					G_IN_POLLING[indx].status = BUTT_STATE_PRESSnRELEASE;
					G_IN_POLLING[indx].status = G_IN_POLLING[indx].exc(G_IN_POLLING[indx].status,(void*) &G_IN_POLLING[indx].t_low);
				}
				G_IN_POLLING[indx].t_low = 0;
				G_IN_POLLING[indx].t_high = 0;
				continue;
			}
			//press n hold
			if (G_IN_POLLING[indx].t_low > PRESSnHOLD_DUTY && POLLING_READ(indx) == 0) {
				if(G_IN_POLLING[indx].status == BUTT_STATE_NONE) {
					G_IN_POLLING[indx].status = BUTT_STATE_PRESSnHOLD;
					G_IN_POLLING[indx].status = G_IN_POLLING[indx].exc(G_IN_POLLING[indx].status,(void*) &G_IN_POLLING[indx].t_low);
				}
				G_IN_POLLING[indx].t_high = 0;
				G_IN_POLLING[indx].t_low= 0;
				continue;
			}
		}
		if (G_IN_POLLING[indx].mode == DET_HIGH) {
			//press n release
			if (G_IN_POLLING[indx].t_high < PRESSnHOLD_DUTY && G_IN_POLLING[indx].t_high >= PRESSnRELEASE_DUTY && POLLING_READ(indx)== 0) {
				if(G_IN_POLLING[indx].status == BUTT_STATE_NONE) {
					G_IN_POLLING[indx].status = BUTT_STATE_PRESSnRELEASE;
					G_IN_POLLING[indx].status = G_IN_POLLING[indx].exc(G_IN_POLLING[indx].status,(void*) &G_IN_POLLING[indx].t_high);
				}
				G_IN_POLLING[indx].t_low = 0;
				G_IN_POLLING[indx].t_high = 0;
				continue;
			}
			//press n hold
			if (G_IN_POLLING[indx].t_high > PRESSnHOLD_DUTY && POLLING_READ(indx) == 0) {
				if(G_IN_POLLING[indx].status == BUTT_STATE_NONE) {
					G_IN_POLLING[indx].status = BUTT_STATE_PRESSnHOLD;
					G_IN_POLLING[indx].status = G_IN_POLLING[indx].exc(G_IN_POLLING[indx].status,(void*) &G_IN_POLLING[indx].t_high);
				}
				G_IN_POLLING[indx].t_high = 0;
				G_IN_POLLING[indx].t_low= 0;
				continue;
			}
		}
	}
	return 0;
}
#ifndef MASTER_CORE
/***************************************************
 * @brief 		:initialization the External GPIO via I2C protocol
 *
 * @param[in] 	:sda+ scl: i2c protocol pin
 *
 * @return	  	:none
 *
 ***************************************************/
void fl_ExIO_init(i2c_sda_pin_e _sda, i2c_scl_pin_e _scl, gpio_pin_e _irq_pin) {

#define I2C_CLOCK						(4*100000)//n*100K
#define I2C_CLOCK_SOURCE				(sys_clk.pclk * 1000 * 1000)

	G_INPUT_EXT.exIO.id = 0x20;
	G_INPUT_EXT.exIO.sda = _sda;
	G_INPUT_EXT.exIO.scl = _scl;
	G_INPUT_EXT.exIO.irq_pin = _irq_pin;

//I2c init
	u8 divClock = (u8) ( I2C_CLOCK_SOURCE / (I2C_CLOCK));
	i2c_master_init();
	i2c_set_master_clk(divClock);
	i2c_set_pin(G_INPUT_EXT.exIO.sda,G_INPUT_EXT.exIO.scl);
//init TCA chip
	u8 rslt = TCA95xx_begin(G_INPUT_EXT.exIO.id,0x00FF);
	LOGA(PERI,"TCA95xxx init 0x%02X(%d): v %s\r\n",TCA95xx_getAddress(),rslt,TCA9555_LIB_VERSION);
//Init led signal
	G_INPUT_EXT.led.network = GPIO_PA6;
	G_INPUT_EXT.led.maneta = GPIO_PA5;

	gpio_function_en(G_INPUT_EXT.led.network | G_INPUT_EXT.led.maneta);
	gpio_set_output(G_INPUT_EXT.led.network | G_INPUT_EXT.led.maneta,1);	//enable output
	gpio_set_input(G_INPUT_EXT.led.network | G_INPUT_EXT.led.maneta,0);	//disable input
	gpio_set_level(G_INPUT_EXT.led.network | G_INPUT_EXT.led.maneta,0);
}
#endif
/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/
void fl_input_external_init(void) {
//init POLLING Container
	InitPOLLING();
#ifndef MASTER_CORE //use to test
	fl_ExIO_init(I2C_GPIO_SDA_E2,I2C_GPIO_SCL_E0,GPIO_PD0);
//	//	//register function callback
	fl_exIO_t GPIO_IN;
	s8 regis=0;
	extern u8 TEST_Buttons(fl_exButton_states_e _state, void *_data);
	GPIO_IN.exc = &TEST_Buttons;
	GPIO_IN.status = BUTT_STATE_NONE;
	GPIO_IN.mode = DET_LOW;
	GPIO_IN.pin_read = (FucRead) &TCA95xx_read1;
	GPIO_IN.pin = (gpio_pin_e) TCA_P11;
//Register polling callback
	regis = RegisterPOLLING(GPIO_IN);
	LOGA(PERI,"Button(%d)Calling Register :%d\r\n",GPIO_IN.pin_read(GPIO_IN.pin),regis);

	extern u8 TEST_Buttons_RST(fl_exButton_states_e _state, void *_data);
	GPIO_IN.exc = &TEST_Buttons_RST;
	GPIO_IN.status = BUTT_STATE_NONE;
	GPIO_IN.mode = DET_LOW;
	GPIO_IN.pin_read = (FucRead) &TCA95xx_read1;
	GPIO_IN.pin = (gpio_pin_e) TCA_P10;
//Register polling callback
	regis = RegisterPOLLING(GPIO_IN);
	LOGA(PERI,"Button(%d)Reset Register :%d\r\n",GPIO_IN.pin_read(GPIO_IN.pin),regis);
#endif
	/* --- Polling read input --- */
	blt_soft_timer_add(_scan_external_input,FL_IO_SCAN_INTERVAL * 1000); //ms
}
