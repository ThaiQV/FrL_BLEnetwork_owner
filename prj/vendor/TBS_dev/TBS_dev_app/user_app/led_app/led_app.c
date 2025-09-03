/**
 * @file led_app.c
 * @author Nghia Hoang
 * @date 2025
 */

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
}

void user_led_app_init(void)
{

	led_manager_init(&hal);

	led_data.led_call_on = 0;
	led_data.led_nwk_on = 0;

//	led_blink(LED_ID_CALL, 1000);
//
//	led_blink_duty(LED_ID_NWK, 500, 25);
	led_set_blink_complete_callback(LED_ID_CALL, led_callback);

//	led_blink_count(led1, 200, 80, 10);

//	uint8_t pattern = led_add_pattern(led1, 100, 50, 3, 500, true);
//	led_start_pattern(led1, pattern);

}

void user_led_app_task(void)
{

	static uint64_t ledTimeTick = 0;
	if(get_system_time_ms() - ledTimeTick > TIME_LED_TASK_MS){
		ledTimeTick = get_system_time_ms()  ; //10ms
	}
	else{
		return ;
	}

	if(led_is_ready(LED_ID_CALL))
	{
		if(led_data.led_call_blink_3)
		{
			led_blink_count(LED_ID_CALL, 500, 50, 3);
			led_data.led_call_blink_3 = 0;

		}
		else if(led_data.led_call_on)
		{
			led_on(LED_ID_CALL);
		}
		else
		{
			led_off(LED_ID_CALL);
		}
	}



//	if(led_data.led_nwk_on)
	if(IsOnline())
	{
//		led_on(LED_ID_NWK);
		gpio_set_level(GPIO_PA6, 0);
	}
	else
	{
//		led_off(LED_ID_NWK);
		gpio_set_level(GPIO_PA6, 1);
	}

	led_process_all();

}

