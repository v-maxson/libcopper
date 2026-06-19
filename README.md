# Copper

A lightweight, cross-platform utility library for C99.

> Disclaimer: This library originated as a personal collection of utilities to avoid rewriting common code across projects. It is provided as-is and is not intended for production use. Contributions are welcome; please refer to `GUIDELINES.md` before submitting.

### Production alternatives

| Library | Why it may be a better fit |
|---------|---------------------------|
| [GLib](https://docs.gtk.org/glib/) | Full-featured C utility library. Data structures, threading, atomics, and more. |
| [APR](https://apr.apache.org/) | Cross-platform runtime from Apache. Solid OS abstraction with decades of production use. |
| [stb](https://github.com/nothings/stb) | Single-file, public domain C libraries. Easy to drop into any project. |

## Features

- **Arena Allocator** — Allocate many objects at once and free them all together in a single call, which is much faster than freeing each one individually. Works with heap memory or a fixed-size buffer you provide.

- **Mutex** — A lock that prevents multiple threads from accessing shared data at the same time. More synchronization primitives coming soon.

- **Byte Size Utilities** — Macros for writing byte sizes in readable units (KB, MB, GB, KiB, MiB, GiB, and more) with conversions between them.

## Supported Platforms

| Platform | |
|----------|-|
| Windows | `CPR_PLATFORM_WINDOWS` |
| macOS | `CPR_PLATFORM_MACOS` |
| iOS | `CPR_PLATFORM_IOS` |
| Linux | `CPR_PLATFORM_LINUX` |
| Android | `CPR_PLATFORM_ANDROID` |
| FreeBSD | `CPR_PLATFORM_FREEBSD` |
| OpenBSD | `CPR_PLATFORM_OPENBSD` |

**Compilers:** MSVC, Clang, GCC

**Architectures:** x86, x86-64, ARM, ARM64, RISC-V 32, RISC-V 64
