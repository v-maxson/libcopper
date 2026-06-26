#include "unity.h"
#include <copper/copper.h>
#include <stdint.h>

void setUp(void)
{
}
void tearDown(void)
{
}

// --- Null guards ---

void test_init_null(void)
{
	cpr_clear_error();
	TEST_ASSERT_FALSE(cpr_atomici32_init(NULL, 0));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);
	cpr_clear_error();
	TEST_ASSERT_FALSE(cpr_atomicu32_init(NULL, 0));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);
	cpr_clear_error();
	TEST_ASSERT_FALSE(cpr_atomici64_init(NULL, 0));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);
	cpr_clear_error();
	TEST_ASSERT_FALSE(cpr_atomicu64_init(NULL, 0));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);
	cpr_clear_error();
	TEST_ASSERT_FALSE(cpr_atomicptr_init(NULL, NULL));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);
}

void test_destroy_null(void)
{
	cpr_atomici32_destroy(NULL); // must not crash
	cpr_atomicu32_destroy(NULL);
	cpr_atomici64_destroy(NULL);
	cpr_atomicu64_destroy(NULL);
	cpr_atomicptr_destroy(NULL);
}

// --- CprAtomicI32 ---

void test_i32_init_load(void)
{
	CprAtomicI32 a;
	cpr_atomici32_init(&a, 42);
	TEST_ASSERT_EQUAL_INT32(42, cpr_atomici32_load(&a));
	cpr_atomici32_destroy(&a);
}

void test_i32_store_load(void)
{
	CprAtomicI32 a;
	cpr_atomici32_init(&a, 0);
	cpr_atomici32_store(&a, -99);
	TEST_ASSERT_EQUAL_INT32(-99, cpr_atomici32_load(&a));
	cpr_atomici32_destroy(&a);
}

void test_i32_exchange(void)
{
	CprAtomicI32 a;
	int32_t prev;
	cpr_atomici32_init(&a, 10);
	prev = cpr_atomici32_exchange(&a, 20);
	TEST_ASSERT_EQUAL_INT32(10, prev);
	TEST_ASSERT_EQUAL_INT32(20, cpr_atomici32_load(&a));
	cpr_atomici32_destroy(&a);
}

void test_i32_compare_exchange_success(void)
{
	CprAtomicI32 a;
	int32_t expected = 5;
	cpr_atomici32_init(&a, 5);
	TEST_ASSERT_TRUE(cpr_atomici32_compare_exchange(&a, &expected, 99));
	TEST_ASSERT_EQUAL_INT32(99, cpr_atomici32_load(&a));
	cpr_atomici32_destroy(&a);
}

void test_i32_compare_exchange_failure(void)
{
	CprAtomicI32 a;
	int32_t expected = 0;
	cpr_atomici32_init(&a, 5);
	TEST_ASSERT_FALSE(cpr_atomici32_compare_exchange(&a, &expected, 99));
	TEST_ASSERT_EQUAL_INT32(5, expected); // updated to actual
	TEST_ASSERT_EQUAL_INT32(5, cpr_atomici32_load(&a)); // unchanged
	cpr_atomici32_destroy(&a);
}

void test_i32_fetch_add(void)
{
	CprAtomicI32 a;
	cpr_atomici32_init(&a, 10);
	TEST_ASSERT_EQUAL_INT32(10, cpr_atomici32_fetch_add(&a, 5));
	TEST_ASSERT_EQUAL_INT32(15, cpr_atomici32_load(&a));
	cpr_atomici32_destroy(&a);
}

void test_i32_fetch_sub(void)
{
	CprAtomicI32 a;
	cpr_atomici32_init(&a, 10);
	TEST_ASSERT_EQUAL_INT32(10, cpr_atomici32_fetch_sub(&a, 3));
	TEST_ASSERT_EQUAL_INT32(7, cpr_atomici32_load(&a));
	cpr_atomici32_destroy(&a);
}

