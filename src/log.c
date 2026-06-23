#include "copper/log.h"

#include "copper/result.h"
#include "copper/sync.h"
#include "copper/time.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(CPR_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

// --- Truncation Markers ---

#define CPR_LOG_MSG_MARKER "...[truncated]"
#define CPR_LOG_OUT_MARKER "...[T]\n"

// --- Internal helpers ---

static void cpr__write_marker(char *buf, size_t n, const char *marker)
{
	size_t mlen;
	if (n == 0)
		return;

	mlen = strlen(marker);
	if (n > mlen + 1)
		memcpy(buf + n - 1 - mlen, marker, mlen);

	buf[n - 1] = '\0';
}

static FILE *cpr__fopen(const char *path, const char *mode)
{
#if defined(CPR_COMPILER_MSVC)
	FILE *fp = NULL;
	fopen_s(&fp, path, mode);
	return fp;
#else
	return fopen(path, mode);
#endif
}

static bool cpr__is_tty(FILE *stream)
{
#if defined(CPR_PLATFORM_WINDOWS)
	DWORD mode;
	HANDLE h = (HANDLE)_get_osfhandle(_fileno(stream));
	return GetConsoleMode(h, &mode) != 0;
#else
	return isatty(fileno(stream));
#endif
}

static int cpr__format_finish(char *buf, size_t n, int written,
			      bool *out_truncated)
{
	if (written < 0) {
		if (n > 0)
			buf[0] = '\0';
		if (out_truncated)
			*out_truncated = false;

		return 0;
	}

	if ((size_t)written >= n) {
		cpr__write_marker(buf, n, CPR_LOG_OUT_MARKER);
		if (out_truncated)
			*out_truncated = true;
		return (int)(n > 0 ? n - 1 : 0);
	}

	if (out_truncated)
		*out_truncated = false;
	return written;
}

#define CPR_ANSI_COLOR_RESET "\033[0m"

static const char *cpr__ansi_color(CprLogLevel level)
{
	switch (level) {
	case CPR_LOG_TRACE:
		return "\033[2m"; // dim
	case CPR_LOG_DEBUG:
		return "\033[36m"; // cyan
	case CPR_LOG_INFO:
		return "\033[32m"; // green
	case CPR_LOG_WARN:
		return "\033[33m"; // yellow
	case CPR_LOG_ERROR:
		return "\033[31m"; // red
	case CPR_LOG_FATAL:
		return "\033[35m"; // magenta
	default:
		return "";
	}
}

// --- Log Level ---

static const char *const S_UPPER[] = { "TRACE", "DEBUG", "INFO", "WARN",
				       "ERROR", "FATAL", "OFF" };

static const char *const S_LOWER[] = { "trace", "debug", "info", "warn",
				       "error", "fatal", "off" };

const char *cpr_log_level_str(CprLogLevel level)
{
	if (level < 0 || level > CPR_LOG_OFF)
		return "UNKNOWN";

	return S_UPPER[level];
}

const char *cpr_log_level_strlc(CprLogLevel level)
{
	if (level < 0 || level > CPR_LOG_OFF)
		return "UNKNOWN";

	return S_LOWER[level];
}

// --- Built-in formatters ---

int cpr_log_format_default(const CprLogMessage *msg, char *buf, size_t buf_size,
			   bool *out_truncated)
{
	CprDateTime dt = cpr_time_local(msg->time_ms);
	int w = snprintf(buf, buf_size, "[%02d:%02d:%02d.%03d] [%-5s] %s\n",
			 dt.hour, dt.minute, dt.second, dt.ms,
			 cpr_log_level_str(msg->level), msg->message);
	return cpr__format_finish(buf, buf_size, w, out_truncated);
}

int cpr_log_format_full(const CprLogMessage *msg, char *buf, size_t buf_size,
			bool *out_truncated)
{
	CprDateTime dt = cpr_time_local(msg->time_ms);
	int w = snprintf(buf, buf_size,
			 "[%02d:%02d:%02d.%03d] [%-5s] %s:%d %s() %s\n",
			 dt.hour, dt.minute, dt.second, dt.ms,
			 cpr_log_level_str(msg->level), msg->file, msg->line,
			 msg->func, msg->message);
	return cpr__format_finish(buf, buf_size, w, out_truncated);
}

