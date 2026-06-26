#ifndef CPR_LOG_H
#define CPR_LOG_H

#include "defs.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// --- Buffer Sizes ---
#ifndef CPR_LOG_BUFFER_SIZE
#define CPR_LOG_BUFFER_SIZE 2048
#endif

#ifndef CPR_LOG_MSG_SIZE
#define CPR_LOG_MSG_SIZE (CPR_LOG_BUFFER_SIZE - 128)
#endif

// --- Log Level ---

typedef enum {
	CPR_LOG_TRACE = 0,
	CPR_LOG_DEBUG = 1,
	CPR_LOG_INFO = 2,
	CPR_LOG_WARN = 3,
	CPR_LOG_ERROR = 4,
	CPR_LOG_FATAL = 5,
	CPR_LOG_OFF = 6
} CprLogLevel;

#ifdef __cplusplus
extern "C" {
#endif

/// "TRACE" "OFF" etc.
CPR_API const char *cpr_log_level_str(CprLogLevel level);

/// "trace" "off" etc.
CPR_API const char *cpr_log_level_strlc(CprLogLevel level);

#ifdef __cplusplus
}
#endif

// --- Log Message ---

typedef struct {
	CprLogLevel level;
	const char *message;
	bool truncated; ///< `true` if the message was truncated.
	const char *file; ///< __FILE__
	int line; ///< __LINE__
	const char *func; ///< __func__
	uint64_t time_ms; ///< UTC MS since Unix epoch.
} CprLogMessage;

// --- Formatter ---

typedef int (*CprLogFormatFn)(const CprLogMessage *msg, char *buf,
			      size_t buf_size, bool *out_truncated);

#ifdef __cplusplus
extern "C" {
#endif

// --- Built-in formatters ---

/// Built-in formatter: `[HH:MM:SS.mmm] [LEVEL] message\n`
CPR_API int cpr_log_format_default(const CprLogMessage *msg, char *buf,
				   size_t buf_size, bool *out_truncated);

/// Built-in formatter: `[HH:MM:SS.mmm] [LEVEL] file:line func() message\n`
CPR_API int cpr_log_format_full(const CprLogMessage *msg, char *buf,
				size_t buf_size, bool *out_truncated);

/// Built-in formatter: `[LEVEL] message\n`
CPR_API int cpr_log_format_minimal(const CprLogMessage *msg, char *buf,
				   size_t buf_size, bool *out_truncated);

#ifdef __cplusplus
}
#endif

// --- Sink ---

typedef struct CprLogSink CprLogSink;

struct CprLogSink {
	CprLogLevel min_level;
	CprLogFormatFn format;

	/// Called for each passed `message`. `buf` is the formatter output.
	void (*write)(CprLogSink *sink, const CprLogMessage *message,
		      const char *buf, size_t len);

	/// NULL if the sink does not buffer.
	void (*flush)(CprLogSink *sink);

	/// NULL if the sink requires no cleanup.
	void (*destroy)(CprLogSink *sink);
};

#ifdef __cplusplus
extern "C" {
#endif

/// Calls `sink->destroy(sink)`. Does not free the sink pointer itself
/// (sinks are responsible for their own storage).
CPR_API void cpr_log_sink_destroy(CprLogSink *sink);

#ifdef __cplusplus
}
#endif

// --- Console Sink ---

typedef struct {
	bool use_color;
	bool use_stderr;
} CprConsoleSinkConfig;

#ifdef __cplusplus
extern "C" {
#endif

CPR_API CprLogSink *cpr_log_console_sink(const CprConsoleSinkConfig *config);

#ifdef __cplusplus
}
#endif

// --- File Sink ---

typedef enum {
	CPR_LOG_ROLL_NONE,
	CPR_LOG_ROLL_SIZE,
	CPR_LOG_ROLL_DAILY,
	CPR_LOG_ROLL_HOURLY
} CprLogRollMode;

typedef enum {
	CPR_LOG_FILE_APPEND, ///< Append to the end of the file (don't overwrite).
	CPR_LOG_FILE_OVERWRITE ///< Clear the contents of the file, then write.
} CprLogFileMode;

typedef struct {
	const char *path;
	CprLogFileMode open_mode;
	CprLogRollMode roll_mode;
	size_t max_bytes; ///< Ignored if `roll_mode != CPR_LOG_ROLL_SIZE`
	int max_files; ///< archived files to keep; 0 = unlimited
} CprFileSinkConfig;

