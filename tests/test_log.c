#include "unity.h"
#include <copper/copper.h>
#include <stdio.h>
#include <string.h>

void setUp(void)
{
}
void tearDown(void)
{
}

// --- Count sink: counts every record that reaches its write fn ---

typedef struct {
	CprLogSink base;
	int count;
} CountSink;

static void count_write(CprLogSink *sink, const CprLogMessage *msg,
			const char *buf, size_t len)
{
	(void)msg;
	(void)buf;
	(void)len;
	((CountSink *)sink)->count++;
}

static CountSink make_count_sink(void)
{
	CountSink s;
	s.base.min_level = CPR_LOG_TRACE;
	s.base.format = NULL;
	s.base.write = count_write;
	s.base.flush = NULL;
	s.base.destroy = NULL;
	s.count = 0;
	return s;
}

// --- Capture sink: stores the last CAPTURE_MAX (level, message, buf) tuples ---

#define CAPTURE_MAX 32

typedef struct {
	CprLogLevel level;
	char message[256];
	char buf[CPR_LOG_BUFFER_SIZE];
} CaptureEntry;

typedef struct {
	CprLogSink base;
	CaptureEntry entries[CAPTURE_MAX];
	int count;
} CaptureSink;

static void capture_write(CprLogSink *sink, const CprLogMessage *msg,
			  const char *buf, size_t len)
{
	CaptureSink *cs = (CaptureSink *)sink;
	if (cs->count >= CAPTURE_MAX)
		return;
	CaptureEntry *e = &cs->entries[cs->count++];
	e->level = msg->level;
	strncpy(e->message, msg->message, sizeof(e->message) - 1);
	e->message[sizeof(e->message) - 1] = '\0';
	size_t copy = len < sizeof(e->buf) - 1 ? len : sizeof(e->buf) - 1;
	memcpy(e->buf, buf, copy);
	e->buf[copy] = '\0';
}

static CaptureSink make_capture_sink(void)
{
	CaptureSink s;
	memset(&s, 0, sizeof s);
	s.base.min_level = CPR_LOG_TRACE;
	s.base.format = NULL;
	s.base.write = capture_write;
	s.base.flush = NULL;
	s.base.destroy = NULL;
	return s;
}

// --- Histogram sink: counts records per CprLogLevel ---

typedef struct {
	CprLogSink base;
	int counts[7];
} HistogramSink;

static void histogram_write(CprLogSink *sink, const CprLogMessage *msg,
			    const char *buf, size_t len)
{
	(void)buf;
	(void)len;
	HistogramSink *hs = (HistogramSink *)sink;
	if ((int)msg->level >= 0 && (int)msg->level < 7)
		hs->counts[(int)msg->level]++;
}

static HistogramSink make_histogram_sink(void)
{
	HistogramSink s;
	memset(&s, 0, sizeof s);
	s.base.min_level = CPR_LOG_TRACE;
	s.base.format = NULL;
	s.base.write = histogram_write;
	s.base.flush = NULL;
	s.base.destroy = NULL;
	return s;
}

typedef struct {
	CprLogSink base;
	CprLogSink *target;
	CprLogLevel last_level;
	char last_message[256];
	int suppressed;
} DedupSink;

static void dedup_write(CprLogSink *sink, const CprLogMessage *msg,
			const char *buf, size_t len)
{
	DedupSink *ds = (DedupSink *)sink;
	if (ds->last_level == msg->level &&
	    strcmp(ds->last_message, msg->message) == 0) {
		ds->suppressed++;
		return;
	}
	ds->last_level = msg->level;
	strncpy(ds->last_message, msg->message, sizeof(ds->last_message) - 1);
	ds->last_message[sizeof(ds->last_message) - 1] = '\0';
	ds->target->write(ds->target, msg, buf, len);
}

static DedupSink make_dedup_sink(CprLogSink *target)
{
	DedupSink s;
	memset(&s, 0, sizeof s);
	s.base.min_level = CPR_LOG_TRACE;
	s.base.format = NULL;
	s.base.write = dedup_write;
	s.base.flush = NULL;
	s.base.destroy = NULL;
	s.target = target;
	return s;
}

// --- Custom formatter ---

