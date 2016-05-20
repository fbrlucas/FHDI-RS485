#ifndef __CIRC_BUFFER__
#define __CIRC_BUFFER__

#ifdef __cplusplus
extern "C" {
#endif

#define CIRC_BUFF_INC(v,mv)   ((((v) + 1) >= (mv)) ? 0 : (v) + 1)

typedef enum circ_buffer_status_s
{
    CIRC_BUFFER_OK = 0,
    CIRC_BUFFER_FULL,
    CIRC_BUFFER_EMPTY,
} circ_buffer_status_t;

typedef struct circ_buffer_s
{
	volatile uint16_t head;
	volatile uint16_t tail;
	uint8_t * buffer;
	uint16_t size;

} circ_buffer_t;

circ_buffer_status_t circ_buffer_init(circ_buffer_t *cb, uint8_t *area, uint16_t size);
circ_buffer_status_t circ_buffer_get(circ_buffer_t *cb, uint8_t *c);
circ_buffer_status_t circ_buffer_put(circ_buffer_t *cb, uint8_t c);
circ_buffer_status_t circ_buffer_flush(circ_buffer_t *cb);

#ifdef __cplusplus
}
#endif

#endif /* __CIRC_BUFFER__ */


