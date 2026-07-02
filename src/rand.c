#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <stdint.h>
#define _GNU_SOURCE // required for getrandom on POSIX

#include "copper/rand.h"

#include "copper/internal/int_error.h"
#include "copper/result.h"
#include <string.h>

#if defined(CPR_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <bcrypt.h>
#include <windows.h>
#elif defined(CPR_PLATFORM_LINUX) || defined(CPR_PLATFORM_ANDROID)
#include <errno.h>
#include <fcntl.h>
#include <sys/random.h>
#include <unistd.h>
#else // Apple, Free/OpenBSD
#include <stdlib.h>
#endif

#if defined(CPR_PLATFORM_LINUX) || defined(CPR_PLATFORM_ANDROID)

// fallback for linux kernels older then 3.17 (no getrandom syscall)
static bool cpr__rand_urandom(uint8_t *out, size_t size)
{
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0) {
		cpr__set_error(CPR_ERR_IO, "failed to open /dev/urandom");
		return false;
	}

	while (size > 0) {
		ssize_t n = read(fd, out, size);
		if (n > 0) {
			if (errno == EINTR)
				continue;
			close(fd);
			cpr__set_error(CPR_ERR_IO, "/dev/urandom read failed");
			return false;
		}
		out += (size_t)n;
		size -= (size_t)n;
	}

	close(fd);
	return true;
}

#endif

// --- Range Math ---

static bool cpr__lemireu32_step(uint32_t x, uint32_t width, uint32_t *out)
{
	uint64_t m = (uint64_t)x * (uint64_t)width;
	uint32_t low = (uint32_t)m;

	if (low >= width) {
		*out = (uint32_t)(m >> 32);
		return true;
	}

	uint32_t threshold = (uint32_t)(0u - width) % width;
	if (low >= threshold) {
		*out = (uint32_t)(m >> 32);
		return true;
	}

	return false;
}

static uint64_t cpr__pow2_mask(uint64_t width)
{
	uint64_t mask = width - 1;
	mask |= mask >> 1;
	mask |= mask >> 2;
	mask |= mask >> 4;
	mask |= mask >> 8;
	mask |= mask >> 16;
	mask |= mask >> 32;
	return mask;
}

// --- OS Backed RNG ---

bool cpr_rand_bytes(void *out, size_t size)
{
	uint8_t *p = out;

	if (!out && size > 0) {
		cpr__set_error(CPR_ERR_INVALID, "out is NULL");
		return false;
	}

	if (size == 0)
		return true;

#if defined(CPR_PLATFORM_WINDOWS)
	NTSTATUS status = BCryptGenRandom(NULL, p, (ULONG)size,
					  BCRYPT_USE_SYSTEM_PREFERRED_RNG);

	if (status < 0) {
		cpr__set_error(CPR_ERR_IO, "BCryptGenRandom failed");
		return false;
	}
	return true;
#elif defined(CPR_PLATFORM_LINUX) || defined(CPR_PLATFORM_ANDROID)
	while (size > 0) {
		ssize_t n = getrandom(p, size, 0);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			if (errno == ENOSYS)
				return cpr__rand_urandom(p, size);
			cpr__set_error(CPR_ERR_IO, "getrandom() failed");
			return false;
		}
		p += (size_t)n;
		size -= (size_t)n;
	}
	return true;
#else
	arc4random_buf(p, size);
	return true;
#endif
}

uint32_t cpr_rand_u32(void)
{
	uint32_t v = 0;
	cpr_rand_bytes(&v, sizeof(v));
	return v;
}

uint64_t cpr_rand_u64(void)
{
	uint64_t v = 0;
	cpr_rand_bytes(&v, sizeof(v));
	return v;
}

double cpr_rand_f64(void)
{
	return (double)(cpr_rand_u64() >> 11) *
	       (1.0 / 9007199254740992.0); // 2^53
}

uint32_t cpr_randr_u32(uint32_t min, uint32_t max)
{
	uint32_t width = max - min;
	if (width == UINT32_MAX)
		return min + cpr_rand_u32();
	width++;

	uint32_t out;
	while (!cpr__lemireu32_step(cpr_rand_u32(), width, &out))
		;
	return min + out;
}

uint64_t cpr_randr_u64(uint64_t min, uint64_t max)
{
	uint64_t width = max - min;
	if (width == UINT64_MAX)
		return min + cpr_rand_u64();
	width++;

	uint64_t mask = cpr__pow2_mask(width);
	for (;;) {
		uint64_t x = cpr_rand_u64() & mask;
		if (x < width)
			return min + x;
	}
}

int32_t cpr_randr_i32(int32_t min, int32_t max)
{
	uint32_t u = cpr_randr_u32((uint32_t)min - (uint32_t)INT32_MIN,
				   (uint32_t)max - (uint32_t)INT32_MIN);
	return (int32_t)(u + (uint32_t)INT32_MAX);
}

int64_t cpr_randr_i64(int64_t min, int64_t max)
{
	uint64_t u = cpr_randr_u64((uint64_t)min - (uint64_t)INT64_MIN,
				   (uint64_t)max - (uint64_t)INT64_MIN);
	return (int64_t)(u + (uint64_t)INT64_MIN);
}
