add_executable(example_platform_info platform_info/main.c)
target_link_libraries(example_platform_info PRIVATE copper)

add_executable(example_log log/main.c)
target_link_libraries(example_log PRIVATE copper)
