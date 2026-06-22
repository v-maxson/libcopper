#ifndef CPR_TIME_H
#define CPR_TIME_H

//* This module provides easy-to-use macros to convert between time units.
//* Milliseconds is the primary unit; literal macros (cpr_ms, cpr_s, etc.) produce milliseconds.

// --- Conversions to milliseconds ---
#define cpr_ns_to_ms(x)  ((x) / 1000000ULL)
#define cpr_us_to_ms(x)  ((x) / 1000ULL)
#define cpr_s_to_ms(x)   ((x) * 1000ULL)
#define cpr_min_to_ms(x) ((x) * 60000ULL)
#define cpr_hr_to_ms(x)  ((x) * 3600000ULL)

// --- Conversions from milliseconds ---
#define cpr_ms_to_ns(x)  ((x) * 1000000ULL)
#define cpr_ms_to_us(x)  ((x) * 1000ULL)
#define cpr_ms_to_s(x)   ((x) / 1000ULL)
#define cpr_ms_to_min(x) ((x) / 60000ULL)
#define cpr_ms_to_hr(x)  ((x) / 3600000ULL)

// --- Other conversions ---
#define cpr_ns_to_us(x)  ((x) / 1000ULL)
#define cpr_us_to_ns(x)  ((x) * 1000ULL)
#define cpr_ns_to_s(x)   ((x) / 1000000000ULL)
#define cpr_s_to_ns(x)   ((x) * 1000000000ULL)
#define cpr_ns_to_min(x) ((x) / 60000000000ULL)
#define cpr_min_to_ns(x) ((x) * 60000000000ULL)
#define cpr_ns_to_hr(x)  ((x) / 3600000000000ULL)
#define cpr_hr_to_ns(x)  ((x) * 3600000000000ULL)
#define cpr_us_to_s(x)   ((x) / 1000000ULL)
#define cpr_s_to_us(x)   ((x) * 1000000ULL)
#define cpr_s_to_min(x)  ((x) / 60ULL)
#define cpr_min_to_s(x)  ((x) * 60ULL)
#define cpr_s_to_hr(x)   ((x) / 3600ULL)
#define cpr_hr_to_s(x)   ((x) * 3600ULL)
#define cpr_min_to_hr(x) ((x) / 60ULL)
#define cpr_hr_to_min(x) ((x) * 60ULL)

// --- Literal duration macros (produce values in milliseconds) ---
#define cpr_ms(x)  (x)
#define cpr_us(x)  cpr_us_to_ms(x)
#define cpr_s(x)   cpr_s_to_ms(x)
#define cpr_min(x) cpr_min_to_ms(x)
#define cpr_hr(x)  cpr_hr_to_ms(x)

#endif // CPR_TIME_H
