#include "unity.h"
#include <copper/copper.h>

#define TEST_ENV_NAME "CPR_TEST_ENV_VAR"

void setUp(void)
{
	cpr_unset_env(TEST_ENV_NAME);
}
void tearDown(void)
{
	cpr_unset_env(TEST_ENV_NAME);
}

// --- Null guards ---

void test_get_env_null_name(void)
{
	char buf[16];
	cpr_clear_error();
	TEST_ASSERT_FALSE(cpr_get_env(NULL, buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);
}

void test_get_env_null_buf(void)
{
	cpr_clear_error();
	TEST_ASSERT_FALSE(cpr_get_env(TEST_ENV_NAME, NULL, 16));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);
}

void test_get_env_zero_buf_size(void)
{
	char buf[16];
	cpr_clear_error();
	TEST_ASSERT_FALSE(cpr_get_env(TEST_ENV_NAME, buf, 0));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);
}

void test_set_env_null_name(void)
{
	cpr_clear_error();
	TEST_ASSERT_FALSE(cpr_set_env(NULL, "value"));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);
}

void test_set_env_null_value(void)
{
	cpr_clear_error();
	TEST_ASSERT_FALSE(cpr_set_env(TEST_ENV_NAME, NULL));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);
}

void test_unset_env_null_name(void)
{
	cpr_clear_error();
	TEST_ASSERT_FALSE(cpr_unset_env(NULL));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);
}

// --- cpr_get_env ---

void test_get_env_not_found(void)
{
	char buf[16];
	cpr_clear_error();
	TEST_ASSERT_FALSE(cpr_get_env(TEST_ENV_NAME, buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_NOT_FOUND, cpr_get_error().code);
}

void test_get_env_buffer_too_small(void)
{
	char buf[3]; // "hello" needs 6 bytes with the terminator

	TEST_ASSERT_TRUE(cpr_set_env(TEST_ENV_NAME, "hello"));
	cpr_clear_error();
	TEST_ASSERT_FALSE(cpr_get_env(TEST_ENV_NAME, buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_OVERFLOW, cpr_get_error().code);
}

void test_get_env_exact_buffer_size(void)
{
	char buf[3]; // "hi" + '\0', exactly fits

	TEST_ASSERT_TRUE(cpr_set_env(TEST_ENV_NAME, "hi"));
	TEST_ASSERT_TRUE(cpr_get_env(TEST_ENV_NAME, buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING("hi", buf);
}

// --- cpr_set_env / cpr_get_env round trip ---

void test_set_then_get(void)
{
	char buf[32];

	TEST_ASSERT_TRUE(cpr_set_env(TEST_ENV_NAME, "hello"));
	TEST_ASSERT_TRUE(cpr_get_env(TEST_ENV_NAME, buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING("hello", buf);
}

void test_set_overwrites_existing(void)
{
	char buf[32];

	TEST_ASSERT_TRUE(cpr_set_env(TEST_ENV_NAME, "first"));
	TEST_ASSERT_TRUE(cpr_set_env(TEST_ENV_NAME, "second"));
	TEST_ASSERT_TRUE(cpr_get_env(TEST_ENV_NAME, buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING("second", buf);
}

void test_set_env_empty_value(void)
{
	char buf[32];

	TEST_ASSERT_TRUE(cpr_set_env(TEST_ENV_NAME, ""));
	TEST_ASSERT_TRUE(cpr_get_env(TEST_ENV_NAME, buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING("", buf);
}

// --- cpr_unset_env ---

void test_unset_env_removes(void)
{
	char buf[32];

	TEST_ASSERT_TRUE(cpr_set_env(TEST_ENV_NAME, "hello"));
	TEST_ASSERT_TRUE(cpr_unset_env(TEST_ENV_NAME));

	cpr_clear_error();
	TEST_ASSERT_FALSE(cpr_get_env(TEST_ENV_NAME, buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_NOT_FOUND, cpr_get_error().code);
}

void test_unset_env_nonexistent_is_ok(void)
{
	TEST_ASSERT_TRUE(cpr_unset_env(TEST_ENV_NAME));
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_get_env_null_name);
	RUN_TEST(test_get_env_null_buf);
	RUN_TEST(test_get_env_zero_buf_size);
	RUN_TEST(test_set_env_null_name);
	RUN_TEST(test_set_env_null_value);
	RUN_TEST(test_unset_env_null_name);

	RUN_TEST(test_get_env_not_found);
	RUN_TEST(test_get_env_buffer_too_small);
	RUN_TEST(test_get_env_exact_buffer_size);

	RUN_TEST(test_set_then_get);
	RUN_TEST(test_set_overwrites_existing);
	RUN_TEST(test_set_env_empty_value);

	RUN_TEST(test_unset_env_removes);
	RUN_TEST(test_unset_env_nonexistent_is_ok);

	return UNITY_END();
}
