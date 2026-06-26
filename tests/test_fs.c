#include "unity.h"
#include <copper/copper.h>
#include <string.h>

#if defined(CPR_PLATFORM_WINDOWS)
#define TEST_DIR "cpr_test_fs"
#else
#define TEST_DIR "/tmp/cpr_test_fs"
#endif

#define TEST_FILE    TEST_DIR CPR_FS_SEP_STR "file.txt"
#define TEST_SUBDIR  TEST_DIR CPR_FS_SEP_STR "sub"
#define TEST_RENAMED TEST_DIR CPR_FS_SEP_STR "renamed.txt"
#define TEST_COPY    TEST_DIR CPR_FS_SEP_STR "copy.txt"
#define TEST_DEEP    TEST_DIR CPR_FS_SEP_STR "a" CPR_FS_SEP_STR "b" CPR_FS_SEP_STR "c"

void setUp(void)
{
    cpr_remove_dir(TEST_DIR, true);
    cpr_mkdir(TEST_DIR);
}

void tearDown(void)
{
    cpr_remove_dir(TEST_DIR, true);
}

static void write_test_file(const char *data, size_t len)
{
    CprResult r = cpr_write_file_all(TEST_FILE, data, len);
    TEST_ASSERT_EQUAL_INT(CPR_OK, r);
}

// =============================================================================
// Path Utilities — pure string ops, no filesystem access needed
// =============================================================================

// --- cpr_path_is_abs ---

void test_is_abs_relative(void)
{
    TEST_ASSERT_FALSE(cpr_path_is_abs("a/b/c"));
    TEST_ASSERT_FALSE(cpr_path_is_abs("./foo"));
    TEST_ASSERT_FALSE(cpr_path_is_abs("../bar"));
}

void test_is_abs_empty(void)
{
    TEST_ASSERT_FALSE(cpr_path_is_abs(""));
}

void test_is_abs_null(void)
{
    TEST_ASSERT_FALSE(cpr_path_is_abs(NULL));
}

#if defined(CPR_PLATFORM_WINDOWS)

void test_is_abs_drive_letter(void)
{
    TEST_ASSERT_TRUE(cpr_path_is_abs("C:\\foo"));
    TEST_ASSERT_TRUE(cpr_path_is_abs("Z:/bar"));
}

void test_is_abs_unc(void)
{
    TEST_ASSERT_TRUE(cpr_path_is_abs("\\\\server\\share"));
}

void test_is_abs_forward_slash_not_abs_on_windows(void)
{
    TEST_ASSERT_FALSE(cpr_path_is_abs("/not/abs"));
}

#else

void test_is_abs_unix_root(void)
{
    TEST_ASSERT_TRUE(cpr_path_is_abs("/a/b"));
    TEST_ASSERT_TRUE(cpr_path_is_abs("/"));
}

#endif

// --- cpr_path_basename ---

void test_basename_with_dir(void)
{
    TEST_ASSERT_EQUAL_STRING("c.txt", cpr_path_basename("/a/b/c.txt"));
}

void test_basename_no_sep(void)
{
    const char *p = "file.txt";
    TEST_ASSERT_EQUAL_PTR(p, cpr_path_basename(p));
}

void test_basename_trailing_sep(void)
{
    /* returns pointer to last component start; trailing sep is included */
    TEST_ASSERT_EQUAL_STRING("b/", cpr_path_basename("/a/b/"));
}

void test_basename_null(void)
{
    TEST_ASSERT_NULL(cpr_path_basename(NULL));
}

// --- cpr_path_ext ---

void test_ext_with_ext(void)
{
    TEST_ASSERT_EQUAL_STRING(".txt", cpr_path_ext("/a/b/c.txt"));
}

void test_ext_no_ext(void)
{
    TEST_ASSERT_EQUAL_STRING("", cpr_path_ext("/a/b/c"));
}

void test_ext_hidden_file(void)
{
    // Leading dot on basename is NOT an extension.
    TEST_ASSERT_EQUAL_STRING("", cpr_path_ext("/a/.hidden"));
}

void test_ext_multiple_dots(void)
{
    TEST_ASSERT_EQUAL_STRING(".gz", cpr_path_ext("archive.tar.gz"));
}

