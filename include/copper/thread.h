#ifndef CPR_THREAD_H
#define CPR_THREAD_H

#include "defs.h"
#include <stdbool.h>
#include <stdint.h>

// --- Thread ---

typedef uint64_t CprThreadId;
typedef void (*CprThreadFn)(void *arg);

/// Opaque thread handle acquired when a thread is created with `cpr_thrd_create`.
typedef struct CprThread CprThread;

// --- TLS ---

#if defined(CPR_COMPILER_MSVC)
#define CPR_THREAD_LOCAL __declspec(thread)
#elif defined(CPR_COMPILER_GCC) || defined(CPR_COMPILER_CLANG)
#define CPR_THREAD_LOCAL __thread
#else
#error "CPR_THREAD_LOCAL: unsupported compiler"
#endif

typedef void (*CprTlsDestructor)(void *value);

/// Opaque TLS (thread-local-storage key).
typedef struct CprTls CprTls;

#ifdef __cplusplus
extern "C" {
#endif

// --- Thread ---

/// Creates a new thread running `fn(arg)`. Returns NULL on failure.
/// The caller is responsible for releasing the handle with `cpr_thrd_join` or `cpr_thrd_detach`.
CPR_API CprThread *cpr_thrd_create(CprThreadFn fn, void *arg);

/// Blocks until the thread exits, then frees the handle. Returns false on failure.
/// `thread` must not be used after this call.
CPR_API bool cpr_thrd_join(CprThread *thread);

/// Detaches the thread so it cleans up automatically on exit, then frees the handle.
/// Returns false on failure. `thread` must not be used after this call.
CPR_API bool cpr_thrd_detach(CprThread *thread);

CPR_API CprThreadId cpr_thrd_get_id(const CprThread *thread);
CPR_API CprThreadId cpr_thrd_current_id(void);
CPR_API void cpr_thrd_sleep(uint32_t ms);
CPR_API void cpr_thrd_yield(void);

// --- TLS ---

/// Creates a TLS key. Returns NULL on failure.
/// `destructor` is called with the slot value when a thread exits; pass NULL if no cleanup needed.
CPR_API CprTls *cpr_tls_create(CprTlsDestructor destructor);

CPR_API void cpr_tls_destroy(CprTls *tls);

/// Sets the value for `tls` on the calling thread. Returns false on failure.
CPR_API bool cpr_tls_set(CprTls *tls, void *value);
CPR_API void *cpr_tls_get(const CprTls *tls);

#ifdef __cplusplus
}
#endif

#endif
