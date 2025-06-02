#include "copper/error.h"
#include <stdarg.h>
#include <stdio.h>

__thread char _error_msg[CPR_ERROR_MSG_SIZE] = { 0 };

void cpr_set_error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	// format the error message
	snprintf(_error_msg, CPR_ERROR_MSG_SIZE, fmt, args);

	// Ensure null termination
	_error_msg[CPR_ERROR_MSG_SIZE - 1] = '\0';

	va_end(args);
}

const char *cpr_get_error(void)
{
	return _error_msg;
}
