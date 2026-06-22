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

- **Arena Allocator** - Allocate many objects at once and free them all together in a single call, which is much faster than freeing each one individually. Works with heap memory or a fixed-size buffer you provide.

- **Synchronization Primitives** - Tools for coordinating access to shared data across threads.
  - **Mutex** - A lock that ensures only one thread can access shared data at a time.
  - **Condition Variable** - Lets a thread sleep until another thread signals that something has changed.
  - **Read-Write Lock** - Allows multiple threads to read shared data at the same time, while ensuring writers get exclusive access.

- **Byte Size Utilities** - Macros for writing byte sizes in readable units (KB, MB, GB, KiB, MiB, GiB, and more) with conversions between them.

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

## Is Copper right for my project?

Copper is aimed at everyday desktop, server, and mobile apps on the supported platforms. If you need portable threading and utilities without pulling in a big framework, it's a reasonable fit.

It's probably **not** what you want if:

- You're targeting **embedded or bare-metal** hardware. Copper assumes a general-purpose OS and has no RTOS support.
- You need **production-grade reliability**. See the disclaimer above.
- You need **C89** compatibility. Copper is C99.

## Coming Soon

- Threading abstractions
- Logging system
- Filesystem
- Timing/Clock
- Dynamic array type
- Base64 encoding
- CLI argument parsing
- Hash map
- UUID generation
- Dynamic string type

## Contributing

If any part of a contribution was written with the assistance of an AI tool, please note that in the pull request description.

## License

This project is licensed under the GPL v3. The source code may not be used to train AI or machine learning models.
