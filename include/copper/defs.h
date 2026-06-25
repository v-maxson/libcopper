#ifndef CPR_DEFS_H
#define CPR_DEFS_H

//* Platform detection & other platform dependencies. CMake sets these via compile definitions; the
//* preprocessor fallback below covers direct inclusion without CMake.

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#ifndef CPR_PLATFORM_WINDOWS
#define CPR_PLATFORM_WINDOWS 1
#endif
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#ifndef CPR_PLATFORM_APPLE
#define CPR_PLATFORM_APPLE 1
#endif
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#ifndef CPR_PLATFORM_IOS
#define CPR_PLATFORM_IOS 1
#endif
#else
#ifndef CPR_PLATFORM_MACOS
#define CPR_PLATFORM_MACOS 1
#endif
#endif
#elif defined(__ANDROID__)
#ifndef CPR_PLATFORM_ANDROID
#define CPR_PLATFORM_ANDROID 1
#endif
#ifndef CPR_PLATFORM_UNIX
#define CPR_PLATFORM_UNIX 1
#endif
#elif defined(__linux__)
#ifndef CPR_PLATFORM_LINUX
#define CPR_PLATFORM_LINUX 1
#endif
#ifndef CPR_PLATFORM_UNIX
#define CPR_PLATFORM_UNIX 1
#endif
#elif defined(__FreeBSD__)
#ifndef CPR_PLATFORM_FREEBSD
#define CPR_PLATFORM_FREEBSD 1
#endif
#ifndef CPR_PLATFORM_UNIX
#define CPR_PLATFORM_UNIX 1
#endif
#elif defined(__OpenBSD__)
#ifndef CPR_PLATFORM_OPENBSD
#define CPR_PLATFORM_OPENBSD 1
#endif
#ifndef CPR_PLATFORM_UNIX
#define CPR_PLATFORM_UNIX 1
#endif
#endif

// --- Compiler ---
#if defined(__clang__)
#ifndef CPR_COMPILER_CLANG
#define CPR_COMPILER_CLANG 1
#endif
#elif defined(_MSC_VER)
#ifndef CPR_COMPILER_MSVC
#define CPR_COMPILER_MSVC 1
#endif
#elif defined(__GNUC__)
#ifndef CPR_COMPILER_GCC
#define CPR_COMPILER_GCC 1
#endif
#endif

// --- Architecture ---
#if defined(__x86_64__) || defined(_M_X64)
#define CPR_ARCH_X86_64 1
#elif defined(__i386__) || defined(_M_IX86)
#define CPR_ARCH_X86 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#define CPR_ARCH_ARM64 1
#elif defined(__arm__) || defined(_M_ARM)
#define CPR_ARCH_ARM 1
#elif defined(__riscv) && __riscv_xlen == 64
#define CPR_ARCH_RISCV64 1
#elif defined(__riscv) && __riscv_xlen == 32
#define CPR_ARCH_RISCV32 1
#endif

// --- Pointer width ---
#if defined(CPR_ARCH_X86_64) || defined(CPR_ARCH_ARM64) || \
	defined(CPR_ARCH_RISCV64)
#define CPR_64BIT 1
#else
#define CPR_32BIT 1
#endif

// ---Byte order ---
#if defined(__BYTE_ORDER__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define CPR_LITTLE_ENDIAN 1
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define CPR_BIG_ENDIAN 1
#endif
#elif defined(_WIN32)
#define CPR_LITTLE_ENDIAN 1
#endif

// --- Visibility ---
#if defined(CPR_PLATFORM_WINDOWS)
#if defined(COPPER_BUILD_SHARED)
#define CPR_API __declspec(dllexport)
#elif defined(COPPER_SHARED)
#define CPR_API __declspec(dllimport)
#else
#define CPR_API
#endif
#elif defined(CPR_COMPILER_GCC) || defined(CPR_COMPILER_CLANG)
#define CPR_API __attribute__((visibility("default")))
#else
#define CPR_API
#endif

// --- Inline ---
#if defined(__cplusplus) || \
	(defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
#define CPR_INLINE inline
#elif defined(CPR_COMPILER_MSVC)
#define CPR_INLINE __forceinline
#elif defined(CPR_COMPILER_GCC) || defined(CPR_COMPILER_CLANG)
#define CPR_INLINE __inline__
#else
#define CPR_INLINE
#endif

// --- Library Internals ---
#if defined(CPR_COMPILER_MSVC)
#define CPR_INTERNAL // nothing extra needed; not exported from the .lib/.dll without explicit __declspec(dllexport)
#elif defined(CPR_COMPILER_GCC) || defined(CPR_COMPILER_CLANG)
#define CPR_INTERNAL __attribute__((visibility("hidden")))
#else
#define CPR_INTERNAL
#endif

// --- Memory Alignment ---

// C99 stand-in for C11's max_align_t. The union's alignment equals the
// largest fundamental alignment required by any scalar type on the platform.
typedef union {
	long long ll;
	long double ld;
	void *ptr;
} CprMaxAlign;

#define CPR_DEFAULT_ALIGNMENT (sizeof(CprMaxAlign))

#if defined(__cplusplus)
#define cpr_alignof(T) alignof(T)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define cpr_alignof(T) _Alignof(T)
#elif defined(CPR_COMPILER_MSVC)
#define cpr_alignof(T) __alignof(T)
#else
#define cpr_alignof(T) __alignof__(T)
#endif

// --- Static Assert ---
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define CPR_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#else
// Negative array size = compile error
#define CPR_STATIC_ASSERT(cond, msg) \
	typedef char static_assert_##msg[(cond) ? 1 : -1]
#endif

#endif /* CPR_DEFS_H */
