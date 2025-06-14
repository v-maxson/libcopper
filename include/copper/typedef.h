#ifndef __COPPER_TYPEDEF_H__
#define __COPPER_TYPEDEF_H__

/**
 * This file simply aliases the stdint header types 
 * to (in my opinion) more readable names.
 * 
 * This is inspired by Rust.
 */

#include <stdint.h>

// TODO - Maybe some way to optionally disable these types (if users either don't want them or to avoid name clashing)

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

#endif
