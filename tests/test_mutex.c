#include "unity.h"
#include <copper/copper.h>

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
	cpr_mutex_destroy(NULL); // must not crash
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

static void cpr__trylock_worker(void *arg)
{
	TrylockArgs *a = (TrylockArgs *)arg;
	a->result = cpr_mutex_trylock(a->mutex);
	if (a->result == CPR_OK)
		cpr_mutex_unlock(a->mutex);
}

static void cpr__counter_worker(void *arg)
{
	CounterArgs *a = (CounterArgs *)arg;
	int i;
	for (i = 0; i < INCREMENTS; i++) {
		CprResult r = cpr_mutex_lock(a->mutex);
		if (cpr_err(r)) {
			a->result = r;
			return;
		}
		(*a->counter)++;
		r = cpr_mutex_unlock(a->mutex);
		if (cpr_err(r)) {
			a->result = r;
			return;
		}
	}
}

void test_trylock_busy_when_held(void)
{
	CprMutex m;
	TrylockArgs args;
	CprThread *t;

	cpr_mutex_init(&m);
	cpr_mutex_lock(&m);

	args.mutex = &m;
	args.result = CPR_OK;
	t = cpr_thrd_create(cpr__trylock_worker, &args, NULL);
	cpr_thrd_join(t);

	TEST_ASSERT_EQUAL_INT(CPR_ERR_BUSY, args.result);
	cpr_mutex_unlock(&m);
	cpr_mutex_destroy(&m);
}

void test_contention_correctness(void)
{
	CprMutex m;
	volatile int counter = 0;
	CprThread *threads[NTHREADS];
	CounterArgs args[NTHREADS];
	int i;

	cpr_mutex_init(&m);
	for (i = 0; i < NTHREADS; i++) {
		args[i].mutex = &m;
		args[i].counter = &counter;
		args[i].result = CPR_OK;
		threads[i] =
			cpr_thrd_create(cpr__counter_worker, &args[i], NULL);
	}

	for (i = 0; i < NTHREADS; i++)
		cpr_thrd_join(threads[i]);

	for (i = 0; i < NTHREADS; i++)
		TEST_ASSERT_EQUAL_INT(CPR_OK, args[i].result);
	TEST_ASSERT_EQUAL_INT(NTHREADS * INCREMENTS, (int)counter);
	cpr_mutex_destroy(&m);
}

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

	RUN_TEST(test_trylock_busy_when_held);
	RUN_TEST(test_contention_correctness);

	return UNITY_END();
}
