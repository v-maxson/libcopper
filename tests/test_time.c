#include "unity.h"
#include <copper/copper.h>

void setUp(void)
{
}
void tearDown(void)
{
}

// --- Conversions to milliseconds ---

void test_ns_to_ms(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL,    cpr_ns_to_ms(999999ULL));
	TEST_ASSERT_EQUAL_UINT64(1ULL,    cpr_ns_to_ms(1000000ULL));
	TEST_ASSERT_EQUAL_UINT64(1000ULL, cpr_ns_to_ms(1000000000ULL));
}

void test_us_to_ms(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL,    cpr_us_to_ms(999ULL));
	TEST_ASSERT_EQUAL_UINT64(1ULL,    cpr_us_to_ms(1000ULL));
	TEST_ASSERT_EQUAL_UINT64(1000ULL, cpr_us_to_ms(1000000ULL));
}

void test_s_to_ms(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL,     cpr_s_to_ms(0ULL));
	TEST_ASSERT_EQUAL_UINT64(1000ULL,  cpr_s_to_ms(1ULL));
	TEST_ASSERT_EQUAL_UINT64(60000ULL, cpr_s_to_ms(60ULL));
}

void test_min_to_ms(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL,       cpr_min_to_ms(0ULL));
	TEST_ASSERT_EQUAL_UINT64(60000ULL,   cpr_min_to_ms(1ULL));
	TEST_ASSERT_EQUAL_UINT64(3600000ULL, cpr_min_to_ms(60ULL));
}

void test_hr_to_ms(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL,       cpr_hr_to_ms(0ULL));
	TEST_ASSERT_EQUAL_UINT64(3600000ULL, cpr_hr_to_ms(1ULL));
	TEST_ASSERT_EQUAL_UINT64(7200000ULL, cpr_hr_to_ms(2ULL));
}

// --- Conversions from milliseconds ---

void test_ms_to_ns(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL,          cpr_ms_to_ns(0ULL));
	TEST_ASSERT_EQUAL_UINT64(1000000ULL,    cpr_ms_to_ns(1ULL));
	TEST_ASSERT_EQUAL_UINT64(500000000ULL,  cpr_ms_to_ns(500ULL));
}

void test_ms_to_us(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL,    cpr_ms_to_us(0ULL));
	TEST_ASSERT_EQUAL_UINT64(1000ULL, cpr_ms_to_us(1ULL));
	TEST_ASSERT_EQUAL_UINT64(5000ULL, cpr_ms_to_us(5ULL));
}

void test_ms_to_s(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL, cpr_ms_to_s(0ULL));
	TEST_ASSERT_EQUAL_UINT64(0ULL, cpr_ms_to_s(999ULL));
	TEST_ASSERT_EQUAL_UINT64(1ULL, cpr_ms_to_s(1000ULL));
	TEST_ASSERT_EQUAL_UINT64(1ULL, cpr_ms_to_s(1001ULL));
}

void test_ms_to_min(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL, cpr_ms_to_min(59999ULL));
	TEST_ASSERT_EQUAL_UINT64(1ULL, cpr_ms_to_min(60000ULL));
	TEST_ASSERT_EQUAL_UINT64(2ULL, cpr_ms_to_min(120000ULL));
}

void test_ms_to_hr(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL, cpr_ms_to_hr(3599999ULL));
	TEST_ASSERT_EQUAL_UINT64(1ULL, cpr_ms_to_hr(3600000ULL));
	TEST_ASSERT_EQUAL_UINT64(2ULL, cpr_ms_to_hr(7200000ULL));
}

// --- Other conversions ---

void test_ns_us(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL,    cpr_ns_to_us(999ULL));
	TEST_ASSERT_EQUAL_UINT64(1ULL,    cpr_ns_to_us(1000ULL));
	TEST_ASSERT_EQUAL_UINT64(1000ULL, cpr_us_to_ns(1ULL));
}

void test_ns_s(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL,           cpr_ns_to_s(999999999ULL));
	TEST_ASSERT_EQUAL_UINT64(1ULL,           cpr_ns_to_s(1000000000ULL));
	TEST_ASSERT_EQUAL_UINT64(1000000000ULL,  cpr_s_to_ns(1ULL));
}

void test_ns_min(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL,            cpr_ns_to_min(59999999999ULL));
	TEST_ASSERT_EQUAL_UINT64(1ULL,            cpr_ns_to_min(60000000000ULL));
	TEST_ASSERT_EQUAL_UINT64(60000000000ULL,  cpr_min_to_ns(1ULL));
}

void test_ns_hr(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL,              cpr_ns_to_hr(3599999999999ULL));
	TEST_ASSERT_EQUAL_UINT64(1ULL,              cpr_ns_to_hr(3600000000000ULL));
	TEST_ASSERT_EQUAL_UINT64(3600000000000ULL,  cpr_hr_to_ns(1ULL));
}

