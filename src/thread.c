#include "copper/thread.h"

#include "copper/result.h"
#include "copper/time.h"
#include <stddef.h>
#include <stdlib.h>

#if defined(CPR_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
#include <pthread.h>
#include <sched.h>
#include <time.h>
#endif

// --- Opaque Definitions ---

struct CprThread {
#if defined(CPR_PLATFORM_WINDOWS)
	HANDLE handle;
	DWORD id;
#else
	pthread_t handle;
#endif
};

struct CprTls {
#if defined(CPR_PLATFORM_WINDOWS)
	DWORD slot;
#else
	pthread_key_t key;
#endif
};

// --- Thread Trampoline ---

typedef struct {
	CprThreadFn fn;
	void *arg;
} CprThreadTrampoline;

#if defined(CPR_PLATFORM_WINDOWS)
static DWORD WINAPI cpr__thread_trampoline(LPVOID arg)
{
	CprThreadTrampoline t = *(CprThreadTrampoline *)arg;
	free(arg);
	t.fn(t.arg);
	return 0;
}
#else
static void *cpr__thread_trampoline(void *arg)
{
	CprThreadTrampoline t = *(CprThreadTrampoline *)arg;
	free(arg);
	t.fn(t.arg);
	return NULL;
}
#endif

// --- Thread ---

CprThread *cpr_thrd_create(CprThreadFn fn, void *arg, CprResult *out_result)
{
#define RETURN_ERR(e)                      \
	do {                               \
		if (out_result)            \
			*out_result = (e); \
		return NULL;               \
	} while (0)

	if (fn == NULL)
		RETURN_ERR(CPR_ERR_INVALID);

	CprThread *thread = malloc(sizeof(CprThread));
	if (thread == NULL)
		RETURN_ERR(CPR_ERR_OOM);

	CprThreadTrampoline *trampoline = malloc(sizeof(CprThreadTrampoline));
	if (!trampoline) {
		free(thread);
		RETURN_ERR(CPR_ERR_OOM);
	}

	trampoline->fn = fn;
	trampoline->arg = arg;

#if defined(CPR_PLATFORM_WINDOWS)
	thread->handle = CreateThread(NULL, 0, cpr__thread_trampoline,
				      trampoline, 0, &thread->id);
	if (thread->handle == NULL) {
		free(trampoline);
		free(thread);
		RETURN_ERR(CPR_ERR_SYNC);
	}
#else
	int r = pthread_create(&thread->handle, NULL, cpr__thread_trampoline,
			       trampoline);
	if (r != 0) {
		free(trampoline);
		free(thread);
		RETURN_ERR(CPR_ERR_SYNC);
	}
#endif

	if (out_result)
		*out_result = CPR_OK;

	return thread;
#undef RETURN_ERR
}

CprResult cpr_thrd_join(CprThread *thread)
{
	if (thread == NULL)
		return CPR_ERR_INVALID;

	CprResult result = CPR_OK;

#if defined(CPR_PLATFORM_WINDOWS)
	if (WaitForSingleObject(thread->handle, INFINITE) != WAIT_OBJECT_0)
		result = CPR_ERR_SYNC;
	CloseHandle(thread->handle);
#else
	if (pthread_join(thread->handle, NULL) != 0)
		result = CPR_ERR_SYNC;
#endif

	free(thread);
	return result;
}

CprResult cpr_thrd_detach(CprThread *thread)
{
	if (thread == NULL)
		return CPR_ERR_INVALID;

	CprResult result = CPR_OK;

#if defined(CPR_PLATFORM_WINDOWS)
	if (!CloseHandle(thread->handle))
		result = CPR_ERR_SYNC;
#else
	if (pthread_detach(thread->handle) != 0)
		result = CPR_ERR_SYNC;
#endif

	free(thread);
	return result;
}

CprThreadId cpr_thrd_get_id(const CprThread *thread)
{
	if (thread == NULL)
		return 0;

#if defined(CPR_PLATFORM_WINDOWS)
	return (CprThreadId)thread->id;
#else
	return (CprThreadId)thread->handle;
#endif
}

CprThreadId cpr_thrd_current_id(void)
{
#if defined(CPR_PLATFORM_WINDOWS)
	return (CprThreadId)GetCurrentThread();
#else
	return (CprThreadId)pthread_self();
#endif
}

void cpr_thrd_sleep(uint32_t ms)
{
#if defined(CPR_PLATFORM_WINDOWS)
	Sleep((DWORD)ms);
#else
	struct timespec ts;
	ts.tv_sec = cpr_ms_to_s(ms);
	ts.tv_nsec = cpr_ms_to_ns(ms % 1000);
	nanosleep(&ts, NULL);
#endif
}

void cpr_thrd_yield(void)
{
#if defined(CPR_PLATFORM_WINDOWS)
	SwitchToThread();
#else
	sched_yield();
#endif
}

// --- TLS ---

CprTls *cpr_tls_create(CprTlsDestructor destructor, CprResult *out_result)
{
#define RETURN_ERR(err)                      \
	do {                                 \
		if (out_result)              \
			*out_result = (err); \
		return NULL;                 \
	} while (0)

	CprTls *tls = malloc(sizeof(CprTls));
	if (tls == NULL)
		RETURN_ERR(CPR_ERR_OOM);

#if defined(CPR_PLATFORM_WINDOWS)
	tls->slot = FlsAlloc((PFLS_CALLBACK_FUNCTION)destructor);
	if (tls->slot == FLS_OUT_OF_INDEXES) {
		free(tls);
		RETURN_ERR(CPR_ERR_EXHAUSTED);
	}
#else
	if (pthread_key_create(&tls->key, destructor) != 0) {
		free(tls);
		RETURN_ERR(CPR_ERR_EXHAUSTED);
	}
#endif

	if (out_result)
		*out_result = CPR_OK;
	return tls;
#undef RETURN_ERR
}

void cpr_tls_destroy(CprTls *tls)
{
	if (tls == NULL)
		return;

#if defined(CPR_PLATFORM_WINDOWS)
	FlsFree(tls->slot);
#else
	pthread_key_delete(tls->key);
#endif

	free(tls);
}

CprResult cpr_tls_set(CprTls *tls, void *value)
{
	if (tls == NULL)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	return FlsSetValue(tls->slot, value) ? CPR_OK : CPR_ERR_SYNC;
#else
	return pthread_setspecific(tls->key, value) == 0 ? CPR_OK :
							   CPR_ERR_SYNC;
#endif
}

void *cpr_tls_get(const CprTls *tls)
{
	if (tls == NULL)
		return NULL;

#if defined(CPR_PLATFORM_WINDOWS)
	return FlsGetValue(tls->slot);
#else
	return pthread_getspecific(tls->key);
#endif
}
