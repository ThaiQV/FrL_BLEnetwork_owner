/**
 * @file data_storage.c
 * @author Nghia Hoang
 * @date 2025
 */

#include "../../user_lib.h"
#include "data_storage.h"
#include "Freelux_libs/storage_weekly_data.h"

static data_save_t pre_data_save;
data_storage_share_data_t data_storage_data;

void user_datastorage_app_init(void)
{
	ble_wifi_protocol_init();
	nvm_init();
	storage_init();
	//read data save;
//	data_storage_data.timestamp = fl_rtc_get();
	storage_get_data(data_storage_data.timestamp, pre_data_save.data, sizeof(data_save_t));

	ULOGA("read data: timestamp: %d, ", data_storage_data.timestamp);
//	for(int i = 0; i < sizeof(pre_data_save); i++)
	{
		printf("is_online: %d, pass: %d, err: %d\n", pre_data_save.is_online, pre_data_save.product_pass, pre_data_save.product_error);
	}

	if(pre_data_save.is_online == 0xff)
	{
		pre_data_save.is_online = 0;
		pre_data_save.product_error = 0;
		pre_data_save.product_pass = 0;
	}

	data_storage_data.product_pass = pre_data_save.product_pass;
	data_storage_data.product_error = pre_data_save.product_error;

	storage_put_data(data_storage_data.timestamp, pre_data_save.data, sizeof(data_save_t));

	storage_get_data(data_storage_data.timestamp, pre_data_save.data, sizeof(data_save_t));

	ULOGA("read data: timestamp: %d, ", data_storage_data.timestamp);
//	for(int i = 0; i < sizeof(pre_data_save); i++)
	{
		printf("is_online: %d, pass: %d, err: %d\n", pre_data_save.is_online, pre_data_save.product_pass, pre_data_save.product_error);
	}


}

void user_datastorage_app_task(void)
{
	static uint64_t datastorageTimeTick = 0;
	if(get_system_time_ms() - datastorageTimeTick > TIME_DTATSTORAGE_TASK_MS){
		datastorageTimeTick = get_system_time_ms()  ; //10ms
	}
	else{
		return ;
	}

	pre_data_save.is_online = data_storage_data.is_online;
	pre_data_save.product_pass = data_storage_data.product_pass;
	pre_data_save.product_error = data_storage_data.product_error;

	storage_put_data(data_storage_data.timestamp, pre_data_save.data, sizeof(data_save_t));
	ULOGA("save data: timestamp %d %d %d %d\n", data_storage_data.timestamp, pre_data_save.is_online, pre_data_save.product_pass, pre_data_save.product_error);
	storage_get_data(data_storage_data.timestamp, pre_data_save.data, sizeof(data_save_t));
	ULOGA("read data: timestamp %d %d %d %d\n", data_storage_data.timestamp, pre_data_save.is_online, pre_data_save.product_pass, pre_data_save.product_error);
}
