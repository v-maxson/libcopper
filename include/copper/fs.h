#ifndef CPR_FS_H
#define CPR_FS_H

#include "defs.h"
#include "result.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// --- Path Constants ---

#define CPR_FS_PATH_MAX 4096
#define CPR_FS_NAME_MAX 256

#if defined(CPR_PLATFORM_WINDOWS)
#define CPR_FS_SEP '\\'
#define CPR_FS_SEP_STR "\\"
#else
#define CPR_FS_SEP '/'
#define CPR_FS_SEP_STR "/"
#endif

// --- Path Utilities ---

#ifdef __cplusplus
extern "C" {
#endif

/// Returns true if `path` is an absolute path.
CPR_API bool cpr_path_is_abs(const char *path);

/// Returns a pointer to the final component of `path` (no allocation).
/// For example, "/random/path.txt" -> "path.txt". If no separator is found, returns the `path` back.
CPR_API const char *cpr_path_basename(const char *path);

/// Returns a pointer to the extension of `path`, including the dot (no allocation).
/// For example, "/random/path.txt" -> ".txt". Returns "" if no separator is found.
CPR_API const char *cpr_path_ext(const char *path);

/// Writes the directory part of `path` into `buf`.
/// For example, "/random/path.txt" -> "/random". Returns "." if no separator is found.
/// Pass of a buffer of `CPR_FS_PATH_MAX` bytes to guarentee the
/// result fits for any valid system path on all platforms.
CPR_API CprResult cpr_path_dirname(char *buf, size_t buf_size,
				   const char *path);

/// Joins `base` and `part` with the platform separator into `buf`
/// If `part` is absolute, it replaces `base` entirely.
/// Pass of a buffer of `CPR_FS_PATH_MAX` bytes to guarentee the
/// result fits for any valid system path on all platforms.
CPR_API CprResult cpr_path_join(char *buf, size_t buf_size, const char *base,
				const char *part);

/// Resolves `.` and `..` components and collapses repeated separators in
/// `path`, writing the result to `buf`. Does not touch the file system.
/// Pass of a buffer of `CPR_FS_PATH_MAX` bytes to guarentee the
/// result fits for any valid system path on all platforms.
CPR_API CprResult cpr_normalize_path(char *buf, size_t buf_size,
				     const char *path);

/// Writes the current working directory into `buf`.
/// Pass of a buffer of `CPR_FS_PATH_MAX` bytes to guarentee the
/// result fits for any valid system path on all platforms.
CPR_API CprResult cpr_cwd(char *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

// --- Stat ---

typedef enum { CPR_FS_TYPE_FILE, CPR_FS_TYPE_DIR, CPR_FS_TYPE_OTHER } CprFsType;

typedef struct {
	uint64_t size; ///< File size in bytes. 0 for directories.
	uint64_t mtime_ms; ///< Time modified in UTC ms since Unix epoch.
	CprFsType type;
} CprFsStat;

#ifdef __cplusplus
extern "C" {
#endif

/// Populates `out_stat` with metadata for `path`. Returns CPR_ERR_IO
/// if `path` does not exsit or cannot be accessed.
CPR_API CprResult cpr_fs_stat(const char *path, CprFsStat *out_stat);

CPR_API bool cpr_path_exists(const char *path);
CPR_API bool cpr_path_is_file(const char *path);
CPR_API bool cpr_path_is_dir(const char *path);

#ifdef __cplusplus
}
#endif

// --- Filesystem Operations ---

#ifdef __cplusplus
extern "C" {
#endif

/// Creates a directory. Fails if it already exists or if parent
/// directories are missing.
CPR_API CprResult cpr_mkdir(const char *path);

/// Creates `path` and all missing parent directories.
CPR_API CprResult cpr_mkdir_all(const char *path);

/// Removes a file. Returns `CPR_ERR_IO` if `path` is a
/// directory, doesn't exist, or can't be accessed.
CPR_API CprResult cpr_remove_file(const char *path);

/// Removes a directory. If `force` is false and the directory is not empty
/// this will return `CPR_ERR_IO`. If `force` is positive, this will forcably
/// remove the directory and all files it contains.
CPR_API CprResult cpr_remove_dir(const char *path, bool force);

/// Renames/moves a file or directory. On Windows, this renames
/// `new_path` atomically if it already exists.
CPR_API CprResult cpr_fs_rename(const char *old_path, const char *new_path);

/// Copies a file from `src_path` to `dst_path`. Creates or overwrites `dst_path`.
/// This will *not* copy directories.
CPR_API CprResult cpr_fs_copy(const char *src_path, const char *dst_path);

#ifdef __cplusplus
}
#endif

// --- File I/O ---

typedef enum {
	CPR_FILE_READ, ///< File must exist.
	CPR_FILE_WRITE, ///< Creates or truncates.
	CPR_FILE_APPEND, ///< Creates if missing.
	CPR_FILE_READ_WRITE ///< File must exist.
} CprFileMode;

typedef enum {
	CPR_SEEK_START = 0, ///< Offset from start of file.
	CPR_SEEK_CURRENT = 1, ///< Offset from current position in file.
	CPR_SEEK_END = 2 ///< Offset from end of file.
} CprFileSeek;

/// Opaque file handle.
typedef struct CprFile CprFile;

#ifdef __cplusplus
extern "C" {
#endif

/// Opens `path` with the given `mode`. Returns NULL on failure;
/// if `out_result` is not NULL, it receives to error code.
CPR_API CprFile *cpr_open_file(const char *path, CprFileMode mode,
			       CprResult *out_result);

/// Flushes and closes `file`. `file` must not be used after this call.
CPR_API void cpr_close_file(CprFile *file);

/// Reads `buf_size` bytes from `file` into `buf` then returns the amount
/// of bytes actually read.
CPR_API size_t cpr_read_file(CprFile *file, void *buf, size_t buf_size,
			     CprResult *out_result);

/// Writes `buf_size` bytes from `buf` into `file` at the current position.
/// Returns the number of bytes written.
CPR_API size_t cpr_write_file(CprFile *file, const void *buf, size_t buf_size,
			      CprResult *out_result);

/// Seeks to `offset` relative to `from`.
CPR_API CprResult cpr_seek_file(CprFile *file, int64_t offset,
				CprFileSeek from);

/// Retunrs the current file position, or -1 on failure.
CPR_API int64_t cpr_tell_file(CprFile *file, CprResult *out_result);

/// Returns the file size in bytes, or -1 on failure.
CPR_API int64_t cpr_file_size(CprFile *file, CprResult *out_result);

/// Flushes any buffered data to the OS.
CPR_API CprResult cpr_flush_file(CprFile *file);

/// Returns true if the end-of-file indicator is set.
CPR_API bool cpr_file_eof(CprFile *file);

// --- High-level Helpers ---

/// Reads the entire contents of `path` into `buf`. Returns `CPR_ERR_OVERFLOW`
/// if the file is larger than `buf_size`. Sets `out_size` to the number of bytes read.
CPR_API CprResult cpr_read_file_all(const char *path, void *buf,
				    size_t buf_size, size_t *out_size);

/// Writes `data_size` bytes from `data` to `path`, creating or overwriting it.
CPR_API CprResult cpr_write_file_all(const char *path, const void *data,
				     size_t data_size);

#ifdef __cplusplus
}
#endif

// --- Directory Iterator ---

typedef struct {
	char name[CPR_FS_NAME_MAX]; ///< Entry name (not full path).
	CprFsType type;
} CprDirEntry;

/// Opaque directory iterator.
typedef struct CprDirIterator CprDirIterator;

#ifdef __cplusplus
extern "C" {
#endif

/// Opens `path` for iteration. Returns NULL on failure and populates
/// `out_result` (if != NULL) with the error code.
CPR_API CprDirIterator *cpr_open_dir(const char *path, CprResult *out_result);
CPR_API void cpr_close_dir(CprDirIterator *iter);

/// Advances to the next entry, writing it into `out_entry`.
/// Skips `.` and `..` automatically. Returns false when
/// there are no more entries.
CPR_API bool cpr_next_dir(CprDirIterator *iter, CprDirEntry *out_entry);

#ifdef __cplusplus
}
#endif

#endif // CPR_FS_H
