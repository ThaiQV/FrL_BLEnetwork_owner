/*
 * lcd_app.c
 *
 *      Author: hoang
 */

#ifndef MASTER_CORE
#include "tl_common.h"
#include "vendor/TBS_dev/TBS_dev_config.h"

#ifdef COUNTER_DEVICE

#include "../../user_lib.h"
#include "vendor/FrL_network/fl_nwk_handler.h"
#include <stdio.h>

extern get_data_t get_data;

typedef enum {
	LCD_PRINT_OFF,
	LCD_PRINT_STARTUP,
	LCD_PRINT_NONE,
	LCD_PRINT_MESS,
	LCD_PRINT_ERR,
	LCD_PRINT_CALL,
	LCD_PRINT_ENDCALL,
	LCD_PRINT_MAC,
	LCD_PRINT_MODE,
	LCD_PRINT_RESET,
	LCD_PRINT_PAIRING,
	LCD_PRINT_FACTORY_RESET,
	LCD_PRINT_CALL_FAIL,
	LCD_PRINT_MESS_NEW,
} lcd_print_type_t;

// SubApp context structure
typedef struct {
	struct {
    	char line1[16];
    	char line2[16];
    } display;
    uint8_t row0_mess_num;
    uint8_t row1_mess_num;
    uint32_t time_off;
	bool print_mode;
    bool enable;            ///< Data validity flag
    bool startup;
    lcd_print_type_t print_type;
} lcd_context_t;

// Static context instance
static lcd_context_t lcd_ctx = {0};
static uint8_t mess_zero[22] = {0};
// Forward declarations
static subapp_result_t lcd_app_init(subapp_t* self);
static subapp_result_t lcd_app_loop(subapp_t* self);
static subapp_result_t lcd_app_deinit(subapp_t* self);
static void lcd_app_event_handler(const event_t* event, void* user_data);
static void LCD_MessageCheck_FlagNew(void);

subapp_t lcd_app = {
        .name = "lcd", 
        .context = &lcd_ctx, 
        .state = SUBAPP_STATE_IDLE, 
        .init = lcd_app_init, 
        .loop = lcd_app_loop, 
        .deinit = lcd_app_deinit,
        .on_event = NULL,
        .pause = NULL, 
        .resume = NULL, 
        .is_registered = false, 
        .event_mask = 0 
    };

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
extern u8 G_COUNTER_LCD[COUNTER_LCD_MESS_MAX][LCD_MESSAGE_SIZE];

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

uint32_t lcd_app_get_tick(void)
{
	return get_system_time_ms();
}

lcd_app_handle_t app_handle;