// A minimal JSON formatter used to test per-sink format override.
static int format_json(const CprLogMessage *msg, char *buf, size_t n,
		       bool *out_truncated)
{
	int w = snprintf(buf, n, "{\"level\":\"%s\",\"msg\":\"%s\"}\n",
			 cpr_log_level_strlc(msg->level), msg->message);
	if (out_truncated)
		*out_truncated = false;
	if (w < 0) {
		if (n)
			buf[0] = '\0';
		return 0;
	}
	if ((size_t)w >= n) {
		buf[n - 1] = '\0';
		if (out_truncated)
			*out_truncated = true;
		return (int)(n - 1);
	}
	return w;
}

// --- Helpers ---

// Build a minimal CprLogMessage for direct formatter tests.
static CprLogMessage make_msg(CprLogLevel level, const char *message)
{
	CprLogMessage m;
	m.level = level;
	m.message = message;
	m.truncated = false;
	m.file = "test.c";
	m.line = 1;
	m.func = "test_fn";
	m.time_ms = 0;
	return m;
}

// --- Level strings ---

void test_level_str_all(void)
{
	TEST_ASSERT_EQUAL_STRING("TRACE", cpr_log_level_str(CPR_LOG_TRACE));
	TEST_ASSERT_EQUAL_STRING("DEBUG", cpr_log_level_str(CPR_LOG_DEBUG));
	TEST_ASSERT_EQUAL_STRING("INFO", cpr_log_level_str(CPR_LOG_INFO));
	TEST_ASSERT_EQUAL_STRING("WARN", cpr_log_level_str(CPR_LOG_WARN));
	TEST_ASSERT_EQUAL_STRING("ERROR", cpr_log_level_str(CPR_LOG_ERROR));
	TEST_ASSERT_EQUAL_STRING("FATAL", cpr_log_level_str(CPR_LOG_FATAL));
	TEST_ASSERT_EQUAL_STRING("OFF", cpr_log_level_str(CPR_LOG_OFF));
}

void test_level_str_unknown(void)
{
	TEST_ASSERT_EQUAL_STRING("UNKNOWN", cpr_log_level_str((CprLogLevel)99));
	TEST_ASSERT_EQUAL_STRING("UNKNOWN", cpr_log_level_str((CprLogLevel)-1));
}

void test_level_strlc_all(void)
{
	TEST_ASSERT_EQUAL_STRING("trace", cpr_log_level_strlc(CPR_LOG_TRACE));
	TEST_ASSERT_EQUAL_STRING("debug", cpr_log_level_strlc(CPR_LOG_DEBUG));
	TEST_ASSERT_EQUAL_STRING("info", cpr_log_level_strlc(CPR_LOG_INFO));
	TEST_ASSERT_EQUAL_STRING("warn", cpr_log_level_strlc(CPR_LOG_WARN));
	TEST_ASSERT_EQUAL_STRING("error", cpr_log_level_strlc(CPR_LOG_ERROR));
	TEST_ASSERT_EQUAL_STRING("fatal", cpr_log_level_strlc(CPR_LOG_FATAL));
	TEST_ASSERT_EQUAL_STRING("off", cpr_log_level_strlc(CPR_LOG_OFF));
}

// --- Formatters ---

void test_format_default_basic(void)
{
	CprLogMessage m = make_msg(CPR_LOG_INFO, "hello");
	char buf[CPR_LOG_BUFFER_SIZE];
	bool trunc = true;
	int n = cpr_log_format_default(&m, buf, sizeof buf, &trunc);
	TEST_ASSERT_GREATER_THAN(0, n);
	TEST_ASSERT_FALSE(trunc);
	TEST_ASSERT_EQUAL_CHAR('\0', buf[n]);
	TEST_ASSERT_NOT_NULL(strstr(buf, "INFO"));
	TEST_ASSERT_NOT_NULL(strstr(buf, "hello"));
}

void test_format_full_contains_source_info(void)
{
	CprLogMessage m = make_msg(CPR_LOG_WARN, "oops");
	char buf[CPR_LOG_BUFFER_SIZE];
	int n = cpr_log_format_full(&m, buf, sizeof buf, NULL);
	TEST_ASSERT_GREATER_THAN(0, n);
	TEST_ASSERT_NOT_NULL(strstr(buf, "test.c"));
	TEST_ASSERT_NOT_NULL(strstr(buf, "test_fn"));
	TEST_ASSERT_NOT_NULL(strstr(buf, "oops"));
}

