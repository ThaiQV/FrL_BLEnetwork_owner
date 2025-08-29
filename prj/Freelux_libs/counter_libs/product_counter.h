/**
 * @file tca9555.h
 * @brief Driver header file for TCA9555 16-bit I2C I/O expander
 * @author Nghia Hoang
 * @date 2025
 */

#ifndef PRODUCT_COUNTER_H
#define PRODUCT_COUNTER_H

#include "led_7_seg_app.h"
#include "tca9555_app.h"
#include "lcd_app.h"

/* Functions Prototypes */
void product_counter_init(void);
void product_counter_process(void);
void lcd_show(uint8_t row,uint8_t col, uint8_t *string);

void product_counter_set_data(uint8_t call_state, uint8_t endcall_state, uint8_t reset_state);
void product_counter_get_data(uint8_t *pdata);
void show_pass_product(void);
void show_error_product(void);
void button_reset_handler(void);
void button_call_handler(void);
void button_endcall_handler(void);
void button_error_down_handler(void);
void button_error_up_handler(void);
void button_pass_down_handler(void);
void button_pass_up_handler(void);
#endif /* PRODUCT_COUNTER_H */
