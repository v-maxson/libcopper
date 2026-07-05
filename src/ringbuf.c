#include "copper/ringbuf.h"

#include "copper/atomic.h"
#include "copper/internal/int_error.h"
#include "copper/result.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// --- Life Cycle ---

CPR_API bool cpr_ringbuf_init(CprRingBuffer *rb, void *buf, uint32_t buf_size)
{
	if (!rb || !buf) {
		cpr__set_error(CPR_ERR_INVALID, "NULL argument");
		return false;
	}

	if (buf_size == 0 || (buf_size & (buf_size - 1)) != 0) {
		cpr__set_error(CPR_ERR_INVALID,
			       "buf_size must be a nonzero power of two");
		return false;
	}

	rb->buf = buf;
	rb->capacity = buf_size;
	rb->mask = rb->capacity - 1;
	rb->head_cache = 0;
	rb->tail_cache = 0;

	if (!cpr_atomicu32_init(&rb->head, 0))
		return false;
	if (!cpr_atomicu32_init(&rb->tail, 0)) {
		cpr_atomicu32_destroy(&rb->head);
		return false;
	}

	return true;
}

CPR_API void cpr_ringbuf_destroy(CprRingBuffer *rb)
{
	if (!rb)
		return;

	cpr_atomicu32_destroy(&rb->head);
	cpr_atomicu32_destroy(&rb->tail);
}

// --- Wraparound-safe helpers ---

static void cpr__ringbuf_copy_in(CprRingBuffer *rb, uint32_t pos,
				 const uint8_t *src, size_t n)
{
	size_t idx = pos & rb->mask;
	size_t until_wrap = rb->capacity - idx;

	if (n <= until_wrap) {
		memcpy(rb->buf + idx, src, n);
	} else {
		memcpy(rb->buf + idx, src, until_wrap);
		memcpy(rb->buf, src + until_wrap, n - until_wrap);
	}
}

static void cpr__ringbuf_copy_out(CprRingBuffer *rb, uint32_t pos, uint8_t *dst,
				  size_t n)
{
	size_t idx = pos & rb->mask;
	size_t until_wrap = rb->capacity - idx;

	if (n <= until_wrap) {
		memcpy(dst, rb->buf + idx, n);
	} else {
		memcpy(dst, rb->buf + idx, until_wrap);
		memcpy(dst + until_wrap, rb->buf, n - until_wrap);
	}
}

// --- Producer ---

CPR_API size_t cpr_ringbuf_write(CprRingBuffer *rb, const void *data,
				 size_t data_size)
{
	if (!rb || (!data && data_size > 0)) {
		cpr__set_error(CPR_ERR_INVALID, "NULL argument");
		return 0;
	}

	uint32_t tail = cpr_atomicu32_load(&rb->tail);
	uint32_t used = rb->head_cache - tail;
	size_t free_space = (size_t)(rb->capacity - used);
	size_t n = data_size < free_space ? data_size : free_space;

	if (n == 0)
		return 0;

	cpr__ringbuf_copy_in(rb, rb->head_cache, data, n);

	rb->head_cache += (uint32_t)n;
	cpr_atomicu32_store(&rb->head, rb->head_cache);
	return n;
}

CPR_API bool cpr_ringbuf_write_all(CprRingBuffer *rb, const void *data,
				   size_t data_size)
{
	if (!rb || (!data && data_size > 0)) {
		cpr__set_error(CPR_ERR_INVALID, "NULL argument");
		return 0;
	}

	uint32_t tail = cpr_atomicu32_load(&rb->tail);
	uint32_t used = rb->head_cache - tail;
	size_t free_space = (size_t)(rb->capacity - used);

	if (data_size > free_space)
		return false;

	cpr__ringbuf_copy_in(rb, rb->head_cache, data, data_size);

	rb->head_cache += (uint32_t)data_size;
	cpr_atomicu32_store(&rb->head, rb->head_cache);
	return true;
}

// --- Consumer ---

CPR_API size_t cpr_ringbuf_read(CprRingBuffer *rb, void *out, size_t out_size)
{
	if (!rb || (!out && out_size > 0)) {
		cpr__set_error(CPR_ERR_INVALID, "NULL argument");
		return 0;
	}

	uint32_t head = cpr_atomicu32_load(&rb->head);
	uint32_t used = head - rb->tail_cache;
	size_t available = (size_t)used;
	size_t n = out_size < available ? out_size : available;

	if (n == 0)
		return 0;

	cpr__ringbuf_copy_out(rb, rb->tail_cache, out, n);

	rb->tail_cache += (uint32_t)n;
	cpr_atomicu32_store(&rb->tail, rb->tail_cache);
	return n;
}

CPR_API bool cpr_ringbuf_read_all(CprRingBuffer *rb, void *out, size_t out_size)
{
	if (!rb || (!out && out_size > 0)) {
		cpr__set_error(CPR_ERR_INVALID, "NULL argument");
		return false;
	}

	uint32_t head = cpr_atomicu32_load(&rb->head);
	uint32_t used = head - rb->tail_cache;
	size_t available = (size_t)used;

	if (out_size > available)
		return false;

	cpr__ringbuf_copy_out(rb, rb->tail_cache, out, out_size);

	rb->tail_cache += (uint32_t)out_size;
	cpr_atomicu32_store(&rb->tail, rb->tail_cache);
	return true;
}

// --- Introspection ---

CPR_API size_t cpr_ringbuf_size(CprRingBuffer *rb)
{
	uint32_t head = cpr_atomicu32_load(&rb->head);
	uint32_t tail = cpr_atomicu32_load(&rb->tail);
	return (size_t)(head - tail);
}

CPR_API size_t cpr_ringbuf_free(CprRingBuffer *rb)
{
	return (size_t)rb->capacity - cpr_ringbuf_size(rb);
}

CPR_API size_t cpr_ringbuf_capacity(CprRingBuffer *rb)
{
	return rb->capacity;
}