void test_format_minimal_shorter_than_default(void)
{
	CprLogMessage m = make_msg(CPR_LOG_ERROR, "boom");
	char min_buf[CPR_LOG_BUFFER_SIZE], def_buf[CPR_LOG_BUFFER_SIZE];
	int min_n = cpr_log_format_minimal(&m, min_buf, sizeof min_buf, NULL);
	int def_n = cpr_log_format_default(&m, def_buf, sizeof def_buf, NULL);
	TEST_ASSERT_NOT_NULL(strstr(min_buf, "ERROR"));
	TEST_ASSERT_NOT_NULL(strstr(min_buf, "boom"));
	TEST_ASSERT_LESS_THAN(def_n, min_n);
}

void test_format_null_truncated_param(void)
{
	CprLogMessage m = make_msg(CPR_LOG_DEBUG, "test");
	char buf[CPR_LOG_BUFFER_SIZE];
	int n = cpr_log_format_default(&m, buf, sizeof buf, NULL);
	TEST_ASSERT_GREATER_THAN(0, n);
}

void test_format_tiny_buffer_sets_truncated(void)
{
	CprLogMessage m = make_msg(CPR_LOG_INFO, "this will be cut off");
	char buf[16];
	bool trunc = false;
	cpr_log_format_default(&m, buf, sizeof buf, &trunc);
	TEST_ASSERT_TRUE(trunc);
	TEST_ASSERT_EQUAL_CHAR('\0', buf[sizeof buf - 1]);
}

// --- Null guards ---

void test_destroy_null(void)
{
	cpr_log_destroy(NULL);
}

void test_add_sink_null_logger(void)
{
	CountSink s = make_count_sink();
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_log_add_sink(NULL, &s.base));
}

void test_add_sink_null_sink(void)
{
	CprLogger *l = cpr_log_create(NULL);
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_log_add_sink(l, NULL));
	cpr_log_destroy(l);
}

void test_remove_sink_null_logger(void)
{
	CountSink s = make_count_sink();
	cpr_log_remove_sink(NULL, &s.base);
}

void test_remove_sink_null_sink(void)
{
	CprLogger *l = cpr_log_create(NULL);
	cpr_log_remove_sink(l, NULL);
	cpr_log_destroy(l);
}

void test_set_level_null(void)
{
	cpr_log_set_level(NULL, CPR_LOG_INFO);
}

void test_get_level_null(void)
{
	TEST_ASSERT_EQUAL_INT(CPR_LOG_OFF, cpr_log_get_level(NULL));
}

void test_flush_null(void)
{
	cpr_log_flush(NULL);
}

void test_write_null_logger(void)
{
	cpr_log_write(NULL, CPR_LOG_INFO, __FILE__, __LINE__, __func__, "noop");
}

void test_sink_destroy_null(void)
{
	cpr_log_sink_destroy(NULL);
}

// --- Logger lifecycle ---

void test_create_null_config_defaults(void)
{
	CprLogger *l = cpr_log_create(NULL);
	TEST_ASSERT_NOT_NULL(l);
	TEST_ASSERT_EQUAL_INT(CPR_LOG_TRACE, cpr_log_get_level(l));
	cpr_log_destroy(l);
}

void test_create_with_config(void)
{
	CprLoggerConfig cfg = { CPR_LOG_WARN, false, NULL };
	CprLogger *l = cpr_log_create(&cfg);
	TEST_ASSERT_NOT_NULL(l);
	TEST_ASSERT_EQUAL_INT(CPR_LOG_WARN, cpr_log_get_level(l));
	cpr_log_destroy(l);
}

void test_set_get_level(void)
{
	CprLogger *l = cpr_log_create(NULL);
	cpr_log_set_level(l, CPR_LOG_ERROR);
	TEST_ASSERT_EQUAL_INT(CPR_LOG_ERROR, cpr_log_get_level(l));
	cpr_log_set_level(l, CPR_LOG_TRACE);
	TEST_ASSERT_EQUAL_INT(CPR_LOG_TRACE, cpr_log_get_level(l));
	cpr_log_destroy(l);
}

// --- Sink management ---

