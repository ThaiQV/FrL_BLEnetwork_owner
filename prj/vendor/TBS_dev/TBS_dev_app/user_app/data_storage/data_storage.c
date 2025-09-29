/**
 * @file data_storage.c
 * @author Nghia Hoang
 * @date 2025
 */

#ifndef MASTER_CORE

#include "vendor/TBS_dev/TBS_dev_config.h"

#ifdef COUNTER_DEVICE

#include "../../user_lib.h"
#include "data_storage.h"
#include "Freelux_libs/storage_weekly_data.h"
#include "Freelux_libs/fl_ble_wifi_protocol.h"

extern get_data_t get_data;

// SubApp context structure
typedef struct {
	bool is_actic;
	bool is_timetemp;
	data_save_t data_save;
} data_storage_context_t;

// Static context instance
static data_storage_context_t data_storage_ctx = {0};

// Forward declarations
static subapp_result_t data_storage_app_init(subapp_t* self);
static subapp_result_t data_storage_app_loop(subapp_t* self);
static subapp_result_t data_storage_app_deinit(subapp_t* self);
//static void data_storage_app_event_handler(const event_t* event, void* user_data);

subapp_t data_storage_app = {
        .name = "data_storage", 
        .context = &data_storage_ctx, 
        .state = SUBAPP_STATE_IDLE, 
        .init = data_storage_app_init, 
        .loop = data_storage_app_loop, 
        .deinit = data_storage_app_deinit, 
        .on_event = NULL, 
        .pause = NULL, 
        .resume = NULL, 
        .is_registered = false, 
        .event_mask = 0 
    };

static data_save_t pre_data_save;

static subapp_result_t data_storage_app_init(subapp_t* self)
{
	hw_storage_init();
//	storage_get_data(data_storage_data.timestamp, pre_data_save.data, sizeof(data_save_t));
//	data_storage_ctx.data_save.product_pass = get_data.pass_product();
//	data_storage_ctx.data_save.product_error = get_data.err_product();
//	ULOGA("%d %d\n", data_storage_ctx.data_save.product_pass, data_storage_ctx.data_save.product_error);
//	ULOGA("%d %d\n", pre_data_save.product_pass, pre_data_save.product_error);
	return SUBAPP_OK;
}

static subapp_result_t data_storage_app_loop(subapp_t* self)
{
	static uint64_t datastorageTimeTick = 0;
	if(get_system_time_ms() - datastorageTimeTick > TIME_DTATSTORAGE_TASK_MS){
		datastorageTimeTick = get_system_time_ms()  ; //10ms
	}
	else{
		return SUBAPP_OK;
	}

	pre_data_save.product_pass = get_data.pass_product();
	pre_data_save.product_error = get_data.err_product();

	uint32_t timetemp1 = get_data.timetamp();

	storage_put_data(timetemp1, pre_data_save.data, sizeof(data_save_t));
	ULOGA("save data: timestamp %d %d %d\n", timetemp1, pre_data_save.product_pass, pre_data_save.product_error);
	// storage_get_data(timetemp1, pre_data_save.data, sizeof(data_save_t));
	// ULOGA("read data: timestamp %d %d %d\n", timetemp1, pre_data_save.product_pass, pre_data_save.product_error);

	return SUBAPP_OK;
}

static subapp_result_t data_storage_app_deinit(subapp_t* self)
{
	return SUBAPP_OK;
}

//static void data_storage_app_event_handler(const event_t* event, void* user_data)
//{
//
//}

void hw_storage_init(void)
{
	nvm_init();
	storage_init();
}

read_data_t read_data(uint32_t timestamp)
{
	data_save_t result;
	read_data_t ret;
	storage_get_data(get_data.timetamp(), result.data, sizeof(data_save_t));
	ret.product_error = (result.product_error == 0xffff) ? 0 : result.product_error;
	ret.product_pass  = (result.product_pass  == 0xffff) ? 0 : result.product_pass;
	return ret;
}

#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/
