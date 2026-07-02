#ifndef CPR_RAND_H
#define CPR_RAND_H

#include "defs.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// --- OS Backed RNG ---

#ifdef __cplusplus
extern "C" {
#endif

/// Fills `out` with `size` cryptographically secure random bytes.
/// Backed by the OS: BCryptGenRandom on Windows, getrandom (falling back to /dev/random)
/// in Linux and Android, arc4random_buf(3) on macOS, iOS, FreeBSD, and OpenBSD.
/// Thread-safe; no setup or cleanup required.
/// Returns false on failure; call cpr_get_error() to check the error code.
CPR_API bool cpr_rand_bytes(void *out, size_t size);

/// Returns a uniformly distributed 32-bit value.
/// 0 on failure; call cpr_get_error() to check the error code.
CPR_API uint32_t cpr_rand_u32(void);

/// Returns a uniformly distributed 64-bit value.
/// 0 on failure; call cpr_get_error() to check the error code.
CPR_API uint64_t cpr_rand_u64(void);

/// Returns a uniformly distributed double in [0, 1).
CPR_API double cpr_rand_f64(void);

/// Returns a uniformly distributed value in [min, max] (inclusive).
/// `min` must be <= `max`.
CPR_API uint32_t cpr_randr_u32(uint32_t min, uint32_t max);

/// Returns a uniformly distributed value in [min, max] (inclusive).
/// `min` must be <= `max`.
CPR_API uint64_t cpr_randr_u64(uint64_t min, uint64_t max);

/// Returns a uniformly distributed value in [min, max] (inclusive).
/// `min` must be <= `max`.
CPR_API int32_t cpr_randr_i32(int32_t min, int32_t max);

/// Returns a uniformly distributed value in [min, max] (inclusive).
/// `min` must be <= `max`.
CPR_API int64_t cpr_randr_i64(int64_t min, int64_t max);

#ifdef __cplusplus
}
#endif

#endif // CPR_RAND_H
