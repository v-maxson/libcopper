# libcopper Coding Guidelines

## Naming

All public symbols are prefixed with `cpr` or `CPR_`.

| Construct | Convention | Example |
|---|---|---|
| `typedef struct` / `typedef enum` | `Cpr` + PascalCase | `CprExampleType` |
| Functions | `cpr_` + snake_case | `cpr_example_fn()` |
| Parameters / struct members | snake_case | `example_param`, `next` |
| Constant macros | `CPR_` + SCREAMING_SNAKE_CASE | `CPR_EXAMPLE_CONSTANT` |
| Function-like macros | `cpr_` + snake_case | `cpr_example_macro(a, b)` |
| Header guards | `CPR_` + SCREAMING_SNAKE_CASE + `_H` | `CPR_EXAMPLE_H` |
| Internal helpers | `cpr__` + snake_case (double underscore) | `cpr__example_helper()` |

## Formatting

Format with `clang-format -i` before committing. Key style points (Linux kernel variant):

- Tabs, 8-column indent; 80-column limit.
- Allman braces for functions, K&R for control statements and structs.
- `int *p`, not `int* p`.

## Language standard

C99 only. Guard all compiler extensions with a macro from `defs.h`:

```c
#if defined(CPR_COMPILER_GCC) || defined(CPR_COMPILER_CLANG)
__attribute__((noinline)) static void cpr__example_fn(void) { ... }
#endif
```

## Cross-platform compatibility

Every public API must compile on Windows (MSVC & MinGW), macOS, Linux, FreeBSD, Android, and iOS.

- Use `CPR_API` on all public declarations.
- Keep platform headers (`<windows.h>`, `<pthread.h>`, etc.) out of public headers; confine them to platform-guarded `.c` files.
- Use `<stdint.h>` types for fixed-width arithmetic; don't assume pointer or `long` width.

**Opaque storage:** Types that wrap an OS handle use a fixed-size byte array in their public struct, sized to the worst-case platform. Always add a compile-time size check in the `.c` file:

```c
typedef uint8_t cpr__foo_size_check
    [sizeof(CprInternalFoo) <= CPR_FOO_STORAGE_SIZE ? 1 : -1];
```

**Fallbacks:** When a feature has no native implementation on a platform, provide a fallback built from existing Copper primitives if one is reasonable. If no fallback exists, exclude the declaration with a preprocessor guard — a compile error is better than silent failure:

```c
#if defined(CPR_PLATFORM_UNIX)
CPR_API int cpr_example_fn(int fd);
#endif
```

## Header documentation

Every non-obvious public symbol needs a short doc-comment — one or two lines. Document ownership explicitly whenever it isn't obvious from the types alone.

```c
/// Allocates `size` bytes from `ctx`. Returns NULL if full.
CPR_API void *cpr_example_alloc(CprExampleCtx *ctx, size_t size);
```

## Comments

Comment the *why*, not the *what*. If removing the comment wouldn't confuse a future reader, don't write it.

## Error handling

Return errors as `CprResult`; no global error state. Document the possible return codes in the header.

## Adding a new module

1. Create `src/<module>.c` and `include/copper/<module>.h`.
2. Add `src/<module>.c` to `COPPER_SOURCES` in `CMakeLists.txt`.
3. Add `tests/test_<module>.c` and register it with `cpr_add_test(test_<module>)` in `tests/CMakeLists.txt`.
4. Run `clang-format -i` on all new files before opening a pull request.
