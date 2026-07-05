#ifndef CPR_RINGBUF_H
#define CPR_RINGBUF_H

#include "atomic.h"
#include "defs.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// A lock-free, single-producer/single-consumer byte ring buffer.
/// Exactly one thread may call the write functions, and one may call the read functions
/// (reader/writer may exist on seperate threads).
///
/// Must be initialized with `cpr_ringbuf_init` before use; must be cleaned up
/// with `cpr_ringbuf_destroy` when finished.
typedef struct {
	uint8_t *buf; ///< Owned by caller.
	uint32_t capacity; ///< Must be a power of 2.
	uint32_t mask;
	CprAtomicU32 head;
	CprAtomicU32 tail;
	uint32_t head_cache;
	uint32_t tail_cache;
} CprRingBuffer;

#ifdef __cplusplus
extern "C" {
#endif

// --- Life Cycle ---

/// Supplied `buf` (and by consequence `buf_size`) must be a nonzero power of 2.
CPR_API bool cpr_ringbuf_init(CprRingBuffer *rb, void *buf, uint32_t buf_size);
CPR_API void cpr_ringbuf_destroy(CprRingBuffer *rb);

// --- Producer (single writer thread only) ---

/// Writes as many bytes from `data` into `rb` as currently fit.
/// Returns the number of bytes actually written.
CPR_API size_t cpr_ringbuf_write(CprRingBuffer *rb, const void *data,
				 size_t data_size);

/// Writes all bytes from `data` (up to `data_size`), or none at all.
/// Returns false (without writing) if `size` exceeds the space currently free.
CPR_API bool cpr_ringbuf_write_all(CprRingBuffer *rb, const void *data,
				   size_t data_size);

// --- Consumer (single reader thread only) ---

/// Reads as many of the `size` bytes requesteed into `out` as are currently available.
/// Returns the number of bytes actually read (0 if `rb` is empty).
CPR_API size_t cpr_ringbuf_read(CprRingBuffer *rb, void *out, size_t out_size);

/// Reads exactly `size` bytes requested into `out` as are currently available.
/// Returns false (without reading anything) if `size` exceeds the data currently available.
CPR_API bool cpr_ringbuf_read_all(CprRingBuffer *rb, void *out,
				  size_t out_size);

// --- Introspection ---
// These functions return snapshots which may be changed
// before the data can be acted upon. Useful for metrics, not suitable
// for determined if any read/write will succeed.

CPR_API size_t cpr_ringbuf_size(CprRingBuffer *rb);

/// This is NOT a cleanup function.
/// Returns the number of bytes currently free to write.
CPR_API size_t cpr_ringbuf_free(CprRingBuffer *rb);
CPR_API size_t cpr_ringbuf_capacity(CprRingBuffer *rb);

#ifdef __cplusplus
}
#endif

#endif // CPR_RINGBUF_H
