#include "copper/copper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Custom sink: accumulates log output into an in-memory buffer.
// Demonstrates how to implement a CprLogSink from scratch.
#define MEMBUF_SIZE (1024 * 16)

typedef struct {
	CprLogSink base;
	char buf[MEMBUF_SIZE];
	size_t len;
} MemSink;

static void mem_write(CprLogSink *sink, const CprLogMessage *msg,
		      const char *buf, size_t len)
{
	(void)msg;
	MemSink *ms = (MemSink *)sink;
	size_t space = MEMBUF_SIZE - ms->len;
	size_t copy = len < space ? len : space;
	memcpy(ms->buf + ms->len, buf, copy);
	ms->len += copy;
}

static MemSink make_mem_sink(void)
{
	MemSink s;
	memset(&s, 0, sizeof s);
	s.base.min_level = CPR_LOG_TRACE;
	s.base.format = NULL;
	s.base.write = mem_write;
	s.base.flush = NULL;
	s.base.destroy = NULL;
	return s;
}

/// Simulate a small server lifecycle to drive log output.
static void simulate_server(CprLogger *log)
{
	cpr_log_info(log, "server starting on port %d", 8080);
	cpr_log_debug(log, "loading config from '/etc/myapp/config.toml'");
	cpr_log_trace(log, "config key 'max_connections' = 256");
	cpr_log_trace(log, "config key 'timeout_ms' = 5000");

	cpr_log_info(log, "connecting to database at 'db.local:5432'");
	cpr_log_debug(log, "opening connection pool (min=2, max=10)");

	// simulate a slow replica
	cpr_log_warn(log, "replica 'db-replica-2' is 4.2s behind primary");

	cpr_log_info(log, "database ready — pool size: 4");
	cpr_log_info(log, "listening for connections");

	// simulate a request
	cpr_log_debug(log, "accepted connection from 192.168.1.42:51200");
	cpr_log_trace(log, "dispatching request: GET /api/users");
	cpr_log_debug(log, "query executed in 12ms, 8 rows returned");
	cpr_log_trace(log, "sending response: 200 OK (1.4 KB)");

	// simulate a transient error
	cpr_log_error(log, "query timeout on replica (retrying on primary)");
	cpr_log_debug(log, "retry succeeded in 42ms");

	cpr_log_info(log, "graceful shutdown requested");
	cpr_log_debug(log, "draining %d active connections", 3);
	cpr_log_info(log, "shutdown complete");
}

int main(void)
{
	// --- 1. Console sink (TRACE and above, colours auto-detected) ---

	CprConsoleSinkConfig console_cfg = { .use_color = true,
					     .use_stderr = false };
	CprLogSink *console = cpr_log_console_sink(&console_cfg);

	// --- 2. File sink (DEBUG and above) ---

	CprFileSinkConfig file_cfg = { .path = "server.log",
				       .open_mode = CPR_LOG_FILE_OVERWRITE,
				       .roll_mode = CPR_LOG_ROLL_NONE };
	CprResult file_result;
	CprLogSink *file = cpr_log_file_sink(&file_cfg, &file_result);
	if (!file) {
		fprintf(stderr, "failed to open log file: %d\n", file_result);
		return 1;
	}
	file->min_level = CPR_LOG_DEBUG;

	// --- 3. In-memory sink (WARN and above only) ---

	MemSink mem = make_mem_sink();
	mem.base.min_level = CPR_LOG_WARN;

	// --- 4. Logger ---

	CprLoggerConfig logger_cfg = { .min_level = CPR_LOG_TRACE,
				       .thread_safe = false,
				       .default_format = NULL };
	CprLogger *log = cpr_log_create(&logger_cfg);

	cpr_log_add_sink(log, console);
	cpr_log_add_sink(log, file);
	cpr_log_add_sink(log, &mem.base);

	// --- 5. Run the simulation ---

	simulate_server(log);

	// --- 6. Set as default so the shortcut macros work ---

	cpr_log_set_default(log);

	cpr_info("default logger is now active");
	cpr_debug("cpr_trace / cpr_debug / cpr_info / etc. route here");

	// --- 7. Show what the in-memory (WARN+) sink captured ---

	cpr_log_flush(log);

	printf("\n--- warnings and above captured by the in-memory sink ---\n");
	if (mem.len > 0)
		printf("%.*s", (int)mem.len, mem.buf);
	else
		printf("(none)\n");

	// --- 8. Clean up ---

	cpr_log_set_default(NULL);
	cpr_log_destroy(log);
	cpr_log_sink_destroy(file);
	cpr_log_sink_destroy(console);

	printf("\nfull log written to: server.log\n");
	return 0;
}