int cpr_log_format_minimal(const CprLogMessage *msg, char *buf, size_t buf_size,
			   bool *out_truncated)
{
	int w = snprintf(buf, buf_size, "[%-5s] %s\n",
			 cpr_log_level_str(msg->level), msg->message);
	return cpr__format_finish(buf, buf_size, w, out_truncated);
}

// --- Sink ---

void cpr_log_sink_destroy(CprLogSink *sink)
{
	if (!sink)
		return;
	if (sink->destroy)
		sink->destroy(sink);
}

// --- Console Sink ---

typedef struct {
	CprLogSink base;
	FILE *stream;
	bool use_color;
	bool is_tty;
} CprConsoleSink;

static void cpr__console_write(CprLogSink *sink, const CprLogMessage *msg,
			       const char *buf, size_t len)
{
	CprConsoleSink *cs = (CprConsoleSink *)sink;
	if (cs->use_color && cs->is_tty) {
		fputs(cpr__ansi_color(msg->level), cs->stream);
		fwrite(buf, 1, len, cs->stream);
		fputs(CPR_ANSI_COLOR_RESET, cs->stream);
	} else {
		fwrite(buf, 1, len, cs->stream);
	}

	// Always flush warnings and above so they aren't lost in the buffer on a crash.
	if (msg->level >= CPR_LOG_WARN)
		fflush(cs->stream);
}

static void cpr__console_flush(CprLogSink *sink)
{
	fflush(((CprConsoleSink *)sink)->stream);
}

static void cpr__console_destroy(CprLogSink *sink)
{
	free(sink);
}

CprLogSink *cpr_log_console_sink(const CprConsoleSinkConfig *config)
{
	CprConsoleSink *cs = malloc(sizeof(*cs));
	if (!cs)
		return NULL;

	cs->stream = (config && config->use_stderr) ? stderr : stdout;
	cs->use_color = config ? config->use_color : 0;
	cs->is_tty = cpr__is_tty(cs->stream);
	cs->base.min_level = CPR_LOG_TRACE;
	cs->base.format = NULL;
	cs->base.write = cpr__console_write;
	cs->base.flush = cpr__console_flush;
	cs->base.destroy = cpr__console_destroy;

	return &cs->base;
}

// --- File Sink ---

typedef struct {
	CprLogSink base;
	char *path;
	CprLogRollMode roll_mode;
	size_t max_bytes;
	int max_files;
	FILE *fp;
	size_t current_size;
	int current_day;
	int current_hour;
} CprFileSink;

static void cpr__file_rotate(CprFileSink *fs)
{
	char old_path[512], new_path[512];
	int top, i;

	if (fs->fp) {
		fclose(fs->fp);
		fs->fp = NULL;
	}

	if (fs->max_files > 0) {
		snprintf(old_path, sizeof(old_path), "%s.%d", fs->path,
			 fs->max_files);
		remove(old_path);
	}

	top = fs->max_files > 0 ? fs->max_files - 1 : 999;
	for (i = top; i >= top; i--) {
		snprintf(old_path, sizeof(old_path), "%s.%d", fs->path, i);
		snprintf(new_path, sizeof(new_path), "%s.%d", fs->path, i + 1);
		rename(old_path,
		       new_path); // ignored if old_path doesn't exist.
	}

	snprintf(new_path, sizeof(new_path), "%s.1", fs->path);
	rename(fs->path, new_path);

	fs->fp = cpr__fopen(fs->path, "w");
	fs->current_size = 0;
}

static void cpr__file_maybe_rotate(CprFileSink *fs, const CprLogMessage *msg)
{
	if (fs->roll_mode == CPR_LOG_ROLL_SIZE) {
		if (fs->max_bytes > 0 && fs->current_size >= fs->max_bytes)
			cpr__file_rotate(fs);
	} else if (fs->roll_mode == CPR_LOG_ROLL_DAILY ||
		   fs->roll_mode == CPR_LOG_ROLL_HOURLY) {
		CprDateTime dt = cpr_time_local(msg->time_ms);
		if (fs->roll_mode == CPR_LOG_ROLL_DAILY &&
		    dt.yday != fs->current_day) {
			cpr__file_rotate(fs);
			fs->current_day = dt.yday;
		} else if (fs->roll_mode == CPR_LOG_ROLL_HOURLY &&
			   dt.hour != fs->current_hour) {
			cpr__file_rotate(fs);
			fs->current_hour = dt.hour;
		}
	}
}

