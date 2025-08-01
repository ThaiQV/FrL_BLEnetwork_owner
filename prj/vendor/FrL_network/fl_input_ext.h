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
void fl_input_external_init(void);
void fl_input_collection_node_handle(blt_timer_callback_t _fnc, u16 _timeout_ms);

#ifdef MASTER_CORE
int fl_serial_send(u8* _data, u8 _len);
void fl_input_serial_init(uart_num_e uart_num, uart_tx_pin_e tx_pin, uart_rx_pin_e rx_pin, u32 baudrate);
void fl_input_serial_rec(void);

#endif

#endif /* VENDOR_FRL_NETWORK_FL_INPUT_EXT_H_ */
