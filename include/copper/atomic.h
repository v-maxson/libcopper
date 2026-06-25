#ifndef CPR_ATOMIC_H
#define CPR_ATOMIC_H

#include "defs.h"
#include "result.h"
#include <stdbool.h>
#include <stdint.h>

#if defined(CPR_COMPILER_GCC) || defined(CPR_COMPILER_CLANG)
#define CPR_HAS_GCC_ATOMICS 1
#define CPR_HAS_NATIVE_ATOMICS 1
#elif defined(CPR_COMPILER_MSVC)
#define CPR_HAS_MSVC_ATOMICS 1
#define CPR_HAS_NATIVE_ATOMICS 1
#endif
// When CPR_HAS_NATIVE_ATOMICS is not defined, falls back to CprMutex

/// The largest possible size of an Atomic type.
/// The native value will never exceed 8 bytes, but in the case of
/// a platform lacking support for atomics, the fallback uses a CprMutex.
/// Worst case: value (8) + alignment padding (8, when CprMaxAlign is 16-byte
/// aligned) + CprMutex (64) = 80 bytes.
#define CPR_ATOMIC_STORAGE_SIZE 80

// Shared internal layout.
#define CPR_ATOMIC_STRUCT(n)                                      \
	typedef struct {                                          \
		union {                                           \
			uint8_t storage[CPR_ATOMIC_STORAGE_SIZE]; \
			CprMaxAlign _align;                       \
		} _internal;                                      \
	} n

/// An atomic int32_t.
/// On platforms that lack native support for atomic operations (`CPR_HAS_NATIVE_ATOMICS`),
/// you *must* call `cpr_atomici32_init` before any operations
/// and `cpr_atomici32_destroy` when you are finished.
CPR_ATOMIC_STRUCT(CprAtomicI32);

/// An atomic uint32_t.
/// On platforms that lack native support for atomic operations (`CPR_HAS_NATIVE_ATOMICS`),
/// you *must* call `cpr_atomicu32_init` before any operations
/// and `cpr_atomicu32_destroy` when you are finished.
CPR_ATOMIC_STRUCT(CprAtomicU32);

/// An atomic int64_t.
/// On platforms that lack native support for atomic operations (`CPR_HAS_NATIVE_ATOMICS`),
/// you *must* call `cpr_atomici64_init` before any operations
/// and `cpr_atomici64_destroy` when you are finished.
CPR_ATOMIC_STRUCT(CprAtomicI64);

/// An atomic uint64_t.
/// On platforms that lack native support for atomic operations (`CPR_HAS_NATIVE_ATOMICS`),
/// you *must* call `cpr_atomicu64_init` before any operations
/// and `cpr_atomicu64_destroy` when you are finished.
CPR_ATOMIC_STRUCT(CprAtomicU64);

/// An atomic pointer.
/// On platforms that lack native support for atomic operations (`CPR_HAS_NATIVE_ATOMICS`),
/// you *must* call `cpr_atomicptr_init` before any operations
/// and `cpr_atomicptr_destroy` when you are finished.
CPR_ATOMIC_STRUCT(CprAtomicPtr);

#undef CPR_ATOMIC_STRUCT

