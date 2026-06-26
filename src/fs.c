#include "copper/fs.h"
#include "copper/internal/int_error.h"

#include "copper/result.h"
#include "copper/time.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(CPR_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <errno.h> // IWYU pragma: keep
#include <sys/stat.h>
#include <unistd.h>
#endif

// --- Internal Helpers ---

static int cpr__fs_is_sep(char c)
{
#if defined(CPR_PLATFORM_WINDOWS)
	return c == '/' || c == '\\';
#else
	return c == '/';
#endif
}

#if defined(CPR_PLATFORM_WINDOWS)

#define CPR__FILETIME_UNIX_OFFSET 116444736000000000ULL
#define CPR__FILETIME_PER_MS \
	(cpr_ms_to_ns(1ULL) / 100ULL) // 100 ns ticks per ms

static bool cpr__to_wide(const char *utf8, wchar_t *out, int out_size)
{
	int n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, out, out_size);
	return n > 0;
}

static bool cpr__to_utf8(const wchar_t *wide, char *out, int out_size)
{
	int n = WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, out_size, NULL,
				    NULL);
	return n > 0;
}

static uint64_t cpr__filetime_to_ms(FILETIME ft)
{
	uint64_t t = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
	return (t - CPR__FILETIME_UNIX_OFFSET) / CPR__FILETIME_PER_MS;
}

#endif // CPR_PLATFORM_WINDOWS

// --- Path Utilities ---

bool cpr_path_is_abs(const char *path)
{
	if (!path)
		return false;

#if defined(CPR_PLATFORM_WINDOWS)
	if (cpr__fs_is_sep(path[0]) && cpr__fs_is_sep(path[1]))
		return true;

	if (path[0] && path[1] == ':' && cpr__fs_is_sep(path[2]))
		return true;

	return false;
#else
	return path[0] == '/';
#endif
}

const char *cpr_path_basename(const char *path)
{
	if (!path)
		return NULL;

	const char *last = path;
	for (const char *p = path; *p; p++) {
		if (cpr__fs_is_sep(*p) && *(p + 1))
			last = p + 1;
	}
	return last;
}

const char *cpr_path_ext(const char *path)
{
	if (!path)
		return NULL;

	const char *base = cpr_path_basename(path);
	const char *dot = strrchr(base, '.');

	return (dot && dot != base) ? dot : "";
}

bool cpr_path_dirname(char *buf, size_t buf_size, const char *path)
{
	if (!buf || !buf_size || !path) {
		cpr__set_error(CPR_ERR_INVALID, "NULL argument");
		return false;
	}

	const char *last_sep = NULL;
	for (const char *p = path; *p; p++) {
		if (cpr__fs_is_sep(*p))
			last_sep = p;
	}

	if (!last_sep) {
		if (buf_size < 2) {
			cpr__set_error(CPR_ERR_OVERFLOW, "buffer too small");
			return false;
		}
		buf[0] = '.';
		buf[1] = '\0';
		return true;
	}

	size_t len = (size_t)(last_sep - path);
	if (len == 0)
		len = 1;

	if (len + 1 > buf_size) {
		cpr__set_error(CPR_ERR_OVERFLOW, "buffer too small");
		return false;
	}

	memcpy(buf, path, len);
	buf[len] = '\0';
	return true;
}

bool cpr_path_join(char *buf, size_t buf_size, const char *base,
		   const char *part)
{
	if (!buf || !buf_size || !base || !part) {
		cpr__set_error(CPR_ERR_INVALID, "NULL argument");
		return false;
	}

	if (cpr_path_is_abs(part)) {
		size_t plen = strlen(part);
		if (plen + 1 > buf_size) {
			cpr__set_error(CPR_ERR_OVERFLOW, "buffer too small");
			return false;
		}
		memcpy(buf, part, plen + 1);
		return true;
	}

	size_t blen = strlen(base);
	size_t plen = strlen(part);
	int need_sep =
		(blen > 0 && !cpr__fs_is_sep(base[blen - 1]) && plen > 0);
	size_t needed = blen + (size_t)need_sep + plen + 1;

	if (needed > buf_size) {
		cpr__set_error(CPR_ERR_OVERFLOW, "buffer too small");
		return false;
	}

	memcpy(buf, base, blen);
	if (need_sep)
		buf[blen++] = CPR_FS_SEP;
	memcpy(buf + blen, part, plen + 1);
	return true;
}