void my_timeout_callback(uint8_t row) {
	ULOGA("my_timeout_callback \n");
	for(int i = 0; i < 10; i++)
	{
	switch (lcd_ctx.print_type)
	{
		case LCD_PRINT_STARTUP:
			lcd_ctx.startup = 0;
			lcd_ctx.print_type = LCD_PRINT_NONE;
			EVENT_PUBLISH_SIMPLE(EVENT_DATA_START_DONE, EVENT_PRIORITY_HIGH);
			continue;

		case LCD_PRINT_CALL:
			lcd_app_set_message(&app_handle, 0, "    Calling   ", 0);
			lcd_app_clear_row(&app_handle, 1);
			
			break;

		case LCD_PRINT_ENDCALL:
			lcd_ctx.print_type = LCD_PRINT_OFF;
			continue;

		case LCD_PRINT_MESS:
			lcd_ctx.print_type = LCD_PRINT_NONE;
			lcd_ctx.print_mode = 0;
			continue;
		
		case LCD_PRINT_ERR:
			lcd_ctx.print_type = LCD_PRINT_OFF;
			continue;

		case LCD_PRINT_MODE:
			lcd_ctx.print_type = LCD_PRINT_OFF;
			continue;

		case LCD_PRINT_RESET:
			lcd_ctx.print_type = LCD_PRINT_OFF;
			EVENT_PUBLISH_SIMPLE(EVENT_LCD_PRINT_SELECT_MODE_TIMEOUT, EVENT_PRIORITY_HIGH);
			continue;

		case LCD_PRINT_NONE:
			if (get_data.is_call())
			{
				lcd_ctx.print_type = LCD_PRINT_CALL;
				continue;
			}
			else
			{
				lcd_ctx.print_type = LCD_PRINT_OFF;
				lcd_app_clear_all(&app_handle);
				lcd_app_set_message(&app_handle, 1, "      ", 15000); //  1, timeout 15s
			}
			
			break;

		case LCD_PRINT_PAIRING:
			fl_db_clearAll();
			sys_reboot();
			break;

		case LCD_PRINT_FACTORY_RESET:
			lcd_ctx.print_type = LCD_PRINT_OFF;
			fl_db_clearAll();
			// storage_clean();
			sys_reboot();
			break;

		case LCD_PRINT_CALL_FAIL:
			lcd_ctx.print_type = LCD_PRINT_OFF;
			continue;

		case LCD_PRINT_MESS_NEW:
			if(get_system_time_ms() > lcd_ctx.time_off)
			{
				lcd_ctx.print_type = LCD_PRINT_OFF;
				continue;
			}

			lcd_ctx.row0_mess_num = (lcd_ctx.row1_mess_num +1) % 10;
			for( int i = 0; i < 10; i++)
			{
				if(memcmp(mess_zero, (uint8_t *)G_COUNTER_LCD[lcd_ctx.row0_mess_num], 20) == 0)
				{
					lcd_ctx.row0_mess_num = (lcd_ctx.row0_mess_num + 1) % 10;
				}
				else
				{
					lcd_ctx.row1_mess_num = (lcd_ctx.row0_mess_num + 1) % 10;
					break;
				}
			}

			for( int i = 0; i < 10; i++)
			{
				if(memcmp(mess_zero, (uint8_t *)G_COUNTER_LCD[lcd_ctx.row1_mess_num], 20) == 0)
				{
					lcd_ctx.row1_mess_num = (lcd_ctx.row1_mess_num +1) % 10;
				}
				else
				{
					break;
				}
			}
			EVENT_PUBLISH_SIMPLE(EVENT_LCD_PRINT_MESS_NEW, EVENT_PRIORITY_HIGH);
			return;

		default:
			lcd_ctx.print_mode = 0;
			if (get_data.is_call())
			{
				lcd_ctx.print_type = LCD_PRINT_CALL;
				continue;
			}
			else
			{
				lcd_ctx.enable = false;
				lcd_app_clear_all(&app_handle);
				lcd_off();
			}
			
			break;
		}
		break;
	}
}

lcd_app_config_t app_config = {
    .timeout_callback = my_timeout_callback,
    .get_tick = lcd_app_get_tick,
    .default_timeout_ms = 5000,
    .default_scroll_delay_ms = 500,
	.default_scroll_start_delay_ms = 1500,
};

void lcd_on(void)
{
	gpio_set_level(GPIO_PC0, 1);
}
void lcd_off(void)
{
	gpio_set_level(GPIO_PC0, 0);
}


static subapp_result_t lcd_app_init(subapp_t* self)
{
	gpio_function_en(GPIO_PC0);
	gpio_set_output(GPIO_PC0, 1); 			//enable output
	gpio_set_input(GPIO_PC0, 0);			//disable input
	gpio_set_up_down_res(GPIO_PC0, GPIO_PIN_PULLUP_10K);
	gpio_set_level(GPIO_PC0, 1);

	lcd16x2_error_t result = lcd16x2_init(&lcd_handle, &lcd_config);
	if(result != 0)
	{
		ULOGA("lcd init erro\n");
	}
	lcd16x2_clear(&lcd_handle);

	lcd_app_init_drv(&app_handle, &lcd_handle, &app_config);

	lcd_app_set_message(&app_handle, 0, "   TBS GROUP   ", 30000); //  0, timeout 10s
	lcd_app_set_message(&app_handle, 1, "Chung Suc Kien Tao Tuong Lai", 13000); //  1, timeout 15s

	uint32_t app_lcd_evt_table[] = get_lcd_event();

	for(int i = 0; i < (sizeof(app_lcd_evt_table)/sizeof(app_lcd_evt_table[0])); i++)
	{
		char name[32];
		snprintf(name, sizeof(name), "app_lcd_evt_table[%d]", i);
		event_bus_subscribe(app_lcd_evt_table[i], lcd_app_event_handler, NULL, name);
	}

	lcd_ctx.startup = 1;
	lcd_ctx.print_type = LCD_PRINT_STARTUP;
	lcd_ctx.enable =1;
	lcd_ctx.row0_mess_num = 0;
	lcd_ctx.row1_mess_num = 1;

	return SUBAPP_OK;
}

