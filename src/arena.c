#include "private/arena.h"

#include <stdlib.h>

copper_arena *copper_arctor(const size_t size)
{
	copper_arena *ar = malloc(sizeof(copper_arena));
	if (ar == NULL) {
		return NULL;
	}

	ar->memory = malloc(size);
	if (ar->memory == NULL) {
		free(ar);
		return NULL;
	}

	ar->size = size;
	ar->offset = 0;

	return ar;
}

void copper_ardtor(copper_arena *ar)
{
	free(ar->memory);
	free(ar);
}

void *copper_armalloc(copper_arena *ar, size_t size)
{
	if (ar->offset + size > ar->size) {
		return NULL;
	}

	void *ptr = (char *)ar->memory + ar->offset;
	ar->offset += size;

	return ptr;
}