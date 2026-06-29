#ifndef CPR_ARENA_H
#define CPR_ARENA_H

#include "defs.h"
#include <stdbool.h>
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
/// Returns false on failure; call cpr_get_error() to check the error code.
CPR_API bool cpr_arena_init(CprArena *arena, CprArenaAllocator allocator,
			    size_t capacity);

/// Initializes the `arena` with an externally-owned buffer.
/// The arena never calls free; the caller is responsible for the lifetime of `buf`.
/// Returns false on failure; call cpr_get_error() to check the error code.
CPR_API bool cpr_arena_init_buf(CprArena *arena, void *buf, size_t size);

// --- Allocation ---

/// Allocates `size` bytes in `arena` at the specified `alignment`.
/// `alignment` must be a non-zero power of two; if not, returns NULL with `CPR_ERR_ALIGN`.
/// Returns NULL with `CPR_ERR_EXHAUSTED` if the arena has run out of space.
/// On failure, call cpr_get_error() to inspect the error code.
CPR_API void *cpr_arena_alloc_aligned(CprArena *arena, size_t size,
				      size_t alignment);

#define cpr_arena_new(arena, T) \
	((T *)cpr_arena_alloc_aligned((arena), sizeof(T), cpr_alignof(T)))

/// Allocates `size` bytes in `arena` aligned at `CPR_DEFAULT_ALIGNMENT`.
/// Returns NULL with `CPR_ERR_EXHAUSTED` if the arena has run out of space.
/// On failure, call cpr_get_error() to inspect the error code.
CPR_API void *cpr_arena_alloc(CprArena *arena, size_t size);

/// Resets the bump pointer to zero without freeing the backing buffer.
/// All previously allocated pointers become invalid.
CPR_API void cpr_arena_reset(CprArena *arena);

/// Rewinds the bump pointer to the previous allocation.
/// Only valid for one level of rewind, calling twice won't do anything.
CPR_API void cpr_arena_rewind(CprArena *arena);

/// Frees the backing buffer and zeroes out the `arena`.
/// This does nothing if `arena->allocator->free` is NULL.
CPR_API void cpr_arena_free(CprArena *arena);

#ifdef __cplusplus
}
#endif

#endif // CPR_ARENA_H
