#include "unity.h"
#include <copper/copper.h>
#include <stdint.h>
#include <string.h>

void setUp(void)
{
}
void tearDown(void)
{
}

// --- cpr_ringbuf_init / cpr_ringbuf_destroy ---

void test_init_null_rb(void)
{
	uint8_t buf[64];

	cpr_clear_error();
	TEST_ASSERT_FALSE(cpr_ringbuf_init(NULL, buf, 64));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);
}

void test_init_null_buf(void)
{
	CprRingBuffer rb;

	cpr_clear_error();
	TEST_ASSERT_FALSE(cpr_ringbuf_init(&rb, NULL, 64));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);
}

void test_init_zero_capacity(void)
{
	CprRingBuffer rb;
	uint8_t buf[1];

	cpr_clear_error();
	TEST_ASSERT_FALSE(cpr_ringbuf_init(&rb, buf, 0));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);
}

void test_init_non_power_of_two(void)
{
	CprRingBuffer rb;
	uint8_t buf[100];

	cpr_clear_error();
	TEST_ASSERT_FALSE(cpr_ringbuf_init(&rb, buf, 100));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);
}

void test_init_succeeds(void)
{
	CprRingBuffer rb;
	uint8_t buf[64];

	TEST_ASSERT_TRUE(cpr_ringbuf_init(&rb, buf, 64));
	TEST_ASSERT_EQUAL_UINT64(64, cpr_ringbuf_capacity(&rb));
	TEST_ASSERT_EQUAL_UINT64(0, cpr_ringbuf_size(&rb));
	TEST_ASSERT_EQUAL_UINT64(64, cpr_ringbuf_free(&rb));

	cpr_ringbuf_destroy(&rb);
}

void test_destroy_null(void)
{
	cpr_ringbuf_destroy(NULL); // must not crash
}

// --- NULL argument errors ---

void test_write_null_rb(void)
{
	uint8_t data[4] = { 1, 2, 3, 4 };

	cpr_clear_error();
	TEST_ASSERT_EQUAL_UINT64(0, cpr_ringbuf_write(NULL, data, 4));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);
}

void test_write_null_data_nonzero_size(void)
{
	CprRingBuffer rb;
	uint8_t buf[64];

	cpr_ringbuf_init(&rb, buf, 64);
	cpr_clear_error();
	TEST_ASSERT_EQUAL_UINT64(0, cpr_ringbuf_write(&rb, NULL, 4));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);

	cpr_ringbuf_destroy(&rb);
}

void test_write_null_data_zero_size_is_ok(void)
{
	CprRingBuffer rb;
	uint8_t buf[64];

	cpr_ringbuf_init(&rb, buf, 64);
	cpr_clear_error();
	TEST_ASSERT_EQUAL_UINT64(0, cpr_ringbuf_write(&rb, NULL, 0));
	TEST_ASSERT_TRUE(cpr_ok(cpr_get_error().code));

	cpr_ringbuf_destroy(&rb);
}

void test_read_null_rb(void)
{
	uint8_t out[4];

	cpr_clear_error();
	TEST_ASSERT_EQUAL_UINT64(0, cpr_ringbuf_read(NULL, out, 4));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);
}

void test_read_null_out_nonzero_size(void)
{
	CprRingBuffer rb;
	uint8_t buf[64];

	cpr_ringbuf_init(&rb, buf, 64);
	cpr_clear_error();
	TEST_ASSERT_EQUAL_UINT64(0, cpr_ringbuf_read(&rb, NULL, 4));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_get_error().code);

	cpr_ringbuf_destroy(&rb);
}

// --- Single-threaded round trip ---

