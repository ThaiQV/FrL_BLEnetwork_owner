/*
 * user_app.c
 *
 *      Author: Nghia Hoang
 */

#ifndef MASTER_CORE

#include "vendor/TBS_dev/TBS_dev_config.h"

#ifdef COUNTER_DEVICE

#include "stdio.h"
#include "../user_lib.h"
#include "led_7_seg_app/led_7_seg_app.h"
#include "tca9555_app/tca9555_app.h"
#include "lcd_app/lcd_app.h"
#include "vendor/FrL_network/fl_nwk_handler.h"
#include "vendor/FrL_network/fl_nwk_api.h"
#include "vendor/FrL_network/fl_nwk_database.h"
#include "Freelux_libs/storage_weekly_data.h"
#include "../TBS_dev/TBS_dev_config.h"

#define TIME_DELAY_REBOOT		5000//ms
#define TIME_OUT_CHECK_RSP		200// 1m
#define NUM_RETRY				3 //


extern lcd16x2_handle_t lcd_handle;
extern tbs_device_counter_t G_COUNTER_DEV;
extern u8 G_COUNTER_LCD[COUNTER_LCD_MESS_MAX][22];

extern subapp_t led7_app;
extern subapp_t led_app;
extern subapp_t lcd_app;
extern subapp_t button_app;
extern subapp_t data_storage_app;

const char mess[10][24] = {
    "0 chao buoi sang",
    "1 hello world",
    "2 1 ngay noi",
    "3 mot hai ba bon nam",
    "4 on twe three for",
    "5 numberone ",
    "6 viet nam  1 2 3 4",
    "7 hi what your name",
    "8 counter",
    "9 test mess end",
};

/******************************************************************/
/************* general app data ***********************************/

static count_product_t count_test_app = {
	.err_product = 0,
	.pass_product = 0,
};
static count_product_t count_actic_app = {
	.err_product = 100,
	.pass_product = 100,
};

app_data_t g_app_data = {
	.bt_call = 0,
	.bt_endcall = 0,
	.bt_rst = 0,
	.count = &count_test_app,
	.timetamp = 0,
	.is_call = false,
	.is_online = false,
	.mode = APP_MODE_TESTS,
};

uint16_t get_data_pass_product(void){ return g_app_data.count->pass_product; }
uint16_t get_data_err_product(void){ return g_app_data.count->err_product; }
bool get_data_is_call(void){ return g_app_data.is_call; }
bool get_data_is_mode_actics(void){ return (g_app_data.mode == APP_MODE_TESTS) ? false : true;  }
uint8_t * get_mac(void){ return G_COUNTER_DEV.mac; }
uint32_t get_data_timetamp(void){ return g_app_data.timetamp; }

get_data_t get_data = {
	.timetamp = get_data_timetamp,
	.pass_product = get_data_pass_product,
	.err_product  = get_data_err_product,
	.is_call = get_data_is_call,
	.is_mode_actic = get_data_is_mode_actics,
	.mac = get_mac,
};

/***********************************************************************/

// SubApp context structure
typedef struct {
	uint32_t value;         ///< Value to display (0-9999)
	uint32_t value_err;
	uint64_t time_out_err;
	bool print_err;
	bool enabled;           ///< Display enabled
	app_mode_t mode;
} data_context_t;

// Static context instance
static data_context_t data_ctx = {0};

// Forward declarations
static subapp_result_t data_app_init(subapp_t* self);
static subapp_result_t data_app_loop(subapp_t* self);
static subapp_result_t data_app_deinit(subapp_t* self);
static void data_app_event_handler(const event_t* event, void* user_data);


subapp_t data_app = {
        .name = "data", 
        .context = &data_ctx, 
        .state = SUBAPP_STATE_IDLE, 
        .init = data_app_init, 
        .loop = data_app_loop, 
        .deinit = data_app_deinit, 
        .on_event = NULL, 
        .pause = NULL, 
        .resume = NULL, 
        .is_registered = false, 
        .event_mask = 0 
    };

/**
 * App
 */
//static void user_app_data_init(void);
static void rst_cb_rsp(void*, void*);
static void call_cb_rsp(void*, void*);
static void endcall_cb_rsp(void*, void*);

