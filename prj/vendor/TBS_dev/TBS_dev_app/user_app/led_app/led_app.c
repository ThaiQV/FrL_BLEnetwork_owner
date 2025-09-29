/**
 * @file led_app.c
 * @author Nghia Hoang
 * @date 2025
 */

#ifndef MASTER_CORE

#include "vendor/TBS_dev/TBS_dev_config.h"

#ifdef COUNTER_DEVICE

#include "../../user_lib.h"
#include "led_app.h"

#define LED_ID_CALL		0
#define LED_ID_NWK		1
// add pin led
#define LED_ID_MAX		2

// pin level to led on/off
#define LED_ON			0
#define LED_OFF			1

typedef struct {
    union {
        struct {
        	gpio_pin_e CALL;
        	gpio_pin_e NWK;
        };
        gpio_pin_e all_pins[2]; //
    };
} Led_Pin_TypeDef;

Led_Pin_TypeDef led_pin_map = {
		.CALL = GPIO_PA5,
		.NWK = GPIO_PA6,
};

// Static context instance
static led_context_t led_ctx = {0};

// Forward declarations
static subapp_result_t led_app_init(subapp_t* self);
static subapp_result_t led_app_loop(subapp_t* self);
static subapp_result_t led_app_deinit(subapp_t* self);
static void led_app_event_handler(const event_t* event, void* user_data);


subapp_t led_app = {
        .name = "led", 
        .context = &led_ctx,
        .state = SUBAPP_STATE_IDLE, 
        .init = led_app_init, 
        .loop = led_app_loop, 
        .deinit = led_app_deinit, 
        .on_event = NULL,
        .pause = NULL, 
        .resume = NULL, 
        .is_registered = false, 
        .event_mask = 0 
    };


/*
 *  Static functions
 */
void led_pin_init_all(void)
{
    for(int i=0; i < LED_ID_MAX ; i++)
    {
        gpio_function_en(led_pin_map.all_pins[i]);
        gpio_set_output(led_pin_map.all_pins[i], 1);
        gpio_set_input(led_pin_map.all_pins[i], 0);
        gpio_set_up_down_res(led_pin_map.all_pins[i], GPIO_PIN_PULLUP_10K);
        gpio_set_level(led_pin_map.all_pins[i], LED_OFF);
    }

    led_add(LED_ID_CALL, false);  //
    led_add(LED_ID_NWK, false);  //
}

bool led_pin_init(led_pin_t pin, bool active_high)
{
//	gpio_function_en(pin);
//	gpio_set_output(pin, 1); 			//enable output
//	gpio_set_input(pin, 0);			//disable input
//	gpio_set_up_down_res(pin, GPIO_PIN_PULLUP_10K);
//	gpio_set_level(pin, active_high);
	return true;
}

void led_pin_set(led_pin_t pin, bool state)
{
    if (pin < LED_ID_MAX) {
        gpio_set_level(led_pin_map.all_pins[pin], state);
    }
}

bool led_pin_get(led_pin_t pin)
{
	if (pin < LED_ID_MAX) {
		return gpio_get_level(led_pin_map.all_pins[pin]);
	}
	return true;
}

uint64_t led_MsTick_get(void)
{
	return get_system_time_ms();
}

led_hal_t hal = {
    .led_pin_init_all = led_pin_init_all,
	.led_pin_init = led_pin_init,
    .led_pin_set = led_pin_set,
    .led_pin_get = led_pin_get,
    .get_system_time_ms = led_MsTick_get,
};

led_shared_data_t led_data;
void led_callback(uint8_t led_id);

void led_callback(uint8_t led_id)
{
	ULOGA("led blink callback \n");
	switch (led_id)
	{
	case LED_ID_CALL:
		if(led_ctx.is_call)
		{
			led_on(LED_ID_CALL);
		}
		else
		{
			led_off(LED_ID_CALL);
		}
		break;

	case LED_ID_NWK:
		break;
	
	default:
		break;
	}
}


static subapp_result_t led_app_init(subapp_t* self)
{
	led_manager_init(&hal);

	led_set_blink_complete_callback(LED_ID_CALL, led_callback);

	led_all_on();

	uint32_t app_led_evt_table[] = get_led_event();

	for(int i = 0; i < (sizeof(app_led_evt_table)/sizeof(app_led_evt_table[0])); i++)
	{
		char name[32];
		snprintf(name, sizeof(name), "app_led_evt_table[%d]", i);
		event_bus_subscribe(app_led_evt_table[i], led_app_event_handler, NULL, name);
	}
	
	return SUBAPP_OK;
}

static subapp_result_t led_app_loop(subapp_t* self)

{
	static uint64_t ledTimeTick = 0;
	if(get_system_time_ms() - ledTimeTick > TIME_LED_TASK_MS){
		ledTimeTick = get_system_time_ms()  ; //10ms
	}
	else{
		return SUBAPP_OK;
	}

	if(led_ctx.start == 0)
	{
		return SUBAPP_OK;
	}

	led_process_all();

	return SUBAPP_OK;
}

static subapp_result_t led_app_deinit(subapp_t* self)
{
	return SUBAPP_OK;
}

static void led_app_event_handler(const event_t* event, void* user_data)
{
	switch (event->type) {
        case EVENT_LED_NWK_ONLINE:
            ULOGA("Handle EVENT_LED_NWK_ONLINE\n");
			led_on(LED_ID_NWK);
            // TODO: turn LED to indicate network online
            break;

        case EVENT_LED_NWK_OFFLINE:
            ULOGA("Handle EVENT_LED_NWK_OFFLINE\n");
			led_off(LED_ID_NWK);
            // TODO: turn LED to indicate network offline
            break;

        case EVENT_LED_CALL_ON:
            ULOGA("Handle EVENT_LED_CALL_ON\n");
			led_on(LED_ID_CALL);
            // TODO: turn LED for call active
            break;

        case EVENT_LED_NWK_CALL_OFF:
            ULOGA("Handle EVENT_LED_NWK_CALL_OFF\n");
			led_off(LED_ID_CALL);
            // TODO: turn off LED for network call
            break;

        case EVENT_DATA_RESET:
            ULOGA("Handle EVENT_LED_CALL_BLINK\n");
			led_blink_count(LED_ID_CALL, 500, 50, 3);
            // TODO: blink LED for call state
            break;

		case EVENT_DATA_CALL:
            ULOGA("Handle EVENT_DATA_CALL\n");
			led_ctx.is_call = 1;
			led_on(LED_ID_CALL);
            // TODO: blink LED for call state
			break;

		case EVENT_DATA_ENDCALL:
            ULOGA("Handle EVENT_DATA_ENDCALL\n");
			led_ctx.is_call = 0;
			led_off(LED_ID_CALL);
            // TODO: blink LED for call state
			break;

		case EVENT_DATA_START_DONE:
			ULOGA("Handle EVENT_DATA_START_DONE\n");
			led_ctx.start = 1;
			led_all_off();
			break;

        default:
            ULOGA("Unknown LED event: %d\n", (uint32_t)event->type);
            break;
    }
}


#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/
