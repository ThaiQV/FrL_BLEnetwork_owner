/*
 * button_app.c
 *
 *      Author: hoang
 */

#ifndef MASTER_CORE
#include "tl_common.h"
#include "vendor/TBS_dev/TBS_dev_config.h"

#ifdef COUNTER_DEVICE

#include "../../user_lib.h"
#include "button_app.h"

/**
 * BUTTON
 */

#define BUTTON_ID_RESET			0
#define BUTTON_ID_CALL			1
#define BUTTON_ID_ENDCALL		2
#define BUTTON_ID_PEU			3
#define BUTTON_ID_PED			4
#define BUTTON_ID_PPD			5
#define BUTTON_ID_PPU			6
// add pin
#define BUTTON_ID_MAX			7

extern tca9555_handle_t tca9555_handle;

typedef enum {
	BUTTON_MODE_NORMAL,
	BUTTON_MODE_HOLD_MASTER_COUNTER,
} button_mode_app_t;

// SubApp context structure
typedef struct {
	bool start;
	button_mode_app_t mode;
} button_context_t;

// Static context instance
static button_context_t button_ctx = {
	.start = 0,
	.mode = BUTTON_MODE_NORMAL,
};

// Forward declarations
static subapp_result_t button_app_init(subapp_t* self);
static subapp_result_t button_app_loop(subapp_t* self);
static subapp_result_t button_app_deinit(subapp_t* self);
static void button_app_event_handler(const event_t* event, void* user_data);


subapp_t button_app = {
        .name = "button", 
        .context = &button_ctx, 
        .state = SUBAPP_STATE_IDLE, 
        .init = button_app_init, 
        .loop = button_app_loop, 
        .deinit = button_app_deinit, 
        .on_event = NULL, 
        .pause = NULL, 
        .resume = NULL, 
        .is_registered = false, 
        .event_mask = 0 
    };

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

static void button_pin_init_all(void)
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

static bool button_pin_init(button_pin_t pin, bool active_low)
{
	return true;
}

static bool button_pin_read(button_pin_t pin)
{
	tca9555_pin_state_t value;
	tca9555_read_pin_input(&tca9555_handle, button_pin_map.all_pins[pin]->tca_port, button_pin_map.all_pins[pin]->tca_pin, &value);
	return value;
}

static uint64_t bt_MsTick_get(void)
{
	return get_system_time_ms();
}

static void button_press(uint8_t button_id);
static void button_release(uint8_t button_id, uint32_t actual_hold_time);
static void button_on_click(uint8_t button_id);
static void button_hold_time(uint8_t button_id, uint32_t actual_hold_time);
//static void button_combo_hold_time(uint8_t *button_id, uint32_t actual_hold_time);
static void peu_rst_hold_5s(uint8_t *button_ids, uint8_t count, uint32_t hold_time);
static void ped_rst_hold_5s(uint8_t *button_ids, uint8_t count, uint32_t hold_time);

button_hal_t button_hal = {
    .button_pin_init_all = button_pin_init_all,
    .button_pin_init = button_pin_init,
    .button_pin_read = button_pin_read,
    .get_system_time_ms = bt_MsTick_get,
};

static subapp_result_t button_app_init(subapp_t* self)
{
    if (!button_manager_init(&button_hal)) {
        ULOGA("Error: Failed to initialize button manager!\n");
        return SUBAPP_ERROR;
    }

	button_set_press_callback(BUTTON_ID_PPU, button_press);
	button_set_release_callback(BUTTON_ID_PPU, button_release);
	
    // // Combo:
    uint8_t combo_buttons1[] = {BUTTON_ID_PEU, BUTTON_ID_RESET};
    uint8_t combo_id1 = button_add_combo(combo_buttons1, 2, 500); // 500ms detection window
    button_set_combo_hold_callback(combo_id1, 5000, peu_rst_hold_5s);

    uint32_t app_button_evt_table[] = get_button_event();

	for(int i = 0; i < (sizeof(app_button_evt_table)/sizeof(app_button_evt_table[0])); i++)
	{
		char name[32];
		snprintf(name, sizeof(name), "app_button_evt_table[%d]", i);
		event_bus_subscribe(app_button_evt_table[i], button_app_event_handler, NULL, name);
	}

	return SUBAPP_OK;
}