void test_ext_null(void)
{
    TEST_ASSERT_NULL(cpr_path_ext(NULL));
}

// --- cpr_path_dirname ---

void test_dirname_with_parent(void)
{
    char buf[CPR_FS_PATH_MAX];
    // Forward slashes in input are recognised on all platforms.
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_path_dirname(buf, sizeof buf, "/a/b/c.txt"));
    TEST_ASSERT_EQUAL_STRING("/a/b", buf);
}

void test_dirname_no_sep(void)
{
    char buf[CPR_FS_PATH_MAX];
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_path_dirname(buf, sizeof buf, "file.txt"));
    TEST_ASSERT_EQUAL_STRING(".", buf);
}

void test_dirname_root(void)
{
    char buf[CPR_FS_PATH_MAX];
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_path_dirname(buf, sizeof buf, "/file.txt"));
    TEST_ASSERT_EQUAL_STRING("/", buf);
}

void test_dirname_null(void)
{
    char buf[CPR_FS_PATH_MAX];
    TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_path_dirname(buf, sizeof buf, NULL));
    TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_path_dirname(NULL, sizeof buf, "/a/b"));
}

void test_dirname_buffer_too_small(void)
{
    char buf[3];
    TEST_ASSERT_EQUAL_INT(CPR_ERR_OVERFLOW,
                          cpr_path_dirname(buf, sizeof buf, "/a/long/path/file.txt"));
}

// --- cpr_path_join ---

void test_join_normal(void)
{
    char buf[CPR_FS_PATH_MAX];
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_path_join(buf, sizeof buf, "base", "part"));
    TEST_ASSERT_EQUAL_STRING("base" CPR_FS_SEP_STR "part", buf);
}

void test_join_base_trailing_sep(void)
{
    char buf[CPR_FS_PATH_MAX];
    TEST_ASSERT_EQUAL_INT(CPR_OK,
                          cpr_path_join(buf, sizeof buf, "base" CPR_FS_SEP_STR, "part"));
    TEST_ASSERT_EQUAL_STRING("base" CPR_FS_SEP_STR "part", buf);
}

void test_join_abs_part_replaces_base(void)
{
    char buf[CPR_FS_PATH_MAX];
#if defined(CPR_PLATFORM_WINDOWS)
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_path_join(buf, sizeof buf, "base", "C:\\abs"));
    TEST_ASSERT_EQUAL_STRING("C:\\abs", buf);
#else
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_path_join(buf, sizeof buf, "base", "/abs"));
    TEST_ASSERT_EQUAL_STRING("/abs", buf);
#endif
}

void test_join_null(void)
{
    char buf[CPR_FS_PATH_MAX];
    TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_path_join(buf, sizeof buf, NULL, "part"));
    TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_path_join(buf, sizeof buf, "base", NULL));
    TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_path_join(NULL, sizeof buf, "base", "part"));
}

void test_join_buffer_too_small(void)
{
    char buf[5];
    TEST_ASSERT_EQUAL_INT(CPR_ERR_OVERFLOW,
                          cpr_path_join(buf, sizeof buf, "longbase", "longpart"));
}

// --- cpr_normalize_path ---

void test_normalize_removes_dot(void)
{
    char buf[CPR_FS_PATH_MAX];
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_normalize_path(buf, sizeof buf, "a/./b"));
    TEST_ASSERT_EQUAL_STRING("a" CPR_FS_SEP_STR "b", buf);
}

void test_normalize_resolves_dotdot(void)
{
    char buf[CPR_FS_PATH_MAX];
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_normalize_path(buf, sizeof buf, "a/b/../c"));
    TEST_ASSERT_EQUAL_STRING("a" CPR_FS_SEP_STR "c", buf);
}

void test_normalize_collapses_seps(void)
{
    char buf[CPR_FS_PATH_MAX];
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_normalize_path(buf, sizeof buf, "a//b///c"));
    TEST_ASSERT_EQUAL_STRING("a" CPR_FS_SEP_STR "b" CPR_FS_SEP_STR "c", buf);
}

void test_normalize_dot_input(void)
{
    char buf[CPR_FS_PATH_MAX];
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_normalize_path(buf, sizeof buf, "."));
    TEST_ASSERT_EQUAL_STRING(".", buf);
}

