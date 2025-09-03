/*
 * button_app.h
 *
 *      Author: hoang
 */

#ifndef BUTTON_APP_H_
#define BUTTON_APP_H_

#ifndef MASTER_CORE
#ifdef COUNTER_DEVICE

void user_button_app_init(void);
void user_button_app_task(void);

#endif /* COUNTER_DEVICE*/
#endif /* MASTER_CORE*/
#endif /* BUTTON_APP_H_ */
