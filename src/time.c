#include "copper/time.h"

#include <time.h>

#if defined(CPR_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif

#if defined(CPR_PLATFORM_WINDOWS)
#define CPR__FILETIME_UNIX_OFFSET 116444736000000000ULL
#define CPR__FILETIME_PER_MS (cpr_ms_to_ns(1ULL) / 100ULL)
#endif

CPR_API uint64_t cpr_time_now(void)
{
#if defined(CPR_PLATFORM_WINDOWS)
	FILETIME ft;
	uint64_t t;
	GetSystemTimeAsFileTime(&ft);
	t = ((uint64_t)ft.dwHighDateTime << 32) | (uint64_t)ft.dwLowDateTime;
	return (t - CPR__FILETIME_UNIX_OFFSET) / CPR__FILETIME_PER_MS;
#else
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return cpr_s_to_ms(ts.tv_sec) + cpr_ns_to_ms(ts.tv_nsec);
#endif
}

CPR_API uint64_t cpr_time_monotonic(void)
{
#if defined(CPR_PLATFORM_WINDOWS)
	static LARGE_INTEGER s_freq = { { 0, 0 } };
	LARGE_INTEGER counter;
	if (s_freq.QuadPart == 0)
		QueryPerformanceFrequency(&s_freq);
	QueryPerformanceCounter(&counter);
	return (uint64_t)counter.QuadPart * 1000ULL / (uint64_t)s_freq.QuadPart;
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return cpr_s_to_ms(ts.tv_sec) + cpr_ns_to_ms(ts.tv_nsec);
#endif
}

/// Returned on conversion failure.
static const CprDateTime CPR__DT_ERR = { -1, 0, 0, 0, 0, 0, 0, 0 };

static CprDateTime cpr__fill(const struct tm *tm, uint64_t ms)
{
	return (CprDateTime){ .year = (int16_t)(tm->tm_year + 1900),
			      .month = (uint8_t)(tm->tm_mon + 1),
			      .day = (uint8_t)tm->tm_mday,
			      .hour = (uint8_t)tm->tm_hour,
			      .minute = (uint8_t)tm->tm_min,
			      .second = (uint8_t)tm->tm_sec,
			      .ms = (uint16_t)(ms % cpr_s_to_ms(1)),
			      .yday = (uint16_t)tm->tm_yday };
}

CPR_API CprDateTime cpr_time_local(uint64_t ms)
{
	time_t t = cpr_ms_to_s(ms);
	struct tm tm_buf;

#if defined(CPR_PLATFORM_WINDOWS)
	if (localtime_s(&tm_buf, &t) != 0)
		return CPR__DT_ERR;
#else
	if (!localtime_r(&t, &tm_buf))
		return CPR__DT_ERR;
#endif
	return cpr__fill(&tm_buf, ms);
}

CPR_API CprDateTime cpr_time_utc(uint64_t ms)
{
	time_t t = cpr_ms_to_s(ms);
	struct tm tm_buf;

#if defined(CPR_PLATFORM_WINDOWS)
	if (gmtime_s(&tm_buf, &t) != 0)
		return CPR__DT_ERR;
#else
	if (!gmtime_r(&t, &tm_buf))
		return CPR__DT_ERR;
#endif

	return cpr__fill(&tm_buf, ms);
}
