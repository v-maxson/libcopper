#ifndef CPR_ARENA_H
#define CPR_ARENA_H

#include "platform.h"
#include "result.h"
#include <stddef.h>
#include <stdint.h>

// --- Allocator ---

typedef void *(*CprAllocFn)(void *user_data, size_t size);
typedef void (*CprFreeFn)(void *user_data, void *ptr);

typedef struct {
	CprAllocFn alloc;
	CprFreeFn free;
	void *user_data;
} CprArenaAllocator;

// --- Arena ---

/// A memory arena.
/// Initialize this with any of the following init functions:
/// - `cpr_arena_init`
/// - `cpr_arena_init_buf`
typedef struct {
	uint8_t *buf;
	size_t cap;
	size_t offset;
	size_t prev_offset; // single-step rewind marker
	CprArenaAllocator allocator;
} CprArena;

#ifdef __cplusplus
extern "C" {
#endif

// --- Built-in Allocators ---

/// The default allocator. Uses malloc/free.
CPR_API CprArenaAllocator cpr_arena_alloc_default(void);

// --- Initializers ---

/// Initializes the `arena` with the given `allocator`. `capacity` is the number of bytes to request from the allocator.
CPR_API CprResult cpr_arena_init(CprArena *arena, CprArenaAllocator allocator,
				 size_t capacity);

/// Initializes the `arena` with an externally-owned buffer.
/// The arena never calls free; the caller is responsible for the lifetime of `buf`
CPR_API CprResult cpr_arena_init_buf(CprArena *arena, void *buf, size_t size);

// --- Allocation ---

/// Allocates `size` bytes in `arena` at the specified `alignment`.
///
/// If `out_result` != NULL, populates it with either `CPR_OK` or `CPR_ERR_EXHAUSTED` in the event
/// that the arena has run out of space.
///
/// Alignment must be a multiple of 2 and mustn't be 0. If it isn't, the returned pointer will be
/// NULL and `out_result` will be set to `CPR_ERR_INVALID`
CPR_API void *cpr_arena_alloc_aligned(CprArena *arena, size_t size,
				      size_t alignment, CprResult *out_result);

#define cpr_arena_new(arena, T, result) \
	((T *)cpr_arena_alloc_aligned((arena), sizeof(T), cpr_(T), (res)))

/// Allocates `size` bytes in `arena` aligned at `CPR_DEFAULT_ALIGNMENT`.
///
/// If `out_result` != NULL, populates it with either `CPR_OK` or `CPR_ERR_EXHAUSTED` in the event
/// that the arena has run out of space.
///
/// Alignment must be a multiple of 2 and mustn't be 0. If it isn't, the returned pointer will be
/// NULL and `out_result` will be set to `CPR_ERR_INVALID`
CPR_API void *cpr_arena_alloc(CprArena *arena, size_t size,
			      CprResult *out_result);

/// Resets the bump pointer to zero without freeing the backing buffer.
/// All previously allocated pointers become invalid.
CPR_API void cpr_arena_reset(CprArena *arena);

/// Rewinds the bunp pointer to the previous allocation.
/// Only valid for one level of rewind, calling twice won't do anything.
CPR_API void cpr_arena_rewind(CprArena *arena);

/// Frees the backing buffer and zeroes out the `arena`.
/// This does nothing if `arena->allocator->free` is NULL.
CPR_API void cpr_arena_free(CprArena *arena);

#ifdef __cplusplus
}
#endif

#endif // CPR_ARENA_H
