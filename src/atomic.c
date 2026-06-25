#include "copper/atomic.h"

#include "copper/sync.h" // IWYU pragma: keep
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(CPR_HAS_MSVC_ATOMICS)
#include <intrin.h>
#endif

typedef struct {
	int32_t value;
#if !defined(CPR_HAS_NATIVE_ATOMICS)
	CprMutex mutex; // fallback
#endif
} CprInternalAtomicI32;

typedef struct {
	uint32_t value;
#if !defined(CPR_HAS_NATIVE_ATOMICS)
	CprMutex mutex; // fallback
#endif
} CprInternalAtomicU32;

typedef struct {
	int64_t value;
#if !defined(CPR_HAS_NATIVE_ATOMICS)
	CprMutex mutex; // fallback
#endif
} CprInternalAtomicI64;

typedef struct {
	uint64_t value;
#if !defined(CPR_HAS_NATIVE_ATOMICS)
	CprMutex mutex; // fallback
#endif
} CprInternalAtomicU64;

typedef struct {
	void *value;
#if !defined(CPR_HAS_NATIVE_ATOMICS)
	CprMutex mutex; // fallback
#endif
} CprInternalAtomicPtr;

// I64/U64 have the largest value type (8 bytes), so they produce the worst-case
// struct size. If they fit, the 32-bit and pointer types fit too.
CPR_STATIC_ASSERT(sizeof(CprInternalAtomicI64) <= CPR_ATOMIC_STORAGE_SIZE,
		  CprInternalAtomicI64_fits_in_storage);

// clang-format off
CPR_INLINE static CprInternalAtomicI32 *cpr__cast_ai32(CprAtomicI32 *a) { return (CprInternalAtomicI32 *)a->_internal.storage; }
CPR_INLINE static CprInternalAtomicU32 *cpr__cast_au32(CprAtomicU32 *a) { return (CprInternalAtomicU32 *)a->_internal.storage; }
CPR_INLINE static CprInternalAtomicI64 *cpr__cast_ai64(CprAtomicI64 *a) { return (CprInternalAtomicI64 *)a->_internal.storage; }
CPR_INLINE static CprInternalAtomicU64 *cpr__cast_au64(CprAtomicU64 *a) { return (CprInternalAtomicU64 *)a->_internal.storage; }
CPR_INLINE static CprInternalAtomicPtr *cpr__cast_aptr(CprAtomicPtr *a) { return (CprInternalAtomicPtr *)a->_internal.storage; }
// clang-format on

// --- CprAtomicI32 ---

