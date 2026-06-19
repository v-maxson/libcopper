#include "copper/sync.h"
#include "copper/result.h"

// --- Platform Includes ---
#if defined(CPR_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(CPR_PLATFORM_UNIX)
#include <errno.h> // IWYU pragma: keep
#include <pthread.h>
#endif

typedef struct {
#if defined(CPR_PLATFORM_WINDOWS)
	CRITICAL_SECTION handle;
#elif defined(CPR_PLATFORM_UNIX)
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
#elif defined(CPR_PLATFORM_UNIX)
	return pthread_mutex_init(&cpr__cast_mutex(mutex)->handle, NULL) == 0 ?
		       CPR_OK :
		       CPR_ERR_INVALID;
#endif

	return CPR_OK;
}

void cpr_mutex_destroy(CprMutex *mutex)
{
	if (mutex == NULL)
		return;

#if defined(CPR_PLATFORM_WINDOWS)
	DeleteCriticalSection(&cpr__cast_mutex(mutex)->handle);
#elif defined(CPR_PLATFORM_UNIX)
	pthread_mutex_destroy(&cpr__cast_mutex(mutex)->handle);
#endif
}

CprResult cpr_mutex_lock(CprMutex *mutex)
{
	if (mutex == NULL)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	EnterCriticalSection(&cpr__cast_mutex(mutex)->handle);
#elif defined(CPR_PLATFORM_UNIX)
	return pthread_mutex_lock(&cpr__cast_mutex(mutex)->handle) == 0 ?
		       CPR_OK :
		       CPR_ERR_INVALID;
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
#elif defined(CPR_PLATFORM_UNIX)
	int r = pthread_mutex_trylock(&cpr__cast_mutex(mutex)->handle);
	if (r == 0)
		return CPR_OK;
	if (r == EBUSY)
		return CPR_ERR_BUSY;
	return CPR_ERR_INVALID;
#endif
}

CprResult cpr_mutex_unlock(CprMutex *mutex)
{
	if (mutex == NULL)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	LeaveCriticalSection(&cpr__cast_mutex(mutex)->handle);
#elif defined(CPR_PLATFORM_UNIX)
	return pthread_mutex_unlock(&cpr__cast_mutex(mutex)->handle) == 0 ?
		       CPR_OK :
		       CPR_ERR_INVALID;
#endif

	return CPR_OK;
}
