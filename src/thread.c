#include "copper/thread.h"

#include "copper/internal/int_error.h"
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

CprThread *cpr_thrd_create(CprThreadFn fn, void *arg)
{
	if (fn == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "fn is NULL");
		return NULL;
	}

	CprThread *thread = malloc(sizeof(CprThread));
	if (thread == NULL) {
		cpr__set_error(CPR_ERR_OOM, "out of memory");
		return NULL;
	}

	CprThreadTrampoline *trampoline = malloc(sizeof(CprThreadTrampoline));
	if (!trampoline) {
		free(thread);
		cpr__set_error(CPR_ERR_OOM, "out of memory");
		return NULL;
	}

	trampoline->fn = fn;
	trampoline->arg = arg;

#if defined(CPR_PLATFORM_WINDOWS)
	thread->handle = CreateThread(NULL, 0, cpr__thread_trampoline,
				      trampoline, 0, &thread->id);
	if (thread->handle == NULL) {
		free(trampoline);
		free(thread);
		cpr__set_error(CPR_ERR_SYNC, "CreateThread failed");
		return NULL;
	}
#else
	int r = pthread_create(&thread->handle, NULL, cpr__thread_trampoline,
			       trampoline);
	if (r != 0) {
		free(trampoline);
		free(thread);
		cpr__set_error(CPR_ERR_SYNC, "pthread_create failed");
		return NULL;
	}
#endif

	return thread;
}

bool cpr_thrd_join(CprThread *thread)
{
	if (thread == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "thread is NULL");
		return false;
	}

	bool ok = true;

#if defined(CPR_PLATFORM_WINDOWS)
	if (WaitForSingleObject(thread->handle, INFINITE) != WAIT_OBJECT_0) {
		cpr__set_error(CPR_ERR_SYNC, "WaitForSingleObject failed");
		ok = false;
	}
	CloseHandle(thread->handle);
#else
	if (pthread_join(thread->handle, NULL) != 0) {
		cpr__set_error(CPR_ERR_SYNC, "pthread_join failed");
		ok = false;
	}
#endif

	free(thread);
	return ok;
}

bool cpr_thrd_detach(CprThread *thread)
{
	if (thread == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "thread is NULL");
		return false;
	}

	bool ok = true;

#if defined(CPR_PLATFORM_WINDOWS)
	if (!CloseHandle(thread->handle)) {
		cpr__set_error(CPR_ERR_SYNC, "CloseHandle failed");
		ok = false;
	}
#else
	if (pthread_detach(thread->handle) != 0) {
		cpr__set_error(CPR_ERR_SYNC, "pthread_detach failed");
		ok = false;
	}
#endif

	free(thread);
	return ok;
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
	return (CprThreadId)GetCurrentThreadId();
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

CprTls *cpr_tls_create(CprTlsDestructor destructor)
{
	CprTls *tls = malloc(sizeof(CprTls));
	if (tls == NULL) {
		cpr__set_error(CPR_ERR_OOM, "out of memory");
		return NULL;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	tls->slot = FlsAlloc((PFLS_CALLBACK_FUNCTION)destructor);
	if (tls->slot == FLS_OUT_OF_INDEXES) {
		free(tls);
		cpr__set_error(CPR_ERR_EXHAUSTED, "FlsAlloc failed");
		return NULL;
	}
#else
	if (pthread_key_create(&tls->key, destructor) != 0) {
		free(tls);
		cpr__set_error(CPR_ERR_EXHAUSTED, "pthread_key_create failed");
		return NULL;
	}
#endif

	return tls;
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

bool cpr_tls_set(CprTls *tls, void *value)
{
	if (tls == NULL) {
		cpr__set_error(CPR_ERR_INVALID, "tls is NULL");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	if (!FlsSetValue(tls->slot, value)) {
		cpr__set_error(CPR_ERR_SYNC, "FlsSetValue failed");
		return false;
	}
	return true;
#else
	if (pthread_setspecific(tls->key, value) != 0) {
		cpr__set_error(CPR_ERR_SYNC, "pthread_setspecific failed");
		return false;
	}
	return true;
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
