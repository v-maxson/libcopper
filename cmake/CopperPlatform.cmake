set(COPPER_PLATFORM_DEFS "")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR WIN32)
        list(APPEND COPPER_PLATFORM_DEFS CPR_PLATFORM_WINDOWS=1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        list(APPEND COPPER_PLATFORM_DEFS CPR_PLATFORM_APPLE=1)
        if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
                list(APPEND COPPER_PLATFORM_DEFS CPR_PLATFORM_IOS=1)
        else()
                list(APPEND COPPER_PLATFORM_DEFS CPR_PLATFORM_MACOS=1)
        endif()
elseif(ANDROID)
        list(APPEND COPPER_PLATFORM_DEFS
                CPR_PLATFORM_ANDROID=1
                CPR_PLATFORM_UNIX=1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        list(APPEND COPPER_PLATFORM_DEFS
                CPR_PLATFORM_LINUX=1
                CPR_PLATFORM_UNIX=1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        list(APPEND COPPER_PLATFORM_DEFS
                CPR_PLATFORM_FREEBSD=1
                CPR_PLATFORM_UNIX=1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
        list(APPEND COPPER_PLATFORM_DEFS
                CPR_PLATFORM_OPENBSD=1
                CPR_PLATFORM_UNIX=1)
endif()

if(MSVC)
        list(APPEND COPPER_PLATFORM_DEFS CPR_COMPILER_MSVC=1)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
        list(APPEND COPPER_PLATFORM_DEFS CPR_COMPILER_CLANG=1)
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        list(APPEND COPPER_PLATFORM_DEFS CPR_COMPILER_GCC=1)
endif()

if(UNIX OR APPLE)
        include(CheckCSourceCompiles)
        find_package(Threads)
        set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
        check_c_source_compiles("
#include <pthread.h>
int main(void) {
    pthread_rwlock_t rw;
    pthread_rwlock_init(&rw, 0);
    pthread_rwlock_destroy(&rw);
    return 0;
}
" CPR_HAS_PTHREAD_RWLOCK)
        unset(CMAKE_REQUIRED_LIBRARIES)
        if(CPR_HAS_PTHREAD_RWLOCK)
                list(APPEND COPPER_PLATFORM_DEFS CPR_HAS_PTHREAD_RWLOCK=1)
        endif()
endif()

message(STATUS "copper: platform defs = ${COPPER_PLATFORM_DEFS}")
