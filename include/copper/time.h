#ifndef CPR_TIME_H
#define CPR_TIME_H

#include "platform.h"
#include <stdint.h>

// --- Conversions to milliseconds ---
#define cpr_ns_to_ms(x) ((x) / 1000000ULL)
#define cpr_us_to_ms(x) ((x) / 1000ULL)
#define cpr_s_to_ms(x) ((x) * 1000ULL)
#define cpr_min_to_ms(x) ((x) * 60000ULL)
#define cpr_hr_to_ms(x) ((x) * 3600000ULL)

// --- Conversions from milliseconds ---
#define cpr_ms_to_ns(x) ((x) * 1000000ULL)
#define cpr_ms_to_us(x) ((x) * 1000ULL)
#define cpr_ms_to_s(x) ((x) / 1000ULL)
#define cpr_ms_to_min(x) ((x) / 60000ULL)
#define cpr_ms_to_hr(x) ((x) / 3600000ULL)

// --- Other conversions ---
#define cpr_ns_to_us(x) ((x) / 1000ULL)
#define cpr_us_to_ns(x) ((x) * 1000ULL)
#define cpr_ns_to_s(x) ((x) / 1000000000ULL)
#define cpr_s_to_ns(x) ((x) * 1000000000ULL)
#define cpr_ns_to_min(x) ((x) / 60000000000ULL)
#define cpr_min_to_ns(x) ((x) * 60000000000ULL)
#define cpr_ns_to_hr(x) ((x) / 3600000000000ULL)
#define cpr_hr_to_ns(x) ((x) * 3600000000000ULL)
#define cpr_us_to_s(x) ((x) / 1000000ULL)
#define cpr_s_to_us(x) ((x) * 1000000ULL)
#define cpr_s_to_min(x) ((x) / 60ULL)
#define cpr_min_to_s(x) ((x) * 60ULL)
#define cpr_s_to_hr(x) ((x) / 3600ULL)
#define cpr_hr_to_s(x) ((x) * 3600ULL)
#define cpr_min_to_hr(x) ((x) / 60ULL)
#define cpr_hr_to_min(x) ((x) * 60ULL)

// --- Literal duration macros (produce values in milliseconds) ---
#define cpr_ms(x) (x)
#define cpr_us(x) cpr_us_to_ms(x)
#define cpr_s(x) cpr_s_to_ms(x)
#define cpr_min(x) cpr_min_to_ms(x)
#define cpr_hr(x) cpr_hr_to_ms(x)

// --- Date Time ---

typedef struct {
	int16_t year; ///< -1 on failure.
	uint8_t month; ///< 1-12
	uint8_t day; ///< 1-31
	uint8_t hour; ///< 0-23
	uint8_t minute; ///< 0-59
	uint8_t second; ///< 0-59
	uint16_t ms; ///< 0-999
	uint16_t yday; ///< 0-365; day of the year
} CprDateTime;

#ifdef __cplusplus
extern "C" {
#endif

/// Returns UTC milliseconds since UNIX epoch.
CPR_API uint64_t cpr_time_now(void);

/// Returns milliseconds on a monotonic clock (epoch unspecified).
/// Never goes backwards, used for measuring elapsed time.
CPR_API uint64_t cpr_time_monotonic(void);

/// If CprDateTime.year is -1, it indicates a failure.
CPR_API CprDateTime cpr_time_local(uint64_t ms);

/// If CprDateTime.year is -1, it indicates a failure.
CPR_API CprDateTime cpr_time_utc(uint64_t ms);

#ifdef __cplusplus
}
#endif

#endif // CPR_TIME_H
