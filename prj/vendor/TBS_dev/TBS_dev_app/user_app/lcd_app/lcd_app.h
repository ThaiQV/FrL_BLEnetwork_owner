/*
 * lcd_app.h
 *
 *      Author: hoang
 */

#ifndef LCD_APP_H_
#define LCD_APP_H_

#ifndef MASTER_CORE
#ifdef COUNTER_DEVICE

#include "../../driver/tca9555/tca9555.h"

#define LCD_TIME_DELAY_OFF		15000

/*
 * lcd
 */

/* Structure for a standard (character)LCD display.   */
typedef struct {
    union {
        struct {
            PortPin_Map *RS;   // Selects command(0) or data(1) register.
            PortPin_Map *RW;   // Selects read(1) or write(0) operation.
            PortPin_Map *E;    // Enable LCD operation.

            PortPin_Map *D0;
            PortPin_Map *D1;
            PortPin_Map *D2;
            PortPin_Map *D3;
            PortPin_Map *D4;
            PortPin_Map *D5;
            PortPin_Map *D6;
            PortPin_Map *D7;
        };
        PortPin_Map *all_pins[11]; // 3 control + 8 data = 11 pins
    };
} LCD_Pin_TypeDef;

/**
 * @brief LCD sub-app shared data
 */

void lcd_on(void);
void lcd_off(void);

#define get_lcd_event() {\
	EVENT_LCD_PRINT_MESS,\
    EVENT_DATA_CALL,\
    EVENT_DATA_ENDCALL,\
    EVENT_DATA_PASS_PRODUCT_UP,\
    EVENT_DATA_ERR_PRODUCT_CHANGE,\
    EVENT_DATA_PASS_PRODUCT_DOWN,\
    EVENT_LCD_PRINT_COUNT_PRODUCT,\
    EVENT_LCD_PRINT_MAC,\
    EVENT_DATA_SWITCH_MODE,\
    EVENT_DATA_RESET,\
}

#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/
#endif /* LCD_APP_H_ */
