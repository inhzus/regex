enable_testing()

include_directories(..)
add_executable(regex_test graph_test.cc id_test.cc utils.cc main.cc)
target_link_libraries(regex_test regex)
