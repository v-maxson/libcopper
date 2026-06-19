include(FetchContent)

FetchContent_Declare(
        unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG        v2.6.1
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(unity)

function(cpr_add_test name)
        add_executable(${name} ${name}.c)
        target_link_libraries(${name} PRIVATE copper unity ${ARGN})
        add_test(NAME ${name} COMMAND ${name})
endfunction()

cpr_add_test(test_platform)
cpr_add_test(test_arena)
cpr_add_test(test_bytesize)
cpr_add_test(test_mutex Threads::Threads)
