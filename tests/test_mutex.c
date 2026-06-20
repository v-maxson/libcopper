#include "unity.h"
#include <copper/copper.h>

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
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_mutex_init(NULL));
}

void test_lock_null(void)
{
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_mutex_lock(NULL));
}

void test_trylock_null(void)
{
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_mutex_trylock(NULL));
}

void test_unlock_null(void)
{
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_mutex_unlock(NULL));
}

void test_destroy_null(void)
{
	cpr_mutex_destroy(NULL); /* must not crash */
}

// --- Lifecycle ---

void test_init_succeeds(void)
{
	CprMutex m;
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_mutex_init(&m));
	cpr_mutex_destroy(&m);
}

void test_lock_unlock(void)
{
	CprMutex m;
	cpr_mutex_init(&m);
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_mutex_lock(&m));
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_mutex_unlock(&m));
	cpr_mutex_destroy(&m);
}

void test_trylock_succeeds_when_unlocked(void)
{
	CprMutex m;
	cpr_mutex_init(&m);
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_mutex_trylock(&m));
	cpr_mutex_unlock(&m);
	cpr_mutex_destroy(&m);
}

void test_repeated_lock_unlock_cycles(void)
{
	CprMutex m;
	int i;
	cpr_mutex_init(&m);
	for (i = 0; i < 16; i++) {
		TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_mutex_lock(&m));
		TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_mutex_unlock(&m));
	}
	cpr_mutex_destroy(&m);
}

// --- Multi-threaded ---

typedef struct {
	CprMutex *mutex;
	CprResult result;
} TrylockArgs;

#define NTHREADS 4
#define INCREMENTS 1000

typedef struct {
	CprMutex *mutex;
	volatile int *counter;
	CprResult result;
} CounterArgs;

#if defined(CPR_PLATFORM_WINDOWS)

static DWORD WINAPI cpr__trylock_worker(LPVOID arg)
{
	TrylockArgs *a = (TrylockArgs *)arg;
	a->result = cpr_mutex_trylock(a->mutex);
	if (a->result == CPR_OK)
		cpr_mutex_unlock(a->mutex);
	return 0;
}

static DWORD WINAPI cpr__counter_worker(LPVOID arg)
{
	CounterArgs *a = (CounterArgs *)arg;
	int i;
	for (i = 0; i < INCREMENTS; i++) {
		CprResult r = cpr_mutex_lock(a->mutex);
		if (cpr_err(r)) { a->result = r; return 0; }
		(*a->counter)++;
		r = cpr_mutex_unlock(a->mutex);
		if (cpr_err(r)) { a->result = r; return 0; }
	}
	return 0;
}

void test_trylock_busy_when_held(void)
{
	CprMutex m;
	TrylockArgs args;
	HANDLE t;

	cpr_mutex_init(&m);
	cpr_mutex_lock(&m);

	args.mutex = &m;
	args.result = CPR_OK;
	t = CreateThread(NULL, 0, cpr__trylock_worker, &args, 0, NULL);
	WaitForSingleObject(t, INFINITE);
	CloseHandle(t);

	TEST_ASSERT_EQUAL_INT(CPR_ERR_BUSY, args.result);
	cpr_mutex_unlock(&m);
	cpr_mutex_destroy(&m);
}

void test_contention_correctness(void)
{
	CprMutex m;
	volatile int counter = 0;
	HANDLE threads[NTHREADS];
	CounterArgs args[NTHREADS];
	int i;

	cpr_mutex_init(&m);
	for (i = 0; i < NTHREADS; i++) {
		args[i].mutex = &m;
		args[i].counter = &counter;
		args[i].result = CPR_OK;
		threads[i] = CreateThread(NULL, 0, cpr__counter_worker,
					  &args[i], 0, NULL);
	}

	WaitForMultipleObjects(NTHREADS, threads, TRUE, INFINITE);
	for (i = 0; i < NTHREADS; i++)
		CloseHandle(threads[i]);

	for (i = 0; i < NTHREADS; i++)
		TEST_ASSERT_EQUAL_INT(CPR_OK, args[i].result);
	TEST_ASSERT_EQUAL_INT(NTHREADS * INCREMENTS, (int)counter);
	cpr_mutex_destroy(&m);
}

#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)

static void *cpr__trylock_worker(void *arg)
{
	TrylockArgs *a = (TrylockArgs *)arg;
	a->result = cpr_mutex_trylock(a->mutex);
	if (a->result == CPR_OK)
		cpr_mutex_unlock(a->mutex);
	return NULL;
}

static void *cpr__counter_worker(void *arg)
{
	CounterArgs *a = (CounterArgs *)arg;
	int i;
	for (i = 0; i < INCREMENTS; i++) {
		CprResult r = cpr_mutex_lock(a->mutex);
		if (cpr_err(r)) { a->result = r; return NULL; }
		(*a->counter)++;
		r = cpr_mutex_unlock(a->mutex);
		if (cpr_err(r)) { a->result = r; return NULL; }
	}
	return NULL;
}

void test_trylock_busy_when_held(void)
{
	CprMutex m;
	TrylockArgs args;
	pthread_t t;

	cpr_mutex_init(&m);
	cpr_mutex_lock(&m);

	args.mutex = &m;
	args.result = CPR_OK;
	pthread_create(&t, NULL, cpr__trylock_worker, &args);
	pthread_join(t, NULL);

	TEST_ASSERT_EQUAL_INT(CPR_ERR_BUSY, args.result);
	cpr_mutex_unlock(&m);
	cpr_mutex_destroy(&m);
}

void test_contention_correctness(void)
{
	CprMutex m;
	volatile int counter = 0;
	pthread_t threads[NTHREADS];
	CounterArgs args[NTHREADS];
	int i;

	cpr_mutex_init(&m);
	for (i = 0; i < NTHREADS; i++) {
		args[i].mutex = &m;
		args[i].counter = &counter;
		args[i].result = CPR_OK;
		pthread_create(&threads[i], NULL, cpr__counter_worker,
			       &args[i]);
	}

	for (i = 0; i < NTHREADS; i++)
		pthread_join(threads[i], NULL);

	for (i = 0; i < NTHREADS; i++)
		TEST_ASSERT_EQUAL_INT(CPR_OK, args[i].result);
	TEST_ASSERT_EQUAL_INT(NTHREADS * INCREMENTS, (int)counter);
	cpr_mutex_destroy(&m);
}

#endif /* threading */

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_init_null);
	RUN_TEST(test_lock_null);
	RUN_TEST(test_trylock_null);
	RUN_TEST(test_unlock_null);
	RUN_TEST(test_destroy_null);

	RUN_TEST(test_init_succeeds);
	RUN_TEST(test_lock_unlock);
	RUN_TEST(test_trylock_succeeds_when_unlocked);
	RUN_TEST(test_repeated_lock_unlock_cycles);

#if defined(CPR_PLATFORM_WINDOWS) || defined(CPR_PLATFORM_UNIX) || \
	defined(CPR_PLATFORM_APPLE)
	RUN_TEST(test_trylock_busy_when_held);
	RUN_TEST(test_contention_correctness);
#endif

	return UNITY_END();
}
