cmake_minimum_required (VERSION 3.16)

project (AgHashMap LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories (${CMAKE_SOURCE_DIR}/src)

# if (NOT DEFINED BUILD_EXAMPLES)
#     set (BUILD_EXAMPLES "on")
# endif ()

# if (NOT DEFINED BUILD_TESTS)
#     set (BUILD_TESTS "on")
# endif ()

# if (NOT DEFINED BUILD_BENCHMARKS)
#     set (BUILD_BENCHMARKS "on")
# endif ()

# string (TOLOWER ${BUILD_EXAMPLES} BUILD_EXAMPLES)
# string (TOLOWER ${BUILD_TESTS} BUILD_TESTS)
# string (TOLOWER ${BUILD_BENCHMARKS} BUILD_BENCHMARKS)

# if (NOT (${BUILD_EXAMPLES} STREQUAL "off"))

#     if (NOT (${BUILD_EXAMPLES} STREQUAL "on"))
#         message ("Value of BUILD_EXAMPLES " ${BUILD_EXAMPLES} " does not match on/off, moving forward with value as on")
#     endif ()

#     add_subdirectory (${CMAKE_SOURCE_DIR}/examples ${CMAKE_SOURCE_DIR}/examples/build)
# endif ()

# if (NOT (${BUILD_TESTS} STREQUAL "off"))

#     if (NOT (${BUILD_EXAMPLES} STREQUAL "on"))
#         message ("Value of BUILD_TESTS " ${BUILD_TESTS} " does not match on/off, moving forward with value as on")
#     endif ()

#     add_subdirectory (${CMAKE_SOURCE_DIR}/tests ${CMAKE_SOURCE_DIR}/tests/build)
# endif ()

# if (NOT (${BUILD_BENCHMARKS} STREQUAL "off"))

#     if (NOT (${BUILD_EXAMPLES} STREQUAL "on"))
#         message ("Value of BUILD_BENCHMARKS " ${BUILD_BENCHMARKS} " does not match on/off, moving forward with value as on")
#     endif ()

#     add_subdirectory (${CMAKE_SOURCE_DIR}/benchmarks ${CMAKE_SOURCE_DIR}/benchmarks/build)
# endif ()

add_subdirectory (${CMAKE_SOURCE_DIR}/examples ${CMAKE_SOURCE_DIR}/examples/build)
