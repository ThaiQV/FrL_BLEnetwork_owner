/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: pmt_app.h
 *Created on		: Jan 24, 2026
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#ifndef VENDOR_TBS_DEV_TBS_POWERMETER_APP_PMT_APP_H_
#define VENDOR_TBS_DEV_TBS_POWERMETER_APP_PMT_APP_H_

void pmt_init(void);
void pmt_serial_proc(u8* _cmd,u8 _len);

#endif /* VENDOR_TBS_DEV_TBS_POWERMETER_APP_PMT_APP_H_ */
