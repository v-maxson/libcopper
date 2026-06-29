#include "copper/env.h"

#include "copper/internal/int_error.h"
#include "copper/result.h"
#include <errno.h> // IWYU pragma: keep
#include <stdlib.h>
#include <string.h>

#if defined(CPR_PLATFORM_WINDOWS)
#include "copper/internal/int_unicode.h"
#include <windows.h>
#endif

bool cpr_get_env(const char *name, char *buf, size_t buf_size)
{
	if (!name || !buf || buf_size == 0) {
		cpr__set_error(CPR_ERR_INVALID, "invalid arguments");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	{
		wchar_t *wname = cpr__to_wide_alloc(name);
		if (!wname) {
			cpr__set_error(CPR_ERR_OOM, "out of memory");
			return false;
		}

		DWORD wlen = GetEnvironmentVariableW(wname, NULL, 0);
		if (wlen == 0) {
			free(wname);
			if (GetLastError() == ERROR_ENVVAR_NOT_FOUND)
				cpr__set_error(
					CPR_ERR_NOT_FOUND,
					"environment variable not found");
			else
				cpr__set_error(
					CPR_ERR_IO,
					"failed to read environment variable");

			return false;
		}

		wchar_t *wval = malloc(wlen * sizeof(wchar_t));
		if (!wval) {
			free(wname);
			cpr__set_error(CPR_ERR_OOM, "out of memory");
			return false;
		}

		GetEnvironmentVariableW(wname, wval, wlen);
		free(wname);

		int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wval, -1, NULL,
						   0, NULL, NULL);
		if (utf8_len <= 0 || (size_t)utf8_len > buf_size) {
			free(wval);
			cpr__set_error(CPR_ERR_OVERFLOW, "buffer too small");
			return false;
		}

		WideCharToMultiByte(CP_UTF8, 0, wval, -1, buf, (int)buf_size,
				    NULL, NULL);
		free(wval);
		return true;
	}
#else
	{
		const char *val = getenv(name);
		if (!val) {
			cpr__set_error(CPR_ERR_NOT_FOUND,
				       "environment variable not found");
			return false;
		}

		size_t len = strlen(val);
		if (len >= buf_size) { // need len+1 for \0
			cpr__set_error(CPR_ERR_OVERFLOW, "buffer too small");
			return false;
		}

		memcpy(buf, val, len + 1);
		return true;
	}
#endif
}

bool cpr_set_env(const char *name, const char *value)
{
	if (!name || !value) {
		cpr__set_error(CPR_ERR_INVALID, "invalid arugment(s)");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	{
		wchar_t *wname = cpr__to_wide_alloc(name);
		if (!wname) {
			cpr__set_error(CPR_ERR_OOM, "out of memory");
			return false;
		}

		wchar_t *wval = cpr__to_wide_alloc(value);
		if (!wval) {
			free(wname);
			cpr__set_error(CPR_ERR_OOM, "out of memory");
			return false;
		}

		BOOL ok = SetEnvironmentVariableW(wname, wval);
		free(wname);
		free(wval);

		if (!ok) {
			cpr__set_error(CPR_ERR_IO,
				       "failed to set environment variable");
			return false;
		}
		return true;
	}
#else
	if (setenv(name, value, 1) != 0) {
		cpr__set_error(CPR_ERR_IO,
			       "failed to set environment variable");
		return false;
	}
	return true;
#endif
}

bool cpr_unset_env(const char *name)
{
	if (name == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "invalid arguments");
		return false;
	}
#if defined(CPR_PLATFORM_WINDOWS)
	{
		wchar_t *name_w = cpr__to_wide_alloc(name);
		if (!name_w) {
			cpr__set_error(CPR_ERR_OOM, "out of memory");
			return false;
		}
		BOOL ok = SetEnvironmentVariableW(name_w, NULL);
		// Save before free() in case it disturbs GetLastError()
		DWORD last_err = !ok ? GetLastError() : 0;
		free(name_w);
		// ERROR_ENVVAR_NOT_FOUND is not a failure; unset is a no-op
		if (!ok && last_err != ERROR_ENVVAR_NOT_FOUND) {
			cpr__set_error(CPR_ERR_IO,
				       "failed to unset environment variable");
			return false;
		}
		return true;
	}
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	if (unsetenv(name) != 0) {
		cpr__set_error(CPR_ERR_IO,
			       "failed to unset environment variable");
		return false;
	}
	return true;
#endif
}
