#include "copper/base64.h"

#include "copper/internal/int_error.h"
#include "copper/result.h"
#include <stdint.h>

// --- Alphabet ---

static char cpr__base64_char(uint8_t v, CprBase64Variant variant)
{
	static const char std_tail[2] = { '+', '/' };
	static const char url_tail[2] = { '-', '_' };

	if (v < 26)
		return (char)('A' + v);
	if (v < 52)
		return (char)('a' + (v - 26));
	if (v < 62)
		return (char)('0' + (v - 52));
	return (variant == CPR_BASE64_STD ? std_tail : url_tail)[v - 62];
}

static int cpr__base64_charval(unsigned char c, CprBase64Variant variant)
{
	if (c >= 'A' && c <= 'Z')
		return c - 'A';
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 26;
	if (c >= '0' && c <= '9')
		return c - '0' + 52;

	if (variant == CPR_BASE64_STD) {
		if (c == '+')
			return 62;
		if (c == '/')
			return 63;

	} else {
		if (c == '-')
			return 62;
		if (c == '_')
			return 63;
	}

	return -1;
}

// --- Sizing ---

size_t cpr_base64e_len(size_t data_size, bool pad)
{
	size_t groups = data_size / 3;
	size_t rem = data_size % 3;
	size_t chars = pad ? (groups + (rem ? 1 : 0)) * 4 :
			     groups * 4 + (rem ? rem + 1 : 0);

	return chars + 1;
}

size_t cpr_base64d_len(size_t text_size)
{
	return ((text_size + 3) / 4) * 4;
}

// --- Encode / Decode ---

bool cpr_base64_encode(const void *data, size_t data_size, char *buf,
		       size_t buf_size, CprBase64Variant variant, bool pad)
{
	if (!buf || (!data && data_size > 0)) {
		cpr__set_error(CPR_ERR_INVALID, "NULL argument");
		return false;
	}

	size_t needed = cpr_base64e_len(data_size, pad);
	if (buf_size < needed) {
		cpr__set_error(CPR_ERR_OVERFLOW, "buffer too small");
		return false;
	}

	const uint8_t *in = data;
	char *out = buf;
	size_t i = 0;

	for (; i + 3 <= data_size; i += 3) {
		uint32_t n = ((uint32_t)in[i] << 16) |
			     ((uint32_t)in[i + 1] << 8) | in[i + 2];
		*out++ = cpr__base64_char((n >> 18) & 0x3F, variant);
		*out++ = cpr__base64_char((n >> 12) & 0x3F, variant);
		*out++ = cpr__base64_char((n >> 6) & 0x3F, variant);
		*out++ = cpr__base64_char(n & 0x3F, variant);
	}

	size_t rem = data_size - i;
	if (rem == 1) {
		uint32_t n = (uint32_t)in[i] << 16;
		*out++ = cpr__base64_char((n >> 18) & 0x3F, variant);
		*out++ = cpr__base64_char((n >> 12) & 0x3F, variant);
		if (pad) {
			*out++ = '=';
			*out++ = '=';
		}
	} else if (rem == 2) {
		uint32_t n = ((uint32_t)in[i] << 16) |
			     ((uint32_t)in[i + 1] << 8);
		*out++ = cpr__base64_char((n >> 18) & 0x3F, variant);
		*out++ = cpr__base64_char((n >> 12) & 0x3F, variant);
		*out++ = cpr__base64_char((n >> 6) & 0x3F, variant);
		if (pad)
			*out++ = '=';
	}

	*out = '\0';
	return true;
}

bool cpr_base64_decode(const char *text, size_t text_size, void *buf,
		       size_t buf_size, CprBase64Variant variant,
		       size_t *out_size)
{
	if (!buf || !out_size || (!text && text_size > 0)) {
		cpr__set_error(CPR_ERR_INVALID, "NULL argument");
		return false;
	}

	if (text_size == 0) {
		*out_size = 0;
		return true;
	}

	if (text_size % 4 == 1) {
		cpr__set_error(CPR_ERR_INVALID, "invalid Base64 length");
		return false;
	}

	size_t pad = 0;
	if (text_size % 4 == 0 && text[text_size - 1] == '=') {
		pad = 1;
		if (text[text_size - 2] == '=')
			pad = 2;
	}

	size_t data_len = text_size - pad;
	size_t full_groups = data_len / 4;
	size_t final_chars = data_len % 4; // 0/2/3

	size_t needed = full_groups * 3 + (final_chars == 3 ? 2 :
					   final_chars == 2 ? 1 :
							      0);
	if (buf_size < needed) {
		cpr__set_error(CPR_ERR_OVERFLOW, "buffer too small");
		return false;
	}

	uint8_t *out = buf;
	size_t i = 0;

	for (size_t g = 0; g < full_groups; g++, i += 4) {
		int v0 = cpr__base64_charval((uint8_t)text[i], variant);
		int v1 = cpr__base64_charval((uint8_t)text[i + 1], variant);
		int v2 = cpr__base64_charval((uint8_t)text[i + 2], variant);
		int v3 = cpr__base64_charval((uint8_t)text[i + 3], variant);

		if (v0 < 0 || v1 < 0 || v2 < 0 || v3 < 0) {
			cpr__set_error(CPR_ERR_INVALID,
				       "invalid Base64 character");
			return false;
		}

		uint32_t n = ((uint32_t)v0 << 18) | ((uint32_t)v1 << 12) |
			     ((uint32_t)v2 << 6) | (uint32_t)v3;
		*out++ = (uint8_t)(n >> 16);
		*out++ = (uint8_t)(n >> 8);
		*out++ = (uint8_t)n;
	}

	if (final_chars == 2) {
		int v0 = cpr__base64_charval((uint8_t)text[i], variant);
		int v1 = cpr__base64_charval((uint8_t)text[i + 1], variant);

		if (v0 < 0 || v1 < 0) {
			cpr__set_error(CPR_ERR_INVALID,
				       "invalid Base64 character");
			return false;
		}

		uint32_t n = ((uint32_t)v0 << 18) | ((uint32_t)v1 << 12);
		*out++ = (uint8_t)(n >> 16);
	} else if (final_chars == 3) {
		int v0 = cpr__base64_charval((uint8_t)text[i], variant);
		int v1 = cpr__base64_charval((uint8_t)text[i + 1], variant);
		int v2 = cpr__base64_charval((uint8_t)text[i + 2], variant);

		if (v0 < 0 || v1 < 0 || v2 < 0) {
			cpr__set_error(CPR_ERR_INVALID,
				       "invalid Base64 character");
			return false;
		}

		uint32_t n = ((uint32_t)v0 << 18) | ((uint32_t)v1 << 12) |
			     ((uint32_t)v2 << 6);
		*out++ = (uint8_t)(n >> 16);
		*out++ = (uint8_t)(n >> 8);
	}

	*out_size = needed;
	return true;
}