static subapp_result_t button_app_loop(subapp_t* self)
{
	static uint64_t buttonTimeTick = 0;
	if(get_system_time_ms() - buttonTimeTick > TIME_BUTTON_TASK_MS){
		buttonTimeTick = get_system_time_ms()  ; //10ms
	}
	else{
		return SUBAPP_OK;
	}

    // if(button_ctx.start == 0)
    // {
    //     return SUBAPP_OK;
    // }

	button_process_all();
	
	return SUBAPP_OK;
}

static subapp_result_t button_app_deinit(subapp_t* self)
{
	return SUBAPP_OK;
}

static void button_app_event_handler(const event_t* event, void* user_data)
{
    switch(event->type)
    {
        case EVENT_DATA_START_DONE:
            ULOGA("button Handle EVENT_DATA_START_DONE\n");
            button_ctx.start = 1;

			button_set_click_callback(BUTTON_ID_PED, button_on_click);
			button_set_click_callback(BUTTON_ID_PEU, button_on_click);
			button_set_click_callback(BUTTON_ID_PPD, button_on_click);
			button_set_click_callback(BUTTON_ID_PPU, button_on_click);
			button_set_click_callback(BUTTON_ID_CALL, button_on_click);
			button_set_click_callback(BUTTON_ID_ENDCALL, button_on_click);
			button_set_click_callback(BUTTON_ID_RESET, button_on_click);

			button_add_hold_level(BUTTON_ID_RESET, 3000, button_hold_time);
			button_add_hold_level(BUTTON_ID_RESET, 5000, button_hold_time);
			button_add_hold_level(BUTTON_ID_CALL, 3000, button_hold_time);
			button_add_hold_level(BUTTON_ID_ENDCALL, 3000, button_hold_time);
			button_add_hold_level(BUTTON_ID_ENDCALL, 5000, button_hold_time);
			button_add_hold_level(BUTTON_ID_PPD, 3000, button_hold_time);
			
			// Combo:

			uint8_t combo_buttons2[] = {BUTTON_ID_PED, BUTTON_ID_RESET};
			uint8_t combo_id2 = button_add_combo(combo_buttons2, 2, 500); // 500ms detection window
			button_set_combo_hold_callback(combo_id2, 5000, ped_rst_hold_5s);

            break;
        default:
            break;
    }
	return;
}

/*****************************************************************/
/**
 *   Button process event
 */
static void button_press(uint8_t button_id)
{
	ULOGA("button pressed id: %d\n", button_id);
	switch (button_id)
	{
	case BUTTON_ID_ENDCALL:

		break;
	
	case BUTTON_ID_CALL:

		break;

	case BUTTON_ID_PED:
		break;

	case BUTTON_ID_PEU:

		break;

	case BUTTON_ID_PPD:

		break;

	case BUTTON_ID_PPU:
		button_ctx.mode = BUTTON_MODE_HOLD_MASTER_COUNTER;

		break;

	case BUTTON_ID_RESET:

		break;

	default:
		break;
	}
}

static void button_release(uint8_t button_id, uint32_t actual_hold_time)
{
	ULOGA("button release id: %d\n", button_id);
	switch (button_id)
	{
	case BUTTON_ID_ENDCALL:

		break;
	
	case BUTTON_ID_CALL:

		break;

	case BUTTON_ID_PED:
		break;

	case BUTTON_ID_PEU:

		break;

	case BUTTON_ID_PPD:

		break;

	case BUTTON_ID_PPU:
		button_ctx.mode = BUTTON_MODE_NORMAL;

		break;

	case BUTTON_ID_RESET:

		break;

	default:
		break;
	}
}

