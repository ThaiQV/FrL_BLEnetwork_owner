/*
 * button_app.c
 *
 *      Author: hoang
 */
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

static void main_on_lick(uint8_t button_id);
static void cmain_on_lick(uint8_t button_id);
static void peu_on_lick(uint8_t button_id);
static void ped_on_lick(uint8_t button_id);
static void ppd_on_lick(uint8_t button_id);
static void ppu_on_lick(uint8_t button_id);
static void reset_hold_3s(uint8_t button_id, uint32_t actual_hold_time);



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

void user_button_app_init(void)
{
    if (!button_manager_init(&button_hal)) {
        printf("Error: Failed to initialize button manager!\n");
        return;
    }

    button_set_click_callback(BUTTON_ID_PED, ped_on_lick);
    button_set_click_callback(BUTTON_ID_PEU, peu_on_lick);
    button_set_click_callback(BUTTON_ID_PPD, ppd_on_lick);
    button_set_click_callback(BUTTON_ID_PPU, ppu_on_lick);
    button_set_click_callback(BUTTON_ID_MAIN, main_on_lick);
    button_set_click_callback(BUTTON_ID_CMAIN, cmain_on_lick);
    button_add_hold_level(BUTTON_ID_RESET, 3000, reset_hold_3s);

}

void user_button_app_task(void)
{
	button_process_all();
}

/*****************************************************************/

static void main_on_lick(uint8_t button_id)
{
	printf("main_on_lick\n");
	app_data.bt_call = 1;
}

static void cmain_on_lick(uint8_t button_id)
{
	printf("cmain_on_lick\n");
	app_data.bt_endcall = 1;
}

static void peu_on_lick(uint8_t button_id)
{
	printf("peu_on_lick\n");
	app_data.err_product++;
}

static void ped_on_lick(uint8_t button_id)
{
	printf("ped_on_lick\n");
	app_data.err_product--;
}

static void ppd_on_lick(uint8_t button_id)
{
	printf("ppd_on_lick\n");
	app_data.pass_product--;
}

static void ppu_on_lick(uint8_t button_id)
{
	printf("ppu_on_lick\n");
	app_data.pass_product++;
}

static void reset_hold_3s(uint8_t button_id, uint32_t actual_hold_time)
{
	printf("reset_hold_3s\n");
	app_data.bt_rst = 1;
}


