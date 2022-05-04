cmake_minimum_required (VERSION 3.16)

# helper macro to set compiler flags for all targets
macro (set_flags)

    message ("Setting flags for benchmarks")

    # clear out all flags which CMake sets by default based on the build mode
    set (CMAKE_CXX_FLAGS "")
    set (CMAKE_CXX_FLAGS_RELEASE "")
    set (CMAKE_CXX_FLAGS_DEBUG "")
    set (CMAKE_CXX_FLAGS_RELMINSIZE "")
    set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "")

    # all warnings (even extra) should be considered as errors
    # additionally, optimize completely for speed
    if (MSVC OR MSVC_IDE)
        target_compile_options (single_threaded PRIVATE "/W4" "/WX" "/EHsc" "/Ox")
        target_compile_options (random_gen PRIVATE "/W4" "/WX" "/EHsc" "/Ox")
        target_compile_options (sequence_gen PRIVATE "/W4" "/WX" "/EHsc" "/Ox")
    else ()
        target_compile_options (single_threaded PRIVATE "-Wall" "-Wextra" "-Werror" "-pedantic-errors" "-O3")
        target_compile_options (random_gen PRIVATE "-Wall" "-Wextra" "-Werror" "-pedantic-errors" "-O3")
        target_compile_options (sequence_gen PRIVATE "-Wall" "-Wextra" "-Werror" "-pedantic-errors" "-O3")
    endif ()

endmacro ()

# helper macro to define preprocessor macros for all targets
macro (set_macros)

    target_compile_definitions (random_gen PRIVATE FILE_OUT="${CMAKE_CURRENT_SOURCE_DIR}/data/random_all.in")
    target_compile_definitions (sequence_gen PRIVATE FILE_OUT="${CMAKE_CURRENT_SOURCE_DIR}/data/sequence_all.in")

endmacro ()

add_executable (
    single_threaded
    single_threaded.cpp
)

add_executable (
    random_gen
    random_gen.cpp
)

add_executable (
    sequence_gen
    sequence_gen.cpp
)

set_flags ()
set_macros ()
