#include "unity.h"
#include <copper/copper.h>

void setUp(void)
{
}
void tearDown(void)
{
}

// --- SI Literal Macros ---

void test_kb_to_bytes(void)
{
	TEST_ASSERT_EQUAL_UINT64(1000ULL, cpr_kb(1));
	TEST_ASSERT_EQUAL_UINT64(4000ULL, cpr_kb(4));
}

void test_mb_to_bytes(void)
{
	TEST_ASSERT_EQUAL_UINT64(1000000ULL, cpr_mb(1));
	TEST_ASSERT_EQUAL_UINT64(8000000ULL, cpr_mb(8));
}

void test_gb_to_bytes(void)
{
	TEST_ASSERT_EQUAL_UINT64(1000000000ULL, cpr_gb(1));
}

void test_tb_to_bytes(void)
{
	TEST_ASSERT_EQUAL_UINT64(1000000000000ULL, cpr_tb(1));
}

// --- IEC Literal Macros ---

void test_kib_to_bytes(void)
{
	TEST_ASSERT_EQUAL_UINT64(1024ULL, cpr_kib(1));
	TEST_ASSERT_EQUAL_UINT64(4096ULL, cpr_kib(4));
}

void test_mib_to_bytes(void)
{
	TEST_ASSERT_EQUAL_UINT64(1048576ULL, cpr_mib(1));
}

void test_gib_to_bytes(void)
{
	TEST_ASSERT_EQUAL_UINT64(1073741824ULL, cpr_gib(1));
}

void test_tib_to_bytes(void)
{
	TEST_ASSERT_EQUAL_UINT64(1099511627776ULL, cpr_tib(1));
}

// --- SI: Bytes-to-unit ---

void test_b_to_kb(void)
{
	TEST_ASSERT_EQUAL_UINT64(1ULL, cpr_b_to_kb(1000));
	TEST_ASSERT_EQUAL_UINT64(3ULL, cpr_b_to_kb(3000));
}

void test_b_to_mb(void)
{
	TEST_ASSERT_EQUAL_UINT64(2ULL, cpr_b_to_mb(2000000));
}

void test_b_to_gb(void)
{
	TEST_ASSERT_EQUAL_UINT64(1ULL, cpr_b_to_gb(1000000000));
}

void test_b_to_tb(void)
{
	TEST_ASSERT_EQUAL_UINT64(1ULL, cpr_b_to_tb(1000000000000ULL));
}

// --- IEC: Bytes-to-unit ---

void test_b_to_kib(void)
{
	TEST_ASSERT_EQUAL_UINT64(1ULL, cpr_b_to_kib(1024));
	TEST_ASSERT_EQUAL_UINT64(4ULL, cpr_b_to_kib(4096));
}

void test_b_to_mib(void)
{
	TEST_ASSERT_EQUAL_UINT64(1ULL, cpr_b_to_mib(1048576));
}

void test_b_to_gib(void)
{
	TEST_ASSERT_EQUAL_UINT64(1ULL, cpr_b_to_gib(1073741824));
}

void test_b_to_tib(void)
{
	TEST_ASSERT_EQUAL_UINT64(1ULL, cpr_b_to_tib(1099511627776ULL));
}

// --- SI: Cross-unit ---

void test_kb_to_mb(void)
{
	TEST_ASSERT_EQUAL_UINT64(1ULL, cpr_kb_to_mb(1000));
}

void test_mb_to_kb(void)
{
	TEST_ASSERT_EQUAL_UINT64(2000ULL, cpr_mb_to_kb(2));
}

void test_mb_to_gb(void)
{
	TEST_ASSERT_EQUAL_UINT64(1ULL, cpr_mb_to_gb(1000));
}

void test_gb_to_mb(void)
{
	TEST_ASSERT_EQUAL_UINT64(3000ULL, cpr_gb_to_mb(3));
}

void test_kb_to_gb(void)
{
	TEST_ASSERT_EQUAL_UINT64(1ULL, cpr_kb_to_gb(1000000));
}

void test_gb_to_kb(void)
{
	TEST_ASSERT_EQUAL_UINT64(2000000ULL, cpr_gb_to_kb(2));
}

// --- IEC: Cross-unit ---

void test_kib_to_mib(void)
{
	TEST_ASSERT_EQUAL_UINT64(1ULL, cpr_kib_to_mib(1024));
}

void test_mib_to_kib(void)
{
	TEST_ASSERT_EQUAL_UINT64(2048ULL, cpr_mib_to_kib(2));
}

void test_mib_to_gib(void)
{
	TEST_ASSERT_EQUAL_UINT64(1ULL, cpr_mib_to_gib(1024));
}

void test_gib_to_mib(void)
{
	TEST_ASSERT_EQUAL_UINT64(3072ULL, cpr_gib_to_mib(3));
}

void test_kib_to_gib(void)
{
	TEST_ASSERT_EQUAL_UINT64(1ULL, cpr_kib_to_gib(1048576));
}

void test_gib_to_kib(void)
{
	TEST_ASSERT_EQUAL_UINT64(2097152ULL, cpr_gib_to_kib(2));
}

// --- Round-trip ---

void test_si_round_trip(void)
{
	TEST_ASSERT_EQUAL_UINT64(5ULL, cpr_b_to_mb(cpr_mb(5)));
}

void test_iec_round_trip(void)
{
	TEST_ASSERT_EQUAL_UINT64(7ULL, cpr_b_to_gib(cpr_gib(7)));
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_kb_to_bytes);
	RUN_TEST(test_mb_to_bytes);
	RUN_TEST(test_gb_to_bytes);
	RUN_TEST(test_tb_to_bytes);

	RUN_TEST(test_kib_to_bytes);
	RUN_TEST(test_mib_to_bytes);
	RUN_TEST(test_gib_to_bytes);
	RUN_TEST(test_tib_to_bytes);

	RUN_TEST(test_b_to_kb);
	RUN_TEST(test_b_to_mb);
	RUN_TEST(test_b_to_gb);
	RUN_TEST(test_b_to_tb);

	RUN_TEST(test_b_to_kib);
	RUN_TEST(test_b_to_mib);
	RUN_TEST(test_b_to_gib);
	RUN_TEST(test_b_to_tib);

	RUN_TEST(test_kb_to_mb);
	RUN_TEST(test_mb_to_kb);
	RUN_TEST(test_mb_to_gb);
	RUN_TEST(test_gb_to_mb);
	RUN_TEST(test_kb_to_gb);
	RUN_TEST(test_gb_to_kb);

	RUN_TEST(test_kib_to_mib);
	RUN_TEST(test_mib_to_kib);
	RUN_TEST(test_mib_to_gib);
	RUN_TEST(test_gib_to_mib);
	RUN_TEST(test_kib_to_gib);
	RUN_TEST(test_gib_to_kib);

	RUN_TEST(test_si_round_trip);
	RUN_TEST(test_iec_round_trip);

	return UNITY_END();
}
