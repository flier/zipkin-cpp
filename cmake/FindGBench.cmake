# Find Google Benchmark

find_path(GBENCH_INCLUDE_PATH NAMES benchmark/benchmark.h)
find_library(GBENCH_LIBRARY NAMES benchmark)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(google-benchmark
    REQUIRED_VARS   GBENCH_INCLUDE_PATH GBENCH_LIBRARY
)
mark_as_advanced(GBENCH_INCLUDE_PATH GBENCH_LIBRARY)

if(GBENCH_FOUND)
    if(NOT GBENCH_FIND_QUIETLY)
        message(STATUS "Found google-benchmark: ${GBENCH_LIBRARY}; includes - ${GBENCH_INCLUDE_PATH}")
    endif(NOT GBENCH_FIND_QUIETLY)
else(GBENCH_FOUND)
    if(GBENCH_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find google-benchmark library.")
    endif(GBENCH_FIND_REQUIRED)
endif(GBENCH_FOUND)