/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_input_ext.h
 *Created on		: Jul 15, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#ifndef VENDOR_FRL_NETWORK_FL_INPUT_EXT_H_
#define VENDOR_FRL_NETWORK_FL_INPUT_EXT_H_

#pragma once
#ifdef MASTER_CORE

#define FL_RXFIFO_SIZE		72
#define FL_RXFIFO_NUM		2
#define FL_TXFIFO_SIZE		72
#define FL_TXFIFO_NUM		256

#define UART_DATA_LEN    	(FL_TXFIFO_SIZE - 2)   // data max 252
#endif

typedef enum {
	BUTT_STATE_NONE = 0xFF,
	BUTT_STATE_PRESSnRELEASE = 0x01,
	BUTT_STATE_PRESSnHOLD = 0x02,
}fl_exButton_states_e;

typedef enum{
//	DET_FALLING_EDGE = 0,
//	DET_RISING_EDGE,
	DET_LOW,
	DET_HIGH,
}fl_gpio_mode_detect_e;

#define LED_NETWORK(x)			gpio_set_level(G_INPUT_EXT.led.network,(x==0)?0:1); //
#define LED_MANETA(x)			gpio_set_level(G_INPUT_EXT.led.maneta,(x==0)?0:1); //

typedef u8 (*FncExc)(fl_exButton_states_e,void*);

void fl_input_external_init(void);
void fl_input_collection_node_handle(blt_timer_callback_t _fnc, u16 _timeout_ms);

#ifdef MASTER_CORE
int fl_serial_send(u8* _data, u8 _len);
void fl_input_serial_init(uart_num_e uart_num, uart_tx_pin_e tx_pin, uart_rx_pin_e rx_pin, u32 baudrate);
void fl_input_serial_rec(void);

#endif

#endif /* VENDOR_FRL_NETWORK_FL_INPUT_EXT_H_ */
