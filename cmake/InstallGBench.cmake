set (GBENCH_VERSION     1.1.0)
set (GBENCH_URL_MD5     66b2a23076cf70739525be0092fc3ae3)

ExternalProject_Add(google-benchmark
    DOWNLOAD_NAME       google-benchmark-${GBENCH_VERSION}.tar.gz
    URL                 https://github.com/google/benchmark/archive/v${GBENCH_VERSION}.tar.gz
    URL_MD5             ${GBENCH_URL_MD5}
    CMAKE_ARGS          -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
)

ExternalProject_Get_Property(google-benchmark INSTALL_DIR)

set (GBENCH_ROOT_DIR        ${INSTALL_DIR})
set (GBENCH_INCLUDE_PATH    ${GBENCH_ROOT_DIR}/include)
set (GBENCH_LIBRARY_PATH    ${GBENCH_ROOT_DIR}/lib)
set (GBENCH_LIBRARY         ${GBENCH_LIBRARY_PATH}/libbenchmark.a)
set (GBENCH_FOUND           YES)

add_library(GBENCH_LIBRARY STATIC IMPORTED)
add_dependencies(GBENCH_LIBRARY google-benchmark)
mark_as_advanced(GBENCH_LIBRARY GBENCH_INCLUDE_PATH)