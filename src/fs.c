#include "copper/fs.h"

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

CprResult cpr_path_dirname(char *buf, size_t buf_size, const char *path)
{
	if (!buf || !buf_size || !path)
		return CPR_ERR_INVALID;

	const char *last_sep = NULL;
	for (const char *p = path; *p; p++) {
		if (cpr__fs_is_sep(*p))
			last_sep = p;
	}

	if (!last_sep) {
		if (buf_size < 2)
			return CPR_ERR_OVERFLOW;

		buf[0] = '.';
		buf[1] = '\0';
		return CPR_OK;
	}

	size_t len = (size_t)(last_sep - path);
	if (len == 0)
		len = 1;

	if (len + 1 > buf_size)
		return CPR_ERR_OVERFLOW;

	memcpy(buf, path, len);
	buf[len] = '\0';
	return CPR_OK;
}

CprResult cpr_path_join(char *buf, size_t buf_size, const char *base,
			const char *part)
{
	if (!buf || !buf_size || !base || !part)
		return CPR_ERR_INVALID;

	if (cpr_path_is_abs(part)) {
		size_t plen = strlen(part);
		if (plen + 1 > buf_size)
			return CPR_ERR_OVERFLOW;

		memcpy(buf, part, plen + 1);
		return CPR_OK;
	}

	size_t blen = strlen(base);
	size_t plen = strlen(part);
	int need_sep =
		(blen > 0 && !cpr__fs_is_sep(base[blen - 1]) && plen > 0);
	size_t needed = blen + (size_t)need_sep + plen + 1;

	if (needed > buf_size)
		return CPR_ERR_OVERFLOW;

	memcpy(buf, base, blen);
	if (need_sep)
		buf[blen++] = CPR_FS_SEP;
	memcpy(buf + blen, part, plen + 1);
	return CPR_OK;
}

CprResult cpr_normalize_path(char *buf, size_t buf_size, const char *path)
{
	if (!buf || !buf_size || !path)
		return CPR_ERR_INVALID;

	size_t plen = strlen(path);
	if (plen + 1 > buf_size)
		return CPR_ERR_OVERFLOW;

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
	return CPR_OK;
}

CprResult cpr_cwd(char *buf, size_t buf_size)
{
	if (!buf || !buf_size)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	{
		wchar_t wtmp[CPR_FS_PATH_MAX];
		DWORD n = GetCurrentDirectoryW(CPR_FS_PATH_MAX, wtmp);
		if (n == 0 || n >= CPR_FS_PATH_MAX)
			return CPR_ERR_IO;
		if (!cpr__to_utf8(wtmp, buf, (int)buf_size))
			return CPR_ERR_OVERFLOW;
		return CPR_OK;
	}
#else
	if (getcwd(buf, buf_size) == NULL)
		return CPR_ERR_IO;
	return CPR_OK;
#endif
}

// --- Stat ---

CprResult cpr_fs_stat(const char *path, CprFsStat *out_stat)
{
	if (!path || !out_stat)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	{
		wchar_t wpath[CPR_FS_PATH_MAX];
		WIN32_FILE_ATTRIBUTE_DATA info;
		if (!cpr__to_wide(path, wpath, CPR_FS_PATH_MAX))
			return CPR_ERR_INVALID;
		if (!GetFileAttributesExW(wpath, GetFileExInfoStandard, &info))
			return CPR_ERR_IO;
		if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			out_stat->size = 0;
			out_stat->type = CPR_FS_TYPE_DIR;
		} else {
			out_stat->size = ((uint64_t)info.nFileSizeHigh << 32) |
					 info.nFileSizeLow;
			out_stat->type = CPR_FS_TYPE_FILE;
		}

		out_stat->mtime_ms = cpr__filetime_to_ms(info.ftLastWriteTime);
		return CPR_OK;
	}
#else
	{
		struct stat st;
		if (stat(path, &st) != 0)
			return CPR_ERR_IO;
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
		return CPR_OK;
	}
#endif
}

bool cpr_path_exists(const char *path)
{
	CprFsStat s;
	return path && cpr_ok(cpr_fs_stat(path, &s));
}

