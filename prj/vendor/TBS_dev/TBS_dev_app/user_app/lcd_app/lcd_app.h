/*
 * lcd_app.h
 *
 *      Author: hoang
 */

#ifndef LCD_APP_H_
#define LCD_APP_H_

#include "../../driver/tca9555/tca9555.h"

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
typedef struct {
    struct {
    	char line1[16];
    	char line2[16];
    } display;

    bool enable;            ///< Data validity flag
} lcd_shared_data_t;

uint8_t user_lcd_app_init(void);
void user_lcd_app_task(void);

void user_lcd_app_test(void);

#endif /* VENDOR_USER_USER_APP_LCD_APP_LCD_APP_H_ */
