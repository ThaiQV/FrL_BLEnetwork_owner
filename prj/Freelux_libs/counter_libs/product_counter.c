#include "product_counter.h"

/* Macros */
#define LINE_LENGTH_MAX	16
#define LCD_LINE_1(str)	memcpy(lcd_data.display.line1, str, LINE_LENGTH_MAX);
#define LCD_LINE_2(str)	memcpy(lcd_data.display.line2, str, LINE_LENGTH_MAX);

/* Variables */

extern led7seg_shared_data_t led7seg_data;
extern lcd_shared_data_t lcd_data;
extern tca9555_handle_t tca9555_handle;

uint8_t call_status = 0;
uint32_t product_pass = 0;
uint32_t product_error = 0;

uint8_t product_counter_data[11]={0};

/* Functions */

void product_counter_init(void)
{
	// Init GPIO
	gpio_set_level(GPIO_PA5, 0);
	delay_ms(100);
	gpio_set_level(GPIO_PA5, 1);

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

	/////////////////////

	// Init driver
	user_led_7_seg_app_init();
	user_tca9555_app_init();
	user_lcd_app_init();

	lcd_show(1,4,"Product");
	lcd_show(2,4,"Counter");
	user_lcd_app_task();
	delay_ms(2000);
	lcd_show(1,0,"Passed:");
	lcd_show(2,0,"Error:");
	user_lcd_app_task();

	led7seg_data.value = 0;
	led7seg_data.value_err = 0;

	product_counter_process();
}

void product_counter_process(void)
{
	led7seg_data.value 		= product_pass;
	led7seg_data.value_err 	= product_error;

	user_lcd_app_task();
	user_led_7_seg_app_task();

	if(call_status == 1)
	{
		gpio_set_level(GPIO_PA5, 0);
	}
	else
	{
		gpio_set_level(GPIO_PA5, 1);
	}
}

void product_counter_set_data(uint8_t call_state, uint8_t endcall_state, uint8_t reset_state)
{
	product_counter_data[0] = call_state;
	product_counter_data[1] = endcall_state;
	product_counter_data[2] = reset_state;
	memcpy(&product_counter_data[3],product_pass,sizeof(product_pass));
	memcpy(&product_counter_data[7],product_error,sizeof(product_error));
}

void product_counter_get_data(uint8_t *pdata)
{
	memcpy(pdata,product_counter_data,sizeof(product_counter_data));
	product_counter_data[0] = 0; // Reset button call value
	product_counter_data[1] = 0; // Reset button call value
	product_counter_data[2] = 0; // Reset button call value
}


void lcd_show(uint8_t row,uint8_t col, uint8_t *string)
{
	uint8_t len;

	len = (uint8_t)strlen((const char *)string);
	if(len > (LINE_LENGTH_MAX - col)) len = (LINE_LENGTH_MAX - col);
	if(row == 1)
	{
		memset(lcd_data.display.line1,' ', LINE_LENGTH_MAX);
		memcpy(&lcd_data.display.line1[col],string,len);
	}
	else
	{
		memset(lcd_data.display.line2,' ', LINE_LENGTH_MAX);
		memcpy(&lcd_data.display.line2[col],string,len);
	}
}

void show_pass_product(void)
{
	uint8_t string[LINE_LENGTH_MAX] = {0};
	sprintf(string,"Passed: %d",product_pass);
	lcd_show(1,0,string);
}

void show_error_product(void)
{
	uint8_t string[LINE_LENGTH_MAX] = {0};
	sprintf(string,"Error: %d",product_error);
	lcd_show(2,0,string);
}


void button_reset_handler(void)
{
	product_counter_set_data(0,0,1);
	// Reset product counter
	product_pass = 0;
	product_error = 0;
	show_pass_product();
	show_error_product();
	product_counter_process();
}

void button_call_handler(void)
{
	product_counter_set_data(1,0,0);
	call_status = 1;
	product_counter_process();
}

void button_endcall_handler(void)
{
	product_counter_set_data(0,1,0);
	call_status = 0;
	product_counter_process();
}

void button_error_down_handler(void)
{
	if(product_error > 0) product_error--;
	show_error_product();
	product_counter_process();
}

void button_error_up_handler(void)
{
	product_error++;
	show_error_product();
	product_counter_process();
}

void button_pass_down_handler(void)
{
	if(product_pass > 0) product_pass--;
	show_pass_product();
	product_counter_process();
}

void button_pass_up_handler(void)
{
	product_pass++;
	show_pass_product();
	product_counter_process();
}