bool cpr_normalize_path(char *buf, size_t buf_size, const char *path)
{
	if (!buf || !buf_size || !path) {
		cpr__set_error(CPR_ERR_INVALID, "NULL argument");
		return false;
	}

	size_t plen = strlen(path);
	if (plen + 1 > buf_size) {
		cpr__set_error(CPR_ERR_OVERFLOW, "buffer too small");
		return false;
	}

	int is_abs = cpr_path_is_abs(path);
	char *out = buf;
	int ncomp = 0;
	const char *p = path;
	char *comp_ends[256]; // 256 levels of nesting

	if (is_abs) {
#if defined(CPR_PLATFORM_WINDOWS)
		if (cpr__fs_is_sep(p[0]) && cpr__fs_is_sep(p[1])) {
			// UNC: copy \\server\share as a single prefix
			*out++ = '\\';
			*out++ = '\\';
			p += 2;
			while (*p && !cpr__fs_is_sep(*p))
				*out++ = *p++;
			if (*p) {
				*out++ = '\\';
				p++;
			}
			while (*p && !cpr__fs_is_sep(*p))
				*out++ = *p++;
			if (*p)
				p++;
		} else if (p[0] && p[1] == ':') {
			*out++ = *p++;
			*out++ = *p++; // C:
			if (cpr__fs_is_sep(*p)) {
				*out++ = '\\';
				p++;
			}
		}
#else
		*out++ = '/';
		while (*p && cpr__fs_is_sep(*p))
			p++;
#endif
	}

	while (*p) {
		const char *start = p;
		while (*p && !cpr__fs_is_sep(*p))
			p++;
		size_t len = (size_t)(p - start);

		if (len == 0 || (len == 1 && start[0] == '.')) {
			// skipped
		} else if (len == 2 && start[0] == '.' && start[1] == '.') {
			if (ncomp > 0) {
				out = comp_ends[--ncomp];
			} else if (!is_abs) {
				if (out != buf && !cpr__fs_is_sep(*(out - 1)))
					*out++ = CPR_FS_SEP;
				*out++ = '.';
				*out++ = '.';
				comp_ends[ncomp++] = out;
			}
		} else {
			char *comp_start = out; // position before sep+component
			if (out != buf && !cpr__fs_is_sep(*(out - 1)))
				*out++ = CPR_FS_SEP;
			memcpy(out, start, len);
			out += len;
			if (ncomp < 256)
				comp_ends[ncomp++] = comp_start;
		}

		while (*p && cpr__fs_is_sep(*p))
			p++;
	}

	if (out == buf ||
	    (out == buf + 1 && (buf[0] == '/' || buf[0] == '\\'))) {
		if (!is_abs)
			*out++ = '.';
	}
	*out = '\0';
	return true;
}