void user_app_init(void)
{
	ULOGA("user_app_init start\n");

	// printf("MAC: ");
	// for(int i = 0; i < 6; i++)
	// {
	// 	printf("%02x ", G_COUNTER_DEV.data.mac[i]);
	// }

	user_tca9555_app_init();

//	read_data_t data_read = read_data(g_app_data.timetamp);

	// 2. init framework
	app_manager_init();

	// 3. register SubApp
	app_manager_register(&led7_app);
	app_manager_register(&led_app);
	app_manager_register(&lcd_app);
	app_manager_register(&button_app);
	app_manager_register(&data_storage_app);
	app_manager_register(&data_app);



	app_manager_start_all();

}
void user_app_loop(void)
{
	static uint64_t appTimeTick = 0;
	if(get_system_time_ms() - appTimeTick > TIME_APP_TASK_MS){
		appTimeTick = get_system_time_ms()  ; //10ms
	}
	else{
		return ;
	}
	
	app_manager_loop();

}


static void rst_cb_rsp(void *arg1, void *arg2)
{
	
}

static void call_cb_rsp(void *arg1, void *arg2)
{

}

static void endcall_cb_rsp(void *arg1, void *arg2)
{

}

static subapp_result_t data_app_init(subapp_t* self)
{
	g_app_data.timetamp = fl_rtc_get();
	g_app_data.is_online = IsOnline();
	
	read_data_t read_cnt = read_data(g_app_data.timetamp);

	g_app_data.count->pass_product = read_cnt.product_pass;
	g_app_data.count->err_product  = read_cnt.product_error;	

	uint32_t app_data_evt_table[] = get_data_event();

	for(int i = 0; i < (sizeof(app_data_evt_table)/sizeof(app_data_evt_table[0])); i++)
	{
		char name[32];
		snprintf(name, sizeof(name), "app_data_evt_table[%d]", i);
		event_bus_subscribe(app_data_evt_table[i], data_app_event_handler, NULL, name);
	}
	
	for (int i = 0; i < 10; i++) {
	    memcpy(G_COUNTER_LCD[i], mess[i], sizeof(G_COUNTER_LCD[i]) - 1);
	    G_COUNTER_LCD[i][sizeof(G_COUNTER_LCD[i]) - 1] = '\0';
	}


	return SUBAPP_OK;
}

static subapp_result_t data_app_loop(subapp_t* self)
{
	static uint64_t dataTimeTick = 0;
	if(get_system_time_ms() - dataTimeTick > TIME_DATA_TASK_MS){
		dataTimeTick = get_system_time_ms()  ; //1s
	}
	else{
		return SUBAPP_OK;
	}

	g_app_data.timetamp = fl_rtc_get();
	g_app_data.is_online = IsOnline();

	return SUBAPP_OK;
}

static subapp_result_t data_app_deinit(subapp_t* self)
{
	return SUBAPP_OK;
}

