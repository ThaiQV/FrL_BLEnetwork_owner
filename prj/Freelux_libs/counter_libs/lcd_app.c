/*
 * lcd_app.c
 *
 *      Author: hoang
 */

#include "types.h"
#include "lcd16x2.h"
#include "led_7_seg.h"
#include "tca9555.h"
#include "tca9555_app.h"
#include "lcd_app.h"
#include "led_7_seg_app.h"

LCD_Pin_TypeDef lcd_pin_map = {
	    .RS = &(PortPin_Map){ .tca_port = TCA9555_PORT_0, .tca_pin = TCA9555_PIN_0 },
	    .RW = &(PortPin_Map){ .tca_port = TCA9555_PORT_0, .tca_pin = TCA9555_PIN_1 },
	    .E  = &(PortPin_Map){ .tca_port = TCA9555_PORT_0, .tca_pin = TCA9555_PIN_2 },

	    .D4 = &(PortPin_Map){ .tca_port = TCA9555_PORT_0, .tca_pin = TCA9555_PIN_3 },
	    .D5 = &(PortPin_Map){ .tca_port = TCA9555_PORT_0, .tca_pin = TCA9555_PIN_4 },
	    .D6 = &(PortPin_Map){ .tca_port = TCA9555_PORT_0, .tca_pin = TCA9555_PIN_5 },
	    .D7 = &(PortPin_Map){ .tca_port = TCA9555_PORT_0, .tca_pin = TCA9555_PIN_6 },
};

extern tca9555_handle_t tca9555_handle;

void lcd_pin_init()
{
	for(uint8_t i = 0; i < LCD16X2_PIN_MAX; i++)
	{
		if(lcd_pin_map.all_pins[i] != NULL)
		{
			tca9555_set_pin_direction(&tca9555_handle, lcd_pin_map.all_pins[i]->tca_port, lcd_pin_map.all_pins[i]->tca_pin, TCA9555_PIN_OUTPUT);
		}
	}
}

// GPIO control functions}

void lcd_pin_set(LCD16X2_PinId_t pin, bool state)
{
    // Set data pin (4-7 for 4-bit mode) high/low
	tca9555_pin_state_t value = state ? TCA9555_PIN_HIGH : TCA9555_PIN_LOW;
	tca9555_set_pin_output(&tca9555_handle, lcd_pin_map.all_pins[pin]->tca_port, lcd_pin_map.all_pins[pin]->tca_pin, value);
}

// Delay functions
void lcd_delay_us(uint32_t us)
{
    // Microsecond delay
	delay_us(us);
}

void lcd_delay_ms(uint32_t ms)
{
    // Millisecond delay
	delay_ms(ms);
}

lcd16x2_config_t lcd_config = {
	.lcd_pin_init = lcd_pin_init,
    .lcd_pin_set = lcd_pin_set,
    .lcd_pin_read = NULL,  //
    .delay_us = lcd_delay_us,
    .delay_ms = lcd_delay_ms,
    .use_4bit_mode = true,
    .use_busy_flag = false,
    .rows = 2,
    .cols = 16
};

lcd16x2_handle_t lcd_handle;

lcd_shared_data_t lcd_data;

uint8_t user_lcd_app_init(void)
{
	lcd16x2_error_t result = lcd16x2_init(&lcd_handle, &lcd_config);
	lcd_data.enable = 1;
	return result;
}

void user_lcd_app_task(void)
{
//	static unsigned int lcdTimeTick = 0;
//
//	LOGA(USER,"lcdTimeTick: %d\n",lcdTimeTick);
//
//	    if(lcdTimeTick <= (stimer_get_tick()/SYSTEM_TIMER_TICK_1MS)){
//	    	lcdTimeTick = (stimer_get_tick()/SYSTEM_TIMER_TICK_1MS) + TIME_LCD_TASK_MS ; //10ms
//	    }
//	    else{
//	        return ;
//	    }

	if(lcd_data.enable)
	{
		lcd16x2_set_cursor(&lcd_handle, 0, 0);
		lcd16x2_print_string(&lcd_handle, lcd_data.display.line1);
		lcd16x2_set_cursor(&lcd_handle, 1, 0);
		lcd16x2_print_string(&lcd_handle, lcd_data.display.line2);
	}
}

void user_lcd_app_test(void)
{
	lcd16x2_error_t result = lcd16x2_init(&lcd_handle, &lcd_config);

	lcd16x2_clear(&lcd_handle);
	lcd16x2_print_string(&lcd_handle, "Hello World!");
	lcd16x2_set_cursor(&lcd_handle, 1, 0);  // Row 1, Column 0
	lcd16x2_printf(&lcd_handle, "Count: %d", 123);

	// 4. Custom character
	lcd16x2_custom_char_t heart = {
	    .pattern = {
	        0b00000,
	        0b01010,
	        0b11111,
	        0b11111,
	        0b11111,
	        0b01110,
	        0b00100,
	        0b00000
	    }
	};
	lcd16x2_create_custom_char(&lcd_handle, 0, &heart);
	lcd16x2_print_custom_char(&lcd_handle, 0);  // Print heart character

//	while(1)
//	{
//		delay_ms(5000);
//		lcd16x2_clear(&lcd_handle);
//		for(int i = 0; i < 7; i++)
//		{
//			delay_ms(5000);
//			lcd16x2_print_custom_char(&lcd_handle, i);  // Print heart character
//		}
//	}
}
