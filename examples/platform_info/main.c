#include "copper/copper.h"

#include <stdio.h>

int main(void)
{
	printf("libcopper %s\n\n", cpr_version_string());

#if defined(CPR_PLATFORM_WINDOWS)
	printf("Platform : Windows\n");
#elif defined(CPR_PLATFORM_MACOS)
	printf("Platform : macOS\n");
#elif defined(CPR_PLATFORM_IOS)
	printf("Platform : iOS\n");
#elif defined(CPR_PLATFORM_ANDROID)
	printf("Platform : Android\n");
#elif defined(CPR_PLATFORM_LINUX)
	printf("Platform : Linux\n");
#elif defined(CPR_PLATFORM_FREEBSD)
	printf("Platform : FreeBSD\n");
#elif defined(CPR_PLATFORM_OPENBSD)
	printf("Platform : OpenBSD\n");
#else
	printf("Platform : Unknown\n");
#endif

#if defined(CPR_ARCH_X86_64)
	printf("Arch     : x86_64\n");
#elif defined(CPR_ARCH_X86)
	printf("Arch     : x86\n");
#elif defined(CPR_ARCH_ARM64)
	printf("Arch     : ARM64\n");
#elif defined(CPR_ARCH_ARM)
	printf("Arch     : ARM\n");
#elif defined(CPR_ARCH_RISCV64)
	printf("Arch     : RISC-V 64\n");
#elif defined(CPR_ARCH_RISCV32)
	printf("Arch     : RISC-V 32\n");
#else
	printf("Arch     : Unknown\n");
#endif

#if defined(CPR_64BIT)
	printf("Width    : 64-bit\n");
#else
	printf("Width    : 32-bit\n");
#endif

#if defined(CPR_LITTLE_ENDIAN)
	printf("Endian   : little\n");
#elif defined(CPR_BIG_ENDIAN)
	printf("Endian   : big\n");
#else
	printf("Endian   : unknown\n");
#endif

#if defined(CPR_COMPILER_MSVC)
	printf("Compiler : MSVC\n");
#elif defined(CPR_COMPILER_CLANG)
	printf("Compiler : Clang\n");
#elif defined(CPR_COMPILER_GCC)
	printf("Compiler : GCC\n");
#else
	printf("Compiler : Unknown\n");
#endif

	return 0;
}