bool cpr_cwd(char *buf, size_t buf_size)
{
	if (!buf || !buf_size) {
		cpr__set_error(CPR_ERR_INVALID, "NULL argument");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	{
		wchar_t wtmp[CPR_FS_PATH_MAX];
		DWORD n = GetCurrentDirectoryW(CPR_FS_PATH_MAX, wtmp);
		if (n == 0 || n >= CPR_FS_PATH_MAX) {
			cpr__set_error(CPR_ERR_IO,
				       "GetCurrentDirectoryW failed");
			return false;
		}
		if (!cpr__to_utf8(wtmp, buf, (int)buf_size)) {
			cpr__set_error(CPR_ERR_OVERFLOW, "buffer too small");
			return false;
		}
		return true;
	}
#else
	if (getcwd(buf, buf_size) == NULL) {
		cpr__set_error(CPR_ERR_IO, "getcwd failed");
		return false;
	}
	return true;
#endif
}

// --- Stat ---

bool cpr_fs_stat(const char *path, CprFsStat *out_stat)
{
	if (!path || !out_stat) {
		cpr__set_error(CPR_ERR_INVALID, "NULL argument");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	{
		wchar_t wpath[CPR_FS_PATH_MAX];
		WIN32_FILE_ATTRIBUTE_DATA info;
		if (!cpr__to_wide(path, wpath, CPR_FS_PATH_MAX)) {
			cpr__set_error(CPR_ERR_INVALID, "path encoding error");
			return false;
		}
		if (!GetFileAttributesExW(wpath, GetFileExInfoStandard,
					  &info)) {
			cpr__set_error(CPR_ERR_IO,
				       "GetFileAttributesExW failed");
			return false;
		}
		if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			out_stat->size = 0;
			out_stat->type = CPR_FS_TYPE_DIR;
		} else {
			out_stat->size = ((uint64_t)info.nFileSizeHigh << 32) |
					 info.nFileSizeLow;
			out_stat->type = CPR_FS_TYPE_FILE;
		}
		out_stat->mtime_ms = cpr__filetime_to_ms(info.ftLastWriteTime);
		return true;
	}
#else
	{
		struct stat st;
		if (stat(path, &st) != 0) {
			cpr__set_error(CPR_ERR_IO, "stat failed");
			return false;
		}
		out_stat->size = S_ISDIR(st.st_mode) ? 0 : (uint64_t)st.st_size;
		if (S_ISREG(st.st_mode))
			out_stat->type = CPR_FS_TYPE_FILE;
		else if (S_ISDIR(st.st_mode))
			out_stat->type = CPR_FS_TYPE_DIR;
		else
			out_stat->type = CPR_FS_TYPE_OTHER;
#if defined(CPR_PLATFORM_MACOS) || defined(CPR_PLATFORM_IOS)
		out_stat->mtime_ms =
			cpr_s_to_ms((uint64_t)st.st_mtimespec.tv_sec) +
			cpr_ns_to_ms((uint64_t)st.st_mtimespec.tv_nsec);
#elif defined(CPR_PLATFORM_LINUX) || defined(CPR_PLATFORM_ANDROID) || \
	defined(CPR_PLATFORM_FREEBSD) || defined(CPR_PLATFORM_OPENBSD)
		out_stat->mtime_ms = cpr_s_to_ms((uint64_t)st.st_mtim.tv_sec) +
				     cpr_ns_to_ms((uint64_t)st.st_mtim.tv_nsec);
#else
		out_stat->mtime_ms = cpr_s_to_ms((uint64_t)st.st_mtime);
#endif
		return true;
	}
#endif
}

// Predicates use OS calls directly to avoid polluting the error state.

bool cpr_path_exists(const char *path)
{
	if (!path)
		return false;
#if defined(CPR_PLATFORM_WINDOWS)
	wchar_t wpath[CPR_FS_PATH_MAX];
	return cpr__to_wide(path, wpath, CPR_FS_PATH_MAX) &&
	       GetFileAttributesW(wpath) != INVALID_FILE_ATTRIBUTES;
#else
	struct stat st;
	return stat(path, &st) == 0;
#endif
}

bool cpr_path_is_file(const char *path)
{
	if (!path)
		return false;
#if defined(CPR_PLATFORM_WINDOWS)
	wchar_t wpath[CPR_FS_PATH_MAX];
	if (!cpr__to_wide(path, wpath, CPR_FS_PATH_MAX))
		return false;
	DWORD attr = GetFileAttributesW(wpath);
	return attr != INVALID_FILE_ATTRIBUTES &&
	       !(attr & FILE_ATTRIBUTE_DIRECTORY);
#else
	struct stat st;
	return stat(path, &st) == 0 && S_ISREG(st.st_mode);
#endif
}

bool cpr_path_is_dir(const char *path)
{
	if (!path)
		return false;
#if defined(CPR_PLATFORM_WINDOWS)
	wchar_t wpath[CPR_FS_PATH_MAX];
	if (!cpr__to_wide(path, wpath, CPR_FS_PATH_MAX))
		return false;
	DWORD attr = GetFileAttributesW(wpath);
	return attr != INVALID_FILE_ATTRIBUTES &&
	       (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
	struct stat st;
	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

// --- Filesystem Operations ---

bool cpr_mkdir(const char *path)
{
	if (!path) {
		cpr__set_error(CPR_ERR_INVALID, "path is NULL");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	{
		wchar_t wpath[CPR_FS_PATH_MAX];
		if (!cpr__to_wide(path, wpath, CPR_FS_PATH_MAX)) {
			cpr__set_error(CPR_ERR_INVALID, "path encoding error");
			return false;
		}
		if (!CreateDirectoryW(wpath, NULL)) {
			cpr__set_error(CPR_ERR_IO, "CreateDirectoryW failed");
			return false;
		}
		return true;
	}
#else
	if (mkdir(path, 0777) != 0) {
		cpr__set_error(CPR_ERR_IO, "mkdir failed");
		return false;
	}
	return true;
#endif
}

bool cpr_mkdir_all(const char *path)
{
	if (!path) {
		cpr__set_error(CPR_ERR_INVALID, "path is NULL");
		return false;
	}

	size_t len = strlen(path);
	if (len == 0 || len >= CPR_FS_PATH_MAX) {
		cpr__set_error(CPR_ERR_INVALID, "invalid path length");
		return false;
	}

	char tmp[CPR_FS_PATH_MAX];
	memcpy(tmp, path, len + 1);

	char *p = tmp;

#if defined(CPR_PLATFORM_WINDOWS)
	if (len >= 2 && tmp[1] == ':')
		p += 2;
#endif

	while (*p && cpr__fs_is_sep(*p))
		p++;

	for (; *p; p++) {
		if (!cpr__fs_is_sep(*p))
			continue;
		*p = '\0';
		if (!cpr_path_is_dir(tmp)) {
			if (!cpr_mkdir(tmp))
				return false;
		}
		*p = CPR_FS_SEP;
	}

	if (!cpr_path_is_dir(tmp))
		return cpr_mkdir(tmp);
	return true;
}

bool cpr_remove_file(const char *path)
{
	if (!path) {
		cpr__set_error(CPR_ERR_INVALID, "path is NULL");
		return false;
	}
#if defined(CPR_PLATFORM_WINDOWS)
	{
		wchar_t wpath[CPR_FS_PATH_MAX];
		if (!cpr__to_wide(path, wpath, CPR_FS_PATH_MAX)) {
			cpr__set_error(CPR_ERR_INVALID, "path encoding error");
			return false;
		}
		if (!DeleteFileW(wpath)) {
			cpr__set_error(CPR_ERR_IO, "DeleteFileW failed");
			return false;
		}
		return true;
	}
#else
	if (unlink(path) != 0) {
		cpr__set_error(CPR_ERR_IO, "unlink failed");
		return false;
	}
	return true;
#endif
}

static CprResult cpr__rmdir(const char *path)
{
#if defined(CPR_PLATFORM_WINDOWS)
	wchar_t wpath[CPR_FS_PATH_MAX];
	if (!cpr__to_wide(path, wpath, CPR_FS_PATH_MAX)) {
		cpr__set_error(CPR_ERR_INVALID, "path encoding error");
		return CPR_ERR_INVALID;
	}
	if (!RemoveDirectoryW(wpath)) {
		cpr__set_error(CPR_ERR_IO, "RemoveDirectoryW failed");
		return CPR_ERR_IO;
	}
	return CPR_OK;
#else
	if (rmdir(path) != 0) {
		cpr__set_error(CPR_ERR_IO, "rmdir failed");
		return CPR_ERR_IO;
	}
	return CPR_OK;
#endif
}

// Forward decl needed for recursion
static CprResult cpr__rmdir_force(const char *path);
static CprResult cpr__rmdir_force(const char *path)
{
	CprDirIterator *it = cpr_open_dir(path);
	if (!it)
		return CPR_ERR_IO;

	CprDirEntry entry;
	while (cpr_next_dir(it, &entry)) {
		char child[CPR_FS_PATH_MAX];
		if (!cpr_path_join(child, sizeof(child), path, entry.name)) {
			cpr_close_dir(it);
			return CPR_ERR_OVERFLOW;
		}
		CprResult r =
			(entry.type == CPR_FS_TYPE_DIR) ?
				cpr__rmdir_force(child) :
				(cpr_remove_file(child) ? CPR_OK : CPR_ERR_IO);
		if (cpr_err(r)) {
			cpr_close_dir(it);
			return r;
		}
	}
	cpr_close_dir(it);
	return cpr__rmdir(path);
}

bool cpr_remove_dir(const char *path, bool force)
{
	if (!path) {
		cpr__set_error(CPR_ERR_INVALID, "path is NULL");
		return false;
	}
	return force ? !cpr_err(cpr__rmdir_force(path)) :
		       !cpr_err(cpr__rmdir(path));
}

bool cpr_fs_rename(const char *old_path, const char *new_path)
{
	if (!old_path || !new_path) {
		cpr__set_error(CPR_ERR_INVALID, "NULL argument");
		return false;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	{
		wchar_t wold[CPR_FS_PATH_MAX], wnew[CPR_FS_PATH_MAX];
		if (!cpr__to_wide(old_path, wold, CPR_FS_PATH_MAX) ||
		    !cpr__to_wide(new_path, wnew, CPR_FS_PATH_MAX)) {
			cpr__set_error(CPR_ERR_INVALID, "path encoding error");
			return false;
		}
		if (!MoveFileExW(wold, wnew, MOVEFILE_REPLACE_EXISTING)) {
			cpr__set_error(CPR_ERR_IO, "MoveFileExW failed");
			return false;
		}
		return true;
	}
#else
	if (rename(old_path, new_path) != 0) {
		cpr__set_error(CPR_ERR_IO, "rename failed");
		return false;
	}
	return true;
#endif
}

bool cpr_fs_copy(const char *src_path, const char *dst_path)
{
	if (!src_path || !dst_path) {
		cpr__set_error(CPR_ERR_INVALID, "NULL argument");
		return false;
	}

	CprFile *in = cpr_open_file(src_path, CPR_FILE_READ);
	if (!in)
		return false;

	CprFile *out = cpr_open_file(dst_path, CPR_FILE_WRITE);
	if (!out) {
		cpr_close_file(in);
		return false;
	}

	char buf[8192];
	bool ok = true;
	size_t n;
	while ((n = cpr_read_file(in, buf, sizeof(buf))) > 0) {
		if (cpr_write_file(out, buf, n) != n) {
			ok = false;
			break;
		}
	}
	if (ok && !cpr_file_eof(in))
		ok = false; // read error already set by cpr_read_file

	cpr_close_file(in);
	cpr_close_file(out);

	if (!ok)
		cpr_remove_file(dst_path);
	return ok;
}

// --- File I/O ---

struct CprFile {
	FILE *fp;
};

static FILE *cpr__fopen(const char *path, const char *mode)
{
#if defined(CPR_PLATFORM_WINDOWS)
	wchar_t wpath[CPR_FS_PATH_MAX];
	wchar_t wmode[8];
	if (!cpr__to_wide(path, wpath, CPR_FS_PATH_MAX))
		return NULL;
	int i;
	for (i = 0; mode[i] && i < 7; i++)
		wmode[i] = (wchar_t)mode[i];
	wmode[i] = L'\0';
#if defined(CPR_COMPILER_MSVC)
	FILE *fp = NULL;
	_wfopen_s(&fp, wpath, wmode);
	return fp;
#else
	return _wfopen(wpath, wmode);
#endif
#else
	return fopen(path, mode);
#endif
}

static const char *cpr__file_mode_str(CprFileMode mode)
{
	switch (mode) {
	case CPR_FILE_READ:
		return "rb";
	case CPR_FILE_WRITE:
		return "wb";
	case CPR_FILE_APPEND:
		return "ab";
	case CPR_FILE_READ_WRITE:
		return "r+b";
	default:
		return NULL;
	}
}

CprFile *cpr_open_file(const char *path, CprFileMode mode)
{
	if (!path) {
		cpr__set_error(CPR_ERR_INVALID, "path is NULL");
		return NULL;
	}

	const char *mstr = cpr__file_mode_str(mode);
	if (!mstr) {
		cpr__set_error(CPR_ERR_INVALID, "invalid mode");
		return NULL;
	}

	FILE *fp = cpr__fopen(path, mstr);
	if (!fp) {
		cpr__set_error(CPR_ERR_IO, "fopen failed");
		return NULL;
	}

	CprFile *file = malloc(sizeof(*file));
	if (!file) {
		fclose(fp);
		cpr__set_error(CPR_ERR_OOM, "out of memory");
		return NULL;
	}

	file->fp = fp;
	return file;
}

void cpr_close_file(CprFile *file)
{
	if (!file)
		return;

	fclose(file->fp);
	free(file);
}

size_t cpr_read_file(CprFile *file, void *buf, size_t buf_size)
{
	if (!file || !buf) {
		cpr__set_error(CPR_ERR_INVALID, "NULL argument");
		return 0;
	}

	size_t n = fread(buf, 1, buf_size, file->fp);
	if (n < buf_size && ferror(file->fp))
		cpr__set_error(CPR_ERR_IO, "read failed");
	return n;
}

size_t cpr_write_file(CprFile *file, const void *buf, size_t buf_size)
{
	if (!file || !buf) {
		cpr__set_error(CPR_ERR_INVALID, "NULL argument");
		return 0;
	}

	size_t n = fwrite(buf, 1, buf_size, file->fp);
	if (n < buf_size)
		cpr__set_error(CPR_ERR_IO, "write failed");
	return n;
}

bool cpr_seek_file(CprFile *file, int64_t offset, CprFileSeek from)
{
	if (!file) {
		cpr__set_error(CPR_ERR_INVALID, "NULL file");
		return false;
	}
	int rc;
#if defined(CPR_PLATFORM_WINDOWS) && defined(CPR_COMPILER_MSVC)
	rc = _fseeki64(file->fp, offset, (int)from);
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	rc = fseeko(file->fp, (off_t)offset, (int)from);
#else
	rc = fseek(file->fp, (long)offset, (int)from);
#endif
	if (rc != 0) {
		cpr__set_error(CPR_ERR_IO, "seek failed");
		return false;
	}
	return true;
}

int64_t cpr_tell_file(CprFile *file)
{
	if (!file) {
		cpr__set_error(CPR_ERR_INVALID, "NULL file");
		return -1;
	}

	int64_t pos;
#if defined(CPR_PLATFORM_WINDOWS) && defined(CPR_COMPILER_MSVC)
	pos = (int64_t)_ftelli64(file->fp);
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	pos = (int64_t)ftello(file->fp);
#else
	pos = (int64_t)ftell(file->fp);
#endif

	if (pos < 0)
		cpr__set_error(CPR_ERR_IO, "tell failed");
	return pos;
}

int64_t cpr_file_size(CprFile *file)
{
	if (!file) {
		cpr__set_error(CPR_ERR_INVALID, "NULL file");
		return -1;
	}

	int64_t cur = cpr_tell_file(file);
	if (cur < 0)
		return -1;

	if (!cpr_seek_file(file, 0, CPR_SEEK_END))
		return -1;

	int64_t end = cpr_tell_file(file);
	cpr_seek_file(file, cur, CPR_SEEK_START); // restore; error ignored
	return end;
}

bool cpr_flush_file(CprFile *file)
{
	if (!file) {
		cpr__set_error(CPR_ERR_INVALID, "NULL file");
		return false;
	}
	if (fflush(file->fp) != 0) {
		cpr__set_error(CPR_ERR_IO, "flush failed");
		return false;
	}
	return true;
}

bool cpr_file_eof(CprFile *file)
{
	return file && feof(file->fp) != 0;
}

bool cpr_read_file_all(const char *path, void *buf, size_t buf_size,
		       size_t *out_size)
{
	if (!path || !buf) {
		cpr__set_error(CPR_ERR_INVALID, "NULL argument");
		return false;
	}

	CprFile *f = cpr_open_file(path, CPR_FILE_READ);
	if (!f)
		return false;

	int64_t sz = cpr_file_size(f);
	if (sz < 0) {
		cpr_close_file(f);
		return false;
	}

	if ((uint64_t)sz > (uint64_t)buf_size) {
		cpr_close_file(f);
		cpr__set_error(CPR_ERR_OVERFLOW, "file too large for buffer");
		return false;
	}

	size_t n = cpr_read_file(f, buf, (size_t)sz);
	cpr_close_file(f);
	if (out_size)
		*out_size = n;

	if (n < (size_t)sz) {
		cpr__set_error(CPR_ERR_IO, "short read");
		return false;
	}
	return true;
}

bool cpr_write_file_all(const char *path, const void *data, size_t data_size)
{
	if (!path || (!data && data_size > 0)) {
		cpr__set_error(CPR_ERR_INVALID, "NULL argument");
		return false;
	}

	CprFile *f = cpr_open_file(path, CPR_FILE_WRITE);
	if (!f)
		return false;

	size_t n = cpr_write_file(f, data, data_size);
	cpr_close_file(f);
	return n >= data_size;
}

// --- Directory Iterator ---

struct CprDirIterator {
#if defined(CPR_PLATFORM_WINDOWS)
	HANDLE handle;
	WIN32_FIND_DATAW data;
	bool first;
#else
	DIR *dir;
#endif
};

CprDirIterator *cpr_open_dir(const char *path)
{
	if (!path) {
		cpr__set_error(CPR_ERR_INVALID, "path is NULL");
		return NULL;
	}

	CprDirIterator *iter = malloc(sizeof(*iter));
	if (!iter) {
		cpr__set_error(CPR_ERR_OOM, "out of memory");
		return NULL;
	}

#if defined(CPR_PLATFORM_WINDOWS)
	{
		wchar_t wpath[CPR_FS_PATH_MAX];
		if (!cpr__to_wide(path, wpath, CPR_FS_PATH_MAX)) {
			free(iter);
			cpr__set_error(CPR_ERR_INVALID, "path encoding error");
			return NULL;
		}
		wchar_t wpattern[CPR_FS_PATH_MAX + 4];
		_snwprintf_s(wpattern, CPR_FS_PATH_MAX + 4, _TRUNCATE, L"%s\\*",
			     wpath);
		iter->handle = FindFirstFileW(wpattern, &iter->data);
		if (iter->handle == INVALID_HANDLE_VALUE) {
			free(iter);
			cpr__set_error(CPR_ERR_IO, "FindFirstFileW failed");
			return NULL;
		}
		iter->first = true;
	}
#else
	iter->dir = opendir(path);
	if (!iter->dir) {
		free(iter);
		cpr__set_error(CPR_ERR_IO, "opendir failed");
		return NULL;
	}
#endif

	return iter;
}

void cpr_close_dir(CprDirIterator *iter)
{
	if (!iter)
		return;

#if defined(CPR_PLATFORM_WINDOWS)
	if (iter->handle != INVALID_HANDLE_VALUE)
		FindClose(iter->handle);
#else
	if (iter->dir)
		closedir(iter->dir);
#endif

	free(iter);
}

bool cpr_next_dir(CprDirIterator *iter, CprDirEntry *out_entry)
{
	if (!iter || !out_entry)
		return false;

#if defined(CPR_PLATFORM_WINDOWS)
	for (;;) {
		WIN32_FIND_DATAW *d;
		if (iter->first) {
			iter->first = false;
			d = &iter->data;
		} else {
			if (!FindNextFileW(iter->handle, &iter->data))
				return false;
			d = &iter->data;
		}

		if (d->cFileName[0] == L'.' &&
		    (d->cFileName[1] == L'\0' ||
		     (d->cFileName[1] == L'.' && d->cFileName[2] == L'\0')))
			continue;

		cpr__to_utf8(d->cFileName, out_entry->name, CPR_FS_NAME_MAX);
		out_entry->name[CPR_FS_NAME_MAX - 1] = '\0';
		out_entry->type =
			(d->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ?
				CPR_FS_TYPE_DIR :
				CPR_FS_TYPE_FILE;
		return true;
	}
#else
	for (;;) {
		struct dirent *de = readdir(iter->dir);
		if (!de)
			return false;
		if (de->d_name[0] == '.' &&
		    (de->d_name[1] == '\0' ||
		     (de->d_name[1] == '.' && de->d_name[2] == '\0')))
			continue;

		strncpy(out_entry->name, de->d_name, CPR_FS_NAME_MAX - 1);
		out_entry->name[CPR_FS_NAME_MAX - 1] = '\0';

#if defined(DT_DIR)
		if (de->d_type == DT_DIR)
			out_entry->type = CPR_FS_TYPE_DIR;
		else if (de->d_type == DT_REG)
			out_entry->type = CPR_FS_TYPE_FILE;
		else {
			// DT_UNKNOWN or other: fall back to fstatat
			struct stat st;
			int fd = dirfd(iter->dir);
			if (fd >= 0 && fstatat(fd, de->d_name, &st, 0) == 0) {
				if (S_ISDIR(st.st_mode))
					out_entry->type = CPR_FS_TYPE_DIR;
				else if (S_ISREG(st.st_mode))
					out_entry->type = CPR_FS_TYPE_FILE;
				else
					out_entry->type = CPR_FS_TYPE_OTHER;
			} else {
				out_entry->type = CPR_FS_TYPE_OTHER;
			}
		}
#else
		// d_type not available; use fstatat via dirfd
		{
			struct stat st;
			int fd = dirfd(iter->dir);
			if (fd >= 0 && fstatat(fd, de->d_name, &st, 0) == 0) {
				if (S_ISDIR(st.st_mode))
					out_entry->type = CPR_FS_TYPE_DIR;
				else if (S_ISREG(st.st_mode))
					out_entry->type = CPR_FS_TYPE_FILE;
				else
					out_entry->type = CPR_FS_TYPE_OTHER;
			} else {
				out_entry->type = CPR_FS_TYPE_OTHER;
			}
		}
#endif
		return true;
	}
#endif
}
