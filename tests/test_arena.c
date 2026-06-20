#include "unity.h"
#include <copper/copper.h>
#include <string.h>

void setUp(void)
{
}
void tearDown(void)
{
}

// --- Init ---

void test_init_buf_null_arena(void)
{
	static char buf[64];
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID,
			      cpr_arena_init_buf(NULL, buf, sizeof(buf)));
}

void test_init_buf_null_buf(void)
{
	CprArena arena;
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID,
			      cpr_arena_init_buf(&arena, NULL, 64));
}

void test_init_buf_zero_size(void)
{
	CprArena arena;
	static char buf[64];
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID,
			      cpr_arena_init_buf(&arena, buf, 0));
}

void test_init_buf_success(void)
{
	CprArena arena;
	static char buf[64];
	TEST_ASSERT_EQUAL_INT(CPR_OK,
			      cpr_arena_init_buf(&arena, buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_PTR(buf, arena.buf);
	TEST_ASSERT_EQUAL_size_t(sizeof(buf), arena.cap);
	TEST_ASSERT_EQUAL_size_t(0, arena.offset);
}

void test_init_null_arena(void)
{
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID,
			      cpr_arena_init(NULL, cpr_arena_alloc_default(),
					     64));
}

void test_init_zero_capacity(void)
{
	CprArena arena;
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID,
			      cpr_arena_init(&arena, cpr_arena_alloc_default(),
					     0));
}

void test_init_success(void)
{
	CprArena arena;
	CprResult res = cpr_arena_init(&arena, cpr_arena_alloc_default(), 256);
	TEST_ASSERT_EQUAL_INT(CPR_OK, res);
	TEST_ASSERT_NOT_NULL(arena.buf);
	TEST_ASSERT_EQUAL_size_t(256, arena.cap);
	cpr_arena_free(&arena);
}

// --- Allocation ---

void test_alloc_null_arena(void)
{
	CprResult res;
	void *ptr = cpr_arena_alloc(NULL, 16, &res);
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, res);
	TEST_ASSERT_NULL(ptr);
}

void test_alloc_returns_non_null(void)
{
	CprArena arena;
	static char buf[64];
	cpr_arena_init_buf(&arena, buf, sizeof(buf));

	CprResult res;
	void *ptr = cpr_arena_alloc(&arena, 16, &res);
	TEST_ASSERT_EQUAL_INT(CPR_OK, res);
	TEST_ASSERT_NOT_NULL(ptr);
}

void test_alloc_advances_offset(void)
{
	CprArena arena;
	static char buf[128];
	cpr_arena_init_buf(&arena, buf, sizeof(buf));

	CprResult res;
	cpr_arena_alloc(&arena, 32, &res);
	TEST_ASSERT_EQUAL_INT(CPR_OK, res);
	TEST_ASSERT_GREATER_THAN_size_t(0, arena.offset);
}

void test_alloc_memory_is_writable(void)
{
	CprArena arena;
	static char buf[128];
	cpr_arena_init_buf(&arena, buf, sizeof(buf));

	CprResult res;
	int *p = cpr_arena_alloc(&arena, sizeof(int), &res);
	TEST_ASSERT_EQUAL_INT(CPR_OK, res);
	TEST_ASSERT_NOT_NULL(p);
	*p = 42;
	TEST_ASSERT_EQUAL_INT(42, *p);
}

void test_alloc_exhaustion(void)
{
	CprArena arena;
	static char buf[16];
	cpr_arena_init_buf(&arena, buf, sizeof(buf));

	CprResult res;
	cpr_arena_alloc(&arena, 16, &res);
	TEST_ASSERT_EQUAL_INT(CPR_OK, res);

	void *ptr = cpr_arena_alloc(&arena, 1, &res);
	TEST_ASSERT_EQUAL_INT(CPR_ERR_EXHAUSTED, res);
	TEST_ASSERT_NULL(ptr);
}

void test_alloc_aligned_power_of_two(void)
{
	CprArena arena;
	static char buf[128];
	cpr_arena_init_buf(&arena, buf, sizeof(buf));

	CprResult res;
	void *ptr = cpr_arena_alloc_aligned(&arena, 8, 16, &res);
	TEST_ASSERT_EQUAL_INT(CPR_OK, res);
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_EQUAL_size_t(0, (size_t)ptr % 16);
}

void test_alloc_aligned_non_pow2_rejected(void)
{
	CprArena arena;
	static char buf[128];
	cpr_arena_init_buf(&arena, buf, sizeof(buf));

	CprResult res;
	void *ptr = cpr_arena_alloc_aligned(&arena, 8, 3, &res);
	TEST_ASSERT_EQUAL_INT(CPR_ERR_ALIGN, res);
	TEST_ASSERT_NULL(ptr);
}

