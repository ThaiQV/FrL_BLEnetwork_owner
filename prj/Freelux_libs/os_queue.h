#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/*-----Definition-----*/

/*-----Structures-----*/
typedef enum
{
	QUEUE_RET_OK = 0,
	QUEUE_RET_FAIL,
	QUEUE_RET_OVERSIZE,
	QUEUE_RET_EMPTY,
}queue_ret_t; // return value

typedef enum
{
	QUEUE_EMPTY = 0,		// head == tail and fifo empty
	QUEUE_NOT_EMPTY,		// queue has data in fifo
	QUEUE_FULL,					// head == tail and fifo full data
	QUEUE_OVERFLOW,			// head is going through tail
}queue_status_t;

typedef struct
{
	uint16_t				size; 	// size of fifo
	uint16_t				head; 	// head of fifo
	uint16_t				tail;		// tail of fifo
	uint8_t					*fifo; 	// fifo buffer for queue
	queue_status_t	state;	// state of queue
}queue_t;

/*-----Prototypes-----*/
void queue_create(queue_t *queue, uint8_t *fifo, uint16_t size);
uint16_t queue_available_data(queue_t *queue);
queue_ret_t queue_put(queue_t *queue, uint8_t *p_data, uint16_t len);
queue_ret_t queue_get(queue_t *queue, uint8_t *p_data, uint16_t len);
uint8_t queue_peek(queue_t *queue);
uint8_t* queue_get_fifo(queue_t *queue);
#endif
