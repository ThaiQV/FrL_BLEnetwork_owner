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


extern led7seg_shared_data_t led7seg_data;
extern lcd_shared_data_t lcd_data;
extern tca9555_handle_t tca9555_handle;
/******************************************************************/

/**
 * BUTTON
 */
typedef struct {
    union {
        struct {
            PortPin_Map *RESET;
            PortPin_Map *MAIN;
            PortPin_Map *CMAIN;
            PortPin_Map *PEU;
            PortPin_Map *PED;
            PortPin_Map *PPD;
            PortPin_Map *PPU;
        };
        PortPin_Map *all_pins[7]; // 3 control + 8 data = 11 pins
    };
} Button_Pin_TypeDef;

Button_Pin_TypeDef button_pin_map = {
		.RESET = 	&(PortPin_Map){ .tca_port = TCA9555_PORT_1, .tca_pin = TCA9555_PIN_0 },
		.MAIN = 	&(PortPin_Map){ .tca_port = TCA9555_PORT_1, .tca_pin = TCA9555_PIN_1 },
		.CMAIN = 	&(PortPin_Map){ .tca_port = TCA9555_PORT_1, .tca_pin = TCA9555_PIN_2 },
		.PEU = 		&(PortPin_Map){ .tca_port = TCA9555_PORT_1, .tca_pin = TCA9555_PIN_3 },
		.PED = 		&(PortPin_Map){ .tca_port = TCA9555_PORT_1, .tca_pin = TCA9555_PIN_4 },
		.PPD = 		&(PortPin_Map){ .tca_port = TCA9555_PORT_1, .tca_pin = TCA9555_PIN_5 },
		.PPU = 		&(PortPin_Map){ .tca_port = TCA9555_PORT_1, .tca_pin = TCA9555_PIN_6 },
};

void button_pin_init_all(void)
{
	for(uint8_t i = 0; i < 7; i++)
	{
		if(button_pin_map.all_pins[i] != NULL)
		{
			tca9555_set_pin_direction(&tca9555_handle, button_pin_map.all_pins[i]->tca_port, button_pin_map.all_pins[i]->tca_pin, TCA9555_PIN_INPUT);
		}
	}

	button_add(BUTTON_PIN0, true);
	button_add(BUTTON_PIN1, true);
	button_add(BUTTON_PIN2, true);
	button_add(BUTTON_PIN3, true);
	button_add(BUTTON_PIN4, true);
	button_add(BUTTON_PIN5, true);
	button_add(BUTTON_PIN6, true);

}

bool button_pin_init(button_pin_t pin, bool active_low)
{
	return true;
}

bool button_pin_read(button_pin_t pin)
{
	tca9555_pin_state_t value;
	tca9555_read_pin_input(&tca9555_handle, button_pin_map.all_pins[pin]->tca_port, button_pin_map.all_pins[pin]->tca_pin, &value);
	return value;
}

void on_button_click(uint8_t button_id) {
    printf("Button %d clicked!\n", button_id);
    led7seg_data.value++;
}

void on_triple_click_specific(uint8_t button_id, uint8_t click_count)
{
	printf(">>> Button %d: Specific TRIPLE CLICK handler triggered\n", button_id);
}
void on_button_multiclick(uint8_t button_id, uint8_t click_count) {
    printf(">>> Button %d: %d-CLICK detected!\n", button_id, click_count);

    // You can handle different click counts here
    switch (click_count) {
        case 1:
            printf("    -> Single click action\n");
            on_button_click(button_id);
            break;
        case 2:
            printf("    -> Double click action\n");
            on_button_click(button_id);
            break;
        case 3:
            printf("    -> Triple click action\n");
            on_button_click(button_id);
            break;
        case 4:
            printf("    -> Quad click action\n");
            on_button_click(button_id);
            break;
        case 5:
            printf("    -> Penta click action\n");
            on_button_click(button_id);
            break;
        default:
            printf("    -> %d clicks - that's impressive!\n", click_count);
            on_button_click(button_id);
            break;
    }
}

uint32_t bt_get_system_time_ms(void)
{
	return reg_system_tick/SYSTEM_TIMER_TICK_1MS;
}

button_hal_t button_hal = {
    .button_pin_init_all = button_pin_init_all,
    .button_pin_init = button_pin_init,
    .button_pin_read = button_pin_read,
    .get_system_time_ms = bt_get_system_time_ms,
};

void user_button_app(void)
{
    if (!button_manager_init(&button_hal)) {
        printf("Error: Failed to initialize button manager!\n");
        return;
    }

    for(int i = 0; i < 7; i++)
    {
//    	button_set_click_callback(i, on_button_click);
    	button_add_multiclick_level(i, 3, on_triple_click_specific);
    	button_set_multiclick_callback(i, on_button_multiclick);

    }

}

/*****************************************************************/

/**
 * App
 */
void user_app_init(void)
{
	printf("user_app_init start\n");

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
	user_button_app();
	char line1[] = {"line1 135678       "};
	char line2[] = {"line2 135678       "};

	memcpy(lcd_data.display.line1, line1, 16);
	memcpy(lcd_data.display.line2, line2, 16);
}
void user_app_loop(void)
{
	user_led_7_seg_app_task();
	user_lcd_app_task();
	button_process_all();
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
	user_button_app();
	char line1[] = {"line1 135678       "};
	char line2[] = {"line2 135678       "};

	memcpy(lcd_data.display.line1, line1, 16);
	memcpy(lcd_data.display.line2, line2, 16);

	led7seg_data.value =0;
	led7seg_data.value_err = 0;
//	char i = 0x30;
	while(1)
	{
//		delay_ms(3000);
//		if(led7seg_data.print_err){
//			led7seg_data.print_err = 0;
//			led7seg_data.value_err++;
//		}
//		else
//		{
//			led7seg_data.print_err = 1;
//			led7seg_data.value++;
//		}
//		lcd_data.display.line1[12]= i++;
//		lcd_data.display.line2[15]= i++;
//		if(i > 0x39)
//		{
//			i = 0x30;
//		}
		user_led_7_seg_app_task();
		user_lcd_app_task();
		button_process_all();
	}

}



