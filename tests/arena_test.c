#include "../include/copper.h"
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

void test_log(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fputs("arena_test: ", stdout);
	vprintf(fmt, args);
	va_end(args);
}

int main(void)
{
	test_log("Running arena test...\n");
	copper_arena *ar = copper_arctor(1024);

	const void *ptr1 = copper_armalloc(ar, 128);
	assert(ptr1 != NULL), assert(ar->offset == 128);
	const void *ptr2 = copper_armalloc(ar, 256);
	assert(ptr2 != NULL), assert(ar->offset == 384);
	const void *ptr3 = copper_armalloc(ar, 512);
	assert(ptr3 != NULL), assert(ar->offset == 896);

	copper_ardtor(ar);
	test_log("arena test passed.\n");
}