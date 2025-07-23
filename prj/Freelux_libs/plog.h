/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: plog.h
 *Created on		: Apr 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#include "types.h"
#include "stdio.h"
#include "stdint.h"
#include "math.h"
#include "string.h"

#define PLOG_ENA         1                    /* printf for debug */ // defined into the setting project

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define str(x) #x
#define xstr(x) str(x)
#define PL_ALL 			0xFFFF

//#define PL_OFF 			0x0000
#define PL_DEFAULT  (PL_OFF|BIT(USER)|BIT(APP)|BIT(ZIG)|BIT(INF)|BIT(FLA)|BIT(INF_FILE)|BIT(PERI)|BIT(BLE))

#define PL_OFF 			0x0000

/* --- PRINTF_BYTE_TO_BINARY macro's --- */
#define PRINTF_BINARY_PATTERN_INT8 "%c%c%c%c%c%c%c%c"
#define PRINTF_BYTE_TO_BINARY_INT8(i)    \
    (((i) & 0x80ll) ? '1' : '0'), \
    (((i) & 0x40ll) ? '1' : '0'), \
    (((i) & 0x20ll) ? '1' : '0'), \
    (((i) & 0x10ll) ? '1' : '0'), \
    (((i) & 0x08ll) ? '1' : '0'), \
    (((i) & 0x04ll) ? '1' : '0'), \
    (((i) & 0x02ll) ? '1' : '0'), \
    (((i) & 0x01ll) ? '1' : '0')

#define PRINTF_BINARY_PATTERN_INT16 \
    PRINTF_BINARY_PATTERN_INT8              PRINTF_BINARY_PATTERN_INT8
#define PRINTF_BYTE_TO_BINARY_INT16(i) \
    PRINTF_BYTE_TO_BINARY_INT8((i) >> 8),   PRINTF_BYTE_TO_BINARY_INT8(i)
#define PRINTF_BINARY_PATTERN_INT32 \
    PRINTF_BINARY_PATTERN_INT16             PRINTF_BINARY_PATTERN_INT16
#define PRINTF_BYTE_TO_BINARY_INT32(i) \
    PRINTF_BYTE_TO_BINARY_INT16((i) >> 16), PRINTF_BYTE_TO_BINARY_INT16(i)
#define PRINTF_BINARY_PATTERN_INT64    \
    PRINTF_BINARY_PATTERN_INT32             PRINTF_BINARY_PATTERN_INT32
#define PRINTF_BYTE_TO_BINARY_INT64(i) \
    PRINTF_BYTE_TO_BINARY_INT32((i) >> 32), PRINTF_BYTE_TO_BINARY_INT32(i)
/* --- end macros --- */

extern volatile uint16_t FmDebug;

typedef enum {
	DRV = 1,
	APP,
	BOOTLOADER,
	BLE,
	ZIG,
	ZIG_GP,
	INF,
	INF_FILE,
	FLA,
	MCU,
	USER,
	PERI,
	ALL,
	DEFAULT,
//	EXSS,
//	RSCOM,
//	DB9,
//	/**@Cmd for testing. Not storage*/
	HELP,
	SETCMD,
	GETCMD,
	RSTCMD,
//	ATCMD,
//	TESTCMD,
//---------------------------------------
	PLEND,
} type_debug_t;

enum {
	E_FONT_BLACK,
	E_FONT_L_RED,
	E_FONT_RED,
	E_FONT_GREEN,
	E_FONT_YELLOW,
	E_FONT_BLUE,
	E_FONT_PURPLE,
	E_FONT_CYAN,
	E_FONT_WHITE,
	E_FONT_ORANGE,
};
#define _COLOR(c, ...)    do {                                      		\
                                        switch (c) {                		\
                                            case DRV:               		\
                                            TERMINAL_FONT_BLACK();  		\
                                            break;                  		\
                                            case APP:               		\
                                            TERMINAL_FONT_CYAN();   		\
                                            break;                  		\
											case BOOTLOADER:        		\
                                            TERMINAL_FONT_WHITE();  		\
                                            break;                  		\
											case BLE:						\
											TERMINAL_FONT_BLUE(); 			\
											break;                      	\
											case ZIG:               		\
											TERMINAL_FONT_PURPLE(); 		\
                                            break;                  		\
                                            case ZIG_GP:               		\
											TERMINAL_FONT_ORANGE(); 		\
                                            break;                  		\
                                            case INF:               		\
                                            TERMINAL_FONT_GREEN();  		\
											break;                  		\
											case INF_FILE:          		\
                                            TERMINAL_FONT_BLUE();   		\
                                            break;                  		\
                                            case FLA:               		\
                                            TERMINAL_FONT_YELLOW(); 		\
                                            break;                  		\
                                            case MCU:              			\
                                            TERMINAL_FONT_WHITE();  		\
                                            break;                  		\
                                            case USER:              		\
                                            TERMINAL_FONT_WHITE();  		\
                                            break;                  		\
                                            case PERI:             		\
                                            TERMINAL_FONT_PURPLE(); 		\
                                            break;                  		\
                                            default:TERMINAL_FONT_DRFAULT();\
                                            break;	\
                                        }                           		\
                                    } while(0)