bool cpr_path_is_file(const char *path)
{
	CprFsStat s;
	return path && cpr_ok(cpr_fs_stat(path, &s)) &&
	       s.type == CPR_FS_TYPE_FILE;
}

bool cpr_path_is_dir(const char *path)
{
	CprFsStat s;
	return path && cpr_ok(cpr_fs_stat(path, &s)) &&
	       s.type == CPR_FS_TYPE_DIR;
}

// --- Filesystem Operations ---

CprResult cpr_mkdir(const char *path)
{
	if (!path)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	{
		wchar_t wpath[CPR_FS_PATH_MAX];
		if (!cpr__to_wide(path, wpath, CPR_FS_PATH_MAX))
			return CPR_ERR_INVALID;
		return CreateDirectoryW(wpath, NULL) ? CPR_OK : CPR_ERR_IO;
	}
#else
	return mkdir(path, 0777) == 0 ? CPR_OK : CPR_ERR_IO;
#endif
}

CprResult cpr_mkdir_all(const char *path)
{
	if (!path)
		return CPR_ERR_INVALID;

	size_t len = strlen(path);
	if (len == 0 || len >= CPR_FS_PATH_MAX)
		return CPR_ERR_INVALID;

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
			CprResult r = cpr_mkdir(tmp);
			if (cpr_err(r))
				return r;
		}
		*p = CPR_FS_SEP;
	}

	if (!cpr_path_is_dir(tmp))
		return cpr_mkdir(tmp);
	return CPR_OK;
}

CprResult cpr_remove_file(const char *path)
{
	if (!path)
		return CPR_ERR_INVALID;
#if defined(CPR_PLATFORM_WINDOWS)
	{
		wchar_t wpath[CPR_FS_PATH_MAX];
		if (!cpr__to_wide(path, wpath, CPR_FS_PATH_MAX))
			return CPR_ERR_INVALID;
		return DeleteFileW(wpath) ? CPR_OK : CPR_ERR_IO;
	}
#else
	return unlink(path) == 0 ? CPR_OK : CPR_ERR_IO;
#endif
}

static CprResult cpr__rmdir(const char *path)
{
#if defined(CPR_PLATFORM_WINDOWS)
	wchar_t wpath[CPR_FS_PATH_MAX];
	if (!cpr__to_wide(path, wpath, CPR_FS_PATH_MAX))
		return CPR_ERR_INVALID;
	return RemoveDirectoryW(wpath) ? CPR_OK : CPR_ERR_IO;
#else
	return rmdir(path) == 0 ? CPR_OK : CPR_ERR_IO;
#endif
}

// Forward decl needed for recursion
static CprResult cpr__rmdir_force(const char *path);
static CprResult cpr__rmdir_force(const char *path)
{
	CprResult r;
	CprDirIterator *it = cpr_open_dir(path, &r);
	if (!it)
		return r;

	CprDirEntry entry;
	while (cpr_next_dir(it, &entry)) {
		char child[CPR_FS_PATH_MAX];
		if (cpr_err(cpr_path_join(child, sizeof(child), path,
					  entry.name))) {
			cpr_close_dir(it);
			return CPR_ERR_OVERFLOW;
		}
		r = (entry.type == CPR_FS_TYPE_DIR) ? cpr__rmdir_force(child) :
						      cpr_remove_file(child);
		if (cpr_err(r)) {
			cpr_close_dir(it);
			return r;
		}
	}
	cpr_close_dir(it);
	return cpr__rmdir(path);
}

CprResult cpr_remove_dir(const char *path, bool force)
{
	if (!path)
		return CPR_ERR_INVALID;

	return force ? cpr__rmdir_force(path) : cpr__rmdir(path);
}

CprResult cpr_fs_rename(const char *old_path, const char *new_path)
{
	if (!old_path || !new_path)
		return CPR_ERR_INVALID;

#if defined(CPR_PLATFORM_WINDOWS)
	{
		wchar_t wold[CPR_FS_PATH_MAX], wnew[CPR_FS_PATH_MAX];
		if (!cpr__to_wide(old_path, wold, CPR_FS_PATH_MAX))
			return CPR_ERR_INVALID;
		if (!cpr__to_wide(new_path, wnew, CPR_FS_PATH_MAX))
			return CPR_ERR_INVALID;
		/* MOVEFILE_REPLACE_EXISTING for atomic replacement on NTFS */
		return MoveFileExW(wold, wnew, MOVEFILE_REPLACE_EXISTING) ?
			       CPR_OK :
			       CPR_ERR_IO;
	}
#else
	return rename(old_path, new_path) == 0 ? CPR_OK : CPR_ERR_IO;
#endif
}