void test_normalize_all_dots_become_dot(void)
{
    char buf[CPR_FS_PATH_MAX];
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_normalize_path(buf, sizeof buf, "a/b/../.."));
    TEST_ASSERT_EQUAL_STRING(".", buf);
}

void test_normalize_null(void)
{
    char buf[CPR_FS_PATH_MAX];
    TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_normalize_path(buf, sizeof buf, NULL));
    TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_normalize_path(NULL, sizeof buf, "a/b"));
}

// --- cpr_cwd ---

void test_cwd_returns_abs_path(void)
{
    char buf[CPR_FS_PATH_MAX];
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_cwd(buf, sizeof buf));
    TEST_ASSERT_TRUE(strlen(buf) > 0);
    TEST_ASSERT_TRUE(cpr_path_is_abs(buf));
}

void test_cwd_null(void)
{
    char buf[CPR_FS_PATH_MAX];
    TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_cwd(NULL, sizeof buf));
    TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_cwd(buf, 0));
}

// =============================================================================
// Stat
// =============================================================================

void test_stat_existing_file(void)
{
    write_test_file("hello", 5);
    CprFsStat st;
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_fs_stat(TEST_FILE, &st));
    TEST_ASSERT_EQUAL_INT(5, (int)st.size);
    TEST_ASSERT_EQUAL_INT(CPR_FS_TYPE_FILE, st.type);
    TEST_ASSERT_GREATER_THAN(0, (int)st.mtime_ms);
}

void test_stat_existing_dir(void)
{
    CprFsStat st;
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_fs_stat(TEST_DIR, &st));
    TEST_ASSERT_EQUAL_INT(CPR_FS_TYPE_DIR, st.type);
    TEST_ASSERT_EQUAL_INT(0, (int)st.size);
}

void test_stat_nonexistent(void)
{
    CprFsStat st;
    TEST_ASSERT_EQUAL_INT(CPR_ERR_IO,
                          cpr_fs_stat(TEST_DIR CPR_FS_SEP_STR "no_such", &st));
}

void test_stat_null(void)
{
    CprFsStat st;
    TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_fs_stat(NULL, &st));
    TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_fs_stat(TEST_DIR, NULL));
}

void test_path_exists_true(void)
{
    TEST_ASSERT_TRUE(cpr_path_exists(TEST_DIR));
}

void test_path_exists_false(void)
{
    TEST_ASSERT_FALSE(cpr_path_exists(TEST_DIR CPR_FS_SEP_STR "no_such"));
}

void test_path_is_file_true(void)
{
    write_test_file("x", 1);
    TEST_ASSERT_TRUE(cpr_path_is_file(TEST_FILE));
}

void test_path_is_file_false_for_dir(void)
{
    TEST_ASSERT_FALSE(cpr_path_is_file(TEST_DIR));
}

void test_path_is_dir_true(void)
{
    TEST_ASSERT_TRUE(cpr_path_is_dir(TEST_DIR));
}

void test_path_is_dir_false_for_file(void)
{
    write_test_file("x", 1);
    TEST_ASSERT_FALSE(cpr_path_is_dir(TEST_FILE));
}

// =============================================================================
// Filesystem Operations
// =============================================================================

void test_mkdir_success(void)
{
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_mkdir(TEST_SUBDIR));
    TEST_ASSERT_TRUE(cpr_path_is_dir(TEST_SUBDIR));
}

void test_mkdir_already_exists(void)
{
    TEST_ASSERT_EQUAL_INT(CPR_ERR_IO, cpr_mkdir(TEST_DIR));
}

void test_mkdir_missing_parent(void)
{
    TEST_ASSERT_EQUAL_INT(CPR_ERR_IO,
                          cpr_mkdir(TEST_DIR CPR_FS_SEP_STR "no_parent" CPR_FS_SEP_STR "child"));
}

void test_mkdir_null(void)
{
    TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_mkdir(NULL));
}

void test_mkdir_all_deep(void)
{
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_mkdir_all(TEST_DEEP));
    TEST_ASSERT_TRUE(cpr_path_is_dir(TEST_DEEP));
}

