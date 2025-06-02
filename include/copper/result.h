#ifndef __COPPER_RESULT_H__
#define __COPPER_RESULT_H__

#include "typedef.h"

#ifndef COPPER_RESULT
/**
 * Defines a result type.
 * 
 * Copper provides many primitive result types,
 * all of which define the `error` field as a `const char *`.
 * 
 * You can also define your own result types using this macro.
 */
#define COPPER_RESULT(type, err)        \
	typedef struct cpr_res_##type { \
		int is_ok;              \
		union {                 \
			type value;     \
			err error;      \
		};                      \
	} cpr_res_##type
#endif

#ifndef cpr_is_ok
/**
 * Checks if a result type is ok.
 * 
 * This macro simply expands to a check of the `is_ok` field of the result type.
 */
#define cpr_is_ok(res) ((res).is_ok)
#endif

#ifndef cpr_value
/**
 * Gets the value of a result type.
 * 
 * This macro simply expands to the `value` field of the result type.
 * 
 * Note: This macro does not check if the result is ok, so you should ensure that
 * `cpr_is_ok(res)` is true before using this macro.
 */
#define cpr_value(res) ((res).value)
#endif

#ifndef cpr_error
/**
 * Gets the error of a result type.
 * 
 * This macro simply expands to the `error` field of the result type.
 * 
 * Note: This macro does not check if the result is ok, so you should ensure that
 * `cpr_is_ok(res)` is false before using this macro.
 */
#define cpr_error(res) ((res).error)
#endif

COPPER_RESULT(i8, const char *);
COPPER_RESULT(u8, const char *);
COPPER_RESULT(i16, const char *);
COPPER_RESULT(u16, const char *);
COPPER_RESULT(i32, const char *);
COPPER_RESULT(u32, const char *);
COPPER_RESULT(i64, const char *);
COPPER_RESULT(u64, const char *);
COPPER_RESULT(f32, const char *);
COPPER_RESULT(f64, const char *);

#endif