void test_add_remove_sink(void)
{
	CprLogger *l = cpr_log_create(NULL);
	CountSink s = make_count_sink();

	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_log_add_sink(l, &s.base));
	cpr_log_info(l, "a");
	TEST_ASSERT_EQUAL_INT(1, s.count);

	cpr_log_remove_sink(l, &s.base);
	cpr_log_info(l, "b");
	TEST_ASSERT_EQUAL_INT(1, s.count);

	cpr_log_destroy(l);
}

void test_add_max_sinks(void)
{
	CprLogger *l = cpr_log_create(NULL);
	CountSink sinks[CPR_LOG_MAX_SINKS];

	for (int i = 0; i < CPR_LOG_MAX_SINKS; i++) {
		sinks[i] = make_count_sink();
		TEST_ASSERT_EQUAL_INT(CPR_OK,
				      cpr_log_add_sink(l, &sinks[i].base));
	}

	CountSink extra = make_count_sink();
	TEST_ASSERT_EQUAL_INT(CPR_ERR_EXHAUSTED,
			      cpr_log_add_sink(l, &extra.base));

	cpr_log_destroy(l);
}

void test_remove_nonexistent_sink(void)
{
	CprLogger *l = cpr_log_create(NULL);
	CountSink s = make_count_sink();
	cpr_log_remove_sink(l, &s.base); /* never added — must not crash */
	cpr_log_destroy(l);
}

void test_all_sinks_receive_write(void)
{
	CprLogger *l = cpr_log_create(NULL);
	CountSink a = make_count_sink();
	CountSink b = make_count_sink();
	CountSink c = make_count_sink();

	cpr_log_add_sink(l, &a.base);
	cpr_log_add_sink(l, &b.base);
	cpr_log_add_sink(l, &c.base);

	cpr_log_warn(l, "broadcast");

	TEST_ASSERT_EQUAL_INT(1, a.count);
	TEST_ASSERT_EQUAL_INT(1, b.count);
	TEST_ASSERT_EQUAL_INT(1, c.count);

	cpr_log_destroy(l);
}

void test_sink_shared_across_loggers(void)
{
	CprLogger *l1 = cpr_log_create(NULL);
	CprLogger *l2 = cpr_log_create(NULL);
	CountSink s = make_count_sink();

	cpr_log_add_sink(l1, &s.base);
	cpr_log_add_sink(l2, &s.base);

	cpr_log_info(l1, "from l1");
	cpr_log_info(l2, "from l2");

	TEST_ASSERT_EQUAL_INT(2, s.count);

	cpr_log_destroy(l1);
	cpr_log_destroy(l2);
}

// --- Level filters ---

void test_logger_level_filter(void)
{
	CprLoggerConfig cfg = { CPR_LOG_WARN, false, NULL };
	CprLogger *l = cpr_log_create(&cfg);
	CountSink s = make_count_sink();
	cpr_log_add_sink(l, &s.base);

	cpr_log_trace(l, "t");
	cpr_log_debug(l, "d");
	cpr_log_info(l, "i");
	cpr_log_warn(l, "w");
	cpr_log_error(l, "e");
	cpr_log_fatal(l, "f");

	TEST_ASSERT_EQUAL_INT(3, s.count);
	cpr_log_destroy(l);
}

void test_sink_level_filter(void)
{
	CprLogger *l = cpr_log_create(NULL);
	CountSink s = make_count_sink();
	s.base.min_level = CPR_LOG_ERROR;
	cpr_log_add_sink(l, &s.base);

	cpr_log_info(l, "i");
	cpr_log_warn(l, "w");
	cpr_log_error(l, "e");
	cpr_log_fatal(l, "f");

	TEST_ASSERT_EQUAL_INT(2, s.count);
	cpr_log_destroy(l);
}

void test_cpr_log_off_silences_all(void)
{
	CprLoggerConfig cfg = { CPR_LOG_OFF, false, NULL };
	CprLogger *l = cpr_log_create(&cfg);
	CountSink s = make_count_sink();
	cpr_log_add_sink(l, &s.base);

	cpr_log_trace(l, "t");
	cpr_log_fatal(l, "f");

	TEST_ASSERT_EQUAL_INT(0, s.count);
	cpr_log_destroy(l);
}

// --- Built-in callback sink ---

typedef struct {
	int count;
	char last_message[256];
} CallbackData;

