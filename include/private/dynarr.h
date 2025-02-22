#ifndef __LIBCOPPER_DYNARR_H__
#define __LIBCOPPER_DYNARR_H__

#include <stddef.h>
#include <stdint.h>

#ifndef COPPER_NO_SHORT_NAMES
	#define array copper_array
	#define arrayctor copper_arrayctor
	#define arraydtor copper_arraydtor
	#define arrayresize copper_arrayresize
#endif

/// A dynamic array.
///
/// This structure is used to store a contiguous block of memory that can be
/// resized as needed. The `data` field points to the start of the memory block.
/// The `elem_size` field is the size of each element in the memory block. The
/// `capacity` field is the total number of elements that can be stored in the
/// memory block. The `size` field is the number of elements currently stored in
/// the memory block.
typedef struct copper_array {
	/// The data stored in the dynamic array.
	void *data;

	/// The size of each element in the dynamic array.
	size_t elem_size;

	/// The capacity of the dynamic array. This is the number of elements that can
	/// be stored in the dynamic array before it needs to be resized.
	size_t capacity;

	/// The number of elements in the dynamic array.
	size_t size;
} copper_array;

/// Allocates a new dynamic array.
///
/// @attention The caller is responsible for freeing the memory allocated by this
/// function via `copper_arraydtor`.
///
/// @param elem_size The size of each element in the dynamic array.
/// @param capacity The initial capacity of the dynamic array. This is the number
/// of elements that can be stored in the dynamic array before it needs to be
/// resized.
copper_array *copper_arrayctor(size_t elem_size, size_t capacity);

/// Deallocates a dynamic array.
///
/// @param arr The dynamic array to deallocate.
void copper_arraydtor(copper_array *arr);

/// Resizes a dynamic array.
///
/// This function resizes a dynamic array to a new capacity. If the new capacity
/// is less than the current capacity, the array will be shrunk. If the new
/// capacity is greater than the current capacity, the array will be expanded.
/// The new capacity must be greater than zero.
///
/// @attention If the array is shrunk, any elements that are removed will be lost.
/// If the array is expanded, the new elements will be zero-initialized.
///
/// @param arr The dynamic array to resize.
/// @param new_capacity The new capacity of the dynamic array.
void copper_arrayresize(copper_array *arr, size_t new_capacity);

size_t copper_arraylen(copper_array *arr);
size_t copper_arraycap(copper_array *arr);
void *copper_arrayat(copper_array *arr, size_t index);

#endif