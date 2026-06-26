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
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_condvar_init(NULL));
}

void test_wait_null_condvar(void)
{
	CprMutex m;
	cpr_mutex_init(&m);
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_condvar_wait(NULL, &m));
	cpr_mutex_destroy(&m);
}

void test_wait_null_mutex(void)
{
	CprCondVar cv;
	cpr_condvar_init(&cv);
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_condvar_wait(&cv, NULL));
	cpr_condvar_destroy(&cv);
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
	cpr_condvar_destroy(NULL); // must not crash
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

static void cpr__wait_worker(void *arg)
{
	WaitArgs *a = (WaitArgs *)arg;
	cpr_mutex_lock(a->mutex);
	(*a->waiting)++;
	while (!*a->ready)
		a->result = cpr_condvar_wait(a->cv, a->mutex);
	cpr_mutex_unlock(a->mutex);
}

static void cpr__spin_until_waiting(CprMutex *m, int *waiting, int target)
{
	do {
		cpr_mutex_lock(m);
		if (*waiting == target)
			return; // returns holding the lock
		cpr_mutex_unlock(m);
	} while (1);
}

void test_signal_wakes_waiter(void)
{
	CprMutex m;
	CprCondVar cv;
	int ready = 0, waiting = 0;
	CprThread *t;
	WaitArgs args;

	cpr_mutex_init(&m);
	cpr_condvar_init(&cv);

	args.mutex = &m;
	args.cv = &cv;
	args.ready = &ready;
	args.waiting = &waiting;
	args.result = CPR_OK;
	t = cpr_thrd_create(cpr__wait_worker, &args, NULL);

	cpr__spin_until_waiting(&m, &waiting, 1);
	ready = 1;
	cpr_condvar_signal(&cv);
	cpr_mutex_unlock(&m);

	cpr_thrd_join(t);

	TEST_ASSERT_EQUAL_INT(CPR_OK, args.result);
	cpr_condvar_destroy(&cv);
	cpr_mutex_destroy(&m);
}

void test_broadcast_wakes_all(void)
{
	CprMutex m;
	CprCondVar cv;
	int ready = 0, waiting = 0;
	CprThread *threads[NWAITERS];
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
		threads[i] = cpr_thrd_create(cpr__wait_worker, &args[i], NULL);
	}

	cpr__spin_until_waiting(&m, &waiting, NWAITERS);
	ready = 1;
	cpr_condvar_broadcast(&cv);
	cpr_mutex_unlock(&m);

	for (i = 0; i < NWAITERS; i++)
		cpr_thrd_join(threads[i]);

	for (i = 0; i < NWAITERS; i++)
		TEST_ASSERT_EQUAL_INT(CPR_OK, args[i].result);

	cpr_condvar_destroy(&cv);
	cpr_mutex_destroy(&m);
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_init_null);
	RUN_TEST(test_wait_null_condvar);
	RUN_TEST(test_wait_null_mutex);
	RUN_TEST(test_signal_null);
	RUN_TEST(test_broadcast_null);
	RUN_TEST(test_destroy_null);

	RUN_TEST(test_init_succeeds);

	RUN_TEST(test_signal_wakes_waiter);
	RUN_TEST(test_broadcast_wakes_all);

	return UNITY_END();
}
