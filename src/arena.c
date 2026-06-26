#include "copper/arena.h"

#include "copper/defs.h"
#include "copper/internal/int_error.h"
#include "copper/result.h"
#include <stdlib.h>
#include <string.h>

// --- Built-in Allocators ---

static void *cpr__default_alloc(void *user_data, size_t size)
{
	(void)user_data;
	return malloc(size);
}

static void cpr__default_free(void *user_data, void *ptr)
{
	(void)user_data;
	free(ptr);
}

CPR_API CprArenaAllocator cpr_arena_alloc_default(void)
{
	return (CprArenaAllocator){ .alloc = cpr__default_alloc,
				    .free = cpr__default_free,
				    .user_data = NULL };
}

// --- Alignment Helpers ---

static size_t cpr__align_up(size_t offset, size_t alignment)
{
	return (offset + (alignment - 1)) & ~(alignment - 1);
}

static int cpr__is_pow2(size_t v)
{
	return v != 0 && (v & (v - 1)) == 0;
}

// --- Initializers ---

CPR_API bool cpr_arena_init(CprArena *arena, CprArenaAllocator allocator,
			    size_t capacity)
{
	void *buf = NULL;

	if (arena == NULL || capacity == 0 || allocator.alloc == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "invalid arguments");
		return false;
	}

	buf = allocator.alloc(allocator.user_data, capacity);
	if (buf == NULL) {
		cpr__set_error(CPR_ERR_OOM, "out of memory");
		return false;
	}

	arena->buf = buf;
	arena->cap = capacity;
	arena->offset = arena->prev_offset = 0;
	arena->allocator = allocator;
	return true;
}

CPR_API bool cpr_arena_init_buf(CprArena *arena, void *buf, size_t size)
{
	if (arena == NULL || buf == NULL || size == 0) {
		cpr__set_error(CPR_ERR_INVALID, "invalid arguments");
		return false;
	}

	arena->buf = buf;
	arena->cap = size;
	arena->offset = 0;
	arena->prev_offset = 0;
	arena->allocator.alloc = NULL;
	arena->allocator.free = NULL;
	arena->allocator.user_data = NULL;

	return true;
}

// --- Allocation ---

CPR_API void *cpr_arena_alloc_aligned(CprArena *arena, size_t size,
				      size_t alignment, CprResult *out_result)
{
	if (arena == NULL) {
		if (out_result)
			*out_result = CPR_ERR_INVALID;
		return NULL;
	}

	size_t aligned_offset = 0;

	if (!cpr__is_pow2(alignment)) {
		if (out_result)
			*out_result = CPR_ERR_ALIGN;
		return NULL;
	}

	aligned_offset = cpr__align_up(arena->offset, alignment);

	if (size > arena->cap - aligned_offset) {
		if (out_result)
			*out_result = CPR_ERR_EXHAUSTED;
		return NULL;
	}

	arena->prev_offset = arena->offset;
	arena->offset = aligned_offset + size;

	*out_result = CPR_OK;
	return arena->buf + aligned_offset;
}

CPR_API void *cpr_arena_alloc(CprArena *arena, size_t size,
			      CprResult *out_result)
{
	return cpr_arena_alloc_aligned(arena, size, CPR_DEFAULT_ALIGNMENT,
				       out_result);
}

CPR_API void cpr_arena_reset(CprArena *arena)
{
	if (arena == NULL)
		return;

	arena->offset = 0;
	arena->prev_offset = 0;
}

CPR_API void cpr_arena_rewind(CprArena *arena)
{
	if (arena == NULL)
		return;

	arena->offset = arena->prev_offset;
}

CPR_API void cpr_arena_free(CprArena *arena)
{
	if (arena == NULL)
		return;

	if (arena->allocator.free != NULL)
		arena->allocator.free(arena->allocator.user_data, arena->buf);

	memset(arena, 0, sizeof(*arena));
}
