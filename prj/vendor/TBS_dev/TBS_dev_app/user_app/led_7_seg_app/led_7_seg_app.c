/**
 * @file led_7_seg_app.c
 * @author Nghia Hoang
 * @date 2025
 */

#include "../../user_lib.h"
#include "led_7_seg_app.h"

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
	    .brightness = 8,
	    .scan_mode = true,
	    .decode_mode = false,
	};

static et6226m_handle_t display_handle;

led7seg_shared_data_t led7seg_data;

void user_led_7_seg_app_init(void)
{
	et6226m_init(&display_handle, &config);
	led7seg_data.value = 0;
	led7seg_data.value_err = 0;
	led7seg_data.enabled = 1;
	led7seg_data.print_err = 0;
	et6226m_display_printf(&display_handle,"E%d", 123);
}

void user_led_7_seg_app_task(void)
{
    static unsigned int buttonTimeTick = 0;
    if(buttonTimeTick <= get_system_time_ms()){
        buttonTimeTick = get_system_time_ms() + TIME_LED7SEG_TASK_MS ; //10ms
    }
    else{
        return ;
    }

    if(!led7seg_data.enabled)
    {
    	et6226m_clear_display(&display_handle);
    	return;
    }

    if(led7seg_data.print_err)
    {
    	et6226m_display_printf(&display_handle, "E%3d", led7seg_data.value_err);
    }
    else
    {
    	et6226m_display_number(&display_handle, led7seg_data.value, false);
    }


}
/*********************************************************************/
