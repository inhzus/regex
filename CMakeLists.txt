cmake_minimum_required(VERSION 3.14)

project(regex)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

execute_process(COMMAND
        sh -c
        "find include src test example | grep -E '\\.(cc|h)$' | xargs -I {} cpplint --root=include {} | grep -E '^\\.\\/'"
        WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY})

include_directories(include)
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(example)
add_subdirectory(rep)

set(LIBRARY_NAME regex)
set(LIBRARY_FOLDER regex)

