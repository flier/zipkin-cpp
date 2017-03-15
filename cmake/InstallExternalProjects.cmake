include(ExternalProject)
include(FindPackageHandleStandardArgs)

set (EXTERNAL_PROJECT_DIR ${CMAKE_BINARY_DIR}/externals)
set_directory_properties(PROPERTIES EP_PREFIX ${EXTERNAL_PROJECT_DIR})

message(STATUS "Install external dependencies to ${EXTERNAL_PROJECT_DIR}")

include_directories(BEFORE SYSTEM ${EXTERNAL_PROJECT_DIR}/include)
link_directories(${EXTERNAL_PROJECT_DIR}/lib)

if (NOT ZLIB_FOUND OR USE_BUNDLED_ZLIB)
    set (ZLIB_VERSION_STRING        1.2.11)

    ExternalProject_Add(zlib
        URL                 http://www.zlib.net/zlib-${ZLIB_VERSION_STRING}.tar.gz
        URL_MD5             1c9f62f0778697a09d36121ead88e08e
        CONFIGURE_COMMAND   <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
        BUILD_COMMAND       make
        INSTALL_COMMAND     make install
    )

    ExternalProject_Get_Property(zlib INSTALL_DIR)

    set (ZLIB_ROOT_DIR          ${INSTALL_DIR})
    set (ZLIB_INCLUDE_DIRS      ${ZLIB_ROOT_DIR}/include)
    set (ZLIB_LIBRARY_DIRS      ${ZLIB_ROOT_DIR}/lib)
    set (ZLIB_LIBRARIES         ${ZLIB_LIBRARY_DIRS}/libz.a)

    find_package_handle_standard_args(zlib
        REQUIRED_ARGS ZLIB_LIBRARIES ZLIB_INCLUDE_DIRS
    )

    mark_as_advanced(ZLIB_LIBRARIES ZLIB_INCLUDE_DIRS)

    message(STATUS "Use bundled zlib v${ZLIB_VERSION_STRING}")
endif ()

if (NOT RAPIDJSON_FOUND OR USE_BUNDLED_RAPIDJSON)
    set (RAPIDJSON_VERSION          1.1.0)

    ExternalProject_Add(RapidJSON
        URL             https://github.com/miloyip/rapidjson/archive/v${RAPIDJSON_VERSION}.tar.gz
        URL_MD5         badd12c511e081fec6c89c43a7027bce
        CMAKE_ARGS      -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    )

    ExternalProject_Get_Property(RapidJSON INSTALL_DIR)

    set (RAPIDJSON_ROOT_DIR     ${INSTALL_DIR})
    set (RAPIDJSON_INCLUDES     ${RAPIDJSON_ROOT_DIR}/include)

    find_package_handle_standard_args(RapidJSON
        REQUIRED_ARGS RAPIDJSON_INCLUDES
    )

    mark_as_advanced(RAPIDJSON_INCLUDES)

    message(STATUS "Use bundled RapidJSON v${RAPIDJSON_VERSION}")
endif ()

if (NOT LIBRDKAFKA_FOUND OR USE_BUNDLED_LIBRDKAFKA)
    set (LibRDKafka_VERSION         0.9.4)

    ExternalProject_Add(LibRDKafka
        URL             https://github.com/edenhill/librdkafka/archive/v${LibRDKafka_VERSION}.tar.gz
        URL_MD5         6f40198e6068475c34ae8c9faafa6e8a
        CMAKE_ARGS      -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    )

    ExternalProject_Get_Property(LibRDKafka INSTALL_DIR)

    set (LibRDKafka_ROOT_DIR        ${INSTALL_DIR})
    set (LibRDKafka_LIBRARY_DIR     ${LibRDKafka_ROOT_DIR}/lib)
    set (LibRDKafka_INCLUDE_DIR     ${LibRDKafka_ROOT_DIR}/include)

    find_library(LibRDKafka_LIBRARIES
        NAMES rdkafka++
        PATH ${LibRDKafka_LIBRARY_DIR}
        NO_SYSTEM_ENVIRONMENT_PATH
    )
    find_library(LibRDKafka_C_LIBRARIES
        NAMES rdkafka
        PATH ${LibRDKafka_LIBRARY_DIR}
        NO_SYSTEM_ENVIRONMENT_PATH
    )

    find_package_handle_standard_args(LibRDKafka
        REQUIRED_ARGS LibRDKafka_LIBRARIES LibRDKafka_C_LIBRARIES LibRDKafka_INCLUDE_DIR
    )

    add_library(LibRDKafka_LIBRARIES UNKNOWN IMPORTED)
    add_library(LibRDKafka_C_LIBRARIES UNKNOWN IMPORTED)
    add_dependencies(LibRDKafka_LIBRARIES LibRDKafka)
    add_dependencies(LibRDKafka_C_LIBRARIES LibRDKafka)
    mark_as_advanced(LibRDKafka_LIBRARIES LibRDKafka_C_LIBRARIES LibRDKafka_INCLUDE_DIR)

    message(STATUS "Use bundled librdkafka v${LibRDKafka_VERSION}")
