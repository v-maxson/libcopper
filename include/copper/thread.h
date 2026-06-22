#ifndef CPR_THREAD_H
#define CPR_THREAD_H

#include "platform.h"
#include "result.h"
#include <stdint.h>

typedef uint64_t CprThreadId;

// --- Thread ---

typedef void (*CprThreadFn)(void *arg);

/// Opaque thread handle acquired when a thread is created with `cpr_thread_create`.
typedef struct CprThread CprThread;

// --- TLS ---

typedef void (*CprTlsDestructor)(void *value);

/// Opaque TLS (thread-local-storage key).
typedef struct CprTls CprTls;

#ifdef __cplusplus
extern "C" {
#endif

// --- Thread ---

/// Creates a new thread running `fn(arg)`. Returns NULL on failure.
/// If `out_result` != NULL, it is set to CPR_OK, or the relevant error code on failure.
/// The caller is responsible for releasing the handle with `cpr_thrd_join` or `cpr_thrd_detach`
CPR_API CprThread *cpr_thrd_create(CprThreadFn fn, void *arg,
				   CprResult *out_result);

/// Blocks until the thread exits, then frees the handle.
/// `thread` must not be used after this call.
CPR_API CprResult cpr_thrd_join(CprThread *thread);

/// Detaches the thread so it cleans up automatically on exit, the frees the handle.
/// `thread` must not be used after this call.
CPR_API CprResult cpr_thrd_detach(CprThread *thread);

CPR_API CprThreadId cpr_thrd_get_id(const CprThread *thread);
CPR_API CprThreadId cpr_thrd_current_id(void);
CPR_API void cpr_thrd_sleep(uint32_t ms);
CPR_API void cpr_thrd_yield(void);

// --- TLS ---

/// Creates a TLS key. Returns NULL on failure.
/// If `out_result` != NULL, it is set to CPR_OK, or the relevant error code on failure.
/// `destructor` is called with the slot value when a thread exits; pass NULL if no cleanup needed.
CPR_API CprTls *cpr_tls_create(CprTlsDestructor destructor,
			       CprResult *out_result);

CPR_API void cpr_tls_destroy(CprTls *tls);
CPR_API CprResult cpr_tls_set(CprTls *tls, void *value);
CPR_API void *cpr_tls_get(const CprTls *tls);

#ifdef __cplusplus
}
#endif

#endif
