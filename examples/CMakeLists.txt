cmake_minimum_required (VERSION 3.16)

macro (set_flags)

    message ("Setting flags for examples")

    # clear out all flags which CMake sets by default based on the build mode
    set (CMAKE_CXX_FLAGS "")
    set (CMAKE_CXX_FLAGS_RELEASE "")
    set (CMAKE_CXX_FLAGS_DEBUG "")
    set (CMAKE_CXX_FLAGS_RELMINSIZE "")
    set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "")

    # all warnings (even extra) should be considered as errors
    if (MSVC OR MSVC_IDE)
        target_compile_options (emails PRIVATE "/W4" "/WX" "/EHsc")
        target_compile_options (concurrent_all PRIVATE "/W4" "/WX" "/EHsc")
        target_compile_options (concurrent_all_threadpool PRIVATE "/W4" "/WX" "/EHsc")
    else ()
        target_compile_options (emails PRIVATE "-Wall" "-Wextra" "-Werror" "-pedantic-errors")
        target_compile_options (concurrent_all PRIVATE "-Wall" "-Wextra" "-Werror" "-pedantic-errors")
        target_compile_options (concurrent_all_threadpool PRIVATE "-Wall" "-Wextra" "-Werror" "-pedantic-errors")
    endif ()

endmacro ()

add_executable (
    emails
    emails.cpp
)

target_link_libraries (
    emails
    pthread
)

add_executable (
    concurrent_all
    concurrent_all.cpp
)

target_link_libraries (
    concurrent_all
    pthread
)

add_executable (
    concurrent_all_threadpool
    concurrent_all_threadpool.cpp
)

target_link_libraries (
    concurrent_all_threadpool
    pthread
)

set_flags ()
