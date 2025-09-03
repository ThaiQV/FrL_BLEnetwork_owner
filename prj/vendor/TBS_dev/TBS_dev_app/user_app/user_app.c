/*
 * user_app.c
 *
 *      Author: Nghia Hoang
 */

#include "stdio.h"
#include "../user_lib.h"
#include "led_7_seg_app/led_7_seg_app.h"
#include "tca9555_app/tca9555_app.h"
#include "lcd_app/lcd_app.h"
#include "vendor/FrL_network/fl_nwk_handler.h"
#include "vendor/FrL_network/fl_nwk_api.h"
#include "vendor/FrL_network/fl_nwk_database.h"
#include "Freelux_libs/storage_weekly_data.h"

#define TIME_DELAY_REBOOT		5000//ms

extern led7seg_shared_data_t led7seg_data;
extern lcd_shared_data_t lcd_data;
extern led_shared_data_t led_data;
extern data_storage_share_data_t data_storage_data;
extern lcd16x2_handle_t lcd_handle;
extern tbs_device_counter_t G_COUNTER_DEV;
/******************************************************************/

//app_share_data_t app_data ={
//		.led7seg = &led7seg_data,
//		.lcd = &lcd_data,
//};

app_share_data_t app_data = {
		.pass_product = 0,
		.err_product = 0,
		.bt_call = 0,
		.bt_endcall = 0,
		.bt_rst = 0,
		.timetamp = 0,
		.is_call = 0,
		.reset_factory = 0,
		.pair = 0,
};

/**
 * App
 */
//static void user_app_data_init(void);
static void user_app_data_sys(void);

void user_app_init(void)
{
	ULOGA("user_app_init start\n");

	data_storage_data.timestamp = fl_rtc_get();
	data_storage_data.is_online = IsOnline();

	user_datastorage_app_init();

	G_COUNTER_DEV.data.pass_product = data_storage_data.product_pass;
	G_COUNTER_DEV.data.err_product = data_storage_data.product_error;
	app_data.pass_product = data_storage_data.product_pass;
	app_data.err_product = data_storage_data.product_error;

	user_led_7_seg_app_init();
	user_tca9555_app_init();
	user_lcd_app_init();
	user_button_app_init();
	user_led_app_init();

	led7seg_data.value = app_data.pass_product;
	led7seg_data.value_err = app_data.err_product;


	char line1[16];
	char line2[16];

	sprintf(line1, "pass %4d", (int)app_data.pass_product);
	sprintf(line2, "err  %4d", (int)app_data.err_product);

	memcpy(lcd_data.display.line1, line1, 16);
	memcpy(lcd_data.display.line2, line2, 16);
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

	if(get_system_time_ms() < 3500)
	{
		return;
	}
	/* input app */
	user_button_app_task();

	/* handler*/
	user_app_data_sys();

	/* output app */
	user_datastorage_app_task();
	user_led_7_seg_app_task();
	user_lcd_app_task();
	user_led_app_task();

}

//static void user_app_data_init(void)
//{
//
//}