static void log_callback(const CprLogMessage *msg, const char *buf, size_t len,
			 void *userdata)
{
	(void)buf;
	(void)len;
	CallbackData *d = (CallbackData *)userdata;
	d->count++;
	strncpy(d->last_message, msg->message, sizeof(d->last_message) - 1);
	d->last_message[sizeof(d->last_message) - 1] = '\0';
}

void test_callback_sink_null_config(void)
{
	TEST_ASSERT_NULL(cpr_log_callback_sink(NULL));
}

void test_callback_sink_null_fn(void)
{
	CprCallbackSinkConfig cfg = { NULL, NULL };
	TEST_ASSERT_NULL(cpr_log_callback_sink(&cfg));
}

void test_callback_sink_receives_message(void)
{
	CallbackData data = { 0, "" };
	CprCallbackSinkConfig cfg = { log_callback, &data };
	CprLogSink *sink = cpr_log_callback_sink(&cfg);
	TEST_ASSERT_NOT_NULL(sink);

	CprLogger *l = cpr_log_create(NULL);
	cpr_log_add_sink(l, sink);
	cpr_log_info(l, "hello callback");

	TEST_ASSERT_EQUAL_INT(1, data.count);
	TEST_ASSERT_EQUAL_STRING("hello callback", data.last_message);

	cpr_log_destroy(l);
	cpr_log_sink_destroy(sink);
}

void test_callback_sink_respects_level(void)
{
	CallbackData data = { 0, "" };
	CprCallbackSinkConfig cfg = { log_callback, &data };
	CprLogSink *sink = cpr_log_callback_sink(&cfg);
	sink->min_level = CPR_LOG_ERROR;

	CprLogger *l = cpr_log_create(NULL);
	cpr_log_add_sink(l, sink);
	cpr_log_info(l, "filtered");
	cpr_log_error(l, "passes");

	TEST_ASSERT_EQUAL_INT(1, data.count);
	TEST_ASSERT_EQUAL_STRING("passes", data.last_message);

	cpr_log_destroy(l);
	cpr_log_sink_destroy(sink);
}

// --- Built-in console sink ---

void test_console_sink_stdout(void)
{
	CprConsoleSinkConfig cfg = { false, false };
	CprLogSink *sink = cpr_log_console_sink(&cfg);
	TEST_ASSERT_NOT_NULL(sink);
	cpr_log_sink_destroy(sink);
}

void test_console_sink_stderr_with_color(void)
{
	CprConsoleSinkConfig cfg = { true, true };
	CprLogSink *sink = cpr_log_console_sink(&cfg);
	TEST_ASSERT_NOT_NULL(sink);
	cpr_log_sink_destroy(sink);
}

void test_console_sink_null_config(void)
{
	CprLogSink *sink = cpr_log_console_sink(NULL);
	TEST_ASSERT_NOT_NULL(sink);
	cpr_log_sink_destroy(sink);
}

// --- Built-in file sink ---

#define TEST_LOG_PATH "/tmp/cpr_test_log.log"

void test_file_sink_null_config(void)
{
	CprResult r = CPR_OK;
	TEST_ASSERT_NULL(cpr_log_file_sink(NULL, &r));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, r);
}

void test_file_sink_null_path(void)
{
	CprFileSinkConfig cfg = { NULL, CPR_LOG_FILE_APPEND, CPR_LOG_ROLL_NONE,
				  0, 0 };
	CprResult r = CPR_OK;
	TEST_ASSERT_NULL(cpr_log_file_sink(&cfg, &r));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, r);
}

void test_file_sink_bad_path(void)
{
	CprFileSinkConfig cfg = { "/no/such/dir/foo.log", CPR_LOG_FILE_APPEND,
				  CPR_LOG_ROLL_NONE, 0, 0 };
	CprResult r = CPR_OK;
	TEST_ASSERT_NULL(cpr_log_file_sink(&cfg, &r));
	TEST_ASSERT_EQUAL_INT(CPR_ERR_IO, r);
}