static void data_app_event_handler(const event_t* event, void* user_data)
{
    switch (event->type) {
        case EVENT_BUTTON_RST_ONCLICK:
            printf("Handle EVENT_BUTTON_RST_ONCLICK\n");
			EVENT_PUBLISH_SIMPLE(EVENT_LCD_PRINT_MESS, EVENT_PRIORITY_HIGH);

            break;

        case EVENT_BUTTON_RST_HOLD_3S:
            printf("Handle EVENT_BUTTON_RST_HOLD_3S\n");

			G_COUNTER_DEV.data.bt_rst = 1;
			fl_api_slave_req(NWK_HDR_55, (u8*)&G_COUNTER_DEV.data,SIZEU8(G_COUNTER_DEV.data), rst_cb_rsp, TIME_OUT_CHECK_RSP, NUM_RETRY);
			G_COUNTER_DEV.data.bt_rst = 0;
			EVENT_PUBLISH_SIMPLE(EVENT_DATA_RESET, EVENT_PRIORITY_HIGH);
			g_app_data.count->pass_product = 0;
			g_app_data.count->err_product = 0;

            break;

        case EVENT_BUTTON_CALL_ONCLICK:
            printf("Handle EVENT_BUTTON_CALL_ONCLICK\n");

			G_COUNTER_DEV.data.bt_call = 1;
			fl_api_slave_req(NWK_HDR_55, (u8*)&G_COUNTER_DEV.data,SIZEU8(G_COUNTER_DEV.data), call_cb_rsp, TIME_OUT_CHECK_RSP, NUM_RETRY);
			G_COUNTER_DEV.data.bt_call = 0;
			EVENT_PUBLISH_SIMPLE(EVENT_DATA_CALL, EVENT_PRIORITY_HIGH);
			g_app_data.is_call = true;

            break;

        case EVENT_BUTTON_ENDCALL_ONCLICK:
            printf("Handle EVENT_BUTTON_ENDCALL_ONCLICK\n");
			if( g_app_data.is_call)
			{
				G_COUNTER_DEV.data.bt_endcall = 1;
				fl_api_slave_req(NWK_HDR_55, (u8*)&G_COUNTER_DEV.data,SIZEU8(G_COUNTER_DEV.data), endcall_cb_rsp, TIME_OUT_CHECK_RSP, NUM_RETRY);
				G_COUNTER_DEV.data.bt_endcall = 0;
				EVENT_PUBLISH_SIMPLE(EVENT_DATA_ENDCALL, EVENT_PRIORITY_HIGH);
				g_app_data.is_call = false;
			}

            break;

        case EVENT_BUTTON_PPD_ONCLICK:
            printf("Handle EVENT_BUTTON_PPD_ONCLICK\n");
			if(g_app_data.count->pass_product > 0)
			{
				g_app_data.count->pass_product--;
			}
			EVENT_PUBLISH_SIMPLE(EVENT_DATA_PASS_PRODUCT_DOWN, EVENT_PRIORITY_HIGH);

            break;

        case EVENT_BUTTON_PPU_ONCLICK:
            printf("Handle EVENT_BUTTON_PPU_ONCLICK\n");
			if(g_app_data.count->pass_product++ >= 10000)
			{
				g_app_data.count->pass_product = 0;
			}
			EVENT_PUBLISH_SIMPLE(EVENT_DATA_PASS_PRODUCT_UP, EVENT_PRIORITY_HIGH);

            break;

        case EVENT_BUTTON_PED_ONCLICK:
            printf("Handle EVENT_BUTTON_PED_ONCLICK\n");
			if(g_app_data.count->err_product)
			{
				g_app_data.count->err_product--;
			}
			EVENT_PUBLISH_SIMPLE(EVENT_DATA_ERR_PRODUCT_CHANGE, EVENT_PRIORITY_HIGH);

            break;

        case EVENT_BUTTON_PEU_ONCLICK:
            printf("Handle EVENT_BUTTON_PEU_ONCLICK\n");
			if(g_app_data.count->err_product++ >= 1000)
			{
				g_app_data.count->err_product = 0;
			}
			EVENT_PUBLISH_SIMPLE(EVENT_DATA_ERR_PRODUCT_CHANGE, EVENT_PRIORITY_HIGH);

            break;

        case EVENT_BUTTON_RST_PED_HOLD_5S:
            printf("Handle EVENT_BUTTON_RST_PED_HOLD_5S\n");
			ULOGA("RST Factory\n");
			
//			fl_db_clearAll();
//			storage_clean();
//			sys_reboot();

            break;

        case EVENT_BUTTON_RST_PEU_HOLD_5S:
            printf("Handle EVENT_BUTTON_RST_PEU_HOLD_5S\n");
			ULOGA("PAIR\n");
//			fl_db_clearAll();
//			sys_reboot();
            break;

        case EVENT_BUTTON_RST_PPU_HOLD_5S:
            printf("Handle EVENT_BUTTON_RST_PPU_HOLD_5S\n");
			if(g_app_data.mode == APP_MODE_TESTS)
			{
				g_app_data.mode = APP_MODE_ACTICS;
				g_app_data.count = &count_actic_app;
				EVENT_PUBLISH_SIMPLE(EVENT_DATA_PASS_PRODUCT_UP, EVENT_PRIORITY_HIGH);
			}
			else 
			{
				g_app_data.mode = APP_MODE_TESTS;
				g_app_data.count = &count_test_app;
				g_app_data.count->err_product = count_actic_app.err_product;
				g_app_data.count->pass_product = count_actic_app.pass_product;
			}
			G_COUNTER_DEV.data.mode = g_app_data.mode;
			fl_api_slave_req(NWK_HDR_55, (u8*)&G_COUNTER_DEV.data,SIZEU8(G_COUNTER_DEV.data), endcall_cb_rsp, TIME_OUT_CHECK_RSP, NUM_RETRY);
			EVENT_PUBLISH_SIMPLE(EVENT_DATA_SWITCH_MODE, EVENT_PRIORITY_HIGH);

            break;

		case EVENT_BUTTON_ENDCALL_HOLD_5S:
            printf("Handle EVENT_BUTTON_ENDCALL_HOLD_5S\n");
			EVENT_PUBLISH_SIMPLE(EVENT_LCD_PRINT_MAC, EVENT_PRIORITY_HIGH);

            break;

        default:
            printf("Unknown event: 0x%lx\n", (uint32_t)event);
            break;
    }

}

#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/

