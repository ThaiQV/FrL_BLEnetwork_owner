/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: TBS_dev_config.h
 *Created on		: Aug 22, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#ifndef VENDOR_FRL_NETWORK_TBS_DEV_CONFIG_H_
#define VENDOR_FRL_NETWORK_TBS_DEV_CONFIG_H_

typedef enum{
	TBS_COUNTER = 0x00,
	TBS_POWERMETER=0x01
}tbs_dev_type_e;
#define tbs_device_gettype(x)			(x[sizeof(u32)+6]) //mac[6] + timetamp (u32)
#ifndef MASTER_CORE
#define COUNTER_DEVICE

#endif

#endif /* VENDOR_FRL_NETWORK_TBS_DEV_CONFIG_H_ */
