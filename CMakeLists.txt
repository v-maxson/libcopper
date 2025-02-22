cmake_minimum_required(VERSION 3.28)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)

project(
        libcopper
        LANGUAGES C
        VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(COPPER_SHARED "Build shared library" OFF)
option(COPPER_STATIC "Build static library" ON)
option(COPPER_BUILD_TESTS "Build tests" OFF)

set(LIBUTIL_SOURCES
        ./src/arena.c
        ./src/string.c
)

include("./cmake/generate_headers.cmake")

if (COPPER_SHARED)
    add_library(${PROJECT_NAME} SHARED ${LIBUTIL_SOURCES})
    target_include_directories(
            ${PROJECT_NAME}
            PUBLIC
            "./include"
            GENERATED_HEADERS_DIR
    )
endif()

if (COPPER_STATIC)
    add_library("${PROJECT_NAME}static" STATIC ${LIBUTIL_SOURCES})
    target_include_directories(
            "${PROJECT_NAME}static"
            PUBLIC
            "./include"
            GENERATED_HEADERS_DIR
    )
endif()

if (COPPER_BUILD_TESTS)
    add_subdirectory(./tests)
endif()
