/**
 * @file led_7_seg_app.c
 * @author Nghia Hoang
 * @date 2025
 */

#ifndef MASTER_CORE
#include "tl_common.h"
#include "vendor/TBS_dev/TBS_dev_config.h"

#ifdef COUNTER_DEVICE

#include "../../user_lib.h"
#include "led_7_seg_app.h"

extern get_data_t get_data;

// SubApp context structure
typedef struct {
	bool start;
	bool print_err;
	uint32_t timeout_print_err;
} led7_context_t;

// Static context instance
static led7_context_t led7_ctx = {0};

// Forward declarations
static subapp_result_t led7_app_init(subapp_t* self);
static subapp_result_t led7_app_loop(subapp_t* self);
static subapp_result_t led7_app_deinit(subapp_t* self);
static void led7_app_event_handler(const event_t* event, void* user_data);


subapp_t led7_app = {
        .name = "led7", 
        .context = &led7_ctx, 
        .state = SUBAPP_STATE_IDLE, 
        .init = led7_app_init, 
        .loop = led7_app_loop, 
        .deinit = led7_app_deinit, 
        .on_event = NULL,
        .pause = NULL, 
        .resume = NULL, 
        .is_registered = false, 
        .event_mask = 0 
    };

/*
 *  Static functions
 */
static void cs_pin_set(bool state)
{

}

static void clk_pin_set(bool state)
{
	gpio_set_level(LED_CLK_PIN, state);
}

static void data_pin_set(bool state)
{
	gpio_set_level(LED_DATA_PIN, state);
}

static void pin_init()
{
	gpio_function_en(LED_DATA_PIN);
	gpio_set_output(LED_DATA_PIN, 1); 			//enable output
	gpio_set_input(LED_DATA_PIN, 0);			//disable input
	gpio_set_up_down_res(LED_DATA_PIN, GPIO_PIN_UP_DOWN_FLOAT);
	gpio_set_level(LED_DATA_PIN, 1);

	gpio_function_en(LED_CLK_PIN);
	gpio_set_output(LED_CLK_PIN, 1); 			//enable output
	gpio_set_input(LED_CLK_PIN, 0);			//disable input
	gpio_set_up_down_res(LED_CLK_PIN, GPIO_PIN_UP_DOWN_FLOAT);
	gpio_set_level(LED_CLK_PIN, 1);

}

static void delay_cycle(uint32_t num_cycle)
{
	uint32_t time_cycle= 1000*1000 / FREQUENCY_CLK;
	delay_us(time_cycle * num_cycle);
}
/******************************************************************/

static et6226m_config_t config = {
		.pin_init = pin_init,
	    .delay_cycle = delay_cycle,
	    .cs_pin_set = cs_pin_set,
	    .clk_pin_set = clk_pin_set,
	    .data_pin_set = data_pin_set,
	    .brightness = 6,
	    .scan_mode = true,
	    .decode_mode = false,
	};

static et6226m_handle_t display_handle;


static subapp_result_t led7_app_init(subapp_t* self)
{
	et6226m_init(&display_handle, &config);
	et6226m_display_printf(&display_handle, ",,,,");

	uint32_t app_led7_evt_table[] = get_led7_event();

	for(int i = 0; i < (sizeof(app_led7_evt_table)/sizeof(app_led7_evt_table[0])); i++)
	{
		char name[32];
		snprintf(name, sizeof(name), "app_led7_evt_table[%d]", i);
		event_bus_subscribe(app_led7_evt_table[i], led7_app_event_handler, NULL, name);
	}

	led7_ctx.print_err = false;
	led7_ctx.timeout_print_err = 0;

	return SUBAPP_OK;
}

static subapp_result_t led7_app_loop(subapp_t* self)
{
	static uint64_t led7segTimeTick = 0;
	if(get_system_time_ms() - led7segTimeTick > TIME_LED7SEG_TASK_MS){
		led7segTimeTick = get_system_time_ms()  ; //10ms
	}
	else{
		return SUBAPP_OK;
	}

	if(led7_ctx.start == 0)
	{
		return SUBAPP_OK;
	}

	if(get_system_time_ms() >= led7_ctx.timeout_print_err && led7_ctx.timeout_print_err != 0){
		led7_ctx.print_err = false;
		led7_ctx.timeout_print_err = 0;
		et6226m_display_number(&display_handle, get_data.pass_product(), false);
	}
    
	return SUBAPP_OK;
}

static subapp_result_t led7_app_deinit(subapp_t* self)
{
	et6226m_deinit(&display_handle);
	return SUBAPP_OK;
}

static void led7_app_event_handler(const event_t* event, void* user_data)
{
	switch (event->type) {
        case EVENT_DATA_START_DONE:
            ULOGA("led Handle EVENT_DATA_START_DONE\n");
			et6226m_display_number(&display_handle, get_data.pass_product(), false);
			led7_ctx.start = 1;
            // TODO: turn LED to indicate network online
            break;

        case EVENT_LED7_PRINT_ERR:
            ULOGA("Handle EVENT_LED7_PRINT_ERR\n");
			
            // TODO: turn LED to indicate network offline
            break;

        case EVENT_DATA_ERR_PRODUCT_CHANGE:
            ULOGA("Handle EVENT_DATA_ERR_PRODUCT_CHANGE\n");
			et6226m_display_printf(&display_handle, "E%3d", get_data.err_product());
			led7_ctx.print_err = 1;
			led7_ctx.timeout_print_err = get_system_time_ms() + TIME_OUT_PRINTF_ERR_MS;
            // TODO: turn LED for call active
            break;

        case EVENT_DATA_PASS_PRODUCT_UP:
            ULOGA("Handle EVENT_DATA_PASS_PRODUCT_UP\n");
			et6226m_display_number(&display_handle, get_data.pass_product(), false);
            // TODO: turn off LED for network call
            break;

        case EVENT_DATA_PASS_PRODUCT_DOWN:
            ULOGA("Handle EVENT_DATA_PASS_PRODUCT_DOWN\n");
			et6226m_display_number(&display_handle, get_data.pass_product(), false);
            // TODO: blink LED for call state
            break;

		case EVENT_DATA_RESET:
            ULOGA("Handle EVENT_DATA_CALL\n");
			et6226m_display_number(&display_handle, get_data.pass_product(), false);
            // TODO: blink LED for call state
			break;

        default:
            ULOGA("Unknown LED7 event: %u\n", (uint32_t)event->type);
            break;
    }
}


#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/
