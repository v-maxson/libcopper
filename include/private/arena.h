#ifndef __LIBCOPPER_ARENA_H__
#define __LIBCOPPER_ARENA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#ifndef COPPER_NO_SHORT_NAMES
	#define arena copper_arena
	#define arctor copper_arctor
	#define ardtor copper_ardtor
	#define armalloc copper_armalloc
#endif

/// A simplistic memory arena.
///
/// This structure is used to allocate memory in a contiguous block. This can
/// be useful for reducing fragmentation and improving cache locality.
///
/// The `memory` field points to the start of the arena's memory block. The
/// `size` field is the size of the memory block in bytes. The `offset` field
/// is the current offset into the memory block. When memory is allocated from
/// the arena, the `offset` field is incremented by the size of the allocation.
/// When the `offset` field reaches the `size` field, the arena is considered
/// full and no more memory can be allocated from it.
///
/// See the following example:
/// @code
/// copper_arena *ar = copper_arctor(1024);
///
/// void *ptr1 = copper_armalloc(ar, 128);
/// void *ptr2 = copper_armalloc(ar, 256);
/// void *ptr3 = copper_armalloc(ar, 512); // ar->offset should now be 896.
/// @endcode
///
/// @attention This structure is not thread-safe. If you need to use an arena
/// in a multithreaded environment, you should use a mutex to protect it.
typedef struct copper_arena {
	/// The start of the memory block.
	void *memory;

	/// The size of the memory block in bytes.
	size_t size;

	/// The current offset into the memory block.
	size_t offset;
} copper_arena;

/// Allocates a new arena.
///
/// @attention The caller is responsible for freeing the memory allocated by
/// this function via `copper_ardtor`.
///
/// @param size The size of the arena in bytes.
/// @return A pointer to the allocated arena. NULL if an error occurred.
copper_arena *copper_arctor(size_t size);

/// Deallocates an arena.
///
/// @param ar The arena to deallocate.
void copper_ardtor(copper_arena *ar);

/// Allocates memory from an arena.
///
/// @param ar The arena to allocate memory from.
/// @param size The size of the memory to allocate.
/// @return A pointer to the allocated memory. NULL if an error occurred.
void *copper_armalloc(copper_arena *ar, size_t size);

#ifdef __cplusplus
}
#endif

#endif