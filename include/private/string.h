#ifndef __LIBCOPPER_STRING_H__
#define __LIBCOPPER_STRING_H__

#include <stdarg.h>

#ifndef vasprintf
/// Cross-platform implementation of vasprintf.
///
/// @attention The caller is responsible for freeing the memory allocated by this function.
///
/// @param strp A pointer to a string that will be set to the allocated string.
/// @param fmt The format string.
/// @param ap The va_list of arguments.
/// @return The number of characters written to the string. -1 if an error occurred.
int vasprintf(char **strp, const char *fmt, va_list ap);
#endif

#ifndef asprintf
/// Cross-platform implementation of asprintf.
///
/// @attention The caller is responsible for freeing the memory allocated by this function.
///
/// @param strp A pointer to a string that will be set to the allocated string.
/// @param fmt The format string.
/// @param ... The arguments to be formatted.
/// @return The number of characters written to the string. -1 if an error occurred.
int asprintf(char **strp, const char *fmt, ...);
#endif

#endif