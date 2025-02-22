#include "private/string.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#ifndef _vscprintf
// NOLINTNEXTLINE
int _vscprintf_so(const char *format, va_list pargs)
{
	va_list argcopy;
	va_copy(argcopy, pargs);
	const int retval = vsnprintf(NULL, 0, format, argcopy);
	va_end(argcopy);
	return retval;
}
#endif


#ifndef vasprintf
int vasprintf(char **strp, const char *fmt, va_list ap)
{
	const int len = _vscprintf_so(fmt, ap);
	if (len == -1) return -1;
	char *str = malloc((size_t)len + 1);
	if (!str) return -1;
	const int r = vsnprintf(str, len + 1, fmt, ap);
	if (r == -1)
	{
		free(str);
		return -1;
	}
	*strp = str;
	return r;
}
#endif

#ifndef asprintf
int asprintf(char **strp, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	const int r = vasprintf(strp, fmt, ap);
	va_end(ap);
	return r;
}
#endif