void test_mkdir_all_already_exists(void)
{
    // Must succeed even if the directory already exists.
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_mkdir_all(TEST_DIR));
}

void test_mkdir_all_null(void)
{
    TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_mkdir_all(NULL));
}

void test_remove_file_success(void)
{
    write_test_file("data", 4);
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_remove_file(TEST_FILE));
    TEST_ASSERT_FALSE(cpr_path_exists(TEST_FILE));
}

void test_remove_file_is_dir(void)
{
    TEST_ASSERT_EQUAL_INT(CPR_ERR_IO, cpr_remove_file(TEST_DIR));
}

void test_remove_file_nonexistent(void)
{
    TEST_ASSERT_EQUAL_INT(CPR_ERR_IO, cpr_remove_file(TEST_FILE));
}

void test_remove_file_null(void)
{
    TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_remove_file(NULL));
}

void test_remove_dir_empty(void)
{
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_mkdir(TEST_SUBDIR));
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_remove_dir(TEST_SUBDIR, false));
    TEST_ASSERT_FALSE(cpr_path_exists(TEST_SUBDIR));
}

void test_remove_dir_nonempty_no_force(void)
{
    write_test_file("x", 1);
    TEST_ASSERT_EQUAL_INT(CPR_ERR_IO, cpr_remove_dir(TEST_DIR, false));
    TEST_ASSERT_TRUE(cpr_path_exists(TEST_DIR));
}

void test_remove_dir_nonempty_force(void)
{
    write_test_file("x", 1);
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_remove_dir(TEST_DIR, true));
    TEST_ASSERT_FALSE(cpr_path_exists(TEST_DIR));
    cpr_mkdir(TEST_DIR); // re-create so tearDown can succeed
}

void test_remove_dir_null(void)
{
    TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_remove_dir(NULL, false));
}

void test_rename_file(void)
{
    write_test_file("hello", 5);
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_fs_rename(TEST_FILE, TEST_RENAMED));
    TEST_ASSERT_FALSE(cpr_path_exists(TEST_FILE));
    TEST_ASSERT_TRUE(cpr_path_exists(TEST_RENAMED));
}

void test_rename_overwrites_dst(void)
{
    write_test_file("src_content", 11);
    cpr_write_file_all(TEST_RENAMED, "old", 3);
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_fs_rename(TEST_FILE, TEST_RENAMED));

    char buf[32] = {0};
    size_t n = 0;
    cpr_read_file_all(TEST_RENAMED, buf, sizeof buf - 1, &n);
    TEST_ASSERT_EQUAL_INT(11, (int)n);
    TEST_ASSERT_EQUAL_STRING("src_content", buf);
}

void test_rename_nonexistent_src(void)
{
    TEST_ASSERT_EQUAL_INT(CPR_ERR_IO, cpr_fs_rename(TEST_FILE, TEST_RENAMED));
}

void test_rename_null(void)
{
    TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_fs_rename(NULL, TEST_RENAMED));
    TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_fs_rename(TEST_FILE, NULL));
}

void test_copy_file(void)
{
    write_test_file("content", 7);
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_fs_copy(TEST_FILE, TEST_COPY));
    TEST_ASSERT_TRUE(cpr_path_exists(TEST_FILE));

    char buf[32] = {0};
    size_t n = 0;
    cpr_read_file_all(TEST_COPY, buf, sizeof buf - 1, &n);
    TEST_ASSERT_EQUAL_INT(7, (int)n);
    TEST_ASSERT_EQUAL_STRING("content", buf);
}

void test_copy_overwrites_dst(void)
{
    write_test_file("new", 3);
    cpr_write_file_all(TEST_COPY, "old_content", 11);
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_fs_copy(TEST_FILE, TEST_COPY));

    char buf[32] = {0};
    size_t n = 0;
    cpr_read_file_all(TEST_COPY, buf, sizeof buf - 1, &n);
    TEST_ASSERT_EQUAL_INT(3, (int)n);
    TEST_ASSERT_EQUAL_STRING("new", buf);
}

void test_copy_null(void)
{
    TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_fs_copy(NULL, TEST_COPY));
    TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_fs_copy(TEST_FILE, NULL));
}

