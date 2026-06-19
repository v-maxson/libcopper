#ifndef CPR_SYNC_H
#define CPR_SYNC_H

//* This module contains synchonization primitives for safe multi-threaded programming.

#include "platform.h"
#include "result.h"
#include <stdint.h>

// --- Mutex ---

#define CPR_MUTEX_STORAGE_SIZE \
	64 // defined as the largest possible size of a mutex across all platforms (64 bytes on macOS/iOS)

/// A mutex. Must be initialized with cpr_mutex_init before use!
typedef struct {
	union {
		uint8_t storage[CPR_MUTEX_STORAGE_SIZE];
		CprMaxAlign _align;
	} _internal;
} CprMutex;

#ifdef __cplusplus
extern "C" {
#endif

CPR_API CprResult cpr_mutex_init(CprMutex *mutex);
CPR_API void cpr_mutex_destroy(CprMutex *mutex);
CPR_API CprResult cpr_mutex_lock(CprMutex *mutex);
CPR_API CprResult
cpr_mutex_trylock(CprMutex *mutex); ///< Returns CPR_ERR_BUSY if not acquired.
CPR_API CprResult cpr_mutex_unlock(CprMutex *mutex);

#ifdef __cplusplus
}
#endif

#endif // CPR_SYNC_H
