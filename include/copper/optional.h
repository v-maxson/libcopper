#ifndef __COPPER_OPTIONAL_H__
#define __COPPER_OPTIONAL_H__

#include "typedef.h"

#ifndef COPPER_OPTIONAL
/**
 * Defines an optional type. 
 * 
 * Copper provides many primitive optional types, 
 * but you can also define your own optional types using this macro.
 */
#define COPPER_OPTIONAL(type)           \
	typedef struct cpr_opt_##type { \
		int has_value;          \
		type value;             \
	} cpr_opt_##type
#endif

#ifndef cpr_has_value
/**
 * Checks if an optional type has a value.
 * 
 * This macro simply expands to a check of the `has_value` field of the optional type.
 */
#define cpr_has_value(opt) ((opt).has_value)
#endif

#ifndef cpr_value
/**
 * Gets the value of an optional type.
 * 
 * This macro simply expands to the `value` field of the optional type.
 * 
 * Note: This macro does not check if the optional has a value, so you should ensure that
 * `cpr_has_value(opt)` is true before using this macro.
 */
#define cpr_value(opt) ((opt).value)
#endif

COPPER_OPTIONAL(i8);
COPPER_OPTIONAL(u8);
COPPER_OPTIONAL(i16);
COPPER_OPTIONAL(u16);
COPPER_OPTIONAL(i32);
COPPER_OPTIONAL(u32);
COPPER_OPTIONAL(i64);
COPPER_OPTIONAL(u64);
COPPER_OPTIONAL(f32);
COPPER_OPTIONAL(f64);

/**
 * Defines an optional pointer type.
 * 
 * Copper provides many primitive optional pointer types, 
 * but you can also define your own optional pointer types using this macro.
 */
#ifndef COPPER_OPTIONAL_PTR
#define COPPER_OPTIONAL_PTR(type)          \
	typedef struct cpr_optptr_##type { \
		int has_value;             \
		type *value;               \
	} cpr_optptr_##type
#endif

COPPER_OPTIONAL_PTR(i8);
COPPER_OPTIONAL_PTR(u8);
COPPER_OPTIONAL_PTR(i16);
COPPER_OPTIONAL_PTR(u16);
COPPER_OPTIONAL_PTR(i32);
COPPER_OPTIONAL_PTR(u32);
COPPER_OPTIONAL_PTR(i64);
COPPER_OPTIONAL_PTR(u64);
COPPER_OPTIONAL_PTR(f32);
COPPER_OPTIONAL_PTR(f64);

#endif