// =============================================================================
// File I/O
// =============================================================================

void test_open_read_nonexistent(void)
{
    CprResult r = CPR_OK;
    CprFile *f = cpr_open_file(TEST_FILE, CPR_FILE_READ, &r);
    TEST_ASSERT_NULL(f);
    TEST_ASSERT_EQUAL_INT(CPR_ERR_IO, r);
}

void test_open_write_creates(void)
{
    CprResult r = CPR_OK;
    CprFile *f = cpr_open_file(TEST_FILE, CPR_FILE_WRITE, &r);
    TEST_ASSERT_NOT_NULL(f);
    TEST_ASSERT_EQUAL_INT(CPR_OK, r);
    cpr_close_file(f);
    TEST_ASSERT_TRUE(cpr_path_exists(TEST_FILE));
}

void test_open_write_truncates(void)
{
    write_test_file("existing content here", 21);
    CprFile *f = cpr_open_file(TEST_FILE, CPR_FILE_WRITE, NULL);
    TEST_ASSERT_NOT_NULL(f);
    cpr_close_file(f);

    CprFsStat st;
    cpr_fs_stat(TEST_FILE, &st);
    TEST_ASSERT_EQUAL_INT(0, (int)st.size);
}

void test_open_append_preserves_content(void)
{
    write_test_file("first", 5);
    CprFile *f = cpr_open_file(TEST_FILE, CPR_FILE_APPEND, NULL);
    TEST_ASSERT_NOT_NULL(f);
    cpr_write_file(f, "_second", 7, NULL);
    cpr_close_file(f);

    char buf[32] = {0};
    size_t n = 0;
    cpr_read_file_all(TEST_FILE, buf, sizeof buf - 1, &n);
    TEST_ASSERT_EQUAL_INT(12, (int)n);
    TEST_ASSERT_EQUAL_STRING("first_second", buf);
}

void test_open_null(void)
{
    TEST_ASSERT_NULL(cpr_open_file(NULL, CPR_FILE_READ, NULL));
}

void test_close_null(void)
{
    cpr_close_file(NULL); // must not crash
}

void test_write_and_read_back(void)
{
    const char *data = "hello world";
    size_t len = strlen(data);

    CprFile *w = cpr_open_file(TEST_FILE, CPR_FILE_WRITE, NULL);
    TEST_ASSERT_NOT_NULL(w);
    TEST_ASSERT_EQUAL_INT((int)len, (int)cpr_write_file(w, data, len, NULL));
    cpr_close_file(w);

    CprFile *r = cpr_open_file(TEST_FILE, CPR_FILE_READ, NULL);
    TEST_ASSERT_NOT_NULL(r);
    char buf[64] = {0};
    TEST_ASSERT_EQUAL_INT((int)len, (int)cpr_read_file(r, buf, sizeof buf, NULL));
    TEST_ASSERT_EQUAL_STRING(data, buf);
    cpr_close_file(r);
}

void test_read_partial(void)
{
    write_test_file("abcdefgh", 8);
    CprFile *f = cpr_open_file(TEST_FILE, CPR_FILE_READ, NULL);
    TEST_ASSERT_NOT_NULL(f);

    char buf[4] = {0};
    TEST_ASSERT_EQUAL_INT(3, (int)cpr_read_file(f, buf, 3, NULL));
    TEST_ASSERT_EQUAL_STRING("abc", buf);
    cpr_close_file(f);
}

void test_seek_and_tell(void)
{
    write_test_file("0123456789", 10);
    CprFile *f = cpr_open_file(TEST_FILE, CPR_FILE_READ, NULL);
    TEST_ASSERT_NOT_NULL(f);

    TEST_ASSERT_EQUAL_INT(0, (int)cpr_tell_file(f, NULL));

    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_seek_file(f, 5, CPR_SEEK_START));
    TEST_ASSERT_EQUAL_INT(5, (int)cpr_tell_file(f, NULL));

    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_seek_file(f, -2, CPR_SEEK_END));
    TEST_ASSERT_EQUAL_INT(8, (int)cpr_tell_file(f, NULL));

    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_seek_file(f, 1, CPR_SEEK_CURRENT));
    TEST_ASSERT_EQUAL_INT(9, (int)cpr_tell_file(f, NULL));

    cpr_close_file(f);
}

