#include "unity.h"
#include <copper/copper.h>
#include <stdint.h>
#include <string.h>

#define RANGE_ITERATIONS 10000

void setUp(void)
{
}
void tearDown(void)
{
}

// --- cpr_rand_bytes ---

void test_rand_bytes_null_out_nonzero_size(void)
{
	cpr_clear_error();
	TEST_ASSERT_FALSE(cpr_rand_bytes(NULL, 16));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);
}

void test_rand_bytes_null_out_zero_size(void)
{
	cpr_clear_error();
	TEST_ASSERT_TRUE(cpr_rand_bytes(NULL, 0));
}

void test_rand_bytes_zero_size_is_noop(void)
{
	uint8_t buf[4] = { 1, 2, 3, 4 };

	TEST_ASSERT_TRUE(cpr_rand_bytes(buf, 0));
	TEST_ASSERT_EQUAL_UINT8(1, buf[0]);
	TEST_ASSERT_EQUAL_UINT8(2, buf[1]);
	TEST_ASSERT_EQUAL_UINT8(3, buf[2]);
	TEST_ASSERT_EQUAL_UINT8(4, buf[3]);
}

void test_rand_bytes_fills_buffer(void)
{
	uint8_t buf[32];

	TEST_ASSERT_TRUE(cpr_rand_bytes(buf, sizeof(buf)));
}

void test_rand_bytes_two_calls_differ(void)
{
	uint8_t a[32];
	uint8_t b[32];

	TEST_ASSERT_TRUE(cpr_rand_bytes(a, sizeof(a)));
	TEST_ASSERT_TRUE(cpr_rand_bytes(b, sizeof(b)));
	TEST_ASSERT_TRUE(memcmp(a, b, sizeof(a)) != 0);
}

// --- cpr_rand_u32 / cpr_rand_u64 ---

void test_u32_varies(void)
{
	uint32_t first = cpr_rand_u32();
	bool differs = false;
	int i;

	for (i = 0; i < 8; i++) {
		if (cpr_rand_u32() != first) {
			differs = true;
			break;
		}
	}
	TEST_ASSERT_TRUE(differs);
}

void test_u64_varies(void)
{
	uint64_t first = cpr_rand_u64();
	bool differs = false;
	int i;

	for (i = 0; i < 8; i++) {
		if (cpr_rand_u64() != first) {
			differs = true;
			break;
		}
	}
	TEST_ASSERT_TRUE(differs);
}

// --- cpr_rand_f64 ---

void test_f64_in_unit_range(void)
{
	int i;

	for (i = 0; i < RANGE_ITERATIONS; i++) {
		double v = cpr_rand_f64();
		TEST_ASSERT_TRUE(v >= 0.0 && v < 1.0);
	}
}

// --- cpr_randr_u32 ---

static void assert_range_u32(uint32_t min, uint32_t max)
{
	int i;

	for (i = 0; i < RANGE_ITERATIONS; i++) {
		uint32_t v = cpr_randr_u32(min, max);
		TEST_ASSERT_TRUE(v >= min && v <= max);
	}
}

void test_randr_u32_non_power_of_two_width(void)
{
	assert_range_u32(1, 6); // dice roll: width 6
}

void test_randr_u32_power_of_two_width(void)
{
	assert_range_u32(0, 15); // width 16
}

void test_randr_u32_near_max(void)
{
	assert_range_u32(UINT32_MAX - 100, UINT32_MAX);
}

void test_randr_u32_full_range(void)
{
	assert_range_u32(0, UINT32_MAX);
}

void test_randr_u32_min_equals_max(void)
{
	int i;

	for (i = 0; i < 100; i++)
		TEST_ASSERT_EQUAL_UINT32(42, cpr_randr_u32(42, 42));
}

// --- cpr_randr_u64 ---

static void assert_range_u64(uint64_t min, uint64_t max)
{
	int i;

	for (i = 0; i < RANGE_ITERATIONS; i++) {
		uint64_t v = cpr_randr_u64(min, max);
		TEST_ASSERT_TRUE(v >= min && v <= max);
	}
}

void test_randr_u64_non_power_of_two_width(void)
{
	assert_range_u64(1, 6);
}

void test_randr_u64_power_of_two_width(void)
{
	assert_range_u64(0, 15);
}

