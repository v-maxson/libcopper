#include "copper/sync.h"
#include "copper/result.h"
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

// fails the build if the mutex is ever larger then CPR_MUTEX_STORAGE_SIZE
typedef uint8_t cpr__mutex_size_check
	[sizeof(CprInternalMutex) <= CPR_MUTEX_STORAGE_SIZE ? 1 : -1];

CPR_INLINE static CprInternalMutex *cpr__cast_mutex(CprMutex *mutex)
{
	return (CprInternalMutex *)mutex->_internal.storage;
}

CprResult cpr_mutex_init(CprMutex *mutex)
{
	if (mutex == NULL)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	InitializeCriticalSection(&cpr__cast_mutex(mutex)->handle);
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	return pthread_mutex_init(&cpr__cast_mutex(mutex)->handle, NULL) == 0 ?
		       CPR_OK :
		       CPR_ERR_SYNC;
#endif

	return CPR_OK;
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

CprResult cpr_mutex_lock(CprMutex *mutex)
{
	if (mutex == NULL)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	EnterCriticalSection(&cpr__cast_mutex(mutex)->handle);
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	return pthread_mutex_lock(&cpr__cast_mutex(mutex)->handle) == 0 ?
		       CPR_OK :
		       CPR_ERR_SYNC;
#endif

	return CPR_OK;
}

CprResult cpr_mutex_trylock(CprMutex *mutex)
{
	if (mutex == NULL)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	return TryEnterCriticalSection(&cpr__cast_mutex(mutex)->handle) ?
		       CPR_OK :
		       CPR_ERR_BUSY;
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	int r = pthread_mutex_trylock(&cpr__cast_mutex(mutex)->handle);
	if (r == 0)
		return CPR_OK;
	if (r == EBUSY)
		return CPR_ERR_BUSY;
	return CPR_ERR_SYNC;
#endif
}

CprResult cpr_mutex_unlock(CprMutex *mutex)
{
	if (mutex == NULL)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	LeaveCriticalSection(&cpr__cast_mutex(mutex)->handle);
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	return pthread_mutex_unlock(&cpr__cast_mutex(mutex)->handle) == 0 ?
		       CPR_OK :
		       CPR_ERR_SYNC;
#endif

	return CPR_OK;
}

// --- Condition Variable ---

typedef struct {
#if defined(CPR_PLATFORM_WINDOWS)
	CONDITION_VARIABLE handle;
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	pthread_cond_t handle;
#endif
} CprInternalCondVar;

// fails to build if the condvar is ever larger then CPR_CONDVAR_STORAGE_SIZE
typedef uint8_t cpr__condvar_size_check
	[sizeof(CprInternalCondVar) <= CPR_CONDVAR_STORAGE_SIZE ? 1 : -1];

CPR_INLINE static CprInternalCondVar *cpr__cast_condvar(CprCondVar *condvar)
{
	return (CprInternalCondVar *)condvar->_internal.storage;
}

CPR_API CprResult cpr_condvar_init(CprCondVar *condvar)
{
	if (condvar == NULL)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	InitializeConditionVariable(&cpr__cast_condvar(condvar)->handle);
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	return pthread_cond_init(&cpr__cast_condvar(condvar)->handle, NULL) ==
			       0 ?
		       CPR_OK :
		       CPR_ERR_SYNC;
#endif

	return CPR_OK;
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

CPR_API CprResult cpr_condvar_wait(CprCondVar *condvar, CprMutex *mutex)
{
	if (condvar == NULL || mutex == NULL)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	return SleepConditionVariableCS(&cpr__cast_condvar(condvar)->handle,
					&cpr__cast_mutex(mutex)->handle,
					INFINITE) ?
		       CPR_OK :
		       CPR_ERR_SYNC;
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	return pthread_cond_wait(&cpr__cast_condvar(condvar)->handle,
				 &cpr__cast_mutex(mutex)->handle) == 0 ?
		       CPR_OK :
		       CPR_ERR_SYNC;
#endif
}

CPR_API CprResult cpr_condvar_signal(CprCondVar *condvar)
{
	if (condvar == NULL)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	WakeConditionVariable(&cpr__cast_condvar(condvar)->handle);
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	return pthread_cond_signal(&cpr__cast_condvar(condvar)->handle) == 0 ?
		       CPR_OK :
		       CPR_ERR_SYNC;
#endif

	return CPR_OK;
}

CPR_API CprResult cpr_condvar_broadcast(CprCondVar *condvar)
{
	if (condvar == NULL)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	WakeAllConditionVariable(&cpr__cast_condvar(condvar)->handle);
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	return pthread_cond_broadcast(&cpr__cast_condvar(condvar)->handle) ==
			       0 ?
		       CPR_OK :
		       CPR_ERR_SYNC;
#endif

	return CPR_OK;
}

// --- Read-Write Lock ---

typedef struct {
#if defined(CPR_PLATFORM_WINDOWS)
	SRWLOCK handle;
#elif (defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)) && \
	defined(CPR_HAS_PTHREAD_RWLOCK)
	pthread_rwlock_t handle;
#else
	CprMutex mutex;
	CprCondVar condvar;
	int32_t readers;
	bool writer;
#endif
} CprInternalRwLock;

typedef uint8_t cpr__rwlock_size_check
	[sizeof(CprInternalRwLock) <= CPR_RWLOCK_STORAGE_SIZE ? 1 : -1];

CPR_INLINE static CprInternalRwLock *cpr__cast_rwlock(CprRwLock *rwlock)
{
	return (CprInternalRwLock *)rwlock->_internal.storage;
}

CPR_API CprResult cpr_rwlock_init(CprRwLock *rwlock)
{
	if (rwlock == NULL)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	InitializeSRWLock(&cpr__cast_rwlock(rwlock)->handle);
#elif (defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)) && \
	defined(CPR_HAS_PTHREAD_RWLOCK)
	return pthread_rwlock_init(&cpr__cast_rwlock(rwlock)->handle, NULL) ==
			       0 ?
		       CPR_OK :
		       CPR_ERR_SYNC;
#else
	{
		CprInternalRwLock *rw = cpr__cast_rwlock(rwlock);
		CprResult r = cpr_mutex_init(&rw->mutex);
		if (cpr_err(r))
			return r;
		r = cpr_condvar_init(&rw->condvar);
		if (cpr_err(r)) {
			cpr_mutex_destroy(&rw->mutex);
			return r;
		}
		rw->readers = 0;
		rw->writer = false;
	}
#endif

	return CPR_OK;
}

CPR_API void cpr_rwlock_destroy(CprRwLock *rwlock)
{
	if (rwlock == NULL)
		return;

#if defined(CPR_PLATFORM_WINDOWS)
	(void)rwlock;
#elif (defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)) && \
	defined(CPR_HAS_PTHREAD_RWLOCK)
	pthread_rwlock_destroy(&cpr__cast_rwlock(rwlock)->handle);
#else
	cpr_condvar_destroy(&cpr__cast_rwlock(rwlock)->condvar);
	cpr_mutex_destroy(&cpr__cast_rwlock(rwlock)->mutex);
#endif
}

CPR_API CprResult cpr_rwlock_lckread(CprRwLock *rwlock)
{
	if (rwlock == NULL)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	AcquireSRWLockShared(&cpr__cast_rwlock(rwlock)->handle);
#elif (defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)) && \
	defined(CPR_HAS_PTHREAD_RWLOCK)
	return pthread_rwlock_rdlock(&cpr__cast_rwlock(rwlock)->handle) == 0 ?
		       CPR_OK :
		       CPR_ERR_SYNC;
#else
	{
		CprInternalRwLock *rw = cpr__cast_rwlock(rwlock);
		CprResult r = cpr_mutex_lock(&rw->mutex);
		if (cpr_err(r))
			return r;
		while (rw->writer) {
			r = cpr_condvar_wait(&rw->condvar, &rw->mutex);
			if (cpr_err(r)) {
				cpr_mutex_unlock(&rw->mutex);
				return r;
			}
		}
		rw->readers++;
		return cpr_mutex_unlock(&rw->mutex);
	}
#endif

	return CPR_OK;
}

CPR_API CprResult cpr_rwlock_try_lckread(CprRwLock *rwlock)
{
	if (rwlock == NULL)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	return TryAcquireSRWLockShared(&cpr__cast_rwlock(rwlock)->handle) ?
		       CPR_OK :
		       CPR_ERR_BUSY;
#elif (defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)) && \
	defined(CPR_HAS_PTHREAD_RWLOCK)
	int r = pthread_rwlock_tryrdlock(&cpr__cast_rwlock(rwlock)->handle);
	if (r == 0)
		return CPR_OK;
	if (r == EBUSY)
		return CPR_ERR_BUSY;
	return CPR_ERR_SYNC;
#else
	{
		CprInternalRwLock *rw = cpr__cast_rwlock(rwlock);
		CprResult r = cpr_mutex_lock(&rw->mutex);
		if (cpr_err(r))
			return r;
		if (rw->writer) {
			cpr_mutex_unlock(&rw->mutex);
			return CPR_ERR_BUSY;
		}
		rw->readers++;
		return cpr_mutex_unlock(&rw->mutex);
	}
#endif
}

CPR_API CprResult cpr_rwlock_ulckread(CprRwLock *rwlock)
{
	if (rwlock == NULL)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	ReleaseSRWLockShared(&cpr__cast_rwlock(rwlock)->handle);
#elif (defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)) && \
	defined(CPR_HAS_PTHREAD_RWLOCK)
	return pthread_rwlock_unlock(&cpr__cast_rwlock(rwlock)->handle) == 0 ?
		       CPR_OK :
		       CPR_ERR_SYNC;
#else
	{
		CprInternalRwLock *rw = cpr__cast_rwlock(rwlock);
		CprResult r = cpr_mutex_lock(&rw->mutex);
		if (cpr_err(r))
			return r;
		rw->readers--;
		if (rw->readers == 0) {
			r = cpr_condvar_broadcast(&rw->condvar);
			if (cpr_err(r)) {
				cpr_mutex_unlock(&rw->mutex);
				return r;
			}
		}
		return cpr_mutex_unlock(&rw->mutex);
	}
#endif

	return CPR_OK;
}

CPR_API CprResult cpr_rwlock_lckwrite(CprRwLock *rwlock)
{
	if (rwlock == NULL)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	AcquireSRWLockExclusive(&cpr__cast_rwlock(rwlock)->handle);
#elif (defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)) && \
	defined(CPR_HAS_PTHREAD_RWLOCK)
	return pthread_rwlock_wrlock(&cpr__cast_rwlock(rwlock)->handle) == 0 ?
		       CPR_OK :
		       CPR_ERR_SYNC;
#else
	{
		CprInternalRwLock *rw = cpr__cast_rwlock(rwlock);
		CprResult r = cpr_mutex_lock(&rw->mutex);
		if (cpr_err(r))
			return r;
		while (rw->readers > 0 || rw->writer) {
			r = cpr_condvar_wait(&rw->condvar, &rw->mutex);
			if (cpr_err(r)) {
				cpr_mutex_unlock(&rw->mutex);
				return r;
			}
		}
		rw->writer = true;
		return cpr_mutex_unlock(&rw->mutex);
	}
#endif

	return CPR_OK;
}

CPR_API CprResult cpr_rwlock_try_lckwrite(CprRwLock *rwlock)
{
	if (rwlock == NULL)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	return TryAcquireSRWLockExclusive(&cpr__cast_rwlock(rwlock)->handle) ?
		       CPR_OK :
		       CPR_ERR_BUSY;
#elif (defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)) && \
	defined(CPR_HAS_PTHREAD_RWLOCK)
	int r = pthread_rwlock_trywrlock(&cpr__cast_rwlock(rwlock)->handle);
	if (r == 0)
		return CPR_OK;
	if (r == EBUSY)
		return CPR_ERR_BUSY;
	return CPR_ERR_SYNC;
#else
	{
		CprInternalRwLock *rw = cpr__cast_rwlock(rwlock);
		CprResult r = cpr_mutex_lock(&rw->mutex);
		if (cpr_err(r))
			return r;
		if (rw->readers > 0 || rw->writer) {
			cpr_mutex_unlock(&rw->mutex);
			return CPR_ERR_BUSY;
		}
		rw->writer = true;
		return cpr_mutex_unlock(&rw->mutex);
	}
#endif
}

CPR_API CprResult cpr_rwlock_ulckwrite(CprRwLock *rwlock)
{
	if (rwlock == NULL)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	ReleaseSRWLockExclusive(&cpr__cast_rwlock(rwlock)->handle);
#elif (defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)) && \
	defined(CPR_HAS_PTHREAD_RWLOCK)
	return pthread_rwlock_unlock(&cpr__cast_rwlock(rwlock)->handle) == 0 ?
		       CPR_OK :
		       CPR_ERR_SYNC;
#else
	{
		CprInternalRwLock *rw = cpr__cast_rwlock(rwlock);
		CprResult r = cpr_mutex_lock(&rw->mutex);
		if (cpr_err(r))
			return r;
		rw->writer = false;
		r = cpr_condvar_broadcast(&rw->condvar);
		if (cpr_err(r)) {
			cpr_mutex_unlock(&rw->mutex);
			return r;
		}
		return cpr_mutex_unlock(&rw->mutex);
	}
#endif

	return CPR_OK;
}