void test_file_size(void)
{
    write_test_file("12345", 5);
    CprFile *f = cpr_open_file(TEST_FILE, CPR_FILE_READ, NULL);
    TEST_ASSERT_NOT_NULL(f);
    TEST_ASSERT_EQUAL_INT(5, (int)cpr_file_size(f, NULL));
    // Position must be restored after the size query.
    TEST_ASSERT_EQUAL_INT(0, (int)cpr_tell_file(f, NULL));
    cpr_close_file(f);
}

void test_file_size_empty(void)
{
    CprFile *f = cpr_open_file(TEST_FILE, CPR_FILE_WRITE, NULL);
    TEST_ASSERT_NOT_NULL(f);
    TEST_ASSERT_EQUAL_INT(0, (int)cpr_file_size(f, NULL));
    cpr_close_file(f);
}

void test_flush(void)
{
    CprFile *f = cpr_open_file(TEST_FILE, CPR_FILE_WRITE, NULL);
    TEST_ASSERT_NOT_NULL(f);
    cpr_write_file(f, "data", 4, NULL);
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_flush_file(f));
    cpr_close_file(f);
}

void test_eof_not_set_after_open(void)
{
    write_test_file("data", 4);
    CprFile *f = cpr_open_file(TEST_FILE, CPR_FILE_READ, NULL);
    TEST_ASSERT_NOT_NULL(f);
    TEST_ASSERT_FALSE(cpr_file_eof(f));
    cpr_close_file(f);
}

void test_eof_set_after_full_read(void)
{
    write_test_file("hi", 2);
    CprFile *f = cpr_open_file(TEST_FILE, CPR_FILE_READ, NULL);
    TEST_ASSERT_NOT_NULL(f);
    char buf[16];
    // Reading past the end sets the EOF indicator.
    cpr_read_file(f, buf, sizeof buf, NULL);
    TEST_ASSERT_TRUE(cpr_file_eof(f));
    cpr_close_file(f);
}

void test_read_file_all_success(void)
{
    write_test_file("hello world", 11);
    char buf[32] = {0};
    size_t n = 0;
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_read_file_all(TEST_FILE, buf, sizeof buf, &n));
    TEST_ASSERT_EQUAL_INT(11, (int)n);
    TEST_ASSERT_EQUAL_STRING("hello world", buf);
}

void test_read_file_all_null_out_size(void)
{
    write_test_file("hello", 5);
    char buf[32] = {0};
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_read_file_all(TEST_FILE, buf, sizeof buf, NULL));
    TEST_ASSERT_EQUAL_STRING("hello", buf);
}

void test_read_file_all_overflow(void)
{
    write_test_file("this is too long for the buffer", 31);
    char buf[4];
    TEST_ASSERT_EQUAL_INT(CPR_ERR_OVERFLOW,
                          cpr_read_file_all(TEST_FILE, buf, sizeof buf, NULL));
}

void test_read_file_all_nonexistent(void)
{
    char buf[32];
    TEST_ASSERT_EQUAL_INT(CPR_ERR_IO,
                          cpr_read_file_all(TEST_FILE, buf, sizeof buf, NULL));
}

void test_write_file_all_creates(void)
{
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_write_file_all(TEST_FILE, "content", 7));
    CprFsStat st;
    cpr_fs_stat(TEST_FILE, &st);
    TEST_ASSERT_EQUAL_INT(7, (int)st.size);
}

void test_write_file_all_overwrites(void)
{
    cpr_write_file_all(TEST_FILE, "long content here!", 18);
    TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_write_file_all(TEST_FILE, "short", 5));
    CprFsStat st;
    cpr_fs_stat(TEST_FILE, &st);
    TEST_ASSERT_EQUAL_INT(5, (int)st.size);
}

// =============================================================================
// Directory Iterator
// =============================================================================

void test_open_dir_success(void)
{
    CprResult r = CPR_OK;
    CprDirIterator *it = cpr_open_dir(TEST_DIR, &r);
    TEST_ASSERT_NOT_NULL(it);
    TEST_ASSERT_EQUAL_INT(CPR_OK, r);
    cpr_close_dir(it);
}

