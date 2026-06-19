#include "copper/copper.h"
#include "unity.h"

void setUp(void)
{
}
void tearDown(void)
{
}

void test_version_major_non_negative(void)
{
	TEST_ASSERT_GREATER_OR_EQUAL(0, cpr_version_major());
}

void test_version_string_non_null(void)
{
	TEST_ASSERT_NOT_NULL(cpr_version_string());
}

void test_platform_macro_defined(void)
{
#if !defined(CPR_PLATFORM_WINDOWS) && !defined(CPR_PLATFORM_MACOS) &&     \
	!defined(CPR_PLATFORM_IOS) && !defined(CPR_PLATFORM_ANDROID) &&   \
	!defined(CPR_PLATFORM_LINUX) && !defined(CPR_PLATFORM_FREEBSD) && \
	!defined(CPR_PLATFORM_OPENBSD)
	TEST_FAIL_MESSAGE("No platform macro defined");
#else
	TEST_PASS();
#endif
}

void test_arch_macro_defined(void)
{
#if !defined(CPR_ARCH_X86_64) && !defined(CPR_ARCH_X86) &&    \
	!defined(CPR_ARCH_ARM64) && !defined(CPR_ARCH_ARM) && \
	!defined(CPR_ARCH_RISCV64) && !defined(CPR_ARCH_RISCV32)
	TEST_FAIL_MESSAGE("No architecture macro defined");
#else
	TEST_PASS();
#endif
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_version_major_non_negative);
	RUN_TEST(test_version_string_non_null);
	RUN_TEST(test_platform_macro_defined);
	RUN_TEST(test_arch_macro_defined);
	return UNITY_END();
}