void test_file_sink_writes_content(void)
{
	remove(TEST_LOG_PATH);

	CprFileSinkConfig cfg = { TEST_LOG_PATH, CPR_LOG_FILE_OVERWRITE,
				  CPR_LOG_ROLL_NONE, 0, 0 };
	CprResult r = CPR_ERR_INVALID;
	CprLogSink *sink = cpr_log_file_sink(&cfg, &r);
	TEST_ASSERT_EQUAL_INT(CPR_OK, r);
	TEST_ASSERT_NOT_NULL(sink);

	CprLogger *l = cpr_log_create(NULL);
	cpr_log_add_sink(l, sink);
	cpr_log_info(l, "written to file");
	cpr_log_flush(l);
	cpr_log_destroy(l);
	cpr_log_sink_destroy(sink);

	FILE *fp = fopen(TEST_LOG_PATH, "r");
	TEST_ASSERT_NOT_NULL(fp);
	char buf[512] = { 0 };
	(void)fread(buf, 1, sizeof buf - 1, fp);
	fclose(fp);
	remove(TEST_LOG_PATH);

	TEST_ASSERT_NOT_NULL(strstr(buf, "written to file"));
}

void test_file_sink_overwrite_clears_file(void)
{
	remove(TEST_LOG_PATH);

	/* First pass: write a unique marker. */
	CprFileSinkConfig cfg = { TEST_LOG_PATH, CPR_LOG_FILE_OVERWRITE,
				  CPR_LOG_ROLL_NONE, 0, 0 };
	CprLogSink *sink = cpr_log_file_sink(&cfg, NULL);
	CprLogger *l = cpr_log_create(NULL);
	cpr_log_add_sink(l, sink);
	cpr_log_info(l, "first pass");
	cpr_log_flush(l);
	cpr_log_destroy(l);
	cpr_log_sink_destroy(sink);

	/* Second pass: overwrite mode must not see "first pass". */
	sink = cpr_log_file_sink(&cfg, NULL);
	l = cpr_log_create(NULL);
	cpr_log_add_sink(l, sink);
	cpr_log_info(l, "second pass");
	cpr_log_flush(l);
	cpr_log_destroy(l);
	cpr_log_sink_destroy(sink);

	FILE *fp = fopen(TEST_LOG_PATH, "r");
	TEST_ASSERT_NOT_NULL(fp);
	char buf[512] = { 0 };
	(void)fread(buf, 1, sizeof buf - 1, fp);
	fclose(fp);
	remove(TEST_LOG_PATH);

	TEST_ASSERT_NULL(strstr(buf, "first pass"));
	TEST_ASSERT_NOT_NULL(strstr(buf, "second pass"));
}

void test_file_sink_append_preserves_content(void)
{
	remove(TEST_LOG_PATH);

	CprFileSinkConfig cfg = { TEST_LOG_PATH, CPR_LOG_FILE_APPEND,
				  CPR_LOG_ROLL_NONE, 0, 0 };

	CprLogSink *sink = cpr_log_file_sink(&cfg, NULL);
	CprLogger *l = cpr_log_create(NULL);
	cpr_log_add_sink(l, sink);
	cpr_log_info(l, "first");
	cpr_log_flush(l);
	cpr_log_destroy(l);
	cpr_log_sink_destroy(sink);

	sink = cpr_log_file_sink(&cfg, NULL);
	l = cpr_log_create(NULL);
	cpr_log_add_sink(l, sink);
	cpr_log_info(l, "second");
	cpr_log_flush(l);
	cpr_log_destroy(l);
	cpr_log_sink_destroy(sink);

	FILE *fp = fopen(TEST_LOG_PATH, "r");
	TEST_ASSERT_NOT_NULL(fp);
	char buf[512] = { 0 };
	(void)fread(buf, 1, sizeof buf - 1, fp);
	fclose(fp);
	remove(TEST_LOG_PATH);

	TEST_ASSERT_NOT_NULL(strstr(buf, "first"));
	TEST_ASSERT_NOT_NULL(strstr(buf, "second"));
}

// --- Macros ---

void test_all_level_macros(void)
{
	CprLogger *l = cpr_log_create(NULL);
	HistogramSink h = make_histogram_sink();
	cpr_log_add_sink(l, &h.base);

	cpr_log_trace(l, "t");
	cpr_log_debug(l, "d");
	cpr_log_info(l, "i");
	cpr_log_warn(l, "w");
	cpr_log_error(l, "e");
	cpr_log_fatal(l, "f");

	TEST_ASSERT_EQUAL_INT(1, h.counts[CPR_LOG_TRACE]);
	TEST_ASSERT_EQUAL_INT(1, h.counts[CPR_LOG_DEBUG]);
	TEST_ASSERT_EQUAL_INT(1, h.counts[CPR_LOG_INFO]);
	TEST_ASSERT_EQUAL_INT(1, h.counts[CPR_LOG_WARN]);
	TEST_ASSERT_EQUAL_INT(1, h.counts[CPR_LOG_ERROR]);
	TEST_ASSERT_EQUAL_INT(1, h.counts[CPR_LOG_FATAL]);

	cpr_log_destroy(l);
}