void test_open_dir_nonexistent(void)
{
    CprResult r = CPR_OK;
    CprDirIterator *it = cpr_open_dir(TEST_DIR CPR_FS_SEP_STR "no_such", &r);
    TEST_ASSERT_NULL(it);
    TEST_ASSERT_EQUAL_INT(CPR_ERR_IO, r);
}

void test_open_dir_null(void)
{
    TEST_ASSERT_NULL(cpr_open_dir(NULL, NULL));
}

void test_close_dir_null(void)
{
    cpr_close_dir(NULL); // must not crash
}

void test_next_dir_empty_dir(void)
{
    CprDirIterator *it = cpr_open_dir(TEST_DIR, NULL);
    TEST_ASSERT_NOT_NULL(it);
    CprDirEntry entry;
    // setUp creates TEST_DIR with nothing in it.
    TEST_ASSERT_FALSE(cpr_next_dir(it, &entry));
    cpr_close_dir(it);
}

void test_next_dir_returns_entries(void)
{
    write_test_file("data", 4);
    cpr_mkdir(TEST_SUBDIR);

    CprDirIterator *it = cpr_open_dir(TEST_DIR, NULL);
    TEST_ASSERT_NOT_NULL(it);

    CprDirEntry entry;
    int files = 0, dirs = 0;
    while (cpr_next_dir(it, &entry)) {
        if (entry.type == CPR_FS_TYPE_FILE)
            files++;
        else if (entry.type == CPR_FS_TYPE_DIR)
            dirs++;
    }

    TEST_ASSERT_EQUAL_INT(1, files);
    TEST_ASSERT_EQUAL_INT(1, dirs);
    cpr_close_dir(it);
}

void test_next_dir_entry_name(void)
{
    write_test_file("x", 1);

    CprDirIterator *it = cpr_open_dir(TEST_DIR, NULL);
    TEST_ASSERT_NOT_NULL(it);
    CprDirEntry entry;
    TEST_ASSERT_TRUE(cpr_next_dir(it, &entry));
    TEST_ASSERT_EQUAL_STRING("file.txt", entry.name);
    cpr_close_dir(it);
}

void test_next_dir_skips_dot_entries(void)
{
    // Even with no files, we must never see "." or ".." entries.
    CprDirIterator *it = cpr_open_dir(TEST_DIR, NULL);
    TEST_ASSERT_NOT_NULL(it);
    CprDirEntry entry;
    while (cpr_next_dir(it, &entry)) {
        TEST_ASSERT_NOT_EQUAL(0, strcmp(entry.name, "."));
        TEST_ASSERT_NOT_EQUAL(0, strcmp(entry.name, ".."));
    }
    cpr_close_dir(it);
}

// =============================================================================
// main
// =============================================================================

