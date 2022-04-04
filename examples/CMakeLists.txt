cmake_minimum_required (VERSION 3.16)

macro (set_flags)

    message ("Setting flags for examples")

    set (CMAKE_CXX_FLAGS "")
    set (CMAKE_CXX_FLAGS_RELEASE "")
    set (CMAKE_CXX_FLAGS_DEBUG "")
    set (CMAKE_CXX_FLAGS_RELMINSIZE "")
    set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "")

    if (MSVC OR MSVC_IDE)
        target_compile_options (main PRIVATE "/W4" "/WX" "/EHsc")
    else ()
        target_compile_options (main PRIVATE "-Wall" "-Wextra" "-Werror" "-pedantic-errors")
    endif ()

endmacro ()

add_executable (
    main
    main.cpp
)

target_link_libraries (
    main
    pthread
)

add_executable (
    simple_concurrency_test
    simple_concurrency_test.cpp
)

target_link_libraries (
    simple_concurrency_test
    pthread
)

add_executable (
    multiread_test
    multiread_test.cpp
)

target_link_libraries (
    multiread_test
    pthread
)

set_flags ()
