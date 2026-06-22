#ifndef CPR_COPPER_H
#define CPR_COPPER_H

// Transient includes.
#include <copper/arena.h>
#include <copper/atomic.h>
#include <copper/bytesize.h>
#include <copper/log.h>
#include <copper/platform.h>
#include <copper/result.h>
#include <copper/sync.h>
#include <copper/thread.h>
#include <copper/time.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- Version ---

CPR_API int cpr_version_major(void);
CPR_API int cpr_version_minor(void);
CPR_API int cpr_version_patch(void);
CPR_API const char *cpr_version_string(void);

#ifdef __cplusplus
}
#endif

#endif /* CPR_COPPER_H */