void test_alloc_aligned_zero_alignment_rejected(void)
{
	CprArena arena;
	static char buf[128];
	cpr_arena_init_buf(&arena, buf, sizeof(buf));

	CprResult res;
	void *ptr = cpr_arena_alloc_aligned(&arena, 8, 0, &res);
	TEST_ASSERT_EQUAL_INT(CPR_ERR_ALIGN, res);
	TEST_ASSERT_NULL(ptr);
}

void test_alloc_multiple_sequential(void)
{
	CprArena arena;
	static char buf[256];
	cpr_arena_init_buf(&arena, buf, sizeof(buf));

	CprResult res;
	void *a = cpr_arena_alloc(&arena, 16, &res);
	TEST_ASSERT_EQUAL_INT(CPR_OK, res);
	void *b = cpr_arena_alloc(&arena, 16, &res);
	TEST_ASSERT_EQUAL_INT(CPR_OK, res);
	TEST_ASSERT_NOT_EQUAL(a, b);
}

// --- Reset and Rewind ---

void test_reset_allows_reallocation(void)
{
	CprArena arena;
	static char buf[64];
	cpr_arena_init_buf(&arena, buf, sizeof(buf));

	CprResult res;
	cpr_arena_alloc(&arena, 64, &res);
	TEST_ASSERT_EQUAL_INT(CPR_OK, res);

	cpr_arena_reset(&arena);
	TEST_ASSERT_EQUAL_size_t(0, arena.offset);

	void *ptr = cpr_arena_alloc(&arena, 64, &res);
	TEST_ASSERT_EQUAL_INT(CPR_OK, res);
	TEST_ASSERT_NOT_NULL(ptr);
}

void test_rewind_reverts_last_alloc(void)
{
	CprArena arena;
	static char buf[128];
	cpr_arena_init_buf(&arena, buf, sizeof(buf));

	CprResult res;
	cpr_arena_alloc(&arena, 16, &res);
	size_t offset_after_first = arena.offset;

	cpr_arena_alloc(&arena, 32, &res);
	cpr_arena_rewind(&arena);

	TEST_ASSERT_EQUAL_size_t(offset_after_first, arena.offset);
}

void test_rewind_idempotent_after_second_call(void)
{
	CprArena arena;
	static char buf[128];
	cpr_arena_init_buf(&arena, buf, sizeof(buf));

	CprResult res;
	cpr_arena_alloc(&arena, 16, &res);
	cpr_arena_alloc(&arena, 32, &res);
	cpr_arena_rewind(&arena);
	size_t offset_after_first_rewind = arena.offset;

	cpr_arena_rewind(&arena);
	TEST_ASSERT_EQUAL_size_t(offset_after_first_rewind, arena.offset);
}

// --- Free ---

void test_free_zeroes_arena(void)
{
	CprArena arena;
	CprResult res = cpr_arena_init(&arena, cpr_arena_alloc_default(), 64);
	TEST_ASSERT_EQUAL_INT(CPR_OK, res);

	cpr_arena_free(&arena);
	TEST_ASSERT_NULL(arena.buf);
	TEST_ASSERT_EQUAL_size_t(0, arena.cap);
	TEST_ASSERT_EQUAL_size_t(0, arena.offset);
}

void test_free_buf_arena_is_safe(void)
{
	CprArena arena;
	static char buf[64];
	cpr_arena_init_buf(&arena, buf, sizeof(buf));
	cpr_arena_free(&arena);
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_init_buf_null_arena);
	RUN_TEST(test_init_buf_null_buf);
	RUN_TEST(test_init_buf_zero_size);
	RUN_TEST(test_init_buf_success);
	RUN_TEST(test_init_null_arena);
	RUN_TEST(test_init_zero_capacity);
	RUN_TEST(test_init_success);

	RUN_TEST(test_alloc_null_arena);
	RUN_TEST(test_alloc_returns_non_null);
	RUN_TEST(test_alloc_advances_offset);
	RUN_TEST(test_alloc_memory_is_writable);
	RUN_TEST(test_alloc_exhaustion);
	RUN_TEST(test_alloc_aligned_power_of_two);
	RUN_TEST(test_alloc_aligned_non_pow2_rejected);
	RUN_TEST(test_alloc_aligned_zero_alignment_rejected);
	RUN_TEST(test_alloc_multiple_sequential);

	RUN_TEST(test_reset_allows_reallocation);
	RUN_TEST(test_rewind_reverts_last_alloc);
	RUN_TEST(test_rewind_idempotent_after_second_call);

	RUN_TEST(test_free_zeroes_arena);
	RUN_TEST(test_free_buf_arena_is_safe);

	return UNITY_END();
}