#if PLOG_ENA
#define PRINTF_COLOR(c, ...)    do {                            \
                                        switch (c) {                \
                                            case E_FONT_BLACK:      \
                                            TERMINAL_FONT_BLACK();  \
                                            break;                  \
                                            case E_FONT_L_RED:      \
                                            TERMINAL_FONT_L_RED();  \
                                            break;                  \
                                            case E_FONT_RED:        \
                                            TERMINAL_FONT_RED();    \
                                            break;                  \
                                            case E_FONT_GREEN:      \
                                            TERMINAL_FONT_GREEN();  \
                                            break;                  \
                                            case E_FONT_YELLOW:     \
                                            TERMINAL_FONT_YELLOW(); \
                                            break;                  \
                                            case E_FONT_BLUE:       \
                                            TERMINAL_FONT_BLUE();   \
                                            break;                  \
                                            case E_FONT_PURPLE:     \
                                            TERMINAL_FONT_PURPLE(); \
                                            break;                  \
                                            case E_FONT_CYAN:       \
                                            TERMINAL_FONT_CYAN();   \
                                            break;                  \
                                            case E_FONT_WHITE:      \
                                            TERMINAL_FONT_WHITE();  \
                                            break;                  \
                                            case E_FONT_ORANGE:     \
                                            TERMINAL_FONT_ORANGE(); \
                                            break;                  \
                                        }                           \
                                        printf(__VA_ARGS__);        \
                                        TERMINAL_FONT_GREEN();      \
                                    } while(0)
#define PRINTF         						printf
#define P_PRINTFHEX(dbg,buffer,size) 				PLG_PrintHexBuffer(dbg,buffer,size,__FILENAME__ , __FUNCTION__, __LINE__)
#define P_PRINTFHEX_A(dbg,buffer,size,fmt,...)		PRINTFHEX_A(dbg,buffer,size,__FILENAME__ , __FUNCTION__, __LINE__,fmt,##__VA_ARGS__)
#else
#define PRINTF(...)         ;
#define SPRINTF(...)        ;
#define PRINTF_COLOR(c, ...);
#define P_PRINTFHEX(dbg,buffer,size);
#define P_PRINTFHEX_A(dbg,buffer,size,fmt,...);
#endif /* CLI_PRINTF */

#if BOOTLOADER_GD32F3
#define PLOG_HEADER()	do{	TERMINAL_FONT_PURPLE(); \
													PRINTF("BOOTLOADER|"); \
													TERMINAL_FONT_BLACK(); \
												}while(0)
#else
#define PLOG_HEADER(...) ;
#endif	
#if PLOG_DATE_ENA																		
#define PLOG_DATE()	do{	TERMINAL_BACK_WHITE(); \
												TERMINAL_FONT_BLACK(); \
												PRINTF("[%s|%s]",__DATE__,__TIME__); \
												TERMINAL_BACK_BLACK(); \
												TERMINAL_FONT_BLACK(); \
											}while(0)																	
#else
#define	PLOG_DATE(...);
#endif											
/* debug----------------------------------------------------------------BEGIN */
#define FORMAT_CONTENT	"[%s] %s(%d):"
#define ERR(dbg,fmt,...)   if(1){ \
														PLOG_DATE(); \
														PLOG_HEADER();	\
														TERMINAL_FONT_L_RED();  \
														PRINTF("[ERR]"FORMAT_CONTENT""fmt"",__FILENAME__,__FUNCTION__, __LINE__ ,##__VA_ARGS__);\
														TERMINAL_FONT_BLACK();}
#define LOGA(dbg,fmt,...)  if ((BIT(dbg) & FmDebug) == BIT(dbg)){                \
														PLOG_DATE(); \
														PLOG_HEADER();	\
														_COLOR(dbg);\
														PRINTF("[LOG]"FORMAT_CONTENT""fmt"",__FILENAME__, __FUNCTION__, __LINE__ ,##__VA_ARGS__);\
														TERMINAL_FONT_BLACK();}
#define LOG_P(dbg,fmt)  		if ((BIT(dbg) & FmDebug) == BIT(dbg)){                \
														PLOG_DATE(); \
														PLOG_HEADER();	\
														_COLOR(dbg);                      \
														PRINTF("[LOG]"FORMAT_CONTENT""fmt"",__FILENAME__ , __FUNCTION__, __LINE__);\
														TERMINAL_FONT_BLACK();}
