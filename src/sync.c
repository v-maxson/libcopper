#include "copper/sync.h"
#include "copper/result.h"

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
	if (condvar == NULL)
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
