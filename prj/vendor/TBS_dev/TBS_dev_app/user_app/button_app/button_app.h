/*
 * button_app.h
 *
 *      Author: hoang
 */

#ifndef BUTTON_APP_H_
#define BUTTON_APP_H_

#ifndef MASTER_CORE
#ifdef COUNTER_DEVICE

#define get_button_event() {\
    EVENT_DATA_START_DONE,\
    EVENT_DISABLE_COUNTER_BT,\
    EVENT_ENABLE_COUNTER_BT,\
}

#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/
#endif /* BUTTON_APP_H_ */