CprResult cpr_fs_copy(const char *src_path, const char *dst_path)
{
	if (!src_path || !dst_path)
		return CPR_ERR_INVALID;

	CprResult r;
	CprFile *in = cpr_open_file(src_path, CPR_FILE_READ, &r);
	if (!in)
		return r;

	CprFile *out = cpr_open_file(dst_path, CPR_FILE_WRITE, &r);
	if (!out) {
		cpr_close_file(in);
		return r;
	}

	char buf[8192];
	r = CPR_OK;
	size_t n;
	while ((n = cpr_read_file(in, buf, sizeof(buf), &r)) > 0) {
		CprResult wr;
		cpr_write_file(out, buf, n, &wr);
		if (cpr_err(wr)) {
			r = wr;
			break;
		}
	}

	cpr_close_file(in);
	cpr_close_file(out);

	if (cpr_err(r))
		cpr_remove_file(dst_path);
	return r;
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

CprFile *cpr_open_file(const char *path, CprFileMode mode,
		       CprResult *out_result)
{
#define RETURN_ERR(err)                    \
	do {                               \
		if (out_result)            \
			*out_result = err; \
		return NULL;               \
	} while (0)

	if (!path)
		RETURN_ERR(CPR_ERR_INVALID);

	const char *mstr = cpr__file_mode_str(mode);
	if (!mstr)
		RETURN_ERR(CPR_ERR_INVALID);

	FILE *fp = cpr__fopen(path, mstr);
	if (!fp)
		RETURN_ERR(CPR_ERR_IO);

	CprFile *file = malloc(sizeof(*file));
	if (!file) {
		fclose(fp);
		RETURN_ERR(CPR_ERR_OOM);
	}

	file->fp = fp;
	if (out_result)
		*out_result = CPR_OK;
	return file;
#undef RETURN_ERR
}

void cpr_close_file(CprFile *file)
{
	if (!file)
		return;

	fclose(file->fp);
	free(file);
}

size_t cpr_read_file(CprFile *file, void *buf, size_t buf_size,
		     CprResult *out_result)
{
	if (!file || !buf) {
		if (out_result)
			*out_result = CPR_ERR_INVALID;
		return 0;
	}

	size_t n = fread(buf, 1, buf_size, file->fp);
	if (out_result)
		*out_result = (n < buf_size && ferror(file->fp)) ? CPR_ERR_IO :
								   CPR_OK;
	return n;
}

size_t cpr_write_file(CprFile *file, const void *buf, size_t buf_size,
		      CprResult *out_result)
{
	if (!file || !buf) {
		if (out_result)
			*out_result = CPR_ERR_INVALID;
		return 0;
	}

	size_t n = fwrite(buf, 1, buf_size, file->fp);
	if (out_result)
		*out_result = (n < buf_size) ? CPR_ERR_IO : CPR_OK;
	return n;
}

CprResult cpr_seek_file(CprFile *file, int64_t offset, CprFileSeek from)
{
	if (!file)
		return CPR_ERR_INVALID;
#if defined(CPR_PLATFORM_WINDOWS) && defined(CPR_COMPILER_MSVC)
	return _fseeki64(file->fp, (int64_t)offset, (int)from) == 0 ?
		       CPR_OK :
		       CPR_ERR_IO;
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
	return fseeko(file->fp, (off_t)offset, (int)from) == 0 ? CPR_OK :
								 CPR_ERR_IO;
#else
	return fseek(file->fp, (int64_t)offset, (int)from) == 0 ? CPR_OK :
								  CPR_ERR_IO;
#endif
}

int64_t cpr_tell_file(CprFile *file, CprResult *out_result)
{
	if (!file) {
		if (out_result)
			*out_result = CPR_ERR_INVALID;
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

	if (out_result)
		*out_result = (pos < 0) ? CPR_ERR_IO : CPR_OK;
	return pos;
}

int64_t cpr_file_size(CprFile *file, CprResult *out_result)
{
#define RETURN_ERR(err)                    \
	do {                               \
		if (out_result)            \
			*out_result = err; \
		return -1;                 \
	} while (0)

	if (!file)
		RETURN_ERR(CPR_ERR_INVALID);

	CprResult r = CPR_OK;
	int64_t cur = cpr_tell_file(file, &r);
	if (cpr_err(r))
		RETURN_ERR(r);

	if (cpr_err(cpr_seek_file(file, 0, CPR_SEEK_END)))
		RETURN_ERR(CPR_ERR_IO);

	int64_t end = cpr_tell_file(file, &r);
	cpr_seek_file(file, cur, CPR_SEEK_START);
	if (out_result)
		*out_result = (end < 0) ? CPR_ERR_IO : CPR_OK;
	return end;

#undef RETURN_ERR
}

CprResult cpr_flush_file(CprFile *file)
{
	if (!file)
		return CPR_ERR_INVALID;

	return fflush(file->fp) == 0 ? CPR_OK : CPR_ERR_IO;
}

bool cpr_file_eof(CprFile *file)
{
	return file && feof(file->fp) != 0;
}

CprResult cpr_read_file_all(const char *path, void *buf, size_t buf_size,
			    size_t *out_size)
{
	if (!path || !buf)
		return CPR_ERR_INVALID;

	CprResult r = CPR_OK;
	CprFile *f = cpr_open_file(path, CPR_FILE_READ, &r);
	if (!f)
		return r;

	int64_t sz = cpr_file_size(f, &r);
	if (cpr_err(r)) {
		cpr_close_file(f);
		return r;
	}

	if ((uint64_t)sz > (uint64_t)buf_size) {
		cpr_close_file(f);
		return CPR_ERR_OVERFLOW;
	}

	size_t n = cpr_read_file(f, buf, (size_t)sz, &r);
	cpr_close_file(f);
	if (out_size)
		*out_size = n;

	return r;
}

CprResult cpr_write_file_all(const char *path, const void *data,
			     size_t data_size)
{
	if (!path || (!data && data_size > 0))
		return CPR_ERR_INVALID;

	CprResult r = CPR_OK;
	CprFile *f = cpr_open_file(path, CPR_FILE_WRITE, &r);
	if (!f)
		return r;

	cpr_write_file(f, data, data_size, &r);
	cpr_close_file(f);
	return r;
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

CprDirIterator *cpr_open_dir(const char *path, CprResult *out_result)
{
#define RETURN_ERR(err)                    \
	do {                               \
		if (out_result)            \
			*out_result = err; \
		return NULL;               \
	} while (0)

	if (!path)
		RETURN_ERR(CPR_ERR_INVALID);

	CprDirIterator *iter = malloc(sizeof(*iter));
	if (!iter)
		RETURN_ERR(CPR_ERR_OOM);

#if defined(CPR_PLATFORM_WINDOWS)
	{
		wchar_t wpath[CPR_FS_PATH_MAX];
		if (!cpr__to_wide(path, wpath, CPR_FS_PATH_MAX)) {
			free(iter);
			if (out_result)
				*out_result = CPR_ERR_INVALID;
			return NULL;
		}
		wchar_t wpattern[CPR_FS_PATH_MAX + 4];
		_snwprintf_s(wpattern, CPR_FS_PATH_MAX + 4, _TRUNCATE,
			     L"%s\\*", wpath);
		iter->handle = FindFirstFileW(wpattern, &iter->data);
		if (iter->handle == INVALID_HANDLE_VALUE) {
			free(iter);
			if (out_result)
				*out_result = CPR_ERR_IO;
			return NULL;
		}
		iter->first = true;
	}
#else
	iter->dir = opendir(path);
	if (!iter->dir) {
		free(iter);
		if (out_result)
			*out_result = CPR_ERR_IO;
		return NULL;
	}
#endif

	if (out_result)
		*out_result = CPR_OK;
	return iter;

#undef RETURN_ERR
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
			/* DT_UNKNOWN or any other type: fall back to fstatat */
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
		/* d_type not available (no _GNU_SOURCE); use fstatat via dirfd */
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
