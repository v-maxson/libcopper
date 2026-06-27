cmake_minimum_required(VERSION 3.16)
project(copper
        VERSION 0.3.1
        DESCRIPTION "Cross-platform C99 utility library"
        LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(COPPER_BUILD_TESTS    "Build the test suite"     ON)
option(COPPER_BUILD_EXAMPLES "Build examples"           ON)
option(COPPER_SHARED         "Build as shared library"  OFF)

if(MSVC)
        add_compile_options(/W4 /WX)
else()
        add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

find_package(Threads REQUIRED)
include(cmake/CopperPlatform.cmake)

set(COPPER_SOURCES
        src/arena.c
        src/atomic.c
        src/copper.c
        src/env.c
        src/error.c
        src/fs.c
        src/log.c
        src/sync.c
        src/thread.c
        src/time.c
)

if(COPPER_SHARED)
        add_library(copper SHARED ${COPPER_SOURCES})
else()
        add_library(copper STATIC ${COPPER_SOURCES})
endif()

add_library(copper::copper ALIAS copper)

target_include_directories(copper
        PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
)

target_link_libraries(copper PRIVATE Threads::Threads)

if(UNIX OR APPLE)
        target_compile_definitions(copper PRIVATE _POSIX_C_SOURCE=200809L)
endif()
if(APPLE)
        # Expose BSD extensions (st_mtimespec, d_type, etc.) alongside POSIX.
        target_compile_definitions(copper PRIVATE _DARWIN_C_SOURCE)
endif()

target_compile_definitions(copper PUBLIC ${COPPER_PLATFORM_DEFS})

include(GNUInstallDirs)

install(TARGETS copper
        EXPORT copperTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/copper
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT copperTargets
        FILE copperTargets.cmake
        NAMESPACE copper::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/copper
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
        cmake/copperConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/copperConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/copper
)
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/copperConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/copperConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/copperConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/copper
)

if(COPPER_BUILD_TESTS)
        enable_testing()
        add_subdirectory(tests)
endif()

if(COPPER_BUILD_EXAMPLES)
        add_subdirectory(examples)
endif()
