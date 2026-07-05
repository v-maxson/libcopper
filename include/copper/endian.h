#ifndef CPR_ENDIAN_H
#define CPR_ENDIAN_H

//* Everything in this module is inlined.

#include "defs.h"
#include <stdint.h>
#include <string.h>

#if defined(CPR_COMPILER_MSVC)
#include <stdlib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// --- Byte Swap ---

/// Reverses the byte order of `v`.
CPR_INLINE static uint16_t cpr_bswap16(uint16_t v)
{
#if defined(CPR_COMPILER_GCC) || defined(CPR_COMPILER_CLANG)
	return __builtin_bswap16(v);
#elif defined(CPR_COMPILER_MSVC)
	return _byteswap_ushort(v);
#else
	return (uint16_t)((v << 8) | (v >> 8));
#endif
}

/// Reverse the byte order of `v`.
CPR_INLINE static uint32_t cpr_bswap32(uint32_t v)
{
#if defined(CPR_COMPILER_GCC) || defined(CPR_COMPILER_CLANG)
	return __builtin_bswap32(v);
#elif defined(CPR_COMPILER_MSVC)
	return _byteswap_ulong(v);
#else
	return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8) |
	       ((v & 0x00FF0000u) >> 8) | ((v & 0xFF000000u) >> 24);
#endif
}

/// Reverses the byte order of `v`.
CPR_INLINE static uint64_t cpr_bswap64(uint64_t v)
{
#if defined(CPR_COMPILER_GCC) || defined(CPR_COMPILER_CLANG)
	return __builtin_bswap64(v);
#elif defined(CPR_COMPILER_MSVC)
	return _byteswap_uint64(v);
#else
	return ((v & 0x00000000000000FFULL) << 56) |
	       ((v & 0x000000000000FF00ULL) << 40) |
	       ((v & 0x0000000000FF0000ULL) << 24) |
	       ((v & 0x00000000FF000000ULL) << 8) |
	       ((v & 0x000000FF00000000ULL) >> 8) |
	       ((v & 0x0000FF0000000000ULL) >> 24) |
	       ((v & 0x00FF000000000000ULL) >> 40) |
	       ((v & 0xFF00000000000000ULL) >> 56);
#endif
}

// --- Host <--> Little or Big Endian ---

// clang-format off
#if defined(CPR_BIG_ENDIAN)

CPR_INLINE static uint16_t cpr_htole16(uint16_t v) { return cpr_bswap16(v); }
CPR_INLINE static uint16_t cpr_letoh16(uint16_t v) { return cpr_bswap16(v); }
CPR_INLINE static uint32_t cpr_htole32(uint32_t v) { return cpr_bswap32(v); }
CPR_INLINE static uint32_t cpr_letoh32(uint32_t v) { return cpr_bswap32(v); }
CPR_INLINE static uint64_t cpr_htole64(uint64_t v) { return cpr_bswap64(v); }
CPR_INLINE static uint64_t cpr_letoh64(uint64_t v) { return cpr_bswap64(v); }

CPR_INLINE static uint16_t cpr_htobe16(uint16_t v) { return v; }
CPR_INLINE static uint16_t cpr_betoh16(uint16_t v) { return v; }
CPR_INLINE static uint32_t cpr_htobe32(uint32_t v) { return v; }
CPR_INLINE static uint32_t cpr_betoh32(uint32_t v) { return v; }
CPR_INLINE static uint64_t cpr_htobe64(uint64_t v) { return v; }
CPR_INLINE static uint64_t cpr_betoh64(uint64_t v) { return v; }

#else // little-endian, or undetected (assumed little-endian)

CPR_INLINE static uint16_t cpr_htole16(uint16_t v) { return v; }
CPR_INLINE static uint16_t cpr_letoh16(uint16_t v) { return v; }
CPR_INLINE static uint32_t cpr_htole32(uint32_t v) { return v; }
CPR_INLINE static uint32_t cpr_letoh32(uint32_t v) { return v; }
CPR_INLINE static uint64_t cpr_htole64(uint64_t v) { return v; }
CPR_INLINE static uint64_t cpr_letoh64(uint64_t v) { return v; }

CPR_INLINE static uint16_t cpr_htobe16(uint16_t v) { return cpr_bswap16(v); }
CPR_INLINE static uint16_t cpr_betoh16(uint16_t v) { return cpr_bswap16(v); }
CPR_INLINE static uint32_t cpr_htobe32(uint32_t v) { return cpr_bswap32(v); }
CPR_INLINE static uint32_t cpr_betoh32(uint32_t v) { return cpr_bswap32(v); }
CPR_INLINE static uint64_t cpr_htobe64(uint64_t v) { return cpr_bswap64(v); }
CPR_INLINE static uint64_t cpr_betoh64(uint64_t v) { return cpr_bswap64(v); }

#endif
// clang-format on

// --- Buffer Load/Store ---

CPR_INLINE static uint16_t cpr_read_le16(const void *src)
{
	uint16_t v;
	memcpy(&v, src, sizeof(v));
	return cpr_letoh16(v);
}

CPR_INLINE static uint32_t cpr_read_le32(const void *src)
{
	uint32_t v;
	memcpy(&v, src, sizeof(v));
	return cpr_letoh32(v);
}

CPR_INLINE static uint64_t cpr_read_le64(const void *src)
{
	uint64_t v;
	memcpy(&v, src, sizeof(v));
	return cpr_letoh64(v);
}

CPR_INLINE static uint16_t cpr_read_be16(const void *src)
{
	uint16_t v;
	memcpy(&v, src, sizeof(v));
	return cpr_betoh16(v);
}

CPR_INLINE static uint32_t cpr_read_be32(const void *src)
{
	uint32_t v;
	memcpy(&v, src, sizeof(v));
	return cpr_betoh32(v);
}

CPR_INLINE static uint64_t cpr_read_be64(const void *src)
{
	uint64_t v;
	memcpy(&v, src, sizeof(v));
	return cpr_betoh64(v);
}

CPR_INLINE static void cpr_write_le16(void *dst, uint16_t v)
{
	uint16_t le = cpr_htole16(v);
	memcpy(dst, &le, sizeof(le));
}

CPR_INLINE static void cpr_write_le32(void *dst, uint32_t v)
{
	uint32_t le = cpr_htole32(v);
	memcpy(dst, &le, sizeof(le));
}

CPR_INLINE static void cpr_write_le64(void *dst, uint64_t v)
{
	uint64_t le = cpr_htole64(v);
	memcpy(dst, &le, sizeof(le));
}

CPR_INLINE static void cpr_write_be16(void *dst, uint16_t v)
{
	uint16_t be = cpr_htobe16(v);
	memcpy(dst, &be, sizeof(be));
}

CPR_INLINE static void cpr_write_be32(void *dst, uint32_t v)
{
	uint32_t be = cpr_htobe32(v);
	memcpy(dst, &be, sizeof(be));
}

CPR_INLINE static void cpr_write_be64(void *dst, uint64_t v)
{
	uint64_t be = cpr_htobe64(v);
	memcpy(dst, &be, sizeof(be));
}

#ifdef __cplusplus
}
#endif

#endif // CPR_ENDIAN_H
