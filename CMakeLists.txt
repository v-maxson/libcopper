cmake_minimum_required(VERSION 3.31.6)

project (
    "copper"
    LANGUAGES C
    VERSION 0.1.0
)

set(
    PROJECT_SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/error.c"
)

set(
    PROJECT_HEADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/include/copper.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/copper/error.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/copper/optional.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/copper/result.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/copper/typedef.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/copper/version.h"
)

# Add static library
add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCE_FILES} ${PROJECT_HEADER_FILES})
add_library(copper::copper ALIAS ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
    COMPILE_FLAGS "-Wall -Wextra -pedantic -Werror"
)

# Ensure a threading library is available
find_package(Threads REQUIRED)

# Link threading library
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)

# Generate the version header
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/copper/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/copper/version.h"
)

include(GNUInstallDirs)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    FILES ${PROJECT_HEADER_FILES}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/copper
)

# Ensure the generated version.h is installed correctly
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/include/copper/version.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/copper
)

install(
    EXPORT ${PROJECT_NAME}-targets
    FILE ${PROJECT_NAME}-targets.cmake
    NAMESPACE copper::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/copperConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/copperConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/copperConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