void test_default_logger_macros(void)
{
	CprLogger *l = cpr_log_create(NULL);
	CountSink s = make_count_sink();
	cpr_log_add_sink(l, &s.base);
	cpr_log_set_default(l);

	cpr_trace("trace via default");
	cpr_debug("debug via default");
	cpr_info("info via default");
	cpr_warn("warn via default");
	cpr_error("error via default");
	cpr_fatal("fatal via default");

	TEST_ASSERT_EQUAL_INT(6, s.count);

	cpr_log_set_default(NULL);
	cpr_log_destroy(l);
}

// --- Default logger ---

void test_default_logger_set_get(void)
{
	CprLogger *l = cpr_log_create(NULL);
	cpr_log_set_default(l);
	TEST_ASSERT_EQUAL_PTR(l, cpr_log_default());
	cpr_log_set_default(NULL);
	TEST_ASSERT_NULL(cpr_log_default());
	cpr_log_destroy(l);
}

// --- Message truncation ---

void test_message_truncation(void)
{
	// Build a message longer than CPR_LOG_MSG_SIZE so it is truncated.
	char long_msg[CPR_LOG_MSG_SIZE + 64];
	memset(long_msg, 'A', sizeof long_msg - 1);
	long_msg[sizeof long_msg - 1] = '\0';

	CprLogger *l = cpr_log_create(NULL);
	CaptureSink cap = make_capture_sink();
	cpr_log_add_sink(l, &cap.base);

	cpr_log_info(l, "%s", long_msg);

	TEST_ASSERT_EQUAL_INT(1, cap.count);
	/* the truncation marker must appear in the captured formatted output */
	TEST_ASSERT_NOT_NULL(strstr(cap.entries[0].buf, "[truncated]"));

	cpr_log_destroy(l);
}

// --- Edge-case sinks ---

void test_histogram_sink(void)
{
	CprLogger *l = cpr_log_create(NULL);
	HistogramSink h = make_histogram_sink();
	cpr_log_add_sink(l, &h.base);

	for (int i = 0; i < 3; i++)
		cpr_log_trace(l, "t%d", i);
	for (int i = 0; i < 2; i++)
		cpr_log_warn(l, "w%d", i);
	cpr_log_error(l, "e");

	TEST_ASSERT_EQUAL_INT(3, h.counts[CPR_LOG_TRACE]);
	TEST_ASSERT_EQUAL_INT(0, h.counts[CPR_LOG_DEBUG]);
	TEST_ASSERT_EQUAL_INT(0, h.counts[CPR_LOG_INFO]);
	TEST_ASSERT_EQUAL_INT(2, h.counts[CPR_LOG_WARN]);
	TEST_ASSERT_EQUAL_INT(1, h.counts[CPR_LOG_ERROR]);
	TEST_ASSERT_EQUAL_INT(0, h.counts[CPR_LOG_FATAL]);

	cpr_log_destroy(l);
}

void test_dedup_sink(void)
{
	CprLogger *l = cpr_log_create(NULL);
	CountSink target = make_count_sink();
	DedupSink dedup = make_dedup_sink(&target.base);
	cpr_log_add_sink(l, &dedup.base);

	cpr_log_info(l, "hello"); /* new → forwarded */
	cpr_log_info(l, "hello"); /* dup → suppressed */
	cpr_log_info(l, "hello"); /* dup → suppressed */
	cpr_log_warn(l, "hello"); /* different level → forwarded */
	cpr_log_warn(l, "world"); /* different message → forwarded */

	TEST_ASSERT_EQUAL_INT(3, target.count);
	TEST_ASSERT_EQUAL_INT(2, dedup.suppressed);

	cpr_log_destroy(l);
}

