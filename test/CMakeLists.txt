enable_testing()

add_executable(regex_test graph_test.cc)
target_link_libraries(regex_test regex)