void test_i32_fetch_and(void)
{
	CprAtomicI32 a;
	cpr_atomici32_init(&a, 0xFF);
	TEST_ASSERT_EQUAL_INT32(0xFF, cpr_atomici32_fetch_and(&a, 0x0F));
	TEST_ASSERT_EQUAL_INT32(0x0F, cpr_atomici32_load(&a));
	cpr_atomici32_destroy(&a);
}

void test_i32_fetch_or(void)
{
	CprAtomicI32 a;
	cpr_atomici32_init(&a, 0xF0);
	TEST_ASSERT_EQUAL_INT32(0xF0, cpr_atomici32_fetch_or(&a, 0x0F));
	TEST_ASSERT_EQUAL_INT32(0xFF, cpr_atomici32_load(&a));
	cpr_atomici32_destroy(&a);
}

void test_i32_fetch_xor(void)
{
	CprAtomicI32 a;
	cpr_atomici32_init(&a, 0xFF);
	TEST_ASSERT_EQUAL_INT32(0xFF, cpr_atomici32_fetch_xor(&a, 0x0F));
	TEST_ASSERT_EQUAL_INT32(0xF0, cpr_atomici32_load(&a));
	cpr_atomici32_destroy(&a);
}

// --- CprAtomicU32 ---

void test_u32_init_load(void)
{
	CprAtomicU32 a;
	cpr_atomicu32_init(&a, 0xDEADBEEFU);
	TEST_ASSERT_EQUAL_UINT32(0xDEADBEEFU, cpr_atomicu32_load(&a));
	cpr_atomicu32_destroy(&a);
}

void test_u32_store_load(void)
{
	CprAtomicU32 a;
	cpr_atomicu32_init(&a, 0);
	cpr_atomicu32_store(&a, 0xFFFFFFFFU);
	TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFU, cpr_atomicu32_load(&a));
	cpr_atomicu32_destroy(&a);
}

void test_u32_exchange(void)
{
	CprAtomicU32 a;
	uint32_t prev;
	cpr_atomicu32_init(&a, 1);
	prev = cpr_atomicu32_exchange(&a, 2);
	TEST_ASSERT_EQUAL_UINT32(1, prev);
	TEST_ASSERT_EQUAL_UINT32(2, cpr_atomicu32_load(&a));
	cpr_atomicu32_destroy(&a);
}

void test_u32_compare_exchange_success(void)
{
	CprAtomicU32 a;
	uint32_t expected = 7;
	cpr_atomicu32_init(&a, 7);
	TEST_ASSERT_TRUE(cpr_atomicu32_compare_exchange(&a, &expected, 42));
	TEST_ASSERT_EQUAL_UINT32(42, cpr_atomicu32_load(&a));
	cpr_atomicu32_destroy(&a);
}

void test_u32_compare_exchange_failure(void)
{
	CprAtomicU32 a;
	uint32_t expected = 0;
	cpr_atomicu32_init(&a, 7);
	TEST_ASSERT_FALSE(cpr_atomicu32_compare_exchange(&a, &expected, 42));
	TEST_ASSERT_EQUAL_UINT32(7, expected);
	TEST_ASSERT_EQUAL_UINT32(7, cpr_atomicu32_load(&a));
	cpr_atomicu32_destroy(&a);
}

void test_u32_fetch_add(void)
{
	CprAtomicU32 a;
	cpr_atomicu32_init(&a, 10);
	TEST_ASSERT_EQUAL_UINT32(10, cpr_atomicu32_fetch_add(&a, 5));
	TEST_ASSERT_EQUAL_UINT32(15, cpr_atomicu32_load(&a));
	cpr_atomicu32_destroy(&a);
}

void test_u32_fetch_sub(void)
{
	CprAtomicU32 a;
	cpr_atomicu32_init(&a, 10);
	TEST_ASSERT_EQUAL_UINT32(10, cpr_atomicu32_fetch_sub(&a, 3));
	TEST_ASSERT_EQUAL_UINT32(7, cpr_atomicu32_load(&a));
	cpr_atomicu32_destroy(&a);
}