void test_us_s(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL,      cpr_us_to_s(999999ULL));
	TEST_ASSERT_EQUAL_UINT64(1ULL,      cpr_us_to_s(1000000ULL));
	TEST_ASSERT_EQUAL_UINT64(1000000ULL, cpr_s_to_us(1ULL));
}

void test_s_min(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL,  cpr_s_to_min(59ULL));
	TEST_ASSERT_EQUAL_UINT64(1ULL,  cpr_s_to_min(60ULL));
	TEST_ASSERT_EQUAL_UINT64(60ULL, cpr_min_to_s(1ULL));
}

void test_s_hr(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL,    cpr_s_to_hr(3599ULL));
	TEST_ASSERT_EQUAL_UINT64(1ULL,    cpr_s_to_hr(3600ULL));
	TEST_ASSERT_EQUAL_UINT64(3600ULL, cpr_hr_to_s(1ULL));
}

void test_min_hr(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL,  cpr_min_to_hr(59ULL));
	TEST_ASSERT_EQUAL_UINT64(1ULL,  cpr_min_to_hr(60ULL));
	TEST_ASSERT_EQUAL_UINT64(60ULL, cpr_hr_to_min(1ULL));
}

// --- Literal duration macros (produce milliseconds) ---

void test_literal_ms(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL,   cpr_ms(0ULL));
	TEST_ASSERT_EQUAL_UINT64(500ULL, cpr_ms(500ULL));
}

void test_literal_us(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL, cpr_us(999ULL));
	TEST_ASSERT_EQUAL_UINT64(1ULL, cpr_us(1000ULL));
	TEST_ASSERT_EQUAL_UINT64(2ULL, cpr_us(2000ULL));
}

void test_literal_s(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL,    cpr_s(0ULL));
	TEST_ASSERT_EQUAL_UINT64(3000ULL, cpr_s(3ULL));
}

void test_literal_min(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL,      cpr_min(0ULL));
	TEST_ASSERT_EQUAL_UINT64(120000ULL, cpr_min(2ULL));
}

void test_literal_hr(void)
{
	TEST_ASSERT_EQUAL_UINT64(0ULL,       cpr_hr(0ULL));
	TEST_ASSERT_EQUAL_UINT64(3600000ULL, cpr_hr(1ULL));
}

// --- Round-trips ---

void test_roundtrip_ms_s(void)
{
	TEST_ASSERT_EQUAL_UINT64(5ULL,    cpr_ms_to_s(cpr_s_to_ms(5ULL)));
	TEST_ASSERT_EQUAL_UINT64(5000ULL, cpr_s_to_ms(cpr_ms_to_s(5000ULL)));
}

void test_roundtrip_s_min(void)
{
	TEST_ASSERT_EQUAL_UINT64(2ULL,   cpr_s_to_min(cpr_min_to_s(2ULL)));
	TEST_ASSERT_EQUAL_UINT64(120ULL, cpr_min_to_s(cpr_s_to_min(120ULL)));
}

void test_roundtrip_min_hr(void)
{
	TEST_ASSERT_EQUAL_UINT64(1ULL,  cpr_min_to_hr(cpr_hr_to_min(1ULL)));
	TEST_ASSERT_EQUAL_UINT64(60ULL, cpr_hr_to_min(cpr_min_to_hr(60ULL)));
}

void test_roundtrip_ns_s(void)
{
	TEST_ASSERT_EQUAL_UINT64(5000000000ULL, cpr_s_to_ns(cpr_ns_to_s(5000000000ULL)));
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_ns_to_ms);
	RUN_TEST(test_us_to_ms);
	RUN_TEST(test_s_to_ms);
	RUN_TEST(test_min_to_ms);
	RUN_TEST(test_hr_to_ms);

	RUN_TEST(test_ms_to_ns);
	RUN_TEST(test_ms_to_us);
	RUN_TEST(test_ms_to_s);
	RUN_TEST(test_ms_to_min);
	RUN_TEST(test_ms_to_hr);

	RUN_TEST(test_ns_us);
	RUN_TEST(test_ns_s);
	RUN_TEST(test_ns_min);
	RUN_TEST(test_ns_hr);
	RUN_TEST(test_us_s);
	RUN_TEST(test_s_min);
	RUN_TEST(test_s_hr);
	RUN_TEST(test_min_hr);

	RUN_TEST(test_literal_ms);
	RUN_TEST(test_literal_us);
	RUN_TEST(test_literal_s);
	RUN_TEST(test_literal_min);
	RUN_TEST(test_literal_hr);

	RUN_TEST(test_roundtrip_ms_s);
	RUN_TEST(test_roundtrip_s_min);
	RUN_TEST(test_roundtrip_min_hr);
	RUN_TEST(test_roundtrip_ns_s);

	return UNITY_END();
}
