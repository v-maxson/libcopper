#ifndef CPR_ERROR_H
#define CPR_ERROR_H

#include "defs.h"
#include "result.h"

typedef struct {
	CprResult code; ///< CPR_OK when no error has occured.
	const char *
		message; ///< Human-readable error description. `""` when no error.
} CprError;

#ifdef __cplusplus
extern "C" {
#endif

/// Returns the last error set on this thread.
/// `code` == `CPR_OK` and `message` == `""` when no error has occured
/// (or after `cpr_clear_error` is called).
CPR_API CprError cpr_get_error(void);

/// Clears the per-thread error state.
CPR_API void cpr_clear_error(void);

#ifdef __cplusplus
}
#endif

#endif // CPR_ERROR_H
