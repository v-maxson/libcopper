#ifndef CPR_INT_UNICODE_H
#define CPR_INT_UNICODE_H

#include "copper/defs.h" // IWYU pragma: keep

#if defined(CPR_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>

static CPR_INLINE bool cpr__to_wide(const char *utf8, wchar_t *out,
				    int out_size)
{
	int n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, out, out_size);
	return n > 0;
}

static CPR_INLINE bool cpr__to_utf8(const wchar_t *wide, char *out,
				    int out_size)
{
	int n = WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, out_size, NULL,
				    NULL);
	return n > 0;
}

static CPR_INLINE wchar_t *cpr__to_wide_alloc(const char *utf8)
{
	int n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	if (n <= 0)
		return NULL;
	wchar_t *buf = malloc((size_t)n * sizeof(wchar_t));
	if (!buf)
		return NULL;
	if (!cpr__to_wide(utf8, buf, n)) {
		free(buf);
		return NULL;
	}
	return buf;
}

#endif

#endif // CPR_INT_UNICODE_H