void test_randr_u64_near_max(void)
{
	assert_range_u64(UINT64_MAX - 100, UINT64_MAX);
}

void test_randr_u64_full_range(void)
{
	assert_range_u64(0, UINT64_MAX);
}

void test_randr_u64_min_equals_max(void)
{
	int i;

	for (i = 0; i < 100; i++)
		TEST_ASSERT_EQUAL_UINT64(42, cpr_randr_u64(42, 42));
}

// --- cpr_randr_i32 ---

static void assert_range_i32(int32_t min, int32_t max)
{
	int i;

	for (i = 0; i < RANGE_ITERATIONS; i++) {
		int32_t v = cpr_randr_i32(min, max);
		TEST_ASSERT_TRUE(v >= min && v <= max);
	}
}

void test_randr_i32_spans_zero(void)
{
	assert_range_i32(-10, 10); // width 21, non-power-of-two
}

void test_randr_i32_all_negative(void)
{
	assert_range_i32(INT32_MIN, INT32_MIN + 50);
}

void test_randr_i32_near_max(void)
{
	assert_range_i32(INT32_MAX - 50, INT32_MAX);
}

void test_randr_i32_full_range(void)
{
	assert_range_i32(INT32_MIN, INT32_MAX);
}

void test_randr_i32_min_equals_max(void)
{
	int i;

	for (i = 0; i < 100; i++)
		TEST_ASSERT_EQUAL_INT32(-5, cpr_randr_i32(-5, -5));
}

// --- cpr_randr_i64 ---

static void assert_range_i64(int64_t min, int64_t max)
{
	int i;

	for (i = 0; i < RANGE_ITERATIONS; i++) {
		int64_t v = cpr_randr_i64(min, max);
		TEST_ASSERT_TRUE(v >= min && v <= max);
	}
}

void test_randr_i64_spans_zero(void)
{
	assert_range_i64(-10, 10);
}

void test_randr_i64_all_negative(void)
{
	assert_range_i64(INT64_MIN, INT64_MIN + 50);
}

void test_randr_i64_near_max(void)
{
	assert_range_i64(INT64_MAX - 50, INT64_MAX);
}

void test_randr_i64_full_range(void)
{
	assert_range_i64(INT64_MIN, INT64_MAX);
}

void test_randr_i64_min_equals_max(void)
{
	int i;

	for (i = 0; i < 100; i++)
		TEST_ASSERT_EQUAL_INT64(-5, cpr_randr_i64(-5, -5));
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_rand_bytes_null_out_nonzero_size);
	RUN_TEST(test_rand_bytes_null_out_zero_size);
	RUN_TEST(test_rand_bytes_zero_size_is_noop);
	RUN_TEST(test_rand_bytes_fills_buffer);
	RUN_TEST(test_rand_bytes_two_calls_differ);

	RUN_TEST(test_u32_varies);
	RUN_TEST(test_u64_varies);

	RUN_TEST(test_f64_in_unit_range);

	RUN_TEST(test_randr_u32_non_power_of_two_width);
	RUN_TEST(test_randr_u32_power_of_two_width);
	RUN_TEST(test_randr_u32_near_max);
	RUN_TEST(test_randr_u32_full_range);
	RUN_TEST(test_randr_u32_min_equals_max);

	RUN_TEST(test_randr_u64_non_power_of_two_width);
	RUN_TEST(test_randr_u64_power_of_two_width);
	RUN_TEST(test_randr_u64_near_max);
	RUN_TEST(test_randr_u64_full_range);
	RUN_TEST(test_randr_u64_min_equals_max);

	RUN_TEST(test_randr_i32_spans_zero);
	RUN_TEST(test_randr_i32_all_negative);
	RUN_TEST(test_randr_i32_near_max);
	RUN_TEST(test_randr_i32_full_range);
	RUN_TEST(test_randr_i32_min_equals_max);

	RUN_TEST(test_randr_i64_spans_zero);
	RUN_TEST(test_randr_i64_all_negative);
	RUN_TEST(test_randr_i64_near_max);
	RUN_TEST(test_randr_i64_full_range);
	RUN_TEST(test_randr_i64_min_equals_max);

	return UNITY_END();
}