static void user_app_data_sys(void)
{
	char line1[16];
	char line2[16];

	if(app_data.print_err_led7)
	{
		if(app_data.print_err_led7 == 1)
		{
			app_data.print_err_led7 = 2;
			led7seg_data.print_err = 1;
			led7seg_data.time_out_err = get_system_time_ms();
		}

		if(get_system_time_ms() - led7seg_data.time_out_err > TIME_OUT_PRINTF_ERR_MS){
			app_data.print_err_led7 = 0;
			led7seg_data.print_err = 0;
			led7seg_data.chage_printf = 1;
		}
	}
	else
	{
		led7seg_data.print_err = 0;
	}

	if(led7seg_data.value != app_data.pass_product)
	{
		led7seg_data.chage_printf = 1;
		led7seg_data.value = app_data.pass_product;
		sprintf(line1, "pass %4d       \n", (int)app_data.pass_product);
		memcpy(lcd_data.display.line1, line1, 16);
		G_COUNTER_DEV.data.pass_product = app_data.pass_product;
	}

	if(led7seg_data.value_err != app_data.err_product)
	{
		led7seg_data.chage_printf = 1;
		led7seg_data.value_err = app_data.err_product;
		sprintf(line2, "err  %4d       \n", (int)app_data.err_product);
		memcpy(lcd_data.display.line2, line2, 16);
		G_COUNTER_DEV.data.err_product = app_data.err_product;
	}

	if(app_data.bt_call)
	{
		if(!(app_data.is_call))
		{
			G_COUNTER_DEV.data.bt_call = 1;
			fl_api_slave_req(NWK_HDR_55, G_COUNTER_DEV.bytes, SIZEU8(G_COUNTER_DEV), NULL, 0);
			G_COUNTER_DEV.data.bt_call = 0;
			led_data.led_call_on = 1;
			app_data.is_call = 1;
		}
		app_data.bt_call = 0;
		led7seg_data.print_err = 0;
		led7seg_data.chage_printf = 1;
	}

	if(app_data.bt_endcall)
	{
		if(app_data.is_call)
		{
			G_COUNTER_DEV.data.bt_endcall = 1;
			fl_api_slave_req(NWK_HDR_55, G_COUNTER_DEV.bytes, SIZEU8(G_COUNTER_DEV), NULL, 0);
			G_COUNTER_DEV.data.bt_endcall = 0;
			led_data.led_call_on = 0;
			app_data.is_call = 0;
		}
		app_data.bt_endcall = 0;
		led7seg_data.print_err = 0;
		led7seg_data.chage_printf = 1;
	}

	if(app_data.bt_rst)
	{
		G_COUNTER_DEV.data.bt_rst = 1;
		fl_api_slave_req(NWK_HDR_55, G_COUNTER_DEV.bytes, SIZEU8(G_COUNTER_DEV), NULL, 0);
		G_COUNTER_DEV.data.bt_rst = 0;
		led_data.led_call_blink_3 = 1;
//		lcd16x2_clear(&lcd_handle);
//		lcd16x2_print_string(&lcd_handle, "Reset all data");
		app_data.err_product = 0;
		app_data.pass_product = 0;
		data_storage_data.product_pass = app_data.pass_product;
		data_storage_data.product_error = app_data.err_product;
		app_data.bt_rst = 0;
		led7seg_data.print_err = 0;
		led7seg_data.chage_printf = 1;
	}

	if(app_data.reset_factory)
	{
		ULOGA("RST Factory\n");
		lcd16x2_clear(&lcd_handle);
		lcd16x2_print_string(&lcd_handle, "Factory Reset");
		fl_db_clearAll();
		storage_clean();
		sys_reboot();
		app_data.reset_factory = 0;
	}

	if(app_data.pair)
	{
		ULOGA("PAIR\n");
		lcd16x2_clear(&lcd_handle);
		lcd16x2_print_string(&lcd_handle, "Pair");
		fl_db_clearAll();
		sys_reboot();
		app_data.pair = 0;
	}



    static unsigned int nwk_check_TimeTick = 0;
	if(get_system_time_ms() - nwk_check_TimeTick > TIME_BUTTON_TASK_MS){
		nwk_check_TimeTick = get_system_time_ms()  ; //10ms
	}
	else{
		return ;
	}

    led_data.led_nwk_on = IsOnline();

	static unsigned int sysdata_TimeTick_ms = 0;
	if(get_system_time_ms() - sysdata_TimeTick_ms > 1){
		sysdata_TimeTick_ms = get_system_time_ms()  ; //10ms
	}
	else{
		return ;
	}

	app_data.timetamp = fl_rtc_get();
	data_storage_data.timestamp = app_data.timetamp;
	data_storage_data.product_pass = app_data.pass_product;
	data_storage_data.product_error = app_data.err_product;

}


void user_app_run(void)
{
	ULOGA("user_app_run start\n");

	gpio_function_en(GPIO_PA5);
	gpio_set_output(GPIO_PA5, 1); 			//enable output
	gpio_set_input(GPIO_PA5, 0);			//disable input
	gpio_set_up_down_res(GPIO_PA5, GPIO_PIN_PULLUP_10K);

	gpio_set_level(GPIO_PA5, 0);
//	delay_ms(5000);
//	gpio_set_level(GPIO_PA5, 1);

	gpio_function_en(GPIO_PA6);
	gpio_set_output(GPIO_PA6, 1); 			//enable output
	gpio_set_input(GPIO_PA6, 0);			//disable input
	gpio_set_up_down_res(GPIO_PA6, GPIO_PIN_PULLUP_10K);
	gpio_set_level(GPIO_PA6, 0);

	gpio_function_en(GPIO_PC0);
	gpio_set_output(GPIO_PC0, 1); 			//enable output
	gpio_set_input(GPIO_PC0, 0);			//disable input
	gpio_set_up_down_res(GPIO_PC0, GPIO_PIN_PULLUP_10K);
	gpio_set_level(GPIO_PC0, 1);

	user_led_7_seg_app_init();
	user_tca9555_app_init();
	user_lcd_app_init();
	user_button_app_init();
	char line1[] = {"line1 135678       "};
	char line2[] = {"line2 135678       "};

	memcpy(lcd_data.display.line1, line1, 16);
	memcpy(lcd_data.display.line2, line2, 16);

	led7seg_data.value =0;
	led7seg_data.value_err = 0;
//	char i = 0x30;
	while(1)
	{
		user_led_7_seg_app_task();
		user_lcd_app_task();
		button_process_all();
	}

}



