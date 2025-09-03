/*
 * button_app.c
 *
 *      Author: hoang
 */

#ifndef MASTER_CORE

#include "vendor/TBS_dev/TBS_dev_config.h"

#ifdef COUNTER_DEVICE

#include "../../user_lib.h"
#include "button_app.h"

/**
 * BUTTON
 */

#define BUTTON_ID_RESET			0
#define BUTTON_ID_MAIN			1
#define BUTTON_ID_CMAIN			2
#define BUTTON_ID_PEU			3
#define BUTTON_ID_PED			4
#define BUTTON_ID_PPD			5
#define BUTTON_ID_PPU			6
// add pin
#define BUTTON_ID_MAX			7

extern tca9555_handle_t tca9555_handle;
extern app_share_data_t app_data;

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
        PortPin_Map *all_pins[BUTTON_ID_MAX]; // 3 control + 8 data = 11 pins
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
	for(uint8_t i = 0; i < BUTTON_ID_MAX; i++)
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

uint64_t bt_MsTick_get(void)
{
	return get_system_time_ms();
}

void on_button_click(uint8_t button_id) {
    ULOGA("Button %d clicked!\n", button_id);
}

void on_triple_click_specific(uint8_t button_id, uint8_t click_count)
{
	ULOGA(">>> Button %d: Specific TRIPLE CLICK handler triggered\n", button_id);
}
void on_button_multiclick(uint8_t button_id, uint8_t click_count) {
    ULOGA(">>> Button %d: %d-CLICK detected!\n", button_id, click_count);

    // You can handle different click counts here
    switch (click_count) {
        case 1:
            ULOGA("    -> Single click action\n");
            on_button_click(button_id);
            break;
        case 2:
            ULOGA("    -> Double click action\n");
            on_button_click(button_id);
            break;
        case 3:
            ULOGA("    -> Triple click action\n");
            on_button_click(button_id);
            break;
        case 4:
            ULOGA("    -> Quad click action\n");
            on_button_click(button_id);
            break;
        case 5:
            ULOGA("    -> Penta click action\n");
            on_button_click(button_id);
            break;
        default:
            ULOGA("    -> %d clicks - that's impressive!\n", click_count);
            on_button_click(button_id);
            break;
    }
}

void my_combo_callback(uint8_t *button_ids, uint8_t count, uint32_t hold_time)
{
	ULOGA("my_combo_callback %d, %d, %d, %d \n", button_ids[0], button_ids[1], count, hold_time);
}

void my_pattern_callback(uint8_t *pattern, uint8_t length)
{
	ULOGA("my_pattern_callback: " );
	for (int i =0; i < length; i++)
	{
		printf(" %d", pattern[i]);
	}
	printf("/n");
}

static void main_on_lick(uint8_t button_id);
static void cmain_on_lick(uint8_t button_id);
static void peu_on_lick(uint8_t button_id);
static void ped_on_lick(uint8_t button_id);
static void ppd_on_lick(uint8_t button_id);
static void ppu_on_lick(uint8_t button_id);
static void reset_hold_3s(uint8_t button_id, uint32_t actual_hold_time);
static void reset_hold_3ss(uint8_t button_id, uint32_t actual_hold_time);
static void peu_rst_hold_5s(uint8_t *button_ids, uint8_t count, uint32_t hold_time);
static void ped_rst_hold_5s(uint8_t *button_ids, uint8_t count, uint32_t hold_time);


button_hal_t button_hal = {
    .button_pin_init_all = button_pin_init_all,
    .button_pin_init = button_pin_init,
    .button_pin_read = button_pin_read,
    .get_system_time_ms = bt_MsTick_get,
};