endif ()

if (NOT DOUBLE_CONVERSION_FOUND OR USE_BUNDLED_DOUBLE_CONVERSION)
    set (DOUBLE_CONVERSION_VERSION      1.1.5)

    ExternalProject_Add(double_conversion
        URL             https://github.com/google/double-conversion/archive/v${DOUBLE_CONVERSION_VERSION}.tar.gz
        URL_MD5         f7c62594d7ecfbc4421da32bc341a919
        CMAKE_ARGS      -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    )

    ExternalProject_Get_Property(double_conversion INSTALL_DIR)

    set (DOUBLE_CONVERSION_ROOT_DIR     ${INSTALL_DIR})
    set (DOUBLE_CONVERSION_INCLUDE_DIR  ${DOUBLE_CONVERSION_ROOT_DIR}/include)
    set (DOUBLE_CONVERSION_LIBRARY_DIR  ${DOUBLE_CONVERSION_ROOT_DIR}/lib)
    set (DOUBLE_CONVERSION_LIBRARY      ${DOUBLE_CONVERSION_LIBRARY_DIR}/libdouble-conversion.a)

    find_package_handle_standard_args(double_conversion
        REQUIRED_ARGS DOUBLE_CONVERSION_LIBRARY DOUBLE_CONVERSION_INCLUDE_DIR
    )

    add_library(DOUBLE_CONVERSION_LIBRARY STATIC IMPORTED)
    add_dependencies(DOUBLE_CONVERSION_LIBRARY double_conversion)
    mark_as_advanced(DOUBLE_CONVERSION_LIBRARY DOUBLE_CONVERSION_INCLUDE_DIR)

    message(STATUS "Use bundled double_conversion v${DOUBLE_CONVERSION_VERSION}")
endif ()

if (NOT GFLAGS_FOUND OR USE_BUNDLED_GFLAGS)
    set (GFLAGS_VERSION     2.2.0)

    ExternalProject_Add(gflags
        URL                 https://github.com/gflags/gflags/archive/v${GFLAGS_VERSION}.tar.gz
        URL_MD5             b99048d9ab82d8c56e876fb1456c285e
        CMAKE_ARGS          -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    )

    ExternalProject_Get_Property(gflags INSTALL_DIR)

    set (GFLAGS_ROOT_DIR        ${INSTALL_DIR})
    set (GFLAGS_INCLUDE_DIRS    ${GFLAGS_ROOT_DIR}/include)
    set (GFLAGS_LIBRARIES_DIR   ${GFLAGS_ROOT_DIR}/lib)
    set (GFLAGS_LIBRARIES       ${GFLAGS_LIBRARIES_DIR}/libgflags.a)

    find_package_handle_standard_args(gflags
        REQUIRED_ARGS GFLAGS_LIBRARIES GFLAGS_LIBRARIES_DIR GFLAGS_INCLUDE_DIRS
    )

    add_library(GFLAGS_LIBRARIES STATIC IMPORTED)
    add_dependencies(GFLAGS_LIBRARIES gflags)
    mark_as_advanced(GFLAGS_LIBRARIES GFLAGS_LIBRARIES_DIR GFLAGS_INCLUDE_DIRS)

    message(STATUS "Use bundled gflags v${GFLAGS_VERSION}")
endif ()

