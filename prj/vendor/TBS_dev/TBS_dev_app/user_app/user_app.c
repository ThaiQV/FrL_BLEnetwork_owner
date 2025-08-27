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
#include "fl_nwk_handler.h"

#define TIME_DELAY_REBOOT		5000//ms

extern led7seg_shared_data_t led7seg_data;
extern lcd_shared_data_t lcd_data;
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
};

/**
 * App
 */
static void user_app_data_init(void);
static void user_app_data_sys(void);

void user_app_init(void)
{
	printf("user_app_init start\n");

	gpio_function_en(GPIO_PC0);
	gpio_set_output(GPIO_PC0, 1); 			//enable output
	gpio_set_input(GPIO_PC0, 0);			//disable input
	gpio_set_up_down_res(GPIO_PC0, GPIO_PIN_PULLUP_10K);
	gpio_set_level(GPIO_PC0, 1);

	user_led_7_seg_app_init();
	user_tca9555_app_init();
	user_lcd_app_init();
	user_button_app_init();
	char line1[16];
	char line2[16];
	sprintf(line1, "pass %4d", (int)app_data.pass_product);
	sprintf(line2, "err  %4d", (int)app_data.err_product);

	memcpy(lcd_data.display.line1, line1, 16);
	memcpy(lcd_data.display.line2, line2, 16);
}
void user_app_loop(void)
{
	/* input app */
	user_button_app_task();

	/* handler*/
	user_app_data_sys();

	/* output app */
	user_led_7_seg_app_task();
	user_lcd_app_task();

}

static void user_app_data_init(void)
{

}

static void user_app_data_sys(void)
{
	char line1[16];
	char line2[16];
	static bool reboot = 0;
	static unsigned int reboot_TimeTick_ms = 0;


	if(led7seg_data.value != app_data.pass_product)
	{
		led7seg_data.value = app_data.pass_product;
		sprintf(line1, "pass %4d", (int)app_data.pass_product);
		memcpy(lcd_data.display.line1, line1, 16);
		G_COUNTER_DEV.data.pass_product = app_data.pass_product;
	}

	if(led7seg_data.value_err != app_data.err_product)
	{
		led7seg_data.value_err == app_data.err_product;
		sprintf(line2, "err  %4d", (int)app_data.err_product);
		memcpy(lcd_data.display.line2, line2, 16);
		G_COUNTER_DEV.data.err_product = app_data.err_product;
	}

	if(app_data.bt_call)
	{
		G_COUNTER_DEV.data.bt_call = 1;
		fl_api_slave_req(NWK_HDR_55, G_COUNTER_DEV.bytes, SIZEU8(G_COUNTER_DEV), NULL, 0);
		led7seg_data.led_call_on = 1;
		app_data.bt_call = 0;
	}

	if(app_data.bt_endcall)
	{
		G_COUNTER_DEV.data.bt_endcall = 1;
		G_COUNTER_DEV.data.bt_call = 0;
		fl_api_slave_req(NWK_HDR_55, G_COUNTER_DEV.bytes, SIZEU8(G_COUNTER_DEV), NULL, 0);
		led7seg_data.led_call_on = 0;
		app_data.bt_endcall = 0;
	}

	if(app_data.bt_rst)
	{
		G_COUNTER_DEV.data.bt_rst = 1;
		fl_api_slave_req(NWK_HDR_55, G_COUNTER_DEV.bytes, SIZEU8(G_COUNTER_DEV), NULL, 0);
		reboot = 1;
		reboot_TimeTick_ms = get_system_time_ms() + TIME_DELAY_REBOOT;
		app_data.bt_rst = 0;
	}

	if(reboot)
	{
		if(reboot_TimeTick_ms >= get_system_time_ms())
		{
			return;
		}
		sys_reboot();
	}

    static unsigned int nwk_check_TimeTick = 0;
    if(nwk_check_TimeTick <= get_system_time_ms()){
    	nwk_check_TimeTick = get_system_time_ms() + 60000 ; //60s
    }
    else{
        return ;
    }

    led7seg_data.led_nwk_on = IsOnline();

}


void user_app_run(void)
{
	printf("user_app_run start\n");

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