CprResult cpr_atomici32_init(CprAtomicI32 *a, int32_t value)
{
	if (a == NULL)
		return CPR_ERR_INVALID;
	CprInternalAtomicI32 *i = cpr__cast_ai32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	__atomic_store_n(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	_InterlockedExchange((volatile long *)&i->value, (long)value);
#else
	CprResult r = cpr_mutex_init(&i->mutex);
	if (cpr_err(r))
		return r;
	i->value = value;
#endif
	return CPR_OK;
}

void cpr_atomici32_destroy(CprAtomicI32 *a)
{
	if (a == NULL)
		return;
#if !defined(CPR_HAS_NATIVE_ATOMICS)
	cpr_mutex_destroy(&cpr__cast_ai32(a)->mutex);
#endif
}

int32_t cpr_atomici32_load(CprAtomicI32 *a)
{
	CprInternalAtomicI32 *i = cpr__cast_ai32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_load_n(&i->value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (int32_t)_InterlockedOr((volatile long *)&i->value, 0L);
#else
	cpr_mutex_lock(&i->mutex);
	int32_t v = i->value;
	cpr_mutex_unlock(&i->mutex);
	return v;
#endif
}

void cpr_atomici32_store(CprAtomicI32 *a, int32_t value)
{
	CprInternalAtomicI32 *i = cpr__cast_ai32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	__atomic_store_n(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	_InterlockedExchange((volatile long *)&i->value, (long)value);
#else
	cpr_mutex_lock(&i->mutex);
	i->value = value;
	cpr_mutex_unlock(&i->mutex);
#endif
}

int32_t cpr_atomici32_exchange(CprAtomicI32 *a, int32_t value)
{
	CprInternalAtomicI32 *i = cpr__cast_ai32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_exchange_n(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (int32_t)_InterlockedExchange((volatile long *)&i->value,
					     (long)value);
#else
	cpr_mutex_lock(&i->mutex);
	int32_t prev = i->value;
	i->value = value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

bool cpr_atomici32_compare_exchange(CprAtomicI32 *a, int32_t *expected,
				    int32_t desired)
{
	CprInternalAtomicI32 *i = cpr__cast_ai32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_compare_exchange_n(&i->value, expected, desired, 0,
					   __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	long prev = _InterlockedCompareExchange((volatile long *)&i->value,
						(long)desired, (long)*expected);
	if (prev == (long)*expected)
		return true;
	*expected = (int32_t)prev;
	return false;
#else
	cpr_mutex_lock(&i->mutex);
	int32_t actual = i->value;
	bool ok = (actual == *expected);
	if (ok)
		i->value = desired;
	else
		*expected = actual;
	cpr_mutex_unlock(&i->mutex);
	return ok;
#endif
}

int32_t cpr_atomici32_fetch_add(CprAtomicI32 *a, int32_t value)
{
	CprInternalAtomicI32 *i = cpr__cast_ai32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_add(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (int32_t)_InterlockedExchangeAdd((volatile long *)&i->value,
						(long)value);
#else
	cpr_mutex_lock(&i->mutex);
	int32_t prev = i->value;
	i->value += value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

int32_t cpr_atomici32_fetch_sub(CprAtomicI32 *a, int32_t value)
{
	CprInternalAtomicI32 *i = cpr__cast_ai32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_sub(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (int32_t)_InterlockedExchangeAdd((volatile long *)&i->value,
						-(long)value);
#else
	cpr_mutex_lock(&i->mutex);
	int32_t prev = i->value;
	i->value -= value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

int32_t cpr_atomici32_fetch_and(CprAtomicI32 *a, int32_t value)
{
	CprInternalAtomicI32 *i = cpr__cast_ai32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_and(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (int32_t)_InterlockedAnd((volatile long *)&i->value,
					(long)value);
#else
	cpr_mutex_lock(&i->mutex);
	int32_t prev = i->value;
	i->value &= value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

int32_t cpr_atomici32_fetch_or(CprAtomicI32 *a, int32_t value)
{
	CprInternalAtomicI32 *i = cpr__cast_ai32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_or(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (int32_t)_InterlockedOr((volatile long *)&i->value, (long)value);
#else
	cpr_mutex_lock(&i->mutex);
	int32_t prev = i->value;
	i->value |= value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

int32_t cpr_atomici32_fetch_xor(CprAtomicI32 *a, int32_t value)
{
	CprInternalAtomicI32 *i = cpr__cast_ai32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_xor(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (int32_t)_InterlockedXor((volatile long *)&i->value,
					(long)value);
#else
	cpr_mutex_lock(&i->mutex);
	int32_t prev = i->value;
	i->value ^= value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

// --- CprAtomicU32 ---

CprResult cpr_atomicu32_init(CprAtomicU32 *a, uint32_t value)
{
	if (a == NULL)
		return CPR_ERR_INVALID;
	CprInternalAtomicU32 *i = cpr__cast_au32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	__atomic_store_n(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	_InterlockedExchange((volatile long *)&i->value, (long)value);
#else
	CprResult r = cpr_mutex_init(&i->mutex);
	if (cpr_err(r))
		return r;
	i->value = value;
#endif
	return CPR_OK;
}

void cpr_atomicu32_destroy(CprAtomicU32 *a)
{
	if (a == NULL)
		return;
#if !defined(CPR_HAS_NATIVE_ATOMICS)
	cpr_mutex_destroy(&cpr__cast_au32(a)->mutex);
#endif
}

uint32_t cpr_atomicu32_load(CprAtomicU32 *a)
{
	CprInternalAtomicU32 *i = cpr__cast_au32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_load_n(&i->value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (uint32_t)_InterlockedOr((volatile long *)&i->value, 0L);
#else
	cpr_mutex_lock(&i->mutex);
	uint32_t v = i->value;
	cpr_mutex_unlock(&i->mutex);
	return v;
#endif
}

void cpr_atomicu32_store(CprAtomicU32 *a, uint32_t value)
{
	CprInternalAtomicU32 *i = cpr__cast_au32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	__atomic_store_n(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	_InterlockedExchange((volatile long *)&i->value, (long)value);
#else
	cpr_mutex_lock(&i->mutex);
	i->value = value;
	cpr_mutex_unlock(&i->mutex);
#endif
}

uint32_t cpr_atomicu32_exchange(CprAtomicU32 *a, uint32_t value)
{
	CprInternalAtomicU32 *i = cpr__cast_au32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_exchange_n(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (uint32_t)_InterlockedExchange((volatile long *)&i->value,
					      (long)value);
#else
	cpr_mutex_lock(&i->mutex);
	uint32_t prev = i->value;
	i->value = value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

bool cpr_atomicu32_compare_exchange(CprAtomicU32 *a, uint32_t *expected,
				    uint32_t desired)
{
	CprInternalAtomicU32 *i = cpr__cast_au32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_compare_exchange_n(&i->value, expected, desired, 0,
					   __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	long prev = _InterlockedCompareExchange((volatile long *)&i->value,
						(long)desired, (long)*expected);
	if ((uint32_t)prev == *expected)
		return true;
	*expected = (uint32_t)prev;
	return false;
#else
	cpr_mutex_lock(&i->mutex);
	uint32_t actual = i->value;
	bool ok = (actual == *expected);
	if (ok)
		i->value = desired;
	else
		*expected = actual;
	cpr_mutex_unlock(&i->mutex);
	return ok;
#endif
}

uint32_t cpr_atomicu32_fetch_add(CprAtomicU32 *a, uint32_t value)
{
	CprInternalAtomicU32 *i = cpr__cast_au32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_add(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (uint32_t)_InterlockedExchangeAdd((volatile long *)&i->value,
						 (long)value);
#else
	cpr_mutex_lock(&i->mutex);
	uint32_t prev = i->value;
	i->value += value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

uint32_t cpr_atomicu32_fetch_sub(CprAtomicU32 *a, uint32_t value)
{
	CprInternalAtomicU32 *i = cpr__cast_au32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_sub(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (uint32_t)_InterlockedExchangeAdd((volatile long *)&i->value,
						 -(long)value);
#else
	cpr_mutex_lock(&i->mutex);
	uint32_t prev = i->value;
	i->value -= value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

uint32_t cpr_atomicu32_fetch_and(CprAtomicU32 *a, uint32_t value)
{
	CprInternalAtomicU32 *i = cpr__cast_au32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_and(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (uint32_t)_InterlockedAnd((volatile long *)&i->value,
					 (long)value);
#else
	cpr_mutex_lock(&i->mutex);
	uint32_t prev = i->value;
	i->value &= value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

uint32_t cpr_atomicu32_fetch_or(CprAtomicU32 *a, uint32_t value)
{
	CprInternalAtomicU32 *i = cpr__cast_au32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_or(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (uint32_t)_InterlockedOr((volatile long *)&i->value,
					(long)value);
#else
	cpr_mutex_lock(&i->mutex);
	uint32_t prev = i->value;
	i->value |= value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

uint32_t cpr_atomicu32_fetch_xor(CprAtomicU32 *a, uint32_t value)
{
	CprInternalAtomicU32 *i = cpr__cast_au32(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_xor(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (uint32_t)_InterlockedXor((volatile long *)&i->value,
					 (long)value);
#else
	cpr_mutex_lock(&i->mutex);
	uint32_t prev = i->value;
	i->value ^= value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

// --- CprAtomicI64 ---

CprResult cpr_atomici64_init(CprAtomicI64 *a, int64_t value)
{
	if (a == NULL)
		return CPR_ERR_INVALID;
	CprInternalAtomicI64 *i = cpr__cast_ai64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	__atomic_store_n(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	_InterlockedExchange64((volatile int64_t *)&i->value, (int64_t)value);
#else
	CprResult r = cpr_mutex_init(&i->mutex);
	if (cpr_err(r))
		return r;
	i->value = value;
#endif
	return CPR_OK;
}

void cpr_atomici64_destroy(CprAtomicI64 *a)
{
	if (a == NULL)
		return;
#if !defined(CPR_HAS_NATIVE_ATOMICS)
	cpr_mutex_destroy(&cpr__cast_ai64(a)->mutex);
#endif
}

int64_t cpr_atomici64_load(CprAtomicI64 *a)
{
	CprInternalAtomicI64 *i = cpr__cast_ai64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_load_n(&i->value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (int64_t)_InterlockedOr64((volatile int64_t *)&i->value, 0LL);
#else
	cpr_mutex_lock(&i->mutex);
	int64_t v = i->value;
	cpr_mutex_unlock(&i->mutex);
	return v;
#endif
}

void cpr_atomici64_store(CprAtomicI64 *a, int64_t value)
{
	CprInternalAtomicI64 *i = cpr__cast_ai64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	__atomic_store_n(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	_InterlockedExchange64((volatile int64_t *)&i->value, (int64_t)value);
#else
	cpr_mutex_lock(&i->mutex);
	i->value = value;
	cpr_mutex_unlock(&i->mutex);
#endif
}

int64_t cpr_atomici64_exchange(CprAtomicI64 *a, int64_t value)
{
	CprInternalAtomicI64 *i = cpr__cast_ai64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_exchange_n(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (int64_t)_InterlockedExchange64((volatile int64_t *)&i->value,
					       (int64_t)value);
#else
	cpr_mutex_lock(&i->mutex);
	int64_t prev = i->value;
	i->value = value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

bool cpr_atomici64_compare_exchange(CprAtomicI64 *a, int64_t *expected,
				    int64_t desired)
{
	CprInternalAtomicI64 *i = cpr__cast_ai64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_compare_exchange_n(&i->value, expected, desired, 0,
					   __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	int64_t prev = _InterlockedCompareExchange64(
		(volatile int64_t *)&i->value, (int64_t)desired,
		(int64_t)*expected);
	if (prev == (int64_t)*expected)
		return true;
	*expected = (int64_t)prev;
	return false;
#else
	cpr_mutex_lock(&i->mutex);
	int64_t actual = i->value;
	bool ok = (actual == *expected);
	if (ok)
		i->value = desired;
	else
		*expected = actual;
	cpr_mutex_unlock(&i->mutex);
	return ok;
#endif
}

int64_t cpr_atomici64_fetch_add(CprAtomicI64 *a, int64_t value)
{
	CprInternalAtomicI64 *i = cpr__cast_ai64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_add(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (int64_t)_InterlockedExchangeAdd64((volatile int64_t *)&i->value,
						  (int64_t)value);
#else
	cpr_mutex_lock(&i->mutex);
	int64_t prev = i->value;
	i->value += value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

int64_t cpr_atomici64_fetch_sub(CprAtomicI64 *a, int64_t value)
{
	CprInternalAtomicI64 *i = cpr__cast_ai64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_sub(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (int64_t)_InterlockedExchangeAdd64((volatile int64_t *)&i->value,
						  -(int64_t)value);
#else
	cpr_mutex_lock(&i->mutex);
	int64_t prev = i->value;
	i->value -= value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

int64_t cpr_atomici64_fetch_and(CprAtomicI64 *a, int64_t value)
{
	CprInternalAtomicI64 *i = cpr__cast_ai64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_and(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (int64_t)_InterlockedAnd64((volatile int64_t *)&i->value,
					  (int64_t)value);
#else
	cpr_mutex_lock(&i->mutex);
	int64_t prev = i->value;
	i->value &= value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

int64_t cpr_atomici64_fetch_or(CprAtomicI64 *a, int64_t value)
{
	CprInternalAtomicI64 *i = cpr__cast_ai64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_or(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (int64_t)_InterlockedOr64((volatile int64_t *)&i->value,
					 (int64_t)value);
#else
	cpr_mutex_lock(&i->mutex);
	int64_t prev = i->value;
	i->value |= value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

int64_t cpr_atomici64_fetch_xor(CprAtomicI64 *a, int64_t value)
{
	CprInternalAtomicI64 *i = cpr__cast_ai64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_xor(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (int64_t)_InterlockedXor64((volatile int64_t *)&i->value,
					  (int64_t)value);
#else
	cpr_mutex_lock(&i->mutex);
	int64_t prev = i->value;
	i->value ^= value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

// --- CprAtomicU64 ---

CprResult cpr_atomicu64_init(CprAtomicU64 *a, uint64_t value)
{
	if (a == NULL)
		return CPR_ERR_INVALID;
	CprInternalAtomicU64 *i = cpr__cast_au64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	__atomic_store_n(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	_InterlockedExchange64((volatile int64_t *)&i->value, (int64_t)value);
#else
	CprResult r = cpr_mutex_init(&i->mutex);
	if (cpr_err(r))
		return r;
	i->value = value;
#endif
	return CPR_OK;
}

void cpr_atomicu64_destroy(CprAtomicU64 *a)
{
	if (a == NULL)
		return;
#if !defined(CPR_HAS_NATIVE_ATOMICS)
	cpr_mutex_destroy(&cpr__cast_au64(a)->mutex);
#endif
}

uint64_t cpr_atomicu64_load(CprAtomicU64 *a)
{
	CprInternalAtomicU64 *i = cpr__cast_au64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_load_n(&i->value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (uint64_t)_InterlockedOr64((volatile int64_t *)&i->value, 0LL);
#else
	cpr_mutex_lock(&i->mutex);
	uint64_t v = i->value;
	cpr_mutex_unlock(&i->mutex);
	return v;
#endif
}

void cpr_atomicu64_store(CprAtomicU64 *a, uint64_t value)
{
	CprInternalAtomicU64 *i = cpr__cast_au64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	__atomic_store_n(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	_InterlockedExchange64((volatile int64_t *)&i->value, (int64_t)value);
#else
	cpr_mutex_lock(&i->mutex);
	i->value = value;
	cpr_mutex_unlock(&i->mutex);
#endif
}

uint64_t cpr_atomicu64_exchange(CprAtomicU64 *a, uint64_t value)
{
	CprInternalAtomicU64 *i = cpr__cast_au64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_exchange_n(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (uint64_t)_InterlockedExchange64((volatile int64_t *)&i->value,
						(int64_t)value);
#else
	cpr_mutex_lock(&i->mutex);
	uint64_t prev = i->value;
	i->value = value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

bool cpr_atomicu64_compare_exchange(CprAtomicU64 *a, uint64_t *expected,
				    uint64_t desired)
{
	CprInternalAtomicU64 *i = cpr__cast_au64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_compare_exchange_n(&i->value, expected, desired, 0,
					   __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	int64_t prev = _InterlockedCompareExchange64(
		(volatile int64_t *)&i->value, (int64_t)desired,
		(int64_t)*expected);
	if ((uint64_t)prev == *expected)
		return true;
	*expected = (uint64_t)prev;
	return false;
#else
	cpr_mutex_lock(&i->mutex);
	uint64_t actual = i->value;
	bool ok = (actual == *expected);
	if (ok)
		i->value = desired;
	else
		*expected = actual;
	cpr_mutex_unlock(&i->mutex);
	return ok;
#endif
}

uint64_t cpr_atomicu64_fetch_add(CprAtomicU64 *a, uint64_t value)
{
	CprInternalAtomicU64 *i = cpr__cast_au64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_add(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (uint64_t)_InterlockedExchangeAdd64(
		(volatile int64_t *)&i->value, (int64_t)value);
#else
	cpr_mutex_lock(&i->mutex);
	uint64_t prev = i->value;
	i->value += value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

uint64_t cpr_atomicu64_fetch_sub(CprAtomicU64 *a, uint64_t value)
{
	CprInternalAtomicU64 *i = cpr__cast_au64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_sub(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (uint64_t)_InterlockedExchangeAdd64(
		(volatile int64_t *)&i->value, -(int64_t)value);
#else
	cpr_mutex_lock(&i->mutex);
	uint64_t prev = i->value;
	i->value -= value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

uint64_t cpr_atomicu64_fetch_and(CprAtomicU64 *a, uint64_t value)
{
	CprInternalAtomicU64 *i = cpr__cast_au64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_and(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (uint64_t)_InterlockedAnd64((volatile int64_t *)&i->value,
					   (int64_t)value);
#else
	cpr_mutex_lock(&i->mutex);
	uint64_t prev = i->value;
	i->value &= value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

uint64_t cpr_atomicu64_fetch_or(CprAtomicU64 *a, uint64_t value)
{
	CprInternalAtomicU64 *i = cpr__cast_au64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_or(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (uint64_t)_InterlockedOr64((volatile int64_t *)&i->value,
					  (int64_t)value);
#else
	cpr_mutex_lock(&i->mutex);
	uint64_t prev = i->value;
	i->value |= value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

uint64_t cpr_atomicu64_fetch_xor(CprAtomicU64 *a, uint64_t value)
{
	CprInternalAtomicU64 *i = cpr__cast_au64(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_fetch_xor(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return (uint64_t)_InterlockedXor64((volatile int64_t *)&i->value,
					   (int64_t)value);
#else
	cpr_mutex_lock(&i->mutex);
	uint64_t prev = i->value;
	i->value ^= value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

// --- CprAtomicPtr ---

CprResult cpr_atomicptr_init(CprAtomicPtr *a, void *value)
{
	if (a == NULL)
		return CPR_ERR_INVALID;
	CprInternalAtomicPtr *i = cpr__cast_aptr(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	__atomic_store_n(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	_InterlockedExchangePointer((void *volatile *)&i->value, value);
#else
	CprResult r = cpr_mutex_init(&i->mutex);
	if (cpr_err(r))
		return r;
	i->value = value;
#endif
	return CPR_OK;
}

void cpr_atomicptr_destroy(CprAtomicPtr *a)
{
	if (a == NULL)
		return;
#if !defined(CPR_HAS_NATIVE_ATOMICS)
	cpr_mutex_destroy(&cpr__cast_aptr(a)->mutex);
#endif
}

void *cpr_atomicptr_load(CprAtomicPtr *a)
{
	CprInternalAtomicPtr *i = cpr__cast_aptr(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_load_n(&i->value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	// No _InterlockedOrPointer exists; OR-with-zero on the underlying integer.
#if defined(CPR_64BIT)
	return (void *)_InterlockedOr64((volatile int64_t *)&i->value, 0LL);
#else
	return (void *)(intptr_t)_InterlockedOr((volatile long *)&i->value, 0L);
#endif
#else
	cpr_mutex_lock(&i->mutex);
	void *v = i->value;
	cpr_mutex_unlock(&i->mutex);
	return v;
#endif
}

void cpr_atomicptr_store(CprAtomicPtr *a, void *value)
{
	CprInternalAtomicPtr *i = cpr__cast_aptr(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	__atomic_store_n(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	_InterlockedExchangePointer((void *volatile *)&i->value, value);
#else
	cpr_mutex_lock(&i->mutex);
	i->value = value;
	cpr_mutex_unlock(&i->mutex);
#endif
}

void *cpr_atomicptr_exchange(CprAtomicPtr *a, void *value)
{
	CprInternalAtomicPtr *i = cpr__cast_aptr(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_exchange_n(&i->value, value, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	return _InterlockedExchangePointer((void *volatile *)&i->value, value);
#else
	cpr_mutex_lock(&i->mutex);
	void *prev = i->value;
	i->value = value;
	cpr_mutex_unlock(&i->mutex);
	return prev;
#endif
}

bool cpr_atomicptr_compare_exchange(CprAtomicPtr *a, void **expected,
				    void *desired)
{
	CprInternalAtomicPtr *i = cpr__cast_aptr(a);
#if defined(CPR_HAS_GCC_ATOMICS)
	return __atomic_compare_exchange_n(&i->value, expected, desired, 0,
					   __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#elif defined(CPR_HAS_MSVC_ATOMICS)
	void *prev = _InterlockedCompareExchangePointer(
		(void *volatile *)&i->value, desired, *expected);
	if (prev == *expected)
		return true;
	*expected = prev;
	return false;
#else
	cpr_mutex_lock(&i->mutex);
	void *actual = i->value;
	bool ok = (actual == *expected);
	if (ok)
		i->value = desired;
	else
		*expected = actual;
	cpr_mutex_unlock(&i->mutex);
	return ok;
#endif
}