static void button_on_click(uint8_t button_id)
{
	ULOGA("button onclick id: %d\n", button_id);

	switch (button_ctx.mode)
	{
	case BUTTON_MODE_HOLD_MASTER_COUNTER:
		switch (button_id)
		{
		case BUTTON_ID_RESET:
			EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_RST_HOLD_3S, EVENT_PRIORITY_HIGH);
			break;
		
		case BUTTON_ID_CALL:
			EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_PPD_HOLD_3S, EVENT_PRIORITY_HIGH);
			break;

		default:
			EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_NONE, EVENT_PRIORITY_HIGH);
			break;
		}	

		break;
	
	default:
		switch (button_id)
		{
		case BUTTON_ID_ENDCALL:
			EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_ENDCALL_ONCLICK, EVENT_PRIORITY_HIGH);

			break;
		
		case BUTTON_ID_CALL:
			EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_CALL_ONCLICK, EVENT_PRIORITY_HIGH);

			break;

		case BUTTON_ID_PED:
			EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_PED_ONCLICK, EVENT_PRIORITY_HIGH);
			break;

		case BUTTON_ID_PEU:
			EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_PEU_ONCLICK, EVENT_PRIORITY_HIGH);
			break;

		case BUTTON_ID_PPD:
			EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_PPD_ONCLICK, EVENT_PRIORITY_HIGH);
			break;

		case BUTTON_ID_PPU:
			EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_PPU_ONCLICK, EVENT_PRIORITY_HIGH);
			break;

		case BUTTON_ID_RESET:
			EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_RST_ONCLICK, EVENT_PRIORITY_HIGH);
			break;

		default:
			EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_NONE, EVENT_PRIORITY_HIGH);
			break;
		}

		break;
	}

	
}

static void button_hold_time(uint8_t button_id, uint32_t actual_hold_time)
{
	ULOGA("button hold time id: %d\n", button_id);
	
	switch (button_ctx.mode)
	{
	case BUTTON_MODE_HOLD_MASTER_COUNTER:
		switch (button_id)
		{
		case BUTTON_ID_CALL:
			EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_CALL_HOLD_3S, EVENT_PRIORITY_HIGH);

			break;

		case BUTTON_ID_ENDCALL:
			EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_ENDCALL_HOLD_5S, EVENT_PRIORITY_HIGH);

			break;

		case BUTTON_ID_PPD:
			EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_RST_PEU_HOLD_5S, EVENT_PRIORITY_HIGH);

			break;

		case BUTTON_ID_RESET:
			switch (actual_hold_time)
			{
			case 3000:
				EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_RST_PED_HOLD_5S, EVENT_PRIORITY_HIGH);

				break;

			default:
				break;
			}

			break;

		default:
			EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_NONE, EVENT_PRIORITY_HIGH);
			break;
		}

		break;
	
	default:
		switch (button_id)
		{
		case BUTTON_ID_ENDCALL:
			EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_ENDCALL_HOLD_5S, EVENT_PRIORITY_HIGH);

			break;

		case BUTTON_ID_PPD:
			EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_PPD_HOLD_3S, EVENT_PRIORITY_HIGH);

			break;

		case BUTTON_ID_RESET:
			switch (actual_hold_time)
			{
			case 3000:
				EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_RST_HOLD_3S, EVENT_PRIORITY_HIGH);

				break;

			default:
				break;
			}

			break;

		default:
			EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_NONE, EVENT_PRIORITY_HIGH);
			break;
		}

		break;
	}

}

//static void button_combo_hold_time(uint8_t *button_id, uint32_t actual_hold_time)
//{
//
//}

static void ped_rst_hold_5s(uint8_t *button_ids, uint8_t count, uint32_t hold_time)
{
	ULOGA("ped_rst_hold_5s\n");
	EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_RST_PED_HOLD_5S, EVENT_PRIORITY_HIGH);
}

static void peu_rst_hold_5s(uint8_t *button_ids, uint8_t count, uint32_t hold_time)
{
	ULOGA("peu_rst_hold_5s\n");
	if(button_ctx.start)
	{
		return;
	}
	EVENT_PUBLISH_SIMPLE(EVENT_BUTTON_RST_PEU_HOLD_5S, EVENT_PRIORITY_HIGH);
}


#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/