void test_u32_fetch_and(void)
{
	CprAtomicU32 a;
	cpr_atomicu32_init(&a, 0xFF);
	TEST_ASSERT_EQUAL_UINT32(0xFF, cpr_atomicu32_fetch_and(&a, 0x0F));
	TEST_ASSERT_EQUAL_UINT32(0x0F, cpr_atomicu32_load(&a));
	cpr_atomicu32_destroy(&a);
}

void test_u32_fetch_or(void)
{
	CprAtomicU32 a;
	cpr_atomicu32_init(&a, 0xF0);
	TEST_ASSERT_EQUAL_UINT32(0xF0, cpr_atomicu32_fetch_or(&a, 0x0F));
	TEST_ASSERT_EQUAL_UINT32(0xFF, cpr_atomicu32_load(&a));
	cpr_atomicu32_destroy(&a);
}

void test_u32_fetch_xor(void)
{
	CprAtomicU32 a;
	cpr_atomicu32_init(&a, 0xFF);
	TEST_ASSERT_EQUAL_UINT32(0xFF, cpr_atomicu32_fetch_xor(&a, 0x0F));
	TEST_ASSERT_EQUAL_UINT32(0xF0, cpr_atomicu32_load(&a));
	cpr_atomicu32_destroy(&a);
}

// --- CprAtomicI64 ---

void test_i64_init_load(void)
{
	CprAtomicI64 a;
	cpr_atomici64_init(&a, -1LL);
	TEST_ASSERT_EQUAL_INT64(-1LL, cpr_atomici64_load(&a));
	cpr_atomici64_destroy(&a);
}

void test_i64_store_load(void)
{
	CprAtomicI64 a;
	cpr_atomici64_init(&a, 0);
	cpr_atomici64_store(&a, INT64_MIN);
	TEST_ASSERT_EQUAL_INT64(INT64_MIN, cpr_atomici64_load(&a));
	cpr_atomici64_destroy(&a);
}

void test_i64_exchange(void)
{
	CprAtomicI64 a;
	int64_t prev;
	cpr_atomici64_init(&a, 100);
	prev = cpr_atomici64_exchange(&a, 200);
	TEST_ASSERT_EQUAL_INT64(100, prev);
	TEST_ASSERT_EQUAL_INT64(200, cpr_atomici64_load(&a));
	cpr_atomici64_destroy(&a);
}

void test_i64_compare_exchange_success(void)
{
	CprAtomicI64 a;
	int64_t expected = 5;
	cpr_atomici64_init(&a, 5);
	TEST_ASSERT_TRUE(cpr_atomici64_compare_exchange(&a, &expected, 999));
	TEST_ASSERT_EQUAL_INT64(999, cpr_atomici64_load(&a));
	cpr_atomici64_destroy(&a);
}

void test_i64_compare_exchange_failure(void)
{
	CprAtomicI64 a;
	int64_t expected = 0;
	cpr_atomici64_init(&a, 5);
	TEST_ASSERT_FALSE(cpr_atomici64_compare_exchange(&a, &expected, 999));
	TEST_ASSERT_EQUAL_INT64(5, expected);
	TEST_ASSERT_EQUAL_INT64(5, cpr_atomici64_load(&a));
	cpr_atomici64_destroy(&a);
}

void test_i64_fetch_add(void)
{
	CprAtomicI64 a;
	cpr_atomici64_init(&a, 10);
	TEST_ASSERT_EQUAL_INT64(10, cpr_atomici64_fetch_add(&a, 5));
	TEST_ASSERT_EQUAL_INT64(15, cpr_atomici64_load(&a));
	cpr_atomici64_destroy(&a);
}