static subapp_result_t lcd_app_loop(subapp_t* self)
{
	static uint64_t lcdTimeTick = 0;
	if(get_system_time_ms() - lcdTimeTick > TIME_BUTTON_TASK_MS){
		lcdTimeTick = get_system_time_ms()  ; //10ms
	}
	else{
		return SUBAPP_OK;
	}

	LCD_MessageCheck_FlagNew();

	if(lcd_ctx.enable == false)
	{
		return SUBAPP_OK;
	}
	else
	{
		lcd_on();
		lcd_app_update(&app_handle);
	}

	return SUBAPP_OK;
}

static subapp_result_t lcd_app_deinit(subapp_t* self)

{
	return SUBAPP_OK;
}

static void lcd_app_event_handler(const event_t* event, void* user_data)
{
	switch (event->type)
	{
        case EVENT_LCD_PRINT_MESS:
            ULOGA("Handle EVENT_LCD_PRINT_MESS\n");
			lcd_ctx.enable = 1;
			lcd_ctx.print_type = LCD_PRINT_MESS;
			
			if(lcd_ctx.print_mode == 0)
			{
				if(get_data.is_mode_actic())
				{
					lcd_app_set_message(&app_handle, 0, " Dang Trong Ca  ", 20000); //  0, timeout 10s
				}
				else
				{
					lcd_app_set_message(&app_handle, 0, " Dang Chay Thu  ", 20000); //  0, timeout 10s
				}

				for(int i = 0; i <= 10; i++)
				{
					if(i == 10)
					{
						lcd_app_set_message(&app_handle, 1, "Khong Co Thong Bao", 15000); //  0, timeout 10s
						break;
					}
					if(memcmp(mess_zero, (uint8_t *)G_COUNTER_LCD[lcd_ctx.row0_mess_num], 20) == 0)
					{
						lcd_ctx.row0_mess_num = (lcd_ctx.row0_mess_num +1) % 10;
						continue;
					}
					else
					{
						lcd_app_set_message(&app_handle, 1, (char *)(G_COUNTER_LCD[lcd_ctx.row0_mess_num]), 15000); //  1, timeout 15s
						lcd_ctx.row0_mess_num = (lcd_ctx.row0_mess_num +1) % 10;
						lcd_ctx.row1_mess_num = (lcd_ctx.row0_mess_num +1) % 10;
						lcd_ctx.print_mode = 1;
						break;
					}
				}
			}
			else
			{
				lcd_app_clear_all(&app_handle);
				
				for( int i = 0; i < 10; i++)
				{
					if(memcmp(mess_zero, (uint8_t *)G_COUNTER_LCD[lcd_ctx.row0_mess_num], 20) == 0)
					{
						lcd_ctx.row0_mess_num = (lcd_ctx.row0_mess_num +1) % 10;
						continue;
					}
					else
					{
						lcd_app_set_message(&app_handle, 0, (char *)G_COUNTER_LCD[lcd_ctx.row0_mess_num], 15000); //  1, timeout 15s
						lcd_ctx.row0_mess_num = (lcd_ctx.row0_mess_num +1) % 10;
						lcd_ctx.row1_mess_num = lcd_ctx.row0_mess_num;
						break;
					}
				}

				for( int i = 0; i < 10; i++)
				{
					if(memcmp(mess_zero, (uint8_t *)G_COUNTER_LCD[lcd_ctx.row1_mess_num], 20) == 0)
					{
						lcd_ctx.row1_mess_num = (lcd_ctx.row1_mess_num +1) % 10;
						continue;
					}
					else
					{
						lcd_app_set_message(&app_handle, 1, (char *)G_COUNTER_LCD[lcd_ctx.row1_mess_num], 15000); //  1, timeout 15s
						lcd_ctx.row1_mess_num = (lcd_ctx.row1_mess_num +1) % 10;
						break;
					}
				}
				
			}
			
			
            // TODO: turn LED to indicate network online
            break;

		case EVENT_DATA_PASS_PRODUCT_UP:
			if(lcd_ctx.print_type != LCD_PRINT_ERR)
			{
				break;
			}
		case EVENT_DATA_ERR_PRODUCT_CHANGE:
		case EVENT_DATA_PASS_PRODUCT_DOWN:
        case EVENT_LCD_PRINT_COUNT_PRODUCT:
            ULOGA("Handle EVENT_LCD_PRINT_COUNT_PRODUCT\n");
			lcd_ctx.enable = 1;
			lcd_ctx.print_type = LCD_PRINT_ERR;
			sprintf(lcd_ctx.display.line1, "pass %4d", get_data.pass_product());
			sprintf(lcd_ctx.display.line2, "err  %4d", get_data.err_product());
			lcd_app_set_message(&app_handle, 0, lcd_ctx.display.line1, 30000); //  0, timeout 10s
			lcd_app_set_message(&app_handle, 1, lcd_ctx.display.line2, 15000); //  0, timeout 10s
			
            // TODO: turn LED to indicate network offline
            break;
		
		case EVENT_DATA_CALL:
            ULOGA("Handle EVENT_DATA_CALL\n");
			lcd_ctx.enable = 1;
			lcd_ctx.print_type = LCD_PRINT_CALL;
			lcd_app_set_message(&app_handle, 0, "    Calling   ", 0);
			lcd_app_clear_row(&app_handle, 1);

            // TODO: blink LED for call state
			break;

		case EVENT_DATA_ENDCALL:
            ULOGA("Handle EVENT_DATA_ENDCALL\n");
			lcd_ctx.enable = 1;
			lcd_ctx.print_type = LCD_PRINT_ENDCALL;
			lcd_app_set_message(&app_handle, 0, "  End Calling  ", 15000); //  0, timeout 10s
			lcd_app_clear_row(&app_handle, 1);

            // TODO: blink LED for call state
			break;

		case EVENT_LCD_PRINT_MAC:
			ULOGA("Handle EVENT_LCD_PRINT_MAC\n");
			lcd_ctx.enable = 1;
			lcd_ctx.print_type = LCD_PRINT_ENDCALL;
			char line0[16];
			char line1[16];
			sprintf(line0, "MAC: %02X %02X %02X", get_data.mac()[0], get_data.mac()[1], get_data.mac()[2] );
			sprintf(line1, "     %02X %02X %02X", get_data.mac()[3], get_data.mac()[4], get_data.mac()[5] );
			lcd_app_set_message(&app_handle, 0, line0, 15000); //  0, timeout 10s
			lcd_app_set_message(&app_handle, 1, line1, 15000); //  0, timeout 10s

			break;

		case EVENT_DATA_SWITCH_MODE:
			ULOGA("Handler EVENT_DATA_SWITCH_MODE\n");
			lcd_ctx.enable = 1;
			lcd_ctx.print_type = LCD_PRINT_MODE;
			if(get_data.is_mode_actic())
			{
				lcd_app_set_message(&app_handle, 0, "    Chuc Ban    ", 30000); //  0, timeout 10s
				lcd_app_set_message(&app_handle, 1, "  Hieu Qua Cao  ", 15000); //  0, timeout 10s
			}
			else
			{
				lcd_app_set_message(&app_handle, 0, "     Che Do     ", 30000); //  0, timeout 10s
				lcd_app_set_message(&app_handle, 1, "Chay Thu Nghiem ", 15000); //  0, timeout 10s
			}

			break;

		case EVENT_DATA_RESET:
			ULOGA("Handle EVENT_DATA_RESET\n");
			// lcd_ctx.enable = 1;
			// lcd_ctx.print_type = LCD_PRINT_RESET;
			// lcd_app_set_message(&app_handle, 0, "    Reset OK    ", 30000); //  0, timeout 10s
			// if(get_data.is_mode_actic())
			// {
			// 	lcd_app_set_message(&app_handle, 1, "    Trong Ca    ", 15000); //  0, timeout 10s
			// }
			// else
			// {
			// 	lcd_app_set_message(&app_handle, 1, "Chay Thu Nghiem ", 15000); //  0, timeout 10s
			// }

			break;

		case EVENT_LCD_PRINT_SELECT_MODE:
			ULOGA("Handler EVENT_LCD_PRINT_SELECT_MODE\n");

			lcd_ctx.enable = 1;
			lcd_ctx.print_type = LCD_PRINT_RESET;
			lcd_app_set_message(&app_handle, 0, "    Reset OK    ", 30000); //  0, timeout 10s
			lcd_app_set_message(&app_handle, 1, " Tinh Trong Ca? ", 10000); //  0, timeout 10s		

			break;

		case EVENT_LCD_PRINT_PAIRING:
			ULOGA("Handler EVENT_LCD_PRINT_PAIRING\n");

			lcd_ctx.enable = 1;
			lcd_ctx.print_type = LCD_PRINT_PAIRING;
			lcd_app_set_message(&app_handle, 0, "    Pairing     ", 30000); //  0, timeout 10s
			lcd_app_set_message(&app_handle, 1, "                ", 3000); //  0, timeout 10s		

			break;

		case EVENT_LCD_PRINT_FACTORY_RESET:
			ULOGA("Handler EVENT_LCD_PRINT_FACTORY_RESET\n");

			lcd_ctx.enable = 1;
			lcd_ctx.print_type = LCD_PRINT_FACTORY_RESET;
			lcd_app_set_message(&app_handle, 0, " Factory Reset  ", 30000); //  0, timeout 10s
			lcd_app_set_message(&app_handle, 1, "                ", 3000); //  0, timeout 10s		

			break;
		
		case EVENT_DATA_CALL_FAIL_OFFLINE:
			ULOGA("Handler EVENT_DATA_CALL_FAIL_OFFLINE\n");

			lcd_ctx.enable = 1;
			lcd_ctx.print_type = LCD_PRINT_CALL_FAIL;
			lcd_app_set_message(&app_handle, 0, "   Call Fail    ", 30000); //  0, timeout 10s
			lcd_app_set_message(&app_handle, 1, "   Offline      ", 3000); //  0, timeout 10s		

			break;

		case EVENT_DATA_CALL_FAIL_NORSP:
			ULOGA("Handler EVENT_DATA_CALL_FAIL_NORSP\n");

			lcd_ctx.enable = 1;
			lcd_ctx.print_type = LCD_PRINT_CALL_FAIL;
			lcd_app_set_message(&app_handle, 0, "   Call Fail    ", 30000); //  0, timeout 10s
			lcd_app_set_message(&app_handle, 1, "   No RSP       ", 3000); //  0, timeout 10s		

			break;

		case EVENT_DATA_ENDCALL_FAIL_NORSP:
			ULOGA("Handler EVENT_DATA_ENDCALL_FAIL_NORSP\n");

			lcd_ctx.enable = 1;
			lcd_ctx.print_type = LCD_PRINT_CALL_FAIL;
			lcd_app_set_message(&app_handle, 0, " EndCall Fail   ", 30000); //  0, timeout 10s
			lcd_app_set_message(&app_handle, 1, " No RSP         ", 3000); //  0, timeout 10s	

			break;

		case EVENT_LCD_PRINT_CALL__:
			ULOGA("Handler EVENT_LCD_PRINT_CALL__\n");

			lcd_ctx.enable = 1;
			lcd_ctx.print_type = LCD_PRINT_CALL_FAIL;
			lcd_app_set_message(&app_handle, 0, "  Call......    ", 30000); //  0, timeout 10s
			lcd_app_set_message(&app_handle, 1, "                ", 3000); //  0, timeout 10s	

			break;

		case EVENT_LCD_PRINT_MESS_NEW:
			ULOGA("Handler EVENT_LCD_PRINT_MESS_NEW\n");

			lcd_ctx.enable = 1;
			lcd_ctx.print_type = LCD_PRINT_MESS_NEW;
			lcd_app_set_message(&app_handle, 0, (char *)G_COUNTER_LCD[lcd_ctx.row0_mess_num], 30000); //  1, timeout 15s
			lcd_app_set_message(&app_handle, 1, (char *)G_COUNTER_LCD[lcd_ctx.row1_mess_num], 15000); //  1, timeout 15s

			break;

        default:
            ULOGA("Unknown LCD event: %d\n", (uint32_t)event->type);
            break;
    }
}

