# libcopper Coding Guidelines

## Naming conventions

All public symbols are prefixed with `cpr` (lower-case) or `CPR_` (upper-case).

| Construct | Convention | Example |
|---|---|---|
| `typedef struct` / `typedef enum` | `Cpr` + PascalCase | `CprString`, `CprArena` |
| Functions | `cpr_` + snake_case | `cpr_str_len()` |
| Function parameters | snake_case (no prefix) | `buf_size`, `out_len` |
| Struct / union members | snake_case (no prefix) | `byte_count`, `next` |
| Constant macros (`#define`, `enum` values) | `CPR_` + SCREAMING_SNAKE_CASE | `CPR_MAX_PATH`, `CPR_ERROR_NONE` |
| Function-like macros | `cpr_` + snake_case | `cpr_min(a, b)`, `cpr_array_len(a)` |
| Header guards | `CPR_` + SCREAMING_SNAKE_CASE + `_H` | `CPR_PLATFORM_H` |
| Internal (non-public) helpers | `cpr__` + snake_case (double underscore) | `cpr__hash_mix()` |

## Formatting

All C source files must be formatted with `clang-format` using the `.clang-format`
at the repository root before committing:

```sh
clang-format -i path/to/file.c
```

Key points from the active style (Linux kernel variant):

- Indentation: **tabs**, displayed as 8 columns.
- Column limit: **80**.
- Opening brace for **functions** goes on its own line (Allman).
- Opening brace for **control statements** and **structs** stays on the same line (K&R).
- Pointer `*` binds to the **right** (`int *p`, not `int* p`).

Example:

```c
typedef struct {
        int x;
        int y;
} CprPoint;

int cpr_point_add(CprPoint a, CprPoint b, CprPoint *out)
{
        if (out == NULL) {
                return -1;
        }
        out->x = a.x + b.x;
        out->y = a.y + b.y;
        return 0;
}
```

## Language standard

Write strictly conforming **C99**. No compiler extensions unless they are
guarded by a platform or compiler macro from `copper/platform.h`.

```c
/* OK */
#if defined(CPR_COMPILER_GCC) || defined(CPR_COMPILER_CLANG)
__attribute__((noinline)) static void cpr__slow_path(void) { ... }
#endif

/* Not OK — unguarded extension */
__attribute__((noinline)) static void cpr__slow_path(void) { ... }
```

## Cross-platform compatibility

- Every public API must compile on **Windows (MSVC & MinGW), macOS, Linux,
  FreeBSD, Android, and iOS** unless a feature is genuinely unavailable there.
- When a feature cannot be provided on a platform, either provide a functioning alternative
  or **exclude its declaration** with a preprocessor guard rather than providing a stub that silently fails.

```c
/* Good — absent on Windows, but cleanly excluded */
#if defined(CPR_PLATFORM_UNIX)
CPR_API int cpr_fd_set_nonblock(int fd);
#endif
```

- Use `CPR_API` on every public function/variable declaration so that shared
  library builds export symbols correctly on all platforms.
- Avoid POSIX-only headers (`<unistd.h>`, `<sys/types.h>`, …) in public headers;
  confine them to platform-guarded `.c` files.
- Do not assume a fixed pointer or `long` width; use `<stdint.h>` types
  (`uint32_t`, `uintptr_t`, …) for sized arithmetic.

## Header documentation

Every symbol declared in a public header (`include/copper/*.h`) must carry a
short doc-comment immediately above it if it is not immediately obvious what it does. 
One or two lines is the norm; only go longer when the contract is genuinely non-obvious.

```c
// no comment here because it is obvious what this does
CPR_API size_t cpr_str_len(CprString s);

/// Allocates `size` bytes from `arena`.
/// Returns NULL when the arena has no remaining capacity.
CPR_API void *cpr_arena_alloc(CprArena *arena, size_t size);
```

When a symbol involves ownership transfer, the doc-comment **must** state it
explicitly — who is responsible for freeing the object, and under what
conditions:

```c
/// Creates a new string by copying `data` (length `len`) into `arena`.
/// Ownership of the returned CprString belongs to `arena`; do not free it
/// directly.
CPR_API CprString cpr_str_from_buf(CprArena *arena, const char *data,
                                   size_t len);

/// Duplicates `s` with malloc.
/// Caller owns the returned pointer and must free it with cpr_str_free().
CPR_API CprString cpr_str_dup(CprString s);
```

Struct and enum members that have non-obvious ownership or lifetime constraints
should carry an inline comment on the member itself:

```c
typedef struct {
        char *data;   ///< owned; free with cpr_str_free()
        size_t len;
} CprString;
```

## Comments

Write comments only when the **why** is non-obvious: a hidden constraint, a
subtle invariant, a workaround for a specific external bug. Do not describe
*what* the code does — well-named identifiers already do that.

## Error handling

Return values carry errors; do not use global state (no `errno`-style globals
in new APIs). Define a `CprResult` enum or integer return code per module and
document the contract in the header.

## Adding a new source file

1. Create `src/<module>.c` and `include/copper/<module>.h`.
2. Add `src/<module>.c` to the `COPPER_SOURCES` list in the root `CMakeLists.txt`.
3. Add at least one test file `tests/test_<module>.c` and register it with
   `cpr_add_test(test_<module>)` in `tests/CMakeLists.txt`.
4. Run `clang-format -i` on both files before opening a pull request.
