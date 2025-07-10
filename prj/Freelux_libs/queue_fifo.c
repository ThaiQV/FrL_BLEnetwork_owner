/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: queue_fifo.c
 *Created on		: Jul 9, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/
#include "queue_fifo.h"
#include "string.h"

/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/


/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
#define SIZE_CONTAINER(x) ((fl_data_container_t)x->mask+1)
/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/


/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/

void FL_QUEUE_CLEAR(fl_data_container_t *pCont, u8 _size) {
	memset(pCont->data,0,sizeof(pCont->data)/sizeof(fl_pack_t));
	pCont->mask = _size - 1;
	pCont->tail_index = 0;
	pCont->head_index = 0;
	pCont->count = 0;
}
/**
 * Returns whether a queue container is empty.
 * @param pcontainer The pcontainer for which it should be returned whether it is empty.
 * @return 1 if empty; 0 otherwise.
 */
u8 FL_QUEUE_ISEMPTY(fl_data_container_t *pcontainer) {
	return (pcontainer->head_index == pcontainer->tail_index);
	//return (pcontainer->count == 0)
}
/**
 * Returns whether a queue container is full.
 * @param pcontainer The buffer for which it should be returned whether it is full.
 * @return 1 if full; 0 otherwise.
 */
u8 FL_QUEUE_ISFULL(fl_data_container_t *pcontainer) {
	//return ((pcontainer->head_index - pcontainer->tail_index) & pcontainer->mask) == pcontainer->mask;
	return (pcontainer->count == pcontainer->mask);
}

/**
 * Adds a parameter to a queue container.
 * @param pCont The pack in which the data should be placed.
 * @param data The pack to place.
 */
s8 FL_QUEUE_ADD(fl_data_container_t *pCont, fl_pack_t *pdata) {
	if (!FL_QUEUE_ISFULL(pCont))
	{
		pCont->data[pCont->tail_index] = *pdata;
		//memcpy(pCont->data[pCont->tail_index],pdata,sizeof(fl_pack_t)/sizeof(u8));
		pCont->tail_index = (pCont->tail_index + 1) & (pCont->mask);
		pCont->count++;
		return pCont->tail_index;
	}
	return -1;
}
/**
 * Returns the pack in a queue container.(FIFO)
 * @param buffer The buffer from which the data should be returned.
 * @param data A pointer to the location at which the data should be placed.
 * @return 1 if data was returned; 0 otherwise.
 */
u8 FL_QUEUE_GET(fl_data_container_t *pCont, fl_pack_t *pdata) {
	if (FL_QUEUE_ISEMPTY(pCont)) {
		/* No items */
		return 0;
	}
	*pdata = pCont->data[pCont->head_index];
	//memcpy(pdata,pCont->data[pCont->head_index].data_arr,sizeof(fl_pack_t)/sizeof(u8));
	pCont->head_index = ((pCont->head_index + 1) & pCont->mask);
	pCont->count--;
	return 1;
}
/**
 * Returns the pack in a queue container AND CLEAR IT.(FIFO)
 * @param buffer The buffer from which the data should be returned.
 * @param data A pointer to the location at which the data should be placed.
 * @return 1 if data was returned; 0 otherwise.
 */
u8 FL_QUEUE_GETnCLEAR(fl_data_container_t *pCont, fl_pack_t *pdata) {
	if (FL_QUEUE_ISEMPTY(pCont)) {
		/* No items */
		return 0;
	}
	*pdata = pCont->data[pCont->head_index];
	memset(pCont->data[pCont->head_index].data_arr,0,sizeof(fl_pack_t));
	pCont->data[pCont->head_index].length =0;
	pCont->head_index = ((pCont->head_index + 1) & pCont->mask);
	pCont->count--;
	return 1;
}
/**
 * Returns the pack in a queue container.(FIFO)
 * @param pack in queue that find.
 * @param
 * @return index of quêu if have; -1 otherwise.
 */
s8 FL_QUEUE_FIND(fl_data_container_t *pCont, fl_pack_t *pdata ,u8 _len){
//	if (FL_QUEUE_ISEMPTY(pCont)) {
//		/* No items */
//		return -1;
//	}
	for(int indx = 0; indx < pCont->mask + 1; indx++) {
		if(memcmp(pCont->data[indx].data_arr,pdata->data_arr,_len) == 0){
			return indx;
		}
	}
	return -1;
}
