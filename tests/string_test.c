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
	fputs("string_test: ", stdout);
	vprintf(fmt, args);
	va_end(args);
}

int main(void)
{
	test_log("Running asprintf test...\n");
	char *asprintf_test = NULL;
	const int written = asprintf(&asprintf_test, "this is a %s!", "string formatting test");

	assert(asprintf_test != NULL);
	assert(written == 33); // 33 is the length of the string "this is a string formatting test!"
	assert(strcmp(asprintf_test, "this is a string formatting test!") == 0);

	free(asprintf_test);
	test_log("asprintf test passed.\n");
}