void test_write_read_round_trip(void)
{
	CprRingBuffer rb;
	uint8_t storage[64];
	uint8_t data[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	uint8_t out[10] = { 0 };

	cpr_ringbuf_init(&rb, storage, 64);

	TEST_ASSERT_EQUAL_UINT64(10, cpr_ringbuf_write(&rb, data, 10));
	TEST_ASSERT_EQUAL_UINT64(10, cpr_ringbuf_size(&rb));
	TEST_ASSERT_EQUAL_UINT64(54, cpr_ringbuf_free(&rb));

	TEST_ASSERT_EQUAL_UINT64(10, cpr_ringbuf_read(&rb, out, 10));
	TEST_ASSERT_EQUAL_UINT8_ARRAY(data, out, 10);
	TEST_ASSERT_EQUAL_UINT64(0, cpr_ringbuf_size(&rb));
	TEST_ASSERT_EQUAL_UINT64(64, cpr_ringbuf_free(&rb));

	cpr_ringbuf_destroy(&rb);
}

// --- Partial write/read ---

void test_write_more_than_free_space_is_partial(void)
{
	CprRingBuffer rb;
	uint8_t storage[8];
	uint8_t data[16];
	int i;

	for (i = 0; i < 16; i++)
		data[i] = (uint8_t)i;

	cpr_ringbuf_init(&rb, storage, 8);

	TEST_ASSERT_EQUAL_UINT64(8, cpr_ringbuf_write(&rb, data, 16));
	TEST_ASSERT_EQUAL_UINT64(0, cpr_ringbuf_free(&rb));
	TEST_ASSERT_EQUAL_UINT64(8, cpr_ringbuf_size(&rb));

	cpr_ringbuf_destroy(&rb);
}

void test_read_more_than_available_is_partial(void)
{
	CprRingBuffer rb;
	uint8_t storage[8];
	uint8_t data[4] = { 10, 20, 30, 40 };
	uint8_t out[8] = { 0 };

	cpr_ringbuf_init(&rb, storage, 8);
	cpr_ringbuf_write(&rb, data, 4);

	TEST_ASSERT_EQUAL_UINT64(4, cpr_ringbuf_read(&rb, out, 8));
	TEST_ASSERT_EQUAL_UINT8_ARRAY(data, out, 4);
	TEST_ASSERT_EQUAL_UINT64(0, cpr_ringbuf_size(&rb));

	cpr_ringbuf_destroy(&rb);
}

// --- All-or-nothing ---

void test_write_all_fails_when_insufficient_room(void)
{
	CprRingBuffer rb;
	uint8_t storage[8];
	uint8_t data[16] = { 0 };

	cpr_ringbuf_init(&rb, storage, 8);

	TEST_ASSERT_FALSE(cpr_ringbuf_write_all(&rb, data, 16));
	TEST_ASSERT_EQUAL_UINT64(0, cpr_ringbuf_size(&rb));
	TEST_ASSERT_EQUAL_UINT64(8, cpr_ringbuf_free(&rb));

	cpr_ringbuf_destroy(&rb);
}

void test_write_all_succeeds_when_enough_room(void)
{
	CprRingBuffer rb;
	uint8_t storage[8];
	uint8_t data[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	uint8_t out[8] = { 0 };

	cpr_ringbuf_init(&rb, storage, 8);

	TEST_ASSERT_TRUE(cpr_ringbuf_write_all(&rb, data, 8));
	TEST_ASSERT_EQUAL_UINT64(8, cpr_ringbuf_size(&rb));

	TEST_ASSERT_TRUE(cpr_ringbuf_read_all(&rb, out, 8));
	TEST_ASSERT_EQUAL_UINT8_ARRAY(data, out, 8);

	cpr_ringbuf_destroy(&rb);
}

void test_read_all_fails_when_insufficient_data(void)
{
	CprRingBuffer rb;
	uint8_t storage[8];
	uint8_t data[2] = { 1, 2 };
	uint8_t out[8] = { 0 };

	cpr_ringbuf_init(&rb, storage, 8);
	cpr_ringbuf_write(&rb, data, 2);

	TEST_ASSERT_FALSE(cpr_ringbuf_read_all(&rb, out, 8));
	TEST_ASSERT_EQUAL_UINT64(2, cpr_ringbuf_size(&rb)); // nothing consumed

	cpr_ringbuf_destroy(&rb);
}

// --- Wraparound correctness ---

void test_wraparound_preserves_order(void)
{
	CprRingBuffer rb;
	uint8_t storage[8];
	uint8_t chunk1[6] = { 1, 2, 3, 4, 5, 6 };
	uint8_t chunk2[6] = { 7, 8, 9, 10, 11, 12 };
	uint8_t out1[6] = { 0 };
	uint8_t out2[6] = { 0 };

	cpr_ringbuf_init(&rb, storage, 8);

	// Fill most of the buffer, drain it, then write again so the second
	// write straddles the physical end of the array (head_cache == 6,
	// so writing 6 more bytes wraps at physical index 6,7,0,1,2,3).
	TEST_ASSERT_EQUAL_UINT64(6, cpr_ringbuf_write(&rb, chunk1, 6));
	TEST_ASSERT_EQUAL_UINT64(6, cpr_ringbuf_read(&rb, out1, 6));
	TEST_ASSERT_EQUAL_UINT8_ARRAY(chunk1, out1, 6);

	TEST_ASSERT_EQUAL_UINT64(6, cpr_ringbuf_write(&rb, chunk2, 6));
	TEST_ASSERT_EQUAL_UINT64(6, cpr_ringbuf_read(&rb, out2, 6));
	TEST_ASSERT_EQUAL_UINT8_ARRAY(chunk2, out2, 6);

	cpr_ringbuf_destroy(&rb);
}

void test_wraparound_does_not_corrupt_pending_data(void)
{
	// Regression test: a non-wrapping write must copy exactly `n` bytes,
	// not the distance to the end of the array. Once the ring has
	// wrapped at least once, a used region and a free region coexist in
	// the array; over-copying past `n` there clobbers valid unread data
	// sitting physically ahead of the write instead of just reading a
	// few extra (harmless if in-bounds) bytes from the source.
	CprRingBuffer rb;
	uint8_t storage[8];
	uint8_t fill[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	uint8_t drain[4] = { 0 };
	uint8_t small_write[2] = { 0xAA, 0xBB };
	uint8_t out[8] = { 0 };
	uint8_t expected[6] = { 5, 6, 7, 8, 0xAA, 0xBB };

	cpr_ringbuf_init(&rb, storage, 8);

	TEST_ASSERT_EQUAL_UINT64(8, cpr_ringbuf_write(&rb, fill, 8));
	TEST_ASSERT_EQUAL_UINT64(4, cpr_ringbuf_read(&rb, drain, 4));
	// Now: valid unread bytes {5,6,7,8} occupy physical [4,8), physical
	// [0,4) is free, and head_cache == 8 -> a write here starts at
	// physical index 0 with until_wrap == 8 (the whole array), even
	// though only 4 bytes are actually free.
	TEST_ASSERT_EQUAL_UINT64(2, cpr_ringbuf_write(&rb, small_write, 2));

	TEST_ASSERT_EQUAL_UINT64(6, cpr_ringbuf_size(&rb));
	TEST_ASSERT_EQUAL_UINT64(6, cpr_ringbuf_read(&rb, out, 6));
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, out, 6);

	cpr_ringbuf_destroy(&rb);
}

// --- Many small operations (exercises the modular counter arithmetic
// repeatedly, without needing a literal UINT32_MAX wrap) ---

void test_many_small_writes_and_reads_stay_consistent(void)
{
	CprRingBuffer rb;
	uint8_t storage[16];
	int i;
	uint8_t next_write = 0;
	uint8_t next_read = 0;

	cpr_ringbuf_init(&rb, storage, 16);

	for (i = 0; i < 100000; i++) {
		uint8_t out;

		cpr_ringbuf_write(&rb, &next_write, 1);
		next_write++;

		if (cpr_ringbuf_read(&rb, &out, 1) == 1) {
			TEST_ASSERT_EQUAL_UINT8(next_read, out);
			next_read++;
		}
	}

	cpr_ringbuf_destroy(&rb);
}

// --- Concurrent producer/consumer stress test ---

typedef struct {
	CprRingBuffer *rb;
	uint32_t total_bytes;
} ProducerArgs;

#define STRESS_TOTAL_BYTES 1000000u

static void cpr__ringbuf_producer(void *arg)
{
	ProducerArgs *a = (ProducerArgs *)arg;
	uint32_t sent = 0;

	while (sent < a->total_bytes) {
		uint8_t value = (uint8_t)sent;

		if (cpr_ringbuf_write(a->rb, &value, 1) == 1)
			sent++;
		else
			cpr_thrd_yield();
	}
}

void test_concurrent_producer_consumer_preserves_order(void)
{
	CprRingBuffer rb;
	uint8_t storage[64];
	ProducerArgs args;
	CprThread *producer;
	uint32_t received = 0;
	uint8_t expected = 0;

	cpr_ringbuf_init(&rb, storage, 64);

	args.rb = &rb;
	args.total_bytes = STRESS_TOTAL_BYTES;
	producer = cpr_thrd_create(cpr__ringbuf_producer, &args);

	while (received < STRESS_TOTAL_BYTES) {
		uint8_t value;

		if (cpr_ringbuf_read(&rb, &value, 1) == 1) {
			TEST_ASSERT_EQUAL_UINT8(expected, value);
			expected++;
			received++;
		} else {
			cpr_thrd_yield();
		}
	}

	cpr_thrd_join(producer);
	cpr_ringbuf_destroy(&rb);
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_init_null_rb);
	RUN_TEST(test_init_null_buf);
	RUN_TEST(test_init_zero_capacity);
	RUN_TEST(test_init_non_power_of_two);
	RUN_TEST(test_init_succeeds);
	RUN_TEST(test_destroy_null);

	RUN_TEST(test_write_null_rb);
	RUN_TEST(test_write_null_data_nonzero_size);
	RUN_TEST(test_write_null_data_zero_size_is_ok);
	RUN_TEST(test_read_null_rb);
	RUN_TEST(test_read_null_out_nonzero_size);

	RUN_TEST(test_write_read_round_trip);
	RUN_TEST(test_write_more_than_free_space_is_partial);
	RUN_TEST(test_read_more_than_available_is_partial);

	RUN_TEST(test_write_all_fails_when_insufficient_room);
	RUN_TEST(test_write_all_succeeds_when_enough_room);
	RUN_TEST(test_read_all_fails_when_insufficient_data);

	RUN_TEST(test_wraparound_preserves_order);
	RUN_TEST(test_wraparound_does_not_corrupt_pending_data);

	RUN_TEST(test_many_small_writes_and_reads_stay_consistent);

	RUN_TEST(test_concurrent_producer_consumer_preserves_order);

	return UNITY_END();
}