void test_i64_fetch_sub(void)
{
	CprAtomicI64 a;
	cpr_atomici64_init(&a, 10);
	TEST_ASSERT_EQUAL_INT64(10, cpr_atomici64_fetch_sub(&a, 3));
	TEST_ASSERT_EQUAL_INT64(7, cpr_atomici64_load(&a));
	cpr_atomici64_destroy(&a);
}

void test_i64_fetch_and(void)
{
	CprAtomicI64 a;
	cpr_atomici64_init(&a, 0xFF);
	TEST_ASSERT_EQUAL_INT64(0xFF, cpr_atomici64_fetch_and(&a, 0x0F));
	TEST_ASSERT_EQUAL_INT64(0x0F, cpr_atomici64_load(&a));
	cpr_atomici64_destroy(&a);
}

void test_i64_fetch_or(void)
{
	CprAtomicI64 a;
	cpr_atomici64_init(&a, 0xF0);
	TEST_ASSERT_EQUAL_INT64(0xF0, cpr_atomici64_fetch_or(&a, 0x0F));
	TEST_ASSERT_EQUAL_INT64(0xFF, cpr_atomici64_load(&a));
	cpr_atomici64_destroy(&a);
}

void test_i64_fetch_xor(void)
{
	CprAtomicI64 a;
	cpr_atomici64_init(&a, 0xFF);
	TEST_ASSERT_EQUAL_INT64(0xFF, cpr_atomici64_fetch_xor(&a, 0x0F));
	TEST_ASSERT_EQUAL_INT64(0xF0, cpr_atomici64_load(&a));
	cpr_atomici64_destroy(&a);
}

// --- CprAtomicU64 ---

void test_u64_init_load(void)
{
	CprAtomicU64 a;
	cpr_atomicu64_init(&a, UINT64_MAX);
	TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, cpr_atomicu64_load(&a));
	cpr_atomicu64_destroy(&a);
}

void test_u64_store_load(void)
{
	CprAtomicU64 a;
	cpr_atomicu64_init(&a, 0);
	cpr_atomicu64_store(&a, UINT64_MAX);
	TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, cpr_atomicu64_load(&a));
	cpr_atomicu64_destroy(&a);
}

void test_u64_exchange(void)
{
	CprAtomicU64 a;
	uint64_t prev;
	cpr_atomicu64_init(&a, 1);
	prev = cpr_atomicu64_exchange(&a, 2);
	TEST_ASSERT_EQUAL_UINT64(1, prev);
	TEST_ASSERT_EQUAL_UINT64(2, cpr_atomicu64_load(&a));
	cpr_atomicu64_destroy(&a);
}

void test_u64_compare_exchange_success(void)
{
	CprAtomicU64 a;
	uint64_t expected = 7;
	cpr_atomicu64_init(&a, 7);
	TEST_ASSERT_TRUE(cpr_atomicu64_compare_exchange(&a, &expected, 42));
	TEST_ASSERT_EQUAL_UINT64(42, cpr_atomicu64_load(&a));
	cpr_atomicu64_destroy(&a);
}

void test_u64_compare_exchange_failure(void)
{
	CprAtomicU64 a;
	uint64_t expected = 0;
	cpr_atomicu64_init(&a, 7);
	TEST_ASSERT_FALSE(cpr_atomicu64_compare_exchange(&a, &expected, 42));
	TEST_ASSERT_EQUAL_UINT64(7, expected);
	TEST_ASSERT_EQUAL_UINT64(7, cpr_atomicu64_load(&a));
	cpr_atomicu64_destroy(&a);
}

void test_u64_fetch_add(void)
{
	CprAtomicU64 a;
	cpr_atomicu64_init(&a, 10);
	TEST_ASSERT_EQUAL_UINT64(10, cpr_atomicu64_fetch_add(&a, 5));
	TEST_ASSERT_EQUAL_UINT64(15, cpr_atomicu64_load(&a));
	cpr_atomicu64_destroy(&a);
}

