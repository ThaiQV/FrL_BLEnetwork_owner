/*
 * user_app.h
 *
 *      Author: Nghia Hoang
 */

#ifndef USER_APP_H
#define USER_APP_H

#ifndef MASTER_CORE
#ifdef COUNTER_DEVICE

#include <stdint.h>

#define TIME_APP_TASK_MS				(1)
#define TIME_LED7SEG_TASK_MS 			(100)  // ms
#define TIME_LCD_TASK_MS 				(100)
#define TIME_BUTTON_TASK_MS				(10)
#define TIME_LED_TASK_MS				(10)
#define TIME_DATA_TASK_MS				(1000)
//#define TIME_DTATSTORAGE_TASK_MS		(30 * 1000) //60s
#define TIME_OUT_PRINTF_ERR_MS			(10 * 1000) //60s

typedef enum{
	APP_MODE_TESTS,
	APP_MODE_ACTICS,
    APP_MODE_SELEC,
} app_mode_t;

typedef struct {
	uint16_t pass_product;
	uint16_t err_product;
} count_product_t;

typedef struct {
	uint32_t timetamp;
	uint8_t bt_call;
	uint8_t bt_endcall;
	uint8_t bt_rst;
	count_product_t *count;
    bool is_call;
    bool is_online;
    bool is_wait_rsp;
	app_mode_t mode;
}app_data_t;

typedef enum{
    BT_ENDCALL_ID = 0,
    BT_PEU_ID,
    BT_PED_ID,
    BT_PPD_ID,
    BT_MAX_ID,
} e_bt_id_t;

typedef struct {
	uint32_t (*timetamp)(void);
    uint16_t (* pass_product)(void);
    uint16_t (* err_product)(void);
	bool (*is_call)(void);
    bool (*is_online)(void);
    bool (*is_mode_actic)(void);
	uint8_t * (*mac)(void);
} get_data_t;

void user_app_init(void);
void user_app_loop(void);
void user_app_run(void);
void ct_remove_nwwk(void);
void ct_add_bt_print(char * mess, e_bt_id_t bt_id);

#define get_data_event()   {\
    EVENT_BUTTON_NONE,\
    EVENT_BUTTON_RST_ONCLICK,\
    EVENT_BUTTON_RST_HOLD_3S,\
    EVENT_BUTTON_PPD_HOLD_3S,\
    EVENT_BUTTON_CALL_ONCLICK,\
    EVENT_BUTTON_ENDCALL_ONCLICK,\
    EVENT_BUTTON_PPD_ONCLICK,\
    EVENT_BUTTON_PPU_ONCLICK,\
    EVENT_BUTTON_PED_ONCLICK,\
    EVENT_BUTTON_PEU_ONCLICK,\
    EVENT_BUTTON_RST_PED_HOLD_5S,\
    EVENT_BUTTON_RST_PEU_HOLD_5S,\
    EVENT_BUTTON_CALL_HOLD_3S,\
	EVENT_BUTTON_ENDCALL_HOLD_5S,\
    EVENT_LCD_PRINT_SELECT_MODE_TIMEOUT,\
    EVENT_DATA_START_DONE,\
}

#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/
#endif
