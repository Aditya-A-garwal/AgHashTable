cmake_minimum_required (VERSION 3.16)

project (AgHashMap LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories (${CMAKE_SOURCE_DIR}/src)

if (NOT DEFINED BUILD_EXAMPLES)
    message ("No value given for BUILD_EXAMPLES, assuming \"ON\"")
    set (BUILD_EXAMPLES "on")
endif ()

if (NOT DEFINED BUILD_TESTS)
    message ("No value given for BUILD_TESTS, assuming \"ON\"")
    set (BUILD_TESTS "on")
endif ()

if (BUILD_EXAMPLES)
    add_subdirectory (${CMAKE_SOURCE_DIR}/examples ${CMAKE_SOURCE_DIR}/examples/build)
endif ()

if (BUILD_TESTS)
    add_subdirectory (${CMAKE_SOURCE_DIR}/tests ${CMAKE_SOURCE_DIR}/tests/build)
endif ()
