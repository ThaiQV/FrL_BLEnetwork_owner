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

void pmt_reset_energy(void *_arg, u8 _size);
void pmt_reset_workingtime(u8 _chn);
void pmt_info(void *_arg, u8 _size);
void pmt_init(void);
void pmt_serial_proc(u8* _cmd,u8 _len);
void pmt_main(void);

#endif /* VENDOR_TBS_DEV_TBS_POWERMETER_APP_PMT_APP_H_ */
