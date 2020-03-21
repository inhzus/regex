add_executable(regex_example
    main.cc)
find_package(fmt)
target_link_libraries(regex_example regex fmt::fmt)