static void cpr__file_write(CprLogSink *sink, const CprLogMessage *msg,
			    const char *buf, size_t len)
{
	CprFileSink *fs = (CprFileSink *)sink;
	cpr__file_maybe_rotate(fs, msg);

	if (!fs->fp)
		return;
	fwrite(buf, 1, len, fs->fp);
	fs->current_size += len;
}

static void cpr__file_flush(CprLogSink *sink)
{
	CprFileSink *fs = (CprFileSink *)sink;
	if (fs->fp)
		fflush(fs->fp);
}

static void cpr__file_destroy(CprLogSink *sink)
{
	CprFileSink *fs = (CprFileSink *)sink;
	if (fs->fp)
		fclose(fs->fp);
	free(fs->path);
	free(fs);
}

CprLogSink *cpr_log_file_sink(const CprFileSinkConfig *config,
			      CprResult *out_result)
{
	CprFileSink *fs;
	const char *mode;
	CprDateTime dt;

	if (!config || !config->path) {
		if (out_result)
			*out_result = CPR_ERR_INVALID;
		return NULL;
	}

	fs = malloc(sizeof(*fs));
	if (!fs) {
		if (out_result)
			*out_result = CPR_ERR_OOM;
		return NULL;
	}

	fs->path = malloc(strlen(config->path) + 1);
	if (!fs->path) {
		free(fs);
		if (out_result)
			*out_result = CPR_ERR_OOM;
		return NULL;
	}
	memcpy(fs->path, config->path, strlen(config->path) + 1);

	fs->roll_mode = config->roll_mode;
	fs->max_bytes = config->max_bytes;
	fs->max_files = config->max_files;

	mode = (config->open_mode == CPR_LOG_FILE_OVERWRITE) ? "w" : "a";
	fs->fp = cpr__fopen(config->path, mode);
	if (!fs->fp) {
		free(fs->path);
		free(fs);
		if (out_result)
			*out_result = CPR_ERR_IO;
		return NULL;
	}

	fs->current_size = 0;
	if (config->open_mode == CPR_LOG_FILE_APPEND) {
		fseek(fs->fp, 0, SEEK_END);
		fs->current_size = ftell(fs->fp);
	}

	dt = cpr_time_local(cpr_time_now());
	fs->current_day = dt.yday;
	fs->current_hour = dt.hour;

	fs->base.min_level = CPR_LOG_TRACE;
	fs->base.format = NULL;
	fs->base.write = cpr__file_write;
	fs->base.flush = cpr__file_flush;
	fs->base.destroy = cpr__file_destroy;

	if (out_result)
		*out_result = CPR_OK;
	return &fs->base;
}

// --- Callback Sink ---

typedef struct {
	CprLogSink base;
	CprLogCallbackFn callback;
	void *userdata;
} CprCallbackSink;

static void cpr__callback_write(CprLogSink *sink, const CprLogMessage *msg,
				const char *buf, size_t len)
{
	CprCallbackSink *cs = (CprCallbackSink *)sink;
	cs->callback(msg, buf, len, cs->userdata);
}

static void cpr__callback_destroy(CprLogSink *sink)
{
	free(sink);
}

CprLogSink *cpr_log_callback_sink(const CprCallbackSinkConfig *config)
{
	if (!config || !config->callback)
		return NULL;

	CprCallbackSink *cs = malloc(sizeof(*cs));
	if (!cs)
		return NULL;

	cs->callback = config->callback;
	cs->userdata = config->userdata;
	cs->base.min_level = CPR_LOG_TRACE;
	cs->base.format = NULL;
	cs->base.write = cpr__callback_write;
	cs->base.flush = NULL;
	cs->base.destroy = cpr__callback_destroy;

	return &cs->base;
}

// --- Logger ---

struct CprLogger {
	CprLogLevel min_level;
	bool thread_safe;
	CprLogFormatFn default_format;
	CprLogSink *sinks[CPR_LOG_MAX_SINKS];
	int sink_count;
	CprMutex mutex;
};

