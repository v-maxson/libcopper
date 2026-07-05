#include "unity.h"
#include <copper/copper.h>
#include <stdint.h>

void setUp(void)
{
}
void tearDown(void)
{
}

// --- cpr_bswap16/32/64 ---

void test_bswap16_known_value(void)
{
	TEST_ASSERT_EQUAL_UINT16(0x3412, cpr_bswap16(0x1234));
}

void test_bswap32_known_value(void)
{
	TEST_ASSERT_EQUAL_UINT32(0x78563412u, cpr_bswap32(0x12345678u));
}

void test_bswap64_known_value(void)
{
	TEST_ASSERT_EQUAL_UINT64(0xEFCDAB8967452301ULL,
				  cpr_bswap64(0x0123456789ABCDEFULL));
}

void test_bswap_is_involution(void)
{
	TEST_ASSERT_EQUAL_UINT16(0x1234, cpr_bswap16(cpr_bswap16(0x1234)));
	TEST_ASSERT_EQUAL_UINT32(0x12345678u,
				  cpr_bswap32(cpr_bswap32(0x12345678u)));
	TEST_ASSERT_EQUAL_UINT64(
		0x0123456789ABCDEFULL,
		cpr_bswap64(cpr_bswap64(0x0123456789ABCDEFULL)));
}

// --- Host <-> LE/BE: host-independent invariant ---
// Exactly one of {htoleN, htobeN} is the identity and the other is bswapN,
// whichever the host actually is; this holds true either way.

void test_htole_htobe_invariant_16(void)
{
	TEST_ASSERT_EQUAL_UINT16(cpr_htole16(0x1234),
				  cpr_bswap16(cpr_htobe16(0x1234)));
}

void test_htole_htobe_invariant_32(void)
{
	TEST_ASSERT_EQUAL_UINT32(cpr_htole32(0x12345678u),
				  cpr_bswap32(cpr_htobe32(0x12345678u)));
}

void test_htole_htobe_invariant_64(void)
{
	TEST_ASSERT_EQUAL_UINT64(
		cpr_htole64(0x0123456789ABCDEFULL),
		cpr_bswap64(cpr_htobe64(0x0123456789ABCDEFULL)));
}

void test_letoh_htole_round_trip(void)
{
	TEST_ASSERT_EQUAL_UINT16(0x1234, cpr_letoh16(cpr_htole16(0x1234)));
	TEST_ASSERT_EQUAL_UINT32(0x12345678u,
				  cpr_letoh32(cpr_htole32(0x12345678u)));
	TEST_ASSERT_EQUAL_UINT64(
		0x0123456789ABCDEFULL,
		cpr_letoh64(cpr_htole64(0x0123456789ABCDEFULL)));
}

void test_betoh_htobe_round_trip(void)
{
	TEST_ASSERT_EQUAL_UINT16(0x1234, cpr_betoh16(cpr_htobe16(0x1234)));
	TEST_ASSERT_EQUAL_UINT32(0x12345678u,
				  cpr_betoh32(cpr_htobe32(0x12345678u)));
	TEST_ASSERT_EQUAL_UINT64(
		0x0123456789ABCDEFULL,
		cpr_betoh64(cpr_htobe64(0x0123456789ABCDEFULL)));
}

// --- Host <-> LE/BE: exact values for the build's actual endianness ---

void test_htole_letoh_exact(void)
{
#if defined(CPR_LITTLE_ENDIAN)
	TEST_ASSERT_EQUAL_UINT32(0x12345678u, cpr_htole32(0x12345678u));
	TEST_ASSERT_EQUAL_UINT32(0x12345678u, cpr_letoh32(0x12345678u));
#elif defined(CPR_BIG_ENDIAN)
	TEST_ASSERT_EQUAL_UINT32(cpr_bswap32(0x12345678u),
				  cpr_htole32(0x12345678u));
	TEST_ASSERT_EQUAL_UINT32(cpr_bswap32(0x12345678u),
				  cpr_letoh32(0x12345678u));
#endif
}

void test_htobe_betoh_exact(void)
{
#if defined(CPR_LITTLE_ENDIAN)
	TEST_ASSERT_EQUAL_UINT32(cpr_bswap32(0x12345678u),
				  cpr_htobe32(0x12345678u));
	TEST_ASSERT_EQUAL_UINT32(cpr_bswap32(0x12345678u),
				  cpr_betoh32(0x12345678u));
#elif defined(CPR_BIG_ENDIAN)
	TEST_ASSERT_EQUAL_UINT32(0x12345678u, cpr_htobe32(0x12345678u));
	TEST_ASSERT_EQUAL_UINT32(0x12345678u, cpr_betoh32(0x12345678u));
#endif
}

// --- cpr_read_le16/32/64 / cpr_read_be16/32/64 ---

void test_read_le16(void)
{
	uint8_t buf[2] = { 0x34, 0x12 };
	TEST_ASSERT_EQUAL_UINT16(0x1234, cpr_read_le16(buf));
}

void test_read_be16(void)
{
	uint8_t buf[2] = { 0x12, 0x34 };
	TEST_ASSERT_EQUAL_UINT16(0x1234, cpr_read_be16(buf));
}

void test_read_le32(void)
{
	uint8_t buf[4] = { 0x78, 0x56, 0x34, 0x12 };
	TEST_ASSERT_EQUAL_UINT32(0x12345678u, cpr_read_le32(buf));
}

