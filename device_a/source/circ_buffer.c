#include <stdint.h>
#include "circ_buffer.h"

circ_buffer_status_t circ_buffer_init(circ_buffer_t *cb, uint8_t *area, uint16_t size)
{
	cb->buffer = area;
	cb->size = size;
	cb->tail = cb->head = 0;

	return CIRC_BUFFER_OK;
}

circ_buffer_status_t circ_buffer_flush(circ_buffer_t *cb)
{
	uint8_t c;
	
	while(circ_buffer_get(cb,&c) != CIRC_BUFFER_EMPTY)
	{}
	
	return CIRC_BUFFER_OK;
}

circ_buffer_status_t circ_buffer_get(circ_buffer_t *cb, uint8_t *c)
{
	if(cb->tail == cb->head)
		return CIRC_BUFFER_EMPTY;

	*c = cb->buffer[cb->tail];
	cb->tail = CIRC_BUFF_INC(cb->tail,cb->size);

	return CIRC_BUFFER_OK;
}

circ_buffer_status_t circ_buffer_put(circ_buffer_t *cb, uint8_t c)
{
	uint16_t next_head = CIRC_BUFF_INC(cb->head,cb->size);

	if(next_head == cb->tail)
		return CIRC_BUFFER_FULL;

	cb->buffer[cb->head] = c;
	cb->head = next_head;

	return CIRC_BUFFER_OK;
}