#define P_PRINTFA(dbg,fmt,...)  	if ((BIT(dbg) & FmDebug) == BIT(dbg)){                \
														PLOG_DATE(); \
														PLOG_HEADER();	\
														_COLOR(dbg);                      \
														PRINTF(fmt,##__VA_ARGS__);\
														TERMINAL_FONT_BLACK();   }
#define P_PRINTF(dbg,fmt)  	if ((BIT(dbg) & FmDebug) == BIT(dbg)){                \
														/*PLOG_DATE();*/ \
														/*PLOG_HEADER();*/	\
														_COLOR(dbg);                      \
														PRINTF(fmt);\
                            TERMINAL_FONT_BLACK();                      \
                        }
#define PRINTFHEX_A(dbg,buffer_p,size,_file_name , _fnc_name, _line,fmt,...)	if((BIT(dbg) & FmDebug) == BIT(dbg)){\
																			uint16_t newline = 0;\
																			uint8_t *buffer=(uint8_t*)&buffer_p; \
																			P_PRINTFA(dbg,"[LOG]"FORMAT_CONTENT,_file_name , _fnc_name, _line);\
																			P_PRINTFA(dbg,fmt,##__VA_ARGS__);\
																			for (uint16_t i = 0; i < (uint16_t)size; i++) {\
																				if (newline != 0) {\
																					newline = 0;\
																				}\
																				P_PRINTFA(dbg, "%02X ", buffer[i]);\
																				if (((i + 1) % 16) == 0)\
																					newline = 1;\
																			}\
																			P_PRINTF(dbg, "\r\n");}