void user_button_app_init(void)
{
    if (!button_manager_init(&button_hal)) {
        ULOGA("Error: Failed to initialize button manager!\n");
        return;
    }

    button_set_click_callback(BUTTON_ID_PED, ped_on_lick);
    button_set_click_callback(BUTTON_ID_PEU, peu_on_lick);
    button_set_click_callback(BUTTON_ID_PPD, ppd_on_lick);
    button_set_click_callback(BUTTON_ID_PPU, ppu_on_lick);
    button_set_click_callback(BUTTON_ID_MAIN, main_on_lick);
    button_set_click_callback(BUTTON_ID_CMAIN, cmain_on_lick);
    button_set_release_callback(BUTTON_ID_RESET, reset_hold_3s);
//    button_add_hold_level(BUTTON_ID_RESET, 3000, reset_hold_3ss);

    // Combo:
    uint8_t combo_buttons1[] = {BUTTON_ID_PED, BUTTON_ID_RESET};
    uint8_t combo_id1 = button_add_combo(combo_buttons1, 2, 500); // 500ms detection window
    button_set_combo_hold_callback(combo_id1, 5000, ped_rst_hold_5s);

    uint8_t combo_buttons2[] = {BUTTON_ID_PEU, BUTTON_ID_RESET};
    uint8_t combo_id2 = button_add_combo(combo_buttons2, 2, 500); // 500ms detection window
    button_set_combo_hold_callback(combo_id2, 5000, peu_rst_hold_5s);
//    button_set_combo_click_callback(combo_id, my_combo_callback);

    // Pattern:
//    uint8_t pattern_buttons[] = {2, 3, 4};
//    uint8_t pattern_id = button_add_pattern(pattern_buttons, 3, 1000); // 1s timeout
//    button_set_pattern_callback(pattern_id, my_pattern_callback);


}

void user_button_app_task(void)
{
	static uint64_t buttonTimeTick = 0;
	if(get_system_time_ms() - buttonTimeTick > TIME_BUTTON_TASK_MS){
		buttonTimeTick = get_system_time_ms()  ; //10ms
	}
	else{
		return ;
	}
	button_process_all();
}

/*****************************************************************/

static void main_on_lick(uint8_t button_id)
{
	ULOGA("main_on_lick\n");
	app_data.bt_call = 1;
}

static void cmain_on_lick(uint8_t button_id)
{
	ULOGA("cmain_on_lick\n");
	app_data.bt_endcall = 1;
}

static void peu_on_lick(uint8_t button_id)
{
	ULOGA("peu_on_lick\n");
	if(app_data.err_product++ >= 1000)
	{
		app_data.err_product = 0;
	}
	app_data.print_err_led7 = 1;

}

static void ped_on_lick(uint8_t button_id)
{
	ULOGA("ped_on_lick\n");
	if(app_data.err_product)
	{
		app_data.err_product--;
	}
	app_data.print_err_led7 = 1;
}

static void ppd_on_lick(uint8_t button_id)
{
	ULOGA("ppd_on_lick\n");
	if(app_data.pass_product)
	{
		app_data.pass_product--;
	}
	app_data.print_err_led7 = 0;
}

static void ppu_on_lick(uint8_t button_id)
{
	ULOGA("ppu_on_lick\n");
	if(app_data.pass_product++ >= 10000)
	{
		app_data.pass_product = 0;
	}
	app_data.print_err_led7 = 0;
}

static void reset_hold_3ss(uint8_t button_id, uint32_t press_duration_ms)
{
	ULOGA("reset_hold_3s: %d\n", press_duration_ms);
		app_data.bt_rst = 1;
}

static void reset_hold_3s(uint8_t button_id, uint32_t press_duration_ms)
{
	ULOGA("reset_hold_3s: %d\n", press_duration_ms);
	if(press_duration_ms > 3000)
	{
		app_data.bt_rst = 1;
	}
}

static void peu_rst_hold_5s(uint8_t *button_ids, uint8_t count, uint32_t hold_time)
{
	ULOGA("peu_rst_hold_5s\n");
	app_data.reset_factory  =1;
}

static void ped_rst_hold_5s(uint8_t *button_ids, uint8_t count, uint32_t hold_time)
{
	ULOGA("peu_rst_hold_5s\n");
	app_data.pair = 1;
}

#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/