void test_custom_formatter_per_sink(void)
{
	CprLogger *l = cpr_log_create(NULL);
	CaptureSink cap = make_capture_sink();
	cap.base.format = format_json;
	cpr_log_add_sink(l, &cap.base);

	cpr_log_error(l, "disk full");

	TEST_ASSERT_EQUAL_INT(1, cap.count);
	TEST_ASSERT_NOT_NULL(strstr(cap.entries[0].buf, "\"level\":\"error\""));
	TEST_ASSERT_NOT_NULL(
		strstr(cap.entries[0].buf, "\"msg\":\"disk full\""));

	cpr_log_destroy(l);
}

// --- Thread safety ---

#define LOG_THREAD_COUNT 8
#define LOG_WRITES_PER_THR 500

typedef struct {
	CprLogger *logger;
} WriteArgs;

static void write_worker(void *arg)
{
	WriteArgs *a = (WriteArgs *)arg;
	for (int i = 0; i < LOG_WRITES_PER_THR; i++)
		cpr_log_info(a->logger, "msg %d", i);
}

void test_thread_safe_concurrent_writes(void)
{
	CprLoggerConfig cfg = { CPR_LOG_TRACE, true, NULL };
	CprLogger *l = cpr_log_create(&cfg);
	CountSink s = make_count_sink();
	cpr_log_add_sink(l, &s.base);

	WriteArgs args = { l };
	CprThread *threads[LOG_THREAD_COUNT];

	for (int i = 0; i < LOG_THREAD_COUNT; i++)
		threads[i] = cpr_thrd_create(write_worker, &args, NULL);

	for (int i = 0; i < LOG_THREAD_COUNT; i++)
		cpr_thrd_join(threads[i]);

	TEST_ASSERT_EQUAL_INT(LOG_THREAD_COUNT * LOG_WRITES_PER_THR, s.count);
	cpr_log_destroy(l);
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_level_str_all);
	RUN_TEST(test_level_str_unknown);
	RUN_TEST(test_level_strlc_all);

	RUN_TEST(test_format_default_basic);
	RUN_TEST(test_format_full_contains_source_info);
	RUN_TEST(test_format_minimal_shorter_than_default);
	RUN_TEST(test_format_null_truncated_param);
	RUN_TEST(test_format_tiny_buffer_sets_truncated);

	RUN_TEST(test_destroy_null);
	RUN_TEST(test_add_sink_null_logger);
	RUN_TEST(test_add_sink_null_sink);
	RUN_TEST(test_remove_sink_null_logger);
	RUN_TEST(test_remove_sink_null_sink);
	RUN_TEST(test_set_level_null);
	RUN_TEST(test_get_level_null);
	RUN_TEST(test_flush_null);
	RUN_TEST(test_write_null_logger);
	RUN_TEST(test_sink_destroy_null);

	RUN_TEST(test_create_null_config_defaults);
	RUN_TEST(test_create_with_config);
	RUN_TEST(test_set_get_level);

	RUN_TEST(test_add_remove_sink);
	RUN_TEST(test_add_max_sinks);
	RUN_TEST(test_remove_nonexistent_sink);
	RUN_TEST(test_all_sinks_receive_write);
	RUN_TEST(test_sink_shared_across_loggers);

	RUN_TEST(test_logger_level_filter);
	RUN_TEST(test_sink_level_filter);
	RUN_TEST(test_cpr_log_off_silences_all);

	RUN_TEST(test_callback_sink_null_config);
	RUN_TEST(test_callback_sink_null_fn);
	RUN_TEST(test_callback_sink_receives_message);
	RUN_TEST(test_callback_sink_respects_level);

	RUN_TEST(test_console_sink_stdout);
	RUN_TEST(test_console_sink_stderr_with_color);
	RUN_TEST(test_console_sink_null_config);

	RUN_TEST(test_file_sink_null_config);
	RUN_TEST(test_file_sink_null_path);
	RUN_TEST(test_file_sink_bad_path);
	RUN_TEST(test_file_sink_writes_content);
	RUN_TEST(test_file_sink_overwrite_clears_file);
	RUN_TEST(test_file_sink_append_preserves_content);

	RUN_TEST(test_all_level_macros);
	RUN_TEST(test_default_logger_macros);

	RUN_TEST(test_default_logger_set_get);

	RUN_TEST(test_message_truncation);

	RUN_TEST(test_histogram_sink);
	RUN_TEST(test_dedup_sink);
	RUN_TEST(test_custom_formatter_per_sink);

	RUN_TEST(test_thread_safe_concurrent_writes);

	return UNITY_END();
}
