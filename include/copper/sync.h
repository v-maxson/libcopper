#ifndef CPR_SYNC_H
#define CPR_SYNC_H

//* This module contains synchonization primitives for safe multi-threaded programming.

#include "platform.h"
#include "result.h"
#include <stdint.h>

// --- Mutex ---

#define CPR_MUTEX_STORAGE_SIZE \
	64 // largest possible size across all platforms (64 bytes on macOS/iOS)

/// Must be initialized with `cpr_mutex_init` before use!
typedef struct {
	union {
		uint8_t storage[CPR_MUTEX_STORAGE_SIZE];
		CprMaxAlign _align;
	} _internal;
} CprMutex;

#ifdef __cplusplus
extern "C" {
#endif

CPR_API CprResult cpr_mutex_init(CprMutex *mutex);
CPR_API void cpr_mutex_destroy(CprMutex *mutex);
CPR_API CprResult cpr_mutex_lock(CprMutex *mutex);
CPR_API CprResult
cpr_mutex_trylock(CprMutex *mutex); ///< Returns CPR_ERR_BUSY if not acquired.
CPR_API CprResult cpr_mutex_unlock(CprMutex *mutex);

#ifdef __cplusplus
}
#endif

// --- Condition Variable ---

#define CPR_CONDVAR_STORAGE_SIZE \
	48 // largest possible size across all platforms (48 bytes on macOS/iOS)

typedef struct {
	union {
		uint8_t storage[CPR_CONDVAR_STORAGE_SIZE];
		CprMaxAlign _align;
	} _internal;
} CprCondVar;

#ifdef __cplusplus
extern "C" {
#endif

CPR_API CprResult cpr_condvar_init(CprCondVar *condvar);
CPR_API void cpr_condvar_destroy(CprCondVar *condvar);

/// Releases `mutex` and blocks until signaled or broadcast.
/// Reacquires `mutex` before returning.
/// This can and will return spuriosly, *ALWAYS* recheck the condition.
CPR_API CprResult cpr_condvar_wait(CprCondVar *condvar, CprMutex *mutex);

/// Wakes one thread waiting on `condvar`. Safe to call with or without the mutex held.
CPR_API CprResult cpr_condvar_signal(CprCondVar *condvar);

/// Wakes all threads waiting on `condvar`. Safe to call with or without the mutex held.
CPR_API CprResult cpr_condvar_broadcast(CprCondVar *condvar);

#ifdef __cplusplus
}
#endif

// --- Read-Write Lock ---

#define CPR_RWLOCK_STORAGE_SIZE \
	200 // largest possible size across all platforms (200 bytes on macOS/iOS)

/// A read-write lock, must be initialized with `cpr_rwlock_init` before use!
/// Mutiple readers may hold the lock concurrently; writers are exclusive.
/// No writer preference is guaranteed; sunstained reader load can starve writers.
typedef struct {
	union {
		uint8_t storage[CPR_RWLOCK_STORAGE_SIZE];
		CprMaxAlign _align;
	} _internal;
} CprRwLock;

#ifdef __cplusplus
extern "C" {
#endif

CPR_API CprResult cpr_rwlock_init(CprRwLock *rwlock);
CPR_API void cpr_rwlock_destroy(CprRwLock *rwlock);

CPR_API CprResult cpr_rwlock_lckread(CprRwLock *rwlock);
/// Returns CPR_ERR_BUSY if the read lock cannot be acquired immediately.
CPR_API CprResult cpr_rwlock_try_lckread(CprRwLock *rwlock);
CPR_API CprResult cpr_rwlock_ulckread(CprRwLock *rwlock);
CPR_API CprResult cpr_rwlock_lckwrite(CprRwLock *rwlock);
/// Returns CPR_ERR_BUSY if the write lock cannot be acquired immediately.
CPR_API CprResult cpr_rwlock_try_lckwrite(CprRwLock *rwlock);
CPR_API CprResult cpr_rwlock_ulckwrite(CprRwLock *rwlock);

#ifdef __cplusplus
}
#endif

#endif // CPR_SYNC_H
