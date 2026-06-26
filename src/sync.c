#include "copper/sync.h"
#include "copper/internal/int_error.h"
#include <stdbool.h>

// --- Platform Includes ---
#if defined(CPR_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
#include <errno.h> // IWYU pragma: keep
#include <pthread.h>
#endif

/// --- Mutex ---

typedef struct {
#if defined(CPR_PLATFORM_WINDOWS)
	CRITICAL_SECTION handle;
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	pthread_mutex_t handle;
#endif
} CprInternalMutex;

CPR_STATIC_ASSERT(sizeof(CprInternalMutex) <= CPR_MUTEX_STORAGE_SIZE,
		  CprInternalMutex_fits_in_storage);

CPR_INLINE static CprInternalMutex *cpr__cast_mutex(CprMutex *mutex)
{
	return (CprInternalMutex *)mutex->_internal.storage;
}

bool cpr_mutex_init(CprMutex *mutex)
{
	if (mutex == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "mutex is NULL");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	InitializeCriticalSection(&cpr__cast_mutex(mutex)->handle);
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	if (pthread_mutex_init(&cpr__cast_mutex(mutex)->handle, NULL) != 0) {
		cpr__set_error(CPR_ERR_SYNC, "failed to initialize mutex");
		return false;
	}
#endif

	return true;
}

void cpr_mutex_destroy(CprMutex *mutex)
{
	if (mutex == NULL)
		return;

#if defined(CPR_PLATFORM_WINDOWS)
	DeleteCriticalSection(&cpr__cast_mutex(mutex)->handle);
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	pthread_mutex_destroy(&cpr__cast_mutex(mutex)->handle);
#endif
}

bool cpr_mutex_lock(CprMutex *mutex)
{
	if (mutex == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "mutex is NULL");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	EnterCriticalSection(&cpr__cast_mutex(mutex)->handle);
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	if (pthread_mutex_lock(&cpr__cast_mutex(mutex)->handle) != 0) {
		cpr__set_error(CPR_ERR_SYNC, "failed to lock mutex");
		return false;
	}
#endif

	return true;
}

bool cpr_mutex_trylock(CprMutex *mutex)
{
	if (mutex == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "mutex is NULL");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	if (!TryEnterCriticalSection(&cpr__cast_mutex(mutex)->handle)) {
		cpr__set_error(CPR_ERR_BUSY, "mutex is already held");
		return false;
	}
	return true;
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	int r = pthread_mutex_trylock(&cpr__cast_mutex(mutex)->handle);
	if (r == 0)
		return true;
	if (r == EBUSY) {
		cpr__set_error(CPR_ERR_BUSY, "mutex is already held");
		return false;
	}
	cpr__set_error(CPR_ERR_SYNC, "failed to try-lock mutex");
	return false;
#endif
}

bool cpr_mutex_unlock(CprMutex *mutex)
{
	if (mutex == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "mutex is NULL");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	LeaveCriticalSection(&cpr__cast_mutex(mutex)->handle);
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	if (pthread_mutex_unlock(&cpr__cast_mutex(mutex)->handle) != 0) {
		cpr__set_error(CPR_ERR_SYNC, "failed to unlock mutex");
		return false;
	}
#endif

	return true;
}

// --- Condition Variable ---

typedef struct {
#if defined(CPR_PLATFORM_WINDOWS)
	CONDITION_VARIABLE handle;
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	pthread_cond_t handle;
#endif
} CprInternalCondVar;

CPR_STATIC_ASSERT(sizeof(CprInternalCondVar) <= CPR_CONDVAR_STORAGE_SIZE,
		  CprInternalCondVar_fits_in_storage);

CPR_INLINE static CprInternalCondVar *cpr__cast_condvar(CprCondVar *condvar)
{
	return (CprInternalCondVar *)condvar->_internal.storage;
}

CPR_API bool cpr_condvar_init(CprCondVar *condvar)
{
	if (condvar == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "condvar is NULL");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	InitializeConditionVariable(&cpr__cast_condvar(condvar)->handle);
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	if (pthread_cond_init(&cpr__cast_condvar(condvar)->handle, NULL) != 0) {
		cpr__set_error(CPR_ERR_SYNC,
			       "failed to initialize condition variable");
		return false;
	}
#endif

	return true;
}

CPR_API void cpr_condvar_destroy(CprCondVar *condvar)
{
	if (condvar == NULL)
		return;

#if defined(CPR_PLATFORM_WINDOWS)
	(void)condvar; // no cleanup here
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	pthread_cond_destroy(&cpr__cast_condvar(condvar)->handle);
#endif
}

CPR_API bool cpr_condvar_wait(CprCondVar *condvar, CprMutex *mutex)
{
	if (condvar == NULL || mutex == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "condvar or mutex is NULL");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	if (!SleepConditionVariableCS(&cpr__cast_condvar(condvar)->handle,
				      &cpr__cast_mutex(mutex)->handle,
				      INFINITE)) {
		cpr__set_error(CPR_ERR_SYNC,
			       "failed to wait on condition variable");
		return false;
	}
	return true;
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	if (pthread_cond_wait(&cpr__cast_condvar(condvar)->handle,
			      &cpr__cast_mutex(mutex)->handle) != 0) {
		cpr__set_error(CPR_ERR_SYNC,
			       "failed to wait on condition variable");
		return false;
	}
	return true;
#endif
}