void test_u64_fetch_sub(void)
{
	CprAtomicU64 a;
	cpr_atomicu64_init(&a, 10);
	TEST_ASSERT_EQUAL_UINT64(10, cpr_atomicu64_fetch_sub(&a, 3));
	TEST_ASSERT_EQUAL_UINT64(7, cpr_atomicu64_load(&a));
	cpr_atomicu64_destroy(&a);
}

void test_u64_fetch_and(void)
{
	CprAtomicU64 a;
	cpr_atomicu64_init(&a, 0xFF);
	TEST_ASSERT_EQUAL_UINT64(0xFF, cpr_atomicu64_fetch_and(&a, 0x0F));
	TEST_ASSERT_EQUAL_UINT64(0x0F, cpr_atomicu64_load(&a));
	cpr_atomicu64_destroy(&a);
}

void test_u64_fetch_or(void)
{
	CprAtomicU64 a;
	cpr_atomicu64_init(&a, 0xF0);
	TEST_ASSERT_EQUAL_UINT64(0xF0, cpr_atomicu64_fetch_or(&a, 0x0F));
	TEST_ASSERT_EQUAL_UINT64(0xFF, cpr_atomicu64_load(&a));
	cpr_atomicu64_destroy(&a);
}

void test_u64_fetch_xor(void)
{
	CprAtomicU64 a;
	cpr_atomicu64_init(&a, 0xFF);
	TEST_ASSERT_EQUAL_UINT64(0xFF, cpr_atomicu64_fetch_xor(&a, 0x0F));
	TEST_ASSERT_EQUAL_UINT64(0xF0, cpr_atomicu64_load(&a));
	cpr_atomicu64_destroy(&a);
}

// --- CprAtomicPtr ---

void test_ptr_init_load(void)
{
	CprAtomicPtr a;
	int x;
	cpr_atomicptr_init(&a, &x);
	TEST_ASSERT_EQUAL_PTR(&x, cpr_atomicptr_load(&a));
	cpr_atomicptr_destroy(&a);
}

void test_ptr_store_load(void)
{
	CprAtomicPtr a;
	int x, y;
	cpr_atomicptr_init(&a, &x);
	cpr_atomicptr_store(&a, &y);
	TEST_ASSERT_EQUAL_PTR(&y, cpr_atomicptr_load(&a));
	cpr_atomicptr_destroy(&a);
}

void test_ptr_null_value(void)
{
	CprAtomicPtr a;
	cpr_atomicptr_init(&a, NULL);
	TEST_ASSERT_NULL(cpr_atomicptr_load(&a));
	cpr_atomicptr_destroy(&a);
}

void test_ptr_exchange(void)
{
	CprAtomicPtr a;
	int x, y;
	void *prev;
	cpr_atomicptr_init(&a, &x);
	prev = cpr_atomicptr_exchange(&a, &y);
	TEST_ASSERT_EQUAL_PTR(&x, prev);
	TEST_ASSERT_EQUAL_PTR(&y, cpr_atomicptr_load(&a));
	cpr_atomicptr_destroy(&a);
}

void test_ptr_compare_exchange_success(void)
{
	CprAtomicPtr a;
	int x, y;
	void *expected = &x;
	cpr_atomicptr_init(&a, &x);
	TEST_ASSERT_TRUE(cpr_atomicptr_compare_exchange(&a, &expected, &y));
	TEST_ASSERT_EQUAL_PTR(&y, cpr_atomicptr_load(&a));
	cpr_atomicptr_destroy(&a);
}

void test_ptr_compare_exchange_failure(void)
{
	CprAtomicPtr a;
	int x, y, z;
	void *expected = &z;
	cpr_atomicptr_init(&a, &x);
	TEST_ASSERT_FALSE(cpr_atomicptr_compare_exchange(&a, &expected, &y));
	TEST_ASSERT_EQUAL_PTR(&x, expected); // updated to actual
	TEST_ASSERT_EQUAL_PTR(&x, cpr_atomicptr_load(&a)); // unchanged
	cpr_atomicptr_destroy(&a);
}