#define P_INFO(fmt,...)			_COLOR(USER);                      \
														PRINTF(fmt,##__VA_ARGS__);\
														TERMINAL_FONT_BLACK();                      
#define NL1()           do { PRINTF("\r\n"); } while(0)
#define NL2()           do { PRINTF("\r\n\r\n"); } while(0)
#define NL3()           do { PRINTF("\r\n\r\n\r\n"); } while(0)

/* debug------------------------------------------------------------------END */
/* assert---------------------------------------------------------------BEGIN */

#define ASSERT(val)                                                     \
if (!(val))                                                             \
{           							\
		printf("\033[1;31m");	\
		printf("(%s) has assert failed at %s.\n", #val, __FUNCTION__);      \
		printf("\033[1;30m");                      \
}

/* assert-----------------------------------------------------------------END */
/* terminal display-----------------------------------------------------BEGIN */

/*
 @links: http://blog.csdn.net/yangguihao/article/details/47734349
 http://blog.csdn.net/kevinshq/article/details/8179252


 @terminal setting commands:
 \033[0m     reset all
 \033[1m     set high brightness
 \03[4m      underline
 \033[5m     flash
 \033[7m     reverse display
 \033[8m     blanking
 \033[30m    --  \033[37m  set font color
 \033[40m    --  \033[47m  set background color
 \033[nA     cursor up up n lines
 \033[nB     cursor move up n lines
 \033[nC     cursor move right n lines
 \033[nD     cursor left up n lines
 \033[y;xH   set cursor position
 \033[2J     clear all display
 \033[K      clear line
 \033[s      save cursor position
 \033[u      restore cursor position
 \033[?25l   cursor invisible
 \33[?25h    cursor visible


 @background color: 40--49           @font color: 30--39
 40: BLACK                           30: black
 41: RED                             31: red
 42: GREEN                           32: green
 43: YELLOW                          33: yellow
 44: BLUE                            34: blue
 45: PURPLE                          35: purple
 46: CYAN                            36: deep green
 47: WHITE                           37: white
 */

/* font color */
#define TERMINAL_FONT_BLACK()       PRINTF("\033[1;30m")
#define TERMINAL_FONT_DRFAULT()     PRINTF("\033[0;30m")
#define TERMINAL_FONT_RED()         PRINTF("\033[0;31m")    /* red */
#define TERMINAL_FONT_GREEN()       PRINTF("\033[0;32m")
#define TERMINAL_FONT_YELLOW()      PRINTF("\033[0;33m")
#define TERMINAL_FONT_BLUE()        PRINTF("\033[0;34m")
#define TERMINAL_FONT_PURPLE()      PRINTF("\033[0;35m")
#define TERMINAL_FONT_CYAN()        PRINTF("\033[0;36m")
#define TERMINAL_FONT_WHITE()       PRINTF("\033[0;37m")
#define TERMINAL_FONT_GRAY()        PRINTF("\033[1;90m")    /* gray */
#define TERMINAL_FONT_ORANGE()      PRINTF("\033[38;5;208m") /* orange (using 256-color) */
#define TERMINAL_FONT_L_RED()       PRINTF("\033[1;31m")    /* light red */
#define TERMINAL_FONT_L_GREEN()     PRINTF("\033[1;32m")    /* light green */
#define TERMINAL_FONT_L_YELLOW()    PRINTF("\033[1;33m")    /* light yellow */
#define TERMINAL_FONT_L_BLUE()      PRINTF("\033[1;34m")    /* light blue */
#define TERMINAL_FONT_L_PURPLE()    PRINTF("\033[1;35m")    /* light purple */
#define TERMINAL_FONT_L_CYAN()      PRINTF("\033[1;36m")    /* light cyan */

/* background color */
#define TERMINAL_BACK_BLACK()       PRINTF("\033[0;40m")
#define TERMINAL_BACK_RED()         PRINTF("\033[1;41m")    /* red */
#define TERMINAL_BACK_GREEN()       PRINTF("\033[0;42m")
#define TERMINAL_BACK_YELLOW()      PRINTF("\033[0;43m")
#define TERMINAL_BACK_BLUE()        PRINTF("\033[0;44m")
#define TERMINAL_BACK_PURPLE()      PRINTF("\033[0;45m")
#define TERMINAL_BACK_CYAN()        PRINTF("\033[0;46m")
#define TERMINAL_BACK_WHITE()       PRINTF("\033[0;47m")
#define TERMINAL_BACK_GRAY()        PRINTF("\033[1;100m")   /* gray background */
#define TERMINAL_BACK_ORANGE()      PRINTF("\033[48;5;208m")/* orange background (256-color) */

#define TERMINAL_BACK_L_RED()       PRINTF("\033[0;41m")    /* light red */
#define TERMINAL_BACK_L_GREEN()     PRINTF("\033[0;42m")    /* light green */
#define TERMINAL_BACK_L_YELLOW()    PRINTF("\033[0;43m")    /* light yellow */
#define TERMINAL_BACK_L_BLUE()      PRINTF("\033[0;44m")    /* light blue */
#define TERMINAL_BACK_L_PURPLE()    PRINTF("\033[0;45m")    /* light purple */
#define TERMINAL_BACK_L_CYAN()      PRINTF("\033[0;46m")    /* light cyan */

/* terminal clear end */
#define TERMINAL_CLEAR_END()        PRINTF("\033[K")

/* terminal clear all */
#define TERMINAL_DISPLAY_CLEAR()    PRINTF("\033[2J")

/* cursor move up */
#define TERMINAL_MOVE_UP(x)         PRINTF("\033[%dA", (x))

/* cursor move down */
#define TERMINAL_MOVE_DOWN(x)       PRINTF("\033[%dB", (x))

/* cursor move left */
#define TERMINAL_MOVE_LEFT(y)       PRINTF("\033[%dD", (y))

/* cursor move right */
#define TERMINAL_MOVE_RIGHT(y)      PRINTF("\033[%dC",(y))

/* cursor move to */
#define TERMINAL_MOVE_TO(x, y)      PRINTF("\033[%d;%dH", (x), (y))

/* cursor reset */
#define TERMINAL_RESET_CURSOR()     PRINTF("\033[H")

/* cursor invisible */
#define TERMINAL_HIDE_CURSOR()      PRINTF("\033[?25l")

/* cursor visible */
#define TERMINAL_SHOW_CURSOR()      PRINTF("\033[?25h")

/* reverse display */
#define TERMINAL_HIGH_LIGHT()       PRINTF("\033[7m")
#define TERMINAL_UN_HIGH_LIGHT()    PRINTF("\033[27m")

/* terminal display-------------------------------------------------------END */
typedef struct {
	u8 major;
	u8 minor;
	u8 patch;
} fl_version_t;

inline s8 plog_IndexOf(u8 *data, u8 *pfind, u8 size) {
	u8 data_len = size + 20;
	if (!data || !pfind || size == 0 || data_len < size)
		return -1;
	for (u8 i = 0; i <= data_len - size; i++) {
		if (memcmp(&data[i],pfind,size) == 0) {
			return i;
		}
	}
	return -1;
}
typedef void (*FncPassing)(type_debug_t,u8*);

void PLOG_Parser_Cmd(u8 *_arr);
void PLOG_Stop(type_debug_t _type);
void PLOG_Start(type_debug_t _type);
void PLOG_HELP(void);
void PLOG_DEVICE_PROFILE(fl_version_t _bootloader, fl_version_t _fw, fl_version_t _hw);
void PLOG_RegisterCbk(FncPassing _fnc);