CPR_API bool cpr_condvar_signal(CprCondVar *condvar)
{
	if (condvar == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "condvar is NULL");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	WakeConditionVariable(&cpr__cast_condvar(condvar)->handle);
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	if (pthread_cond_signal(&cpr__cast_condvar(condvar)->handle) != 0) {
		cpr__set_error(CPR_ERR_SYNC,
			       "failed to signal condition variable");
		return false;
	}
#endif

	return true;
}

CPR_API bool cpr_condvar_broadcast(CprCondVar *condvar)
{
	if (condvar == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "condvar is NULL");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	WakeAllConditionVariable(&cpr__cast_condvar(condvar)->handle);
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	if (pthread_cond_broadcast(&cpr__cast_condvar(condvar)->handle) != 0) {
		cpr__set_error(CPR_ERR_SYNC,
			       "failed to broadcast on condition variable");
		return false;
	}
#endif

	return true;
}

// --- Read-Write Lock ---

typedef struct {
#if defined(CPR_PLATFORM_WINDOWS)
	SRWLOCK handle;
#else
	pthread_rwlock_t handle;
#endif
} CprInternalRwLock;

CPR_STATIC_ASSERT(sizeof(CprInternalRwLock) <= CPR_RWLOCK_STORAGE_SIZE,
		  CprInternalRwLock_fits_in_storage);

CPR_INLINE static CprInternalRwLock *cpr__cast_rwlock(CprRwLock *rwlock)
{
	return (CprInternalRwLock *)rwlock->_internal.storage;
}

CPR_API bool cpr_rwlock_init(CprRwLock *rwlock)
{
	if (rwlock == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "rwlock is NULL");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	InitializeSRWLock(&cpr__cast_rwlock(rwlock)->handle);
#else
	if (pthread_rwlock_init(&cpr__cast_rwlock(rwlock)->handle, NULL) != 0) {
		cpr__set_error(CPR_ERR_SYNC, "failed to initialize rwlock");
		return false;
	}
#endif

	return true;
}

CPR_API void cpr_rwlock_destroy(CprRwLock *rwlock)
{
	if (rwlock == NULL)
		return;

#if defined(CPR_PLATFORM_WINDOWS)
	(void)rwlock;
#else
	pthread_rwlock_destroy(&cpr__cast_rwlock(rwlock)->handle);
#endif
}

CPR_API bool cpr_rwlock_lckread(CprRwLock *rwlock)
{
	if (rwlock == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "rwlock is NULL");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	AcquireSRWLockShared(&cpr__cast_rwlock(rwlock)->handle);
#else
	if (pthread_rwlock_rdlock(&cpr__cast_rwlock(rwlock)->handle) != 0) {
		cpr__set_error(CPR_ERR_SYNC, "failed to acquire read lock");
		return false;
	}
#endif

	return true;
}

CPR_API bool cpr_rwlock_try_lckread(CprRwLock *rwlock)
{
	if (rwlock == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "rwlock is NULL");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	if (!TryAcquireSRWLockShared(&cpr__cast_rwlock(rwlock)->handle)) {
		cpr__set_error(CPR_ERR_BUSY, "read lock is exclusively held");
		return false;
	}
	return true;
#else
	int r = pthread_rwlock_tryrdlock(&cpr__cast_rwlock(rwlock)->handle);
	if (r == 0)
		return true;
	if (r == EBUSY) {
		cpr__set_error(CPR_ERR_BUSY, "read lock is exclusively held");
		return false;
	}
	cpr__set_error(CPR_ERR_SYNC, "failed to try-acquire read lock");
	return false;
#endif
}

CPR_API bool cpr_rwlock_ulckread(CprRwLock *rwlock)
{
	if (rwlock == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "rwlock is NULL");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	ReleaseSRWLockShared(&cpr__cast_rwlock(rwlock)->handle);
#else
	if (pthread_rwlock_unlock(&cpr__cast_rwlock(rwlock)->handle) != 0) {
		cpr__set_error(CPR_ERR_SYNC, "failed to release read lock");
		return false;
	}
#endif

	return true;
}

CPR_API bool cpr_rwlock_lckwrite(CprRwLock *rwlock)
{
	if (rwlock == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "rwlock is NULL");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	AcquireSRWLockExclusive(&cpr__cast_rwlock(rwlock)->handle);
#else
	if (pthread_rwlock_wrlock(&cpr__cast_rwlock(rwlock)->handle) != 0) {
		cpr__set_error(CPR_ERR_SYNC, "failed to acquire write lock");
		return false;
	}
#endif

	return true;
}

CPR_API bool cpr_rwlock_try_lckwrite(CprRwLock *rwlock)
{
	if (rwlock == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "rwlock is NULL");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	if (!TryAcquireSRWLockExclusive(&cpr__cast_rwlock(rwlock)->handle)) {
		cpr__set_error(CPR_ERR_BUSY, "lock is already held");
		return false;
	}
	return true;
#else
	int r = pthread_rwlock_trywrlock(&cpr__cast_rwlock(rwlock)->handle);
	if (r == 0)
		return true;
	if (r == EBUSY) {
		cpr__set_error(CPR_ERR_BUSY, "lock is already held");
		return false;
	}
	cpr__set_error(CPR_ERR_SYNC, "failed to try-acquire write lock");
	return false;
#endif
}

CPR_API bool cpr_rwlock_ulckwrite(CprRwLock *rwlock)
{
	if (rwlock == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "rwlock is NULL");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	ReleaseSRWLockExclusive(&cpr__cast_rwlock(rwlock)->handle);
#else
	if (pthread_rwlock_unlock(&cpr__cast_rwlock(rwlock)->handle) != 0) {
		cpr__set_error(CPR_ERR_SYNC, "failed to release write lock");
		return false;
	}
#endif

	return true;
}
