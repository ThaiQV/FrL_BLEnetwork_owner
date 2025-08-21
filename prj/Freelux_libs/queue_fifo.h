/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: queue_fifo.h
 *Created on		: Jul 9, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#ifndef FREELUX_LIBS_QUEUE_FIFO_H_
#define FREELUX_LIBS_QUEUE_FIFO_H_

#include "types.h"
//#include "stdbool.h"
//#include "math.h"

typedef struct {
	u8 length;
	u8 data_arr[32]; //modify if we need
}__attribute__((packed)) fl_pack_t;

typedef struct {
	fl_pack_t* data;
	u16 mask;
	u16 head_index;
	u16 tail_index;
	u16 count;
}__attribute__((packed)) fl_data_container_t;

void FL_QUEUE_CLEAR(fl_data_container_t *pCont, u16 _size);
/**
 * Returns whether a queue container is empty.
 * @param pcontainer The pcontainer for which it should be returned whether it is empty.
 * @return 1 if empty; 0 otherwise.
 */
u16 FL_QUEUE_ISEMPTY(fl_data_container_t *pcontainer);
/**
 * Returns whether a queue container is full.
 * @param pcontainer The buffer for which it should be returned whether it is full.
 * @return 1 if full; 0 otherwise.
 */
u16 FL_QUEUE_ISFULL(fl_data_container_t *pcontainer);
/**
 * Adds a parameter to a queue container.
 * @param pCont The pack in which the data should be placed.
 * @param data The pack to place.
 */
s16 FL_QUEUE_ADD(fl_data_container_t *pCont, fl_pack_t *pdata);
/**
 * Returns the pack in a queue container.(FIFO)
 * @param buffer The buffer from which the data should be returned.
 * @param data A pointer to the location at which the data should be placed.
 * @return 1 if data was returned; 0 otherwise.
 */
u16 FL_QUEUE_GET(fl_data_container_t *pCont, fl_pack_t *pdata);
/**
 * Returns the pack in a queue container.(FIFO)
 * @param buffer The buffer from which the data should be returned.
 * @param data A pointer to the location at which the data should be placed.
 * @return 1 if data was returned; 0 otherwise.
 */
u16 FL_QUEUE_GET_LOOP(fl_data_container_t *pCont, fl_pack_t *pdata) ;
/**
 * Returns the pack in a queue container AND CLEAR IT.(FIFO)
 * @param buffer The buffer from which the data should be returned.
 * @param data A pointer to the location at which the data should be placed.
 * @return 1 if data was returned; 0 otherwise.
 */
u16 FL_QUEUE_GETnCLEAR(fl_data_container_t *pCont, fl_pack_t *pdata);
/**
 * Returns the pack in a queue container.(FIFO)
 * @param pack in queue that find.
 * @param
 * @return index of quêu if have; -1 otherwise.
 */
s16 FL_QUEUE_FIND(fl_data_container_t *pCont, fl_pack_t *pdata ,u8 _len);
#endif /* FREELUX_LIBS_QUEUE_FIFO_H_ */