// --- Concurrent increment (I32) ---

#define NTHREADS 4
#define INCREMENTS 10000

typedef struct {
	CprAtomicI32 *counter;
} AtomicWorkerArgs;

static void cpr__atomic_worker(void *arg)
{
	AtomicWorkerArgs *a = (AtomicWorkerArgs *)arg;
	int i;
	for (i = 0; i < INCREMENTS; i++)
		cpr_atomici32_fetch_add(a->counter, 1);
}

void test_i32_concurrent_increment(void)
{
	CprAtomicI32 counter;
	AtomicWorkerArgs args[NTHREADS];
	CprThread *threads[NTHREADS];
	int i;

	cpr_atomici32_init(&counter, 0);
	for (i = 0; i < NTHREADS; i++) {
		args[i].counter = &counter;
		threads[i] = cpr_thrd_create(cpr__atomic_worker, &args[i]);
	}
	for (i = 0; i < NTHREADS; i++)
		cpr_thrd_join(threads[i]);

	TEST_ASSERT_EQUAL_INT32(NTHREADS * INCREMENTS,
				cpr_atomici32_load(&counter));
	cpr_atomici32_destroy(&counter);
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_init_null);
	RUN_TEST(test_destroy_null);

	RUN_TEST(test_i32_init_load);
	RUN_TEST(test_i32_store_load);
	RUN_TEST(test_i32_exchange);
	RUN_TEST(test_i32_compare_exchange_success);
	RUN_TEST(test_i32_compare_exchange_failure);
	RUN_TEST(test_i32_fetch_add);
	RUN_TEST(test_i32_fetch_sub);
	RUN_TEST(test_i32_fetch_and);
	RUN_TEST(test_i32_fetch_or);
	RUN_TEST(test_i32_fetch_xor);

	RUN_TEST(test_u32_init_load);
	RUN_TEST(test_u32_store_load);
	RUN_TEST(test_u32_exchange);
	RUN_TEST(test_u32_compare_exchange_success);
	RUN_TEST(test_u32_compare_exchange_failure);
	RUN_TEST(test_u32_fetch_add);
	RUN_TEST(test_u32_fetch_sub);
	RUN_TEST(test_u32_fetch_and);
	RUN_TEST(test_u32_fetch_or);
	RUN_TEST(test_u32_fetch_xor);

	RUN_TEST(test_i64_init_load);
	RUN_TEST(test_i64_store_load);
	RUN_TEST(test_i64_exchange);
	RUN_TEST(test_i64_compare_exchange_success);
	RUN_TEST(test_i64_compare_exchange_failure);
	RUN_TEST(test_i64_fetch_add);
	RUN_TEST(test_i64_fetch_sub);
	RUN_TEST(test_i64_fetch_and);
	RUN_TEST(test_i64_fetch_or);
	RUN_TEST(test_i64_fetch_xor);

	RUN_TEST(test_u64_init_load);
	RUN_TEST(test_u64_store_load);
	RUN_TEST(test_u64_exchange);
	RUN_TEST(test_u64_compare_exchange_success);
	RUN_TEST(test_u64_compare_exchange_failure);
	RUN_TEST(test_u64_fetch_add);
	RUN_TEST(test_u64_fetch_sub);
	RUN_TEST(test_u64_fetch_and);
	RUN_TEST(test_u64_fetch_or);
	RUN_TEST(test_u64_fetch_xor);

	RUN_TEST(test_ptr_init_load);
	RUN_TEST(test_ptr_store_load);
	RUN_TEST(test_ptr_null_value);
	RUN_TEST(test_ptr_exchange);
	RUN_TEST(test_ptr_compare_exchange_success);
	RUN_TEST(test_ptr_compare_exchange_failure);

	RUN_TEST(test_i32_concurrent_increment);

	return UNITY_END();
}
