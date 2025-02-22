cmake_minimum_required(VERSION 3.28)

project(tests C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(COPPER_STATIC ON)

macro(make_test name)
    add_executable(${name} ${name}.c)
    add_test(${name} ${name})
    target_link_libraries(${name} libcopperstatic)
endmacro()

make_test(arena_test)
make_test(string_test)