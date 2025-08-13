/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_adv_repeat.h
 *Created on		: Jul 10, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#ifndef VENDOR_FRL_NETWORK_FL_ADV_REPEAT_H_
#define VENDOR_FRL_NETWORK_FL_ADV_REPEAT_H_

#include "types.h"
#include "stdbool.h"
#include "math.h"

//#define REPEAT_LEVEL			2
//#define REPEAT_RSSI_THRES		-60

void fl_repeater_init(void);
void fl_repeat_run(fl_pack_t *_pack);

#endif /* VENDOR_FRL_NETWORK_FL_ADV_REPEAT_H_ */