static void LCD_MessageCheck_FlagNew(void){
	for (u8 var = 0; var < COUNTER_LCD_MESS_MAX; ++var) 
	{
		tbs_counter_lcd_t *mess_lcd = (tbs_counter_lcd_t *)&G_COUNTER_LCD[var][0];
		if(mess_lcd->f_new == 1)
		{
			lcd_ctx.row0_mess_num = var;
			for(; var < COUNTER_LCD_MESS_MAX; ++var)
			{
				tbs_counter_lcd_t *mess_lcd1 = (tbs_counter_lcd_t *)&G_COUNTER_LCD[var][0];
				if(mess_lcd1->f_new == 1)
				{
					lcd_ctx.row1_mess_num = var;
					mess_lcd1->f_new = 0;
				}
			}
			lcd_ctx.row1_mess_num = lcd_ctx.row0_mess_num;
			ULOGA("lcd_ctx.row0_mess_num %d\n", lcd_ctx.row0_mess_num);
			lcd_ctx.time_off = get_system_time_ms() + LCD_TIME_DELAY_PRINT;
			EVENT_PUBLISH_SIMPLE(EVENT_LCD_PRINT_MESS_NEW, EVENT_PRIORITY_HIGH);
			mess_lcd->f_new = 0;
		}
	}
}

#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/