#ifdef __cplusplus
extern "C" {
#endif

CPR_API CprLogSink *cpr_log_file_sink(const CprFileSinkConfig *config);

#ifdef __cplusplus
}
#endif

// --- Callback Sink ---

typedef void (*CprLogCallbackFn)(const CprLogMessage *msg, const char *buf,
				 size_t len, void *userdata);

typedef struct {
	CprLogCallbackFn callback;
	void *userdata;
} CprCallbackSinkConfig;

#ifdef __cplusplus
extern "C" {
#endif

CPR_API CprLogSink *cpr_log_callback_sink(const CprCallbackSinkConfig *config);

#ifdef __cplusplus
}
#endif

// --- Logger ---

typedef struct CprLogger CprLogger;

#define CPR_LOG_MAX_SINKS 8

typedef struct {
	CprLogLevel min_level;
	bool thread_safe;
	CprLogFormatFn default_format;
} CprLoggerConfig;

#ifdef __cplusplus
extern "C" {
#endif

CPR_API CprLogger *cpr_log_create(const CprLoggerConfig *config);

/// Flushes and frees the `logger`. `logger` is not valid after this is caled.
CPR_API void cpr_log_destroy(CprLogger *logger);

/// Sinks are *not* owned by the logger. Call `cpr_log_sink_destroy` separately.
/// Not thread-safe. Returns false if the logger already has the maximum number of sinks.
CPR_API bool cpr_log_add_sink(CprLogger *logger, CprLogSink *sink);

/// Sinks are *not* owned by the logger. Call `cpr_log_sink_destroy` seperately.
/// Not thread-safe.
CPR_API void cpr_log_remove_sink(CprLogger *logger, CprLogSink *sink);
CPR_API void cpr_log_set_level(CprLogger *logger, CprLogLevel level);
CPR_API CprLogLevel cpr_log_get_level(CprLogger *logger);

/// Calls `flush` on every sink that provides one.
CPR_API void cpr_log_flush(CprLogger *logger);

CPR_API void cpr_log_write(CprLogger *logger, CprLogLevel level,
			   const char *file, int line, const char *func,
			   const char *fmt, ...);
CPR_API void cpr_log_writev(CprLogger *logger, CprLogLevel level,
			    const char *file, int line, const char *func,
			    const char *fmt, va_list varargs);

/// Returns NULL until `cpr_log_set_default` is called. Not thread-safe.
CPR_API CprLogger *cpr_log_default(void);
CPR_API void cpr_log_set_default(CprLogger *logger);

#ifdef __cplusplus
}
#endif

// --- Macros ---

#ifndef CPR_LOG_MIN_LEVEL
#define CPR_LOG_MIN_LEVEL CPR_LOG_TRACE
#endif

#define cpr_log(logger, level, ...)                                      \
	do {                                                             \
		if ((level) >= CPR_LOG_MIN_LEVEL)                        \
			cpr_log_write(logger, level, __FILE__, __LINE__, \
				      __func__, __VA_ARGS__);            \
	} while (0)

#define cpr_log_trace(logger, ...) cpr_log(logger, CPR_LOG_TRACE, __VA_ARGS__)
#define cpr_log_debug(logger, ...) cpr_log(logger, CPR_LOG_DEBUG, __VA_ARGS__)
#define cpr_log_info(logger, ...) cpr_log(logger, CPR_LOG_INFO, __VA_ARGS__)
#define cpr_log_warn(logger, ...) cpr_log(logger, CPR_LOG_WARN, __VA_ARGS__)
#define cpr_log_error(logger, ...) cpr_log(logger, CPR_LOG_ERROR, __VA_ARGS__)
#define cpr_log_fatal(logger, ...) cpr_log(logger, CPR_LOG_FATAL, __VA_ARGS__)

#define cpr_trace(...) cpr_log_trace(cpr_log_default(), __VA_ARGS__)
#define cpr_debug(...) cpr_log_debug(cpr_log_default(), __VA_ARGS__)
#define cpr_info(...) cpr_log_info(cpr_log_default(), __VA_ARGS__)
#define cpr_warn(...) cpr_log_warn(cpr_log_default(), __VA_ARGS__)
#define cpr_error(...) cpr_log_error(cpr_log_default(), __VA_ARGS__)
#define cpr_fatal(...) cpr_log_fatal(cpr_log_default(), __VA_ARGS__)

#endif // CPR_LOG_H