if (NOT GLOG_FOUND OR USE_BUNDLED_GLOG)
    set (GLOG_VERSION       0.3.4)

    ExternalProject_Add(glog
        URL                 https://github.com/google/glog/archive/v${GLOG_VERSION}.tar.gz
        URL_MD5             df92e05c9d02504fb96674bc776a41cb
        CONFIGURE_COMMAND   <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
        BUILD_COMMAND       make
        INSTALL_COMMAND     make install
    )

    ExternalProject_Get_Property(glog INSTALL_DIR)

    set (GLOG_ROOT_DIR          ${INSTALL_DIR})
    set (GLOG_INCLUDE_PATH      ${GLOG_ROOT_DIR}/include)
    set (GLOG_LIBRARY_PATH      ${GLOG_ROOT_DIR}/lib)
    set (GLOG_LIBRARY           ${GLOG_LIBRARY_PATH}/libglog.a)

    find_package_handle_standard_args(glog
        REQUIRED_ARGS GLOG_LIBRARY GLOG_LIBRARY_PATH GLOG_INCLUDE_PATH
    )

    add_library(GLOG_LIBRARY STATIC IMPORTED)
    add_dependencies(GLOG_LIBRARY glog)
    mark_as_advanced(GLOG_LIBRARY GLOG_LIBRARY_PATH GLOG_INCLUDE_PATH)

    message(STATUS "Use bundled glog v${GLOG_VERSION}")
endif ()

if (NOT GTEST_FOUND OR USE_BUNDLED_GTEST)
    set (GTEST_VERSION      1.8.0)

    ExternalProject_Add(google-test
        URL                 https://github.com/google/googletest/archive/release-${GTEST_VERSION}.tar.gz
        URL_MD5             16877098823401d1bf2ed7891d7dce36
        CMAKE_ARGS          -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    )

    ExternalProject_Get_Property(google-test INSTALL_DIR)

    set (GTEST_ROOT             ${INSTALL_DIR})
    set (GTEST_INCLUDE_DIRS     ${GTEST_ROOT}/include)
    set (GTEST_LIBRARY_DIRS     ${GTEST_ROOT}/lib)
    set (GTEST_LIBRARIES        ${GTEST_LIBRARY_DIRS}/libgtest.a)
    set (GTEST_MAIN_LIBRARIES   ${GTEST_LIBRARY_DIRS}/libgtest-main.a)
    set (GTEST_BOTH_LIBRARIES   ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES})
    set (GMOCK_LIBRARIES        ${GTEST_LIBRARY_DIRS}/libgmock.a)
    set (GMOCK_MAIN_LIBRARIES   ${GTEST_LIBRARY_DIRS}/libgmock-main.a)
    set (GMOCK_BOTH_LIBRARIES   ${GMOCK_LIBRARIES} ${GMOCK_MAIN_LIBRARIES})

    find_package_handle_standard_args(google-test
        REQUIRED_ARGS GTEST_LIBRARIES GTEST_MAIN_LIBRARIES GMOCK_LIBRARIES GMOCK_MAIN_LIBRARIES GTEST_INCLUDE_DIRS
    )

    foreach (lib GTEST_LIBRARIES GTEST_MAIN_LIBRARIES GMOCK_LIBRARIES GMOCK_MAIN_LIBRARIES)
        add_library(${lib} STATIC IMPORTED)
        add_dependencies(${lib} google-test)
    endforeach ()

    mark_as_advanced(GTEST_LIBRARIES GTEST_MAIN_LIBRARIES GMOCK_LIBRARIES GMOCK_MAIN_LIBRARIES GTEST_INCLUDE_DIRS)

    message(STATUS "Use bundled gtest v${GTEST_VERSION}")
endif ()

if (NOT GBENCH_FOUND OR USE_BUNDLED_GBENCH)
    set (GBENCH_VERSION     1.1.0)

    ExternalProject_Add(google-benchmark
        URL                 https://github.com/google/benchmark/archive/v${GBENCH_VERSION}.tar.gz
        URL_MD5             66b2a23076cf70739525be0092fc3ae3
        CMAKE_ARGS          -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    )

    ExternalProject_Get_Property(google-benchmark INSTALL_DIR)

    set (GBENCH_ROOT_DIR        ${INSTALL_DIR})
    set (GBENCH_INCLUDE_PATH    ${GBENCH_ROOT_DIR}/include)
    set (GBENCH_LIBRARY_PATH    ${GBENCH_ROOT_DIR}/lib)
    set (GBENCH_LIBRARY         ${GBENCH_LIBRARY_PATH}/libbenchmark.a)

    find_package_handle_standard_args(google-benchmark
        REQUIRED_ARGS GBENCH_LIBRARY GBENCH_INCLUDE_PATH
    )

    add_library(GBENCH_LIBRARY STATIC IMPORTED)
    add_dependencies(GBENCH_LIBRARY google-benchmark)
    mark_as_advanced(GBENCH_LIBRARY GBENCH_INCLUDE_PATH)

    message(STATUS "Use bundled google-benchmark v${GBENCH_VERSION}")
endif ()