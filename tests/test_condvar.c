#include "copper/platform.h" // IWYU pragma: keep
#include "copper/result.h"
#include "copper/sync.h"
#include "unity.h"

#if defined(CPR_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
#include <pthread.h>
#endif

void setUp(void)
{
}
void tearDown(void)
{
}

// --- Null guards ---

void test_init_null(void)
{
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_condvar_init(NULL));
}

void test_wait_null_condvar(void)
{
	CprMutex m;
	cpr_mutex_init(&m);
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_condvar_wait(NULL, &m));
	cpr_mutex_destroy(&m);
}

void test_signal_null(void)
{
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_condvar_signal(NULL));
}

void test_broadcast_null(void)
{
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_condvar_broadcast(NULL));
}

void test_destroy_null(void)
{
	cpr_condvar_destroy(NULL); /* must not crash */
}

// --- Lifecycle ---

void test_init_succeeds(void)
{
	CprCondVar cv;
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_condvar_init(&cv));
	cpr_condvar_destroy(&cv);
}

// --- Multi-threaded ---

#define NWAITERS 4

typedef struct {
	CprMutex *mutex;
	CprCondVar *cv;
	int *ready;
	int *waiting;
	CprResult result;
} WaitArgs;

#if defined(CPR_PLATFORM_WINDOWS)

static DWORD WINAPI cpr__wait_worker(LPVOID arg)
{
	WaitArgs *a = (WaitArgs *)arg;
	cpr_mutex_lock(a->mutex);
	(*a->waiting)++;
	while (!*a->ready)
		a->result = cpr_condvar_wait(a->cv, a->mutex);
	cpr_mutex_unlock(a->mutex);
	return 0;
}

static void cpr__spin_until_waiting(CprMutex *m, int *waiting, int target)
{
	do {
		cpr_mutex_lock(m);
		if (*waiting == target)
			return; /* returns holding the lock */
		cpr_mutex_unlock(m);
	} while (1);
}

void test_signal_wakes_waiter(void)
{
	CprMutex m;
	CprCondVar cv;
	int ready = 0, waiting = 0;
	HANDLE t;
	WaitArgs args;

	cpr_mutex_init(&m);
	cpr_condvar_init(&cv);

	args.mutex = &m;
	args.cv = &cv;
	args.ready = &ready;
	args.waiting = &waiting;
	args.result = CPR_OK;
	t = CreateThread(NULL, 0, cpr__wait_worker, &args, 0, NULL);

	cpr__spin_until_waiting(&m, &waiting, 1);
	ready = 1;
	cpr_condvar_signal(&cv);
	cpr_mutex_unlock(&m);

	WaitForSingleObject(t, INFINITE);
	CloseHandle(t);

	TEST_ASSERT_EQUAL_INT(CPR_OK, args.result);
	cpr_condvar_destroy(&cv);
	cpr_mutex_destroy(&m);
}

void test_broadcast_wakes_all(void)
{
	CprMutex m;
	CprCondVar cv;
	int ready = 0, waiting = 0;
	HANDLE threads[NWAITERS];
	WaitArgs args[NWAITERS];
	int i;

	cpr_mutex_init(&m);
	cpr_condvar_init(&cv);

	for (i = 0; i < NWAITERS; i++) {
		args[i].mutex = &m;
		args[i].cv = &cv;
		args[i].ready = &ready;
		args[i].waiting = &waiting;
		args[i].result = CPR_OK;
		threads[i] = CreateThread(NULL, 0, cpr__wait_worker, &args[i],
					  0, NULL);
	}

	cpr__spin_until_waiting(&m, &waiting, NWAITERS);
	ready = 1;
	cpr_condvar_broadcast(&cv);
	cpr_mutex_unlock(&m);

	WaitForMultipleObjects(NWAITERS, threads, TRUE, INFINITE);
	for (i = 0; i < NWAITERS; i++)
		CloseHandle(threads[i]);

	for (i = 0; i < NWAITERS; i++)
		TEST_ASSERT_EQUAL_INT(CPR_OK, args[i].result);

	cpr_condvar_destroy(&cv);
	cpr_mutex_destroy(&m);
}

#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)

static void *cpr__wait_worker(void *arg)
{
	WaitArgs *a = (WaitArgs *)arg;
	cpr_mutex_lock(a->mutex);
	(*a->waiting)++;
	while (!*a->ready)
		a->result = cpr_condvar_wait(a->cv, a->mutex);
	cpr_mutex_unlock(a->mutex);
	return NULL;
}

static void cpr__spin_until_waiting(CprMutex *m, int *waiting, int target)
{
	do {
		cpr_mutex_lock(m);
		if (*waiting == target)
			return; /* returns holding the lock */
		cpr_mutex_unlock(m);
	} while (1);
}

void test_signal_wakes_waiter(void)
{
	CprMutex m;
	CprCondVar cv;
	int ready = 0, waiting = 0;
	pthread_t t;
	WaitArgs args;

	cpr_mutex_init(&m);
	cpr_condvar_init(&cv);

	args.mutex = &m;
	args.cv = &cv;
	args.ready = &ready;
	args.waiting = &waiting;
	args.result = CPR_OK;
	pthread_create(&t, NULL, cpr__wait_worker, &args);

	cpr__spin_until_waiting(&m, &waiting, 1);
	ready = 1;
	cpr_condvar_signal(&cv);
	cpr_mutex_unlock(&m);

	pthread_join(t, NULL);

	TEST_ASSERT_EQUAL_INT(CPR_OK, args.result);
	cpr_condvar_destroy(&cv);
	cpr_mutex_destroy(&m);
}

void test_broadcast_wakes_all(void)
{
	CprMutex m;
	CprCondVar cv;
	int ready = 0, waiting = 0;
	pthread_t threads[NWAITERS];
	WaitArgs args[NWAITERS];
	int i;

	cpr_mutex_init(&m);
	cpr_condvar_init(&cv);

	for (i = 0; i < NWAITERS; i++) {
		args[i].mutex = &m;
		args[i].cv = &cv;
		args[i].ready = &ready;
		args[i].waiting = &waiting;
		args[i].result = CPR_OK;
		pthread_create(&threads[i], NULL, cpr__wait_worker, &args[i]);
	}

	cpr__spin_until_waiting(&m, &waiting, NWAITERS);
	ready = 1;
	cpr_condvar_broadcast(&cv);
	cpr_mutex_unlock(&m);

	for (i = 0; i < NWAITERS; i++)
		pthread_join(threads[i], NULL);

	for (i = 0; i < NWAITERS; i++)
		TEST_ASSERT_EQUAL_INT(CPR_OK, args[i].result);

	cpr_condvar_destroy(&cv);
	cpr_mutex_destroy(&m);
}

#endif /* threading */

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_init_null);
	RUN_TEST(test_wait_null_condvar);
	RUN_TEST(test_signal_null);
	RUN_TEST(test_broadcast_null);
	RUN_TEST(test_destroy_null);

	RUN_TEST(test_init_succeeds);

#if defined(CPR_PLATFORM_WINDOWS) || defined(CPR_PLATFORM_UNIX) || \
	defined(CPR_PLATFORM_APPLE)
	RUN_TEST(test_signal_wakes_waiter);
	RUN_TEST(test_broadcast_wakes_all);
#endif

	return UNITY_END();
}