#ifdef __cplusplus
extern "C" {
#endif

// --- CprAtomicI32 ---

CPR_API CprResult cpr_atomici32_init(CprAtomicI32 *a, int32_t value);
CPR_API void cpr_atomici32_destroy(CprAtomicI32 *a);

CPR_API int32_t cpr_atomici32_load(CprAtomicI32 *a);
CPR_API void cpr_atomici32_store(CprAtomicI32 *a, int32_t value);
CPR_API int32_t cpr_atomici32_exchange(CprAtomicI32 *a, int32_t value);
CPR_API bool cpr_atomici32_compare_exchange(CprAtomicI32 *a, int32_t *expected,
					    int32_t desired);
CPR_API int32_t cpr_atomici32_fetch_add(CprAtomicI32 *a, int32_t value);
CPR_API int32_t cpr_atomici32_fetch_sub(CprAtomicI32 *a, int32_t value);
CPR_API int32_t cpr_atomici32_fetch_and(CprAtomicI32 *a, int32_t value);
CPR_API int32_t cpr_atomici32_fetch_or(CprAtomicI32 *a, int32_t value);
CPR_API int32_t cpr_atomici32_fetch_xor(CprAtomicI32 *a, int32_t value);

// --- CprAtomicU32 ---

CPR_API CprResult cpr_atomicu32_init(CprAtomicU32 *a, uint32_t value);
CPR_API void cpr_atomicu32_destroy(CprAtomicU32 *a);

CPR_API uint32_t cpr_atomicu32_load(CprAtomicU32 *a);
CPR_API void cpr_atomicu32_store(CprAtomicU32 *a, uint32_t value);
CPR_API uint32_t cpr_atomicu32_exchange(CprAtomicU32 *a, uint32_t value);
CPR_API bool cpr_atomicu32_compare_exchange(CprAtomicU32 *a, uint32_t *expected,
					    uint32_t desired);
CPR_API uint32_t cpr_atomicu32_fetch_add(CprAtomicU32 *a, uint32_t value);
CPR_API uint32_t cpr_atomicu32_fetch_sub(CprAtomicU32 *a, uint32_t value);
CPR_API uint32_t cpr_atomicu32_fetch_and(CprAtomicU32 *a, uint32_t value);
CPR_API uint32_t cpr_atomicu32_fetch_or(CprAtomicU32 *a, uint32_t value);
CPR_API uint32_t cpr_atomicu32_fetch_xor(CprAtomicU32 *a, uint32_t value);

// --- CprAtomicI64 ---

CPR_API CprResult cpr_atomici64_init(CprAtomicI64 *a, int64_t value);
CPR_API void cpr_atomici64_destroy(CprAtomicI64 *a);

CPR_API int64_t cpr_atomici64_load(CprAtomicI64 *a);
CPR_API void cpr_atomici64_store(CprAtomicI64 *a, int64_t value);
CPR_API int64_t cpr_atomici64_exchange(CprAtomicI64 *a, int64_t value);
CPR_API bool cpr_atomici64_compare_exchange(CprAtomicI64 *a, int64_t *expected,
					    int64_t desired);
CPR_API int64_t cpr_atomici64_fetch_add(CprAtomicI64 *a, int64_t value);
CPR_API int64_t cpr_atomici64_fetch_sub(CprAtomicI64 *a, int64_t value);
CPR_API int64_t cpr_atomici64_fetch_and(CprAtomicI64 *a, int64_t value);
CPR_API int64_t cpr_atomici64_fetch_or(CprAtomicI64 *a, int64_t value);
CPR_API int64_t cpr_atomici64_fetch_xor(CprAtomicI64 *a, int64_t value);

// --- CprAtomicU64 ---

CPR_API CprResult cpr_atomicu64_init(CprAtomicU64 *a, uint64_t value);
CPR_API void cpr_atomicu64_destroy(CprAtomicU64 *a);

CPR_API uint64_t cpr_atomicu64_load(CprAtomicU64 *a);
CPR_API void cpr_atomicu64_store(CprAtomicU64 *a, uint64_t value);
CPR_API uint64_t cpr_atomicu64_exchange(CprAtomicU64 *a, uint64_t value);
CPR_API bool cpr_atomicu64_compare_exchange(CprAtomicU64 *a, uint64_t *expected,
					    uint64_t desired);
CPR_API uint64_t cpr_atomicu64_fetch_add(CprAtomicU64 *a, uint64_t value);
CPR_API uint64_t cpr_atomicu64_fetch_sub(CprAtomicU64 *a, uint64_t value);
CPR_API uint64_t cpr_atomicu64_fetch_and(CprAtomicU64 *a, uint64_t value);
CPR_API uint64_t cpr_atomicu64_fetch_or(CprAtomicU64 *a, uint64_t value);
CPR_API uint64_t cpr_atomicu64_fetch_xor(CprAtomicU64 *a, uint64_t value);

// --- CprAtomicPtr ---
// Pointer atomics omit arithmetic; only init/destroy/load/store/exchange/CAS.

CPR_API CprResult cpr_atomicptr_init(CprAtomicPtr *a, void *value);
CPR_API void cpr_atomicptr_destroy(CprAtomicPtr *a);

CPR_API void *cpr_atomicptr_load(CprAtomicPtr *a);
CPR_API void cpr_atomicptr_store(CprAtomicPtr *a, void *value);
CPR_API void *cpr_atomicptr_exchange(CprAtomicPtr *a, void *value);
/// Returns true on success; writes actual pointer back to `*expected` on failure.
CPR_API bool cpr_atomicptr_compare_exchange(CprAtomicPtr *a, void **expected,
					    void *desired);

#ifdef __cplusplus
}
#endif

#endif // CPR_ATOMIC_H
