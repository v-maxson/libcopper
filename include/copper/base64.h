#ifndef CPR_BASE64_H
#define CPR_BASE64_H

#include "defs.h"
#include <stdbool.h>
#include <stddef.h>

// Standard/URL Variants

typedef enum {
	CPR_BASE64_STD, ///< Standard Base64 encoding.
	CPR_BASE64_URL ///< Suitable for URLs and filenames
} CprBase64Variant;

#ifdef __cplusplus
extern "C" {
#endif

// --- Sizing ---

/// Returns the number of bytes needed to hold `data_size` bytes encoded as
/// Base64, including a null terminator. Pass the same `pad` value you'll pass
/// to `cpr_base64_encode`.
CPR_API size_t cpr_base64e_len(size_t data_size, bool pad);

/// Returns a safe upper bound on the number of bytes neede to decode a
/// Base64 string of `text_size` bytes. The actual decoded size
/// (written to `out_size` by `cpr_base64_decode`) may be smaller.
CPR_API size_t cpr_base64d_len(size_t text_size);

// --- Encode / Decode ---

/// Encodes `data_size` bytes from `data` as Base64 into `buf`, using the
/// `variant` alphabet. Null-terminates the result.
/// If `pad` is true, the output is padded with '=' to a multiple of 4 chars.
/// Returns false if `buf_size` is too small; call `cpr_base64e_len` to size `buf` correctly.
CPR_API bool cpr_base64_encode(const void *data, size_t data_size, char *buf,
			       size_t buf_size, CprBase64Variant variant,
			       bool pad);

/// Decodes `text_size` bytes of Base64 text from `text` into `buf`, using the
/// `variant` alphabet. Accepts both padded and unpadded input.
/// Writes the number of decoded bytes to `out_size`.
/// Returns false if `text` contains invalid characters or malformed padding,
/// or if `buf` is too small; call `cpr_get_error` to check the error code.
CPR_API bool cpr_base64_decode(const char *text, size_t text_size, void *buf,
			       size_t buf_size, CprBase64Variant variant,
			       size_t *out_size);

#ifdef __cplusplus
}
#endif

#endif // CPR_BASE64_H