int main(void)
{
    UNITY_BEGIN();

    // --- cpr_path_is_abs ---
    RUN_TEST(test_is_abs_relative);
    RUN_TEST(test_is_abs_empty);
    RUN_TEST(test_is_abs_null);
#if defined(CPR_PLATFORM_WINDOWS)
    RUN_TEST(test_is_abs_drive_letter);
    RUN_TEST(test_is_abs_unc);
    RUN_TEST(test_is_abs_forward_slash_not_abs_on_windows);
#else
    RUN_TEST(test_is_abs_unix_root);
#endif

    // --- cpr_path_basename ---
    RUN_TEST(test_basename_with_dir);
    RUN_TEST(test_basename_no_sep);
    RUN_TEST(test_basename_trailing_sep);
    RUN_TEST(test_basename_null);

    // --- cpr_path_ext ---
    RUN_TEST(test_ext_with_ext);
    RUN_TEST(test_ext_no_ext);
    RUN_TEST(test_ext_hidden_file);
    RUN_TEST(test_ext_multiple_dots);
    RUN_TEST(test_ext_null);

    // --- cpr_path_dirname ---
    RUN_TEST(test_dirname_with_parent);
    RUN_TEST(test_dirname_no_sep);
    RUN_TEST(test_dirname_root);
    RUN_TEST(test_dirname_null);
    RUN_TEST(test_dirname_buffer_too_small);

    // --- cpr_path_join ---
    RUN_TEST(test_join_normal);
    RUN_TEST(test_join_base_trailing_sep);
    RUN_TEST(test_join_abs_part_replaces_base);
    RUN_TEST(test_join_null);
    RUN_TEST(test_join_buffer_too_small);

    // --- cpr_normalize_path ---
    RUN_TEST(test_normalize_removes_dot);
    RUN_TEST(test_normalize_resolves_dotdot);
    RUN_TEST(test_normalize_collapses_seps);
    RUN_TEST(test_normalize_dot_input);
    RUN_TEST(test_normalize_all_dots_become_dot);
    RUN_TEST(test_normalize_null);

    // --- cpr_cwd ---
    RUN_TEST(test_cwd_returns_abs_path);
    RUN_TEST(test_cwd_null);

    // --- cpr_fs_stat / query helpers ---
    RUN_TEST(test_stat_existing_file);
    RUN_TEST(test_stat_existing_dir);
    RUN_TEST(test_stat_nonexistent);
    RUN_TEST(test_stat_null);
    RUN_TEST(test_path_exists_true);
    RUN_TEST(test_path_exists_false);
    RUN_TEST(test_path_is_file_true);
    RUN_TEST(test_path_is_file_false_for_dir);
    RUN_TEST(test_path_is_dir_true);
    RUN_TEST(test_path_is_dir_false_for_file);

    // --- Filesystem operations ---
    RUN_TEST(test_mkdir_success);
    RUN_TEST(test_mkdir_already_exists);
    RUN_TEST(test_mkdir_missing_parent);
    RUN_TEST(test_mkdir_null);
    RUN_TEST(test_mkdir_all_deep);
    RUN_TEST(test_mkdir_all_already_exists);
    RUN_TEST(test_mkdir_all_null);
    RUN_TEST(test_remove_file_success);
    RUN_TEST(test_remove_file_is_dir);
    RUN_TEST(test_remove_file_nonexistent);
    RUN_TEST(test_remove_file_null);
    RUN_TEST(test_remove_dir_empty);
    RUN_TEST(test_remove_dir_nonempty_no_force);
    RUN_TEST(test_remove_dir_nonempty_force);
    RUN_TEST(test_remove_dir_null);
    RUN_TEST(test_rename_file);
    RUN_TEST(test_rename_overwrites_dst);
    RUN_TEST(test_rename_nonexistent_src);
    RUN_TEST(test_rename_null);
    RUN_TEST(test_copy_file);
    RUN_TEST(test_copy_overwrites_dst);
    RUN_TEST(test_copy_null);

    // --- File I/O ---
    RUN_TEST(test_open_read_nonexistent);
    RUN_TEST(test_open_write_creates);
    RUN_TEST(test_open_write_truncates);
    RUN_TEST(test_open_append_preserves_content);
    RUN_TEST(test_open_null);
    RUN_TEST(test_close_null);
    RUN_TEST(test_write_and_read_back);
    RUN_TEST(test_read_partial);
    RUN_TEST(test_seek_and_tell);
    RUN_TEST(test_file_size);
    RUN_TEST(test_file_size_empty);
    RUN_TEST(test_flush);
    RUN_TEST(test_eof_not_set_after_open);
    RUN_TEST(test_eof_set_after_full_read);
    RUN_TEST(test_read_file_all_success);
    RUN_TEST(test_read_file_all_null_out_size);
    RUN_TEST(test_read_file_all_overflow);
    RUN_TEST(test_read_file_all_nonexistent);
    RUN_TEST(test_write_file_all_creates);
    RUN_TEST(test_write_file_all_overwrites);

    // --- Directory iterator ---
    RUN_TEST(test_open_dir_success);
    RUN_TEST(test_open_dir_nonexistent);
    RUN_TEST(test_open_dir_null);
    RUN_TEST(test_close_dir_null);
    RUN_TEST(test_next_dir_empty_dir);
    RUN_TEST(test_next_dir_returns_entries);
    RUN_TEST(test_next_dir_entry_name);
    RUN_TEST(test_next_dir_skips_dot_entries);

    return UNITY_END();
}
