#ifndef CPR_ENV_H
#define CPR_ENV_H

#include "defs.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Get the value of an environment variable.
/// Note: On POSIX, these functions are *not* thread-safe. It's best to treat
/// it as though they aren't on Windows either.
CPR_API bool cpr_get_env(const char *name, char *buf, size_t buf_size);

/// Set the value of an environment variable.
/// Note: On POSIX, these functions are *not* thread-safe. It's best to treat
/// it as though they aren't on Windows either.
CPR_API bool cpr_set_env(const char *name, const char *value);

/// Unset an environment variable.
/// Note: On POSIX, these functions are *not* thread-safe. It's best to treat
/// it as though they aren't on Windows either.
CPR_API bool cpr_unset_env(const char *name);

#ifdef __cplusplus
}
#endif

// TODO: Iterator?

#endif // CPR_ENV_H