void test_read_be32(void)
{
	uint8_t buf[4] = { 0x12, 0x34, 0x56, 0x78 };
	TEST_ASSERT_EQUAL_UINT32(0x12345678u, cpr_read_be32(buf));
}

void test_read_le64(void)
{
	uint8_t buf[8] = { 0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01 };
	TEST_ASSERT_EQUAL_UINT64(0x0123456789ABCDEFULL, cpr_read_le64(buf));
}

void test_read_be64(void)
{
	uint8_t buf[8] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF };
	TEST_ASSERT_EQUAL_UINT64(0x0123456789ABCDEFULL, cpr_read_be64(buf));
}

// --- cpr_write_le16/32/64 / cpr_write_be16/32/64 ---

void test_write_le16(void)
{
	uint8_t buf[2] = { 0 };
	uint8_t expected[2] = { 0x34, 0x12 };
	cpr_write_le16(buf, 0x1234);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buf, 2);
}

void test_write_be16(void)
{
	uint8_t buf[2] = { 0 };
	uint8_t expected[2] = { 0x12, 0x34 };
	cpr_write_be16(buf, 0x1234);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buf, 2);
}

void test_write_le32(void)
{
	uint8_t buf[4] = { 0 };
	uint8_t expected[4] = { 0xEF, 0xBE, 0xAD, 0xDE };
	cpr_write_le32(buf, 0xDEADBEEFu);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buf, 4);
}

void test_write_be32(void)
{
	uint8_t buf[4] = { 0 };
	uint8_t expected[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
	cpr_write_be32(buf, 0xDEADBEEFu);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buf, 4);
}

void test_write_le64(void)
{
	uint8_t buf[8] = { 0 };
	uint8_t expected[8] = { 0xEF, 0xCD, 0xAB, 0x89,
				 0x67, 0x45, 0x23, 0x01 };
	cpr_write_le64(buf, 0x0123456789ABCDEFULL);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buf, 8);
}

void test_write_be64(void)
{
	uint8_t buf[8] = { 0 };
	uint8_t expected[8] = { 0x01, 0x23, 0x45, 0x67,
				 0x89, 0xAB, 0xCD, 0xEF };
	cpr_write_be64(buf, 0x0123456789ABCDEFULL);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buf, 8);
}

// --- Round trip through a buffer ---

void test_write_read_round_trip_le(void)
{
	uint8_t buf[8];

	cpr_write_le16(buf, 0xBEEF);
	TEST_ASSERT_EQUAL_UINT16(0xBEEF, cpr_read_le16(buf));

	cpr_write_le32(buf, 0xDEADBEEFu);
	TEST_ASSERT_EQUAL_UINT32(0xDEADBEEFu, cpr_read_le32(buf));

	cpr_write_le64(buf, 0x0123456789ABCDEFULL);
	TEST_ASSERT_EQUAL_UINT64(0x0123456789ABCDEFULL, cpr_read_le64(buf));
}

void test_write_read_round_trip_be(void)
{
	uint8_t buf[8];

	cpr_write_be16(buf, 0xBEEF);
	TEST_ASSERT_EQUAL_UINT16(0xBEEF, cpr_read_be16(buf));

	cpr_write_be32(buf, 0xDEADBEEFu);
	TEST_ASSERT_EQUAL_UINT32(0xDEADBEEFu, cpr_read_be32(buf));

	cpr_write_be64(buf, 0x0123456789ABCDEFULL);
	TEST_ASSERT_EQUAL_UINT64(0x0123456789ABCDEFULL, cpr_read_be64(buf));
}

// --- Unaligned access ---

void test_read_write_unaligned_offsets(void)
{
	uint8_t storage[32];
	int offset;

	for (offset = 0; offset < 8; offset++) {
		uint8_t *p = storage + offset;

		cpr_write_le32(p, 0xCAFEF00Du);
		TEST_ASSERT_EQUAL_UINT32(0xCAFEF00Du, cpr_read_le32(p));

		cpr_write_be64(p, 0x1122334455667788ULL);
		TEST_ASSERT_EQUAL_UINT64(0x1122334455667788ULL,
					  cpr_read_be64(p));
	}
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_bswap16_known_value);
	RUN_TEST(test_bswap32_known_value);
	RUN_TEST(test_bswap64_known_value);
	RUN_TEST(test_bswap_is_involution);

	RUN_TEST(test_htole_htobe_invariant_16);
	RUN_TEST(test_htole_htobe_invariant_32);
	RUN_TEST(test_htole_htobe_invariant_64);
	RUN_TEST(test_letoh_htole_round_trip);
	RUN_TEST(test_betoh_htobe_round_trip);
	RUN_TEST(test_htole_letoh_exact);
	RUN_TEST(test_htobe_betoh_exact);

	RUN_TEST(test_read_le16);
	RUN_TEST(test_read_be16);
	RUN_TEST(test_read_le32);
	RUN_TEST(test_read_be32);
	RUN_TEST(test_read_le64);
	RUN_TEST(test_read_be64);

	RUN_TEST(test_write_le16);
	RUN_TEST(test_write_be16);
	RUN_TEST(test_write_le32);
	RUN_TEST(test_write_be32);
	RUN_TEST(test_write_le64);
	RUN_TEST(test_write_be64);

	RUN_TEST(test_write_read_round_trip_le);
	RUN_TEST(test_write_read_round_trip_be);

	RUN_TEST(test_read_write_unaligned_offsets);

	return UNITY_END();
}
