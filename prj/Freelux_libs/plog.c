/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: plog.h
 *Created on		: Apr 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/
#include "plog.h"
#include "stdbool.h"
#include "stdarg.h"
#include "stdio.h"
#include <string.h>
#include "ctype.h"
#include "gpio.h"
/******************************************************************************/
/******************************************************************************/
/***                      Private functions declare                          **/
/******************************************************************************/
/******************************************************************************/
void PLOG_Start(type_debug_t _type);
void PLOG_Stop(type_debug_t _type);
//void _set_plog (bool _mode,type_debug_t _type);
void _set_plog(type_debug_t _type, void* arg);
void _process_cmd(type_debug_t _type, void* arg);
void _Simulate(type_debug_t _type, void* arg);
void _help(type_debug_t _type, void* arg);
void _ResetHW(type_debug_t _type, void* arg);
void _ATCmdTest(type_debug_t _type, void * arg);
/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/

typedef struct {
	type_debug_t type;
	char *cmd_txt;
//	void (*Cbk_fnc)(bool,type_debug_t);
	void (*Cbk_fnc)(type_debug_t, void * arg);
} plog_cmd_t;
volatile uint16_t FmDebug = PL_ALL;

const plog_cmd_t plog_cmd[] = {
	{ DRV, ( char *) "p drv", _set_plog },
	{ INF, ( char *) "p inf", _set_plog },
	{ INF_FILE,( char *) "p file", _set_plog },
	{ ZIG, ( char *) "p zig",_set_plog },
	{ ZIG_GP, ( char *) "p zgp",_set_plog },
	{ APP, ( char *) "p app", _set_plog },
	{ BOOTLOADER,( char *) "p bootloader", _set_plog },
	{ FLA,( char *) "p fla", _set_plog },
	{ USER, ( char *) "p users",_set_plog },
	{ PERI, ( char *) "p peri", _set_plog },
	{ BLE,( char *) "p ble", _set_plog },
	{ ALL, ( char *) "p all",_set_plog },
	{ DEFAULT, ( char *) "p default", _set_plog },
	/**@Cmd for testing. Not save*/
	{ HELP, ( char *) "p help", _help },
	{SETCMD,(char *)"p set",_Simulate},
	{GETCMD,(char *)"p get",_Simulate},
//	{TESTCMD,(const char *)"p test",_Simulate},
//	{ATCMD,(const char *)"p atcmd",_ATCmdTest},
	{RSTCMD,(char *)"p reset",_Simulate},
//---------------------------------------
	{ PLEND, ( char *) "None", NULL }, };


/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
FncPassing Passing;
/******************************************************************************/
/******************************************************************************/
/***                            Private functions                            **/
/******************************************************************************/
/******************************************************************************/
void _Simulate(type_debug_t _type,void* arg){
	u8 *_arr = (u8*)arg;
	Passing(_type,_arr);
}

bool bool_dbg_fnc(char *_input) {
	u8 ON[2] = {'o','n'};
	if (plog_IndexOf((u8*)_input, ON, 2)!= -1)
		return true;
//	else if(strncmp(_input,"off",3) == 0)
	return false;
}
/******************************************************************************/
/******************************************************************************/
/***                            Private ProtoThreads                         **/
/******************************************************************************/
/******************************************************************************/
const char help[] ={
	"\r\n*************** FreeLux Log (v1.0) ********************\r\n"
	"** p reset : Reset HW\r\n"
	"** p set utc <value>: set datetime(yyMMddhhmmss)\r\n"
	"** p get runtime: Get time running\r\n"
};

void _help(type_debug_t _type, void* arg) {
	P_INFO("\r\n%s", help);
	P_INFO("** Plog commands line: <cmd> on/off \r\n");
	uint8_t i = 0;
	while (plog_cmd[i].type != PLEND) {
		P_INFO("|-> ");
		P_INFO("%s\r\n", plog_cmd[i].cmd_txt);
		i++;
	}
	P_INFO("******************* (C)FreeLux-2025 **********************\r\n");
}
void _ResetHW(type_debug_t _type, void* arg) {
	ERR(APP,"Reseting..........\r\n");
}

void _set_plog(type_debug_t _type, void * arg) {
	uint32_t buf = 0x00000000U;
	char *_arr = (char*) arg;
	bool _mode = bool_dbg_fnc(_arr);
	if (_mode == true)
		buf = 0xFFFFFFFFU;
	switch ((uint8_t) _type) {
	case ALL: {
		FmDebug = (buf & PL_ALL);
		break;
	}
	case DEFAULT: {
		FmDebug = PL_DEFAULT;
		break;
	}

	default: {
		if (_mode == false)
			FmDebug = FmDebug & (~BIT(_type));
		else
			FmDebug = FmDebug | BIT(_type);
		break;
	}
	};
	//printf("fmDBG("PRINTF_BINARY_PATTERN_INT16 ")\r\n",PRINTF_BYTE_TO_BINARY_INT16(FmDebug));
}
/* 			p|space|TYPE_DBG|space|MODE|\r\n| */
/*	ex: p drv on\r\n : turn on driver debug
 *				p drv off\r\n : turn off driver debug
 */
void PLOG_Parser_Cmd(u8 *_arr) {//, uint8_t _arr_sz
	uint8_t i = 0;
	//P_INFO("%s",_arr);
	while (PLEND != plog_cmd[i].type) {
		if (plog_IndexOf( _arr,(u8*)plog_cmd[i].cmd_txt,strlen(plog_cmd[i].cmd_txt))!= -1) {
//			plog_cmd[i].Cbk_fnc(bool_dbg_fnc(&_arr[strlen(plog_cmd[i].cmd_txt)+1]),plog_cmd[i].type);
			plog_cmd[i].Cbk_fnc(plog_cmd[i].type,&_arr[strlen(plog_cmd[i].cmd_txt) + 1]);
		}
		i++;
	};
}

/******************************************************************************/
/***                            Library callback                             **/
/******************************************************************************/

/******************************************************************************/
/******************************************************************************/
/***                            MainRun functions                            **/
/******************************************************************************/
/******************************************************************************/

/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/
void PLOG_RegisterCbk(FncPassing _fnc){
	Passing=_fnc;
}
void PLOG_Stop(type_debug_t _type) {
	_set_plog(_type, "off");
}
void PLOG_Start(type_debug_t _type) {
	_set_plog(_type, "on");
}

/****************************************************************************************************
 * @brief 		Display the device's profile function
 *
 * @param[in] 	bootloader version,fw version,hw version
 *
 * @return	  	none.
 */
void PLOG_DEVICE_PROFILE(fl_version_t _bootloader, fl_version_t _fw,fl_version_t _hw) {
	const char device_info[] = "\n*****************************************\n"
			"*** FreeLux @2025 - RnD Team		*\n"
			"*** BOOTLOADER : %d.%d.%d			*\n"
			"*** FIRMWARE   : %d.%d.%d			*\n"
			"*** HARDWARE   : %d.%d.%d			*\n"
			"*****************************************\n";
	P_INFO(device_info, _bootloader.major, _bootloader.minor, _bootloader.patch,
			_fw.major, _fw.minor, _fw.patch, _hw.major, _hw.minor, _hw.patch);
}
/****************************************************************************************************
 * @brief 		platform initialization function
 *
 * @param[in] 	none
 *
 * @return	  	1: startup with ram retention;
 *             	0: no ram retention.
 */
void PLOG_HELP(void) {
	_help(HELP, 0);
}