CprLogger *cpr_log_create(const CprLoggerConfig *config)
{
	CprLogger *logger = malloc(sizeof(*logger));
	if (!logger)
		return NULL;

	logger->min_level = config ? config->min_level : CPR_LOG_TRACE;
	logger->thread_safe = config ? config->thread_safe : true;
	logger->default_format = (config && config->default_format) ?
					 config->default_format :
					 cpr_log_format_default;

	logger->sink_count = 0;
	memset(logger->sinks, 0, sizeof(logger->sinks));

	if (logger->thread_safe)
		cpr_mutex_init(&logger->mutex);

	return logger;
}

void cpr_log_destroy(CprLogger *logger)
{
	if (!logger)
		return;
	cpr_log_flush(logger);

	if (logger->thread_safe)
		cpr_mutex_destroy(&logger->mutex);

	free(logger);
}

CprResult cpr_log_add_sink(CprLogger *logger, CprLogSink *sink)
{
	if (!logger || !sink)
		return CPR_ERR_INVALID;
	if (logger->sink_count >= CPR_LOG_MAX_SINKS)
		return CPR_ERR_EXHAUSTED;

	logger->sinks[logger->sink_count++] = sink;
	return CPR_OK;
}

void cpr_log_remove_sink(CprLogger *logger, CprLogSink *sink)
{
	if (!logger || !sink)
		return;

	for (int i = 0; i < logger->sink_count; i++) {
		if (logger->sinks[i] != sink)
			continue;

		memmove(&logger->sinks[i], &logger->sinks[i + 1],
			(logger->sink_count - i - 1) *
				sizeof(logger->sinks[0]));

		logger->sinks[--logger->sink_count] = NULL;
		return;
	}
}

void cpr_log_set_level(CprLogger *logger, CprLogLevel level)
{
	if (logger)
		logger->min_level = level;
}

CprLogLevel cpr_log_get_level(CprLogger *logger)
{
	return logger ? logger->min_level : CPR_LOG_OFF;
}

void cpr_log_flush(CprLogger *logger)
{
	if (!logger)
		return;

	if (logger->thread_safe)
		cpr_mutex_lock(&logger->mutex);

	for (int i = 0; i < logger->sink_count; i++) {
		if (logger->sinks[i] && logger->sinks[i]->flush)
			logger->sinks[i]->flush(logger->sinks[i]);
	}

	if (logger->thread_safe)
		cpr_mutex_unlock(&logger->mutex);
}

void cpr_log_write(CprLogger *logger, CprLogLevel level, const char *file,
		   int line, const char *func, const char *fmt, ...)
{
	va_list varargs;
	va_start(varargs, fmt);
	cpr_log_writev(logger, level, file, line, func, fmt, varargs);
	va_end(varargs);
}

void cpr_log_writev(CprLogger *logger, CprLogLevel level, const char *file,
		    int line, const char *func, const char *fmt,
		    va_list varargs)
{
	if (!logger || level < logger->min_level || level >= CPR_LOG_OFF)
		return;

	char msg[CPR_LOG_MSG_SIZE];
	int needed = vsnprintf(msg, sizeof(msg), fmt, varargs);
	CprLogMessage log_msg =
		(CprLogMessage){ .truncated = (size_t)needed >= sizeof(msg),
				 .level = level,
				 .message = msg,
				 .file = file,
				 .line = line,
				 .func = func,
				 .time_ms = cpr_time_now() };

	if (log_msg.truncated)
		cpr__write_marker(msg, sizeof(msg), CPR_LOG_MSG_MARKER);

	char out[CPR_LOG_BUFFER_SIZE];
	if (logger->thread_safe)
		cpr_mutex_lock(&logger->mutex);

	for (int i = 0; i < logger->sink_count; i++) {
		CprLogSink *sink = logger->sinks[i];
		if (!sink || level < sink->min_level)
			continue;

		CprLogFormatFn fmt_fn = sink->format ? sink->format :
						       logger->default_format;
		bool _unused; // fmt functions may not handle NULL here, so provide a stub
		int len = fmt_fn(&log_msg, out, sizeof(out), &_unused);
		if (len > 0)
			sink->write(sink, &log_msg, out, len);
	}

	if (logger->thread_safe)
		cpr_mutex_unlock(&logger->mutex);
}

// --- Global Default Logger ---

static CprLogger *S_DEFAULT = NULL;

CprLogger *cpr_log_default(void)
{
	return S_DEFAULT;
}

void cpr_log_set_default(CprLogger *logger)
{
	S_DEFAULT = logger;
}
