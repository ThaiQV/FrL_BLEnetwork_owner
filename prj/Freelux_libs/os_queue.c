/**
  * @file       queue.c
  * @author     LVD
  * @date       30/09/24
  * @version    V1.0.0
  * @brief     	functions for queue
  */
#include "os_queue.h"

/*------------Definitions------------*/
#define	OS_QUEUE_DEBUG		0
#if OS_QUEUE_DEBUG
#define OS_QUEUE_LOG(...)	LOGA(DRV,__VA_ARGS__)
#else
#define OS_QUEUE_LOG(...)
#endif

/* Variables */
/* Functions */

/**
* @brief: Create a queue
* @param: see below
* @retval: None
*/
void queue_create(queue_t *queue, uint8_t *fifo, uint16_t size)
{
	queue->size = size;
	queue->head = 0;
	queue->tail = 0;
	queue->state = QUEUE_EMPTY;
	queue->fifo = fifo;
}

/**
* @brief: check available data in a queue
* @param: see below
* @retval: None
*/
uint16_t queue_available_data(queue_t *queue)
{
	if(queue->head > queue->tail)
	{
		return (queue->head - queue->tail);
	}
	else if(queue->head == queue->tail)
	{
		if(queue->state == QUEUE_FULL)
		{
			return (queue->size);
		}
		else // QUEUE_EMPTY
		{
			return 0;
		}
	}
	else //(queue->head < queue->tail)
	{
		return (queue->size - (queue->tail - queue->head));
	}
}

/**
* @brief: put data in a queue
* @param: see below
* @retval: queue_ret_t
*/
queue_ret_t queue_put(queue_t *queue, uint8_t *p_data, uint16_t len)
{
	uint16_t head_space; 	// space from head to the end of fifo
	uint16_t fifo_remain;	// remain space of fifo that can put data in without overwrite
	
	if(len > queue->size) // put more than queue size
	{
		return QUEUE_RET_OVERSIZE;
		OS_QUEUE_LOG("QUEUE_RET_OVERSIZE\n");
	}
	
	head_space = queue->size - queue->head;
	fifo_remain = queue->size - queue_available_data(queue);
	if(len > (head_space)) // fifo overflow
	{
		if(head_space > 0)
		{
			memcpy(&queue->fifo[queue->head],p_data,head_space);		// put data in the end of queue
		}
		memcpy(&queue->fifo[0],&p_data[head_space],len - head_space);				// put remain data in the start of queue
		queue->head = len - head_space;
	}
	else
	{
		memcpy(&queue->fifo[queue->head],p_data,len);		// put data in the queue
		queue->head += len;															// increase head
	}
	
	if(fifo_remain == len)
	{
		queue->state = QUEUE_FULL;
		OS_QUEUE_LOG("QUEUE_FULL\n");
	}
	else if(fifo_remain < len)
	{
		queue->state = QUEUE_OVERFLOW;
		OS_QUEUE_LOG("QUEUE_OVERFLOW\n");
	}
	
	return QUEUE_RET_OK;
}

/**
* @brief: get queue state
* @param: see below
* @retval: queue_status_t
*/
queue_status_t queue_state_get(queue_t *queue)
{
	return queue->state;
}

/**
* @brief: get data from queue
* @param: see below
* @retval: queue_ret_t
*/
queue_ret_t queue_get(queue_t *queue, uint8_t *p_data, uint16_t len)
{
	uint16_t tail_space; 	// space from tail to the end of fifo
	
	if((queue->head == queue->tail) && (queue->state != QUEUE_FULL))
	{
		return QUEUE_RET_EMPTY;
	}

	if(queue_available_data(queue) >= len)
	{
		tail_space = queue->size - queue->tail;
		if(len > (tail_space)) // fifo overflow
		{
			if(tail_space > 0)
			{
				memcpy(p_data,&queue->fifo[queue->tail],tail_space);		// get data in the end of queue
			}
			
			memcpy(&p_data[tail_space],&queue->fifo[0],len - tail_space);				// get remain data in the start of queue
			queue->tail = len - tail_space;
		}
		else
		{
			memcpy(p_data,&queue->fifo[queue->tail],len);		// get data in the queue
			queue->tail += len;															// increase tail
		}
		
		if(queue->tail == queue->head)
		{
			queue->state = QUEUE_EMPTY;
			OS_QUEUE_LOG("QUEUE_EMPTY\n");
		}
		else
		{
			queue->state = QUEUE_NOT_EMPTY;
		}
		return QUEUE_RET_OK;
	}
	else
	{
		return QUEUE_RET_OVERSIZE;
	}
}

/**
* @brief: queue peek
* @param: see below
* @retval: data in queue tail
*/
uint8_t queue_peek(queue_t *queue)
{
	return queue->fifo[queue->tail];
}

/**
* @brief: queue get fifo pointer
* @param: see below
* @retval: pointer of fifo
*/
uint8_t* queue_get_fifo(queue_t *queue)
{
	return &queue->fifo[queue->tail];
}

