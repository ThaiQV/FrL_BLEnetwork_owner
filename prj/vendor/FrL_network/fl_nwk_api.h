/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_nwk_api.h
 *Created on		: Aug 19, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#ifndef VENDOR_FRL_NETWORK_FL_NWK_API_H_
#define VENDOR_FRL_NETWORK_FL_NWK_API_H_

#include "fl_nwk_handler.h"
#include "fl_input_ext.h"
#ifdef MASTER_CORE
#else
s8 fl_api_slave_req(u8 _cmdid, u8* _data, u8 _len, fl_rsp_callback_fnc _cb, u32 _timeout_ms,u8 _retry) ;
bool IsJoinedNetwork(void);
bool IsOnline(void);
#endif
#endif /* VENDOR_FRL_NETWORK_FL_NWK_API_H_ */
