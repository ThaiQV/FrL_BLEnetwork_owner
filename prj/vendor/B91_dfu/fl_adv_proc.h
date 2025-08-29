/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_adv_proc.h
 *Created on		: Jul 9, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#ifndef VENDOR_FRL_NETWORK_FL_ADV_PROC_H_
#define VENDOR_FRL_NETWORK_FL_ADV_PROC_H_

typedef enum{
	FL_FROM_MASTER = 0,
	FL_FROM_SLAVE = 0x01,
	FL_FROM_MASTER_ACK = 0x02,
	FL_FROM_SLAVE_ACK = 0x03, //Max
}fl_packet_from_e;

void fl_adv_collection_channel_init(void);
void fl_adv_collection_channel_deinit(void);
void fl_adv_scanner_init(void);
void fl_adv_init(void);
void fl_adv_run(void);
int fl_adv_sendFIFO_add(fl_pack_t _pack);

#endif /* VENDOR_FRL_NETWORK_FL_ADV_PROC_H_ */
