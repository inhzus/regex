enable_testing()

include_directories(..)
add_executable(regex_test
        graph_test.cc
        exp_test.cc
        main.cc
        utils.cc)
target_link_libraries(regex_test regex)
