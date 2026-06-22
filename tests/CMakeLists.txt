include(FetchContent)

FetchContent_Declare(
        unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG        v2.6.1
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(unity)

# Unity is a third-party library; don't apply our strict warning flags to it.
if(MSVC)
        target_compile_options(unity PRIVATE /W0 /WX-)
else()
        target_compile_options(unity PRIVATE -w)
endif()

function(cpr_add_test name)
        add_executable(${name} ${name}.c)
        target_link_libraries(${name} PRIVATE copper unity ${ARGN})
        add_test(NAME ${name} COMMAND ${name})
endfunction()

cpr_add_test(test_platform)
cpr_add_test(test_arena)
cpr_add_test(test_bytesize)
cpr_add_test(test_mutex Threads::Threads)
cpr_add_test(test_condvar Threads::Threads)
cpr_add_test(test_rwlock Threads::Threads)
cpr_add_test(test_atomic Threads::Threads)
cpr_add_test(test_time)
