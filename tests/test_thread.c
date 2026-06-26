#include "unity.h"
#include <copper/copper.h>

void setUp(void)
{
}
void tearDown(void)
{
}

// --- Null guards ---

void test_create_null_fn(void)
{
	CprResult r = CPR_OK;
	CprThread *t = cpr_thrd_create(NULL, NULL, &r);
	TEST_ASSERT_NULL(t);
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, r);
}

void test_join_null(void)
{
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_thrd_join(NULL));
}

void test_detach_null(void)
{
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_thrd_detach(NULL));
}

void test_get_id_null(void)
{
	TEST_ASSERT_EQUAL_UINT64(0, cpr_thrd_get_id(NULL));
}

// --- Current ID ---

void test_current_id_nonzero(void)
{
	TEST_ASSERT_NOT_EQUAL(0, cpr_thrd_current_id());
}

// --- Create and join ---

static void noop_worker(void *arg)
{
	(void)arg;
}

void test_create_join(void)
{
	CprResult r = CPR_ERR_INVALID;
	CprThread *t = cpr_thrd_create(noop_worker, NULL, &r);
	TEST_ASSERT_NOT_NULL(t);
	TEST_ASSERT_EQUAL_INT(CPR_OK, r);
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_thrd_join(t));
}

static void flag_worker(void *arg)
{
	int *flag = (int *)arg;
	*flag = 1;
}

void test_thread_runs(void)
{
	int flag = 0;
	CprThread *t = cpr_thrd_create(flag_worker, &flag, NULL);
	cpr_thrd_join(t);
	TEST_ASSERT_EQUAL_INT(1, flag);
}

// --- IDs ---

typedef struct {
	CprThreadId self_id;
} IdArgs;

static void id_worker(void *arg)
{
	IdArgs *a = (IdArgs *)arg;
	a->self_id = cpr_thrd_current_id();
}

void test_id_matches_current(void)
{
	IdArgs args = { 0 };
	CprThread *t = cpr_thrd_create(id_worker, &args, NULL);
	CprThreadId id = cpr_thrd_get_id(t);
	cpr_thrd_join(t);
	TEST_ASSERT_EQUAL_UINT64(id, args.self_id);
}

static void id_store_worker(void *arg)
{
	CprThreadId *id = (CprThreadId *)arg;
	*id = cpr_thrd_current_id();
}

void test_distinct_ids(void)
{
	CprThreadId id1 = 0, id2 = 0;
	CprThread *t1 = cpr_thrd_create(id_store_worker, &id1, NULL);
	CprThread *t2 = cpr_thrd_create(id_store_worker, &id2, NULL);
	cpr_thrd_join(t1);
	cpr_thrd_join(t2);
	TEST_ASSERT_NOT_EQUAL(id1, id2);
}

// --- Sleep and yield ---

void test_sleep_zero(void)
{
	cpr_thrd_sleep(0); // must not crash
}

void test_sleep_nonzero(void)
{
	cpr_thrd_sleep(1); // must not crash
}

void test_yield(void)
{
	cpr_thrd_yield(); // must not crash
}

// --- Detach ---

void test_detach(void)
{
	CprThread *t = cpr_thrd_create(noop_worker, NULL, NULL);
	TEST_ASSERT_NOT_NULL(t);
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_thrd_detach(t));
}

// --- TLS ---

void test_tls_destroy_null(void)
{
	cpr_tls_destroy(NULL); // must not crash
}

void test_tls_set_null(void)
{
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_tls_set(NULL, NULL));
}

void test_tls_get_null(void)
{
	TEST_ASSERT_NULL(cpr_tls_get(NULL));
}

void test_tls_create_destroy(void)
{
	CprResult r = CPR_ERR_INVALID;
	CprTls *tls = cpr_tls_create(NULL, &r);
	TEST_ASSERT_NOT_NULL(tls);
	TEST_ASSERT_EQUAL_INT(CPR_OK, r);
	cpr_tls_destroy(tls);
}

void test_tls_set_get(void)
{
	int x = 42;
	CprTls *tls = cpr_tls_create(NULL, NULL);
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_tls_set(tls, &x));
	TEST_ASSERT_EQUAL_PTR(&x, cpr_tls_get(tls));
	cpr_tls_destroy(tls);
}

typedef struct {
	CprTls *tls;
	void *value;
	void *got;
} TlsArgs;

static void tls_rw_worker(void *arg)
{
	TlsArgs *a = (TlsArgs *)arg;
	cpr_tls_set(a->tls, a->value);
	a->got = cpr_tls_get(a->tls);
}

void test_tls_per_thread(void)
{
	int x = 1, y = 2;
	TlsArgs a1, a2;
	CprTls *tls = cpr_tls_create(NULL, NULL);
	CprThread *t1, *t2;

	a1.tls = tls;
	a1.value = &x;
	a1.got = NULL;
	a2.tls = tls;
	a2.value = &y;
	a2.got = NULL;

	t1 = cpr_thrd_create(tls_rw_worker, &a1, NULL);
	t2 = cpr_thrd_create(tls_rw_worker, &a2, NULL);
	cpr_thrd_join(t1);
	cpr_thrd_join(t2);

	TEST_ASSERT_EQUAL_PTR(&x, a1.got);
	TEST_ASSERT_EQUAL_PTR(&y, a2.got);
	cpr_tls_destroy(tls);
}

static volatile int tls_destructor_count = 0;
static int tls_sentinel = 0;

static void tls_destructor(void *value)
{
	(void)value;
	tls_destructor_count++;
}

static void tls_destructor_worker(void *arg)
{
	CprTls *tls = (CprTls *)arg;
	cpr_tls_set(tls, &tls_sentinel);
}

void test_tls_destructor_called(void)
{
	CprTls *tls;
	CprThread *t;

	tls_destructor_count = 0;
	tls = cpr_tls_create(tls_destructor, NULL);
	t = cpr_thrd_create(tls_destructor_worker, tls, NULL);
	cpr_thrd_join(t);
	cpr_tls_destroy(tls);

	TEST_ASSERT_EQUAL_INT(1, tls_destructor_count);
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_create_null_fn);
	RUN_TEST(test_join_null);
	RUN_TEST(test_detach_null);
	RUN_TEST(test_get_id_null);

	RUN_TEST(test_current_id_nonzero);
	RUN_TEST(test_create_join);
	RUN_TEST(test_thread_runs);
	RUN_TEST(test_id_matches_current);
	RUN_TEST(test_distinct_ids);

	RUN_TEST(test_sleep_zero);
	RUN_TEST(test_sleep_nonzero);
	RUN_TEST(test_yield);

	RUN_TEST(test_detach);

	RUN_TEST(test_tls_destroy_null);
	RUN_TEST(test_tls_set_null);
	RUN_TEST(test_tls_get_null);
	RUN_TEST(test_tls_create_destroy);
	RUN_TEST(test_tls_set_get);
	RUN_TEST(test_tls_per_thread);
	RUN_TEST(test_tls_destructor_called);

	return UNITY_END();
}
