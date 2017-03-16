include(ExternalProject)

set (EXTERNAL_PROJECT_DIR ${CMAKE_BINARY_DIR}/externals)
set_directory_properties(PROPERTIES EP_PREFIX ${EXTERNAL_PROJECT_DIR})

message(STATUS "Install external dependencies to ${EXTERNAL_PROJECT_DIR}")

include_directories(BEFORE SYSTEM ${EXTERNAL_PROJECT_DIR}/include)
link_directories(${EXTERNAL_PROJECT_DIR}/lib)

macro(install_external_project project_name)
    include(Install${project_name})
endmacro()

if (NOT ZLIB_FOUND OR USE_BUNDLED_ZLIB)
    if (NOT ZLIB_VERSION_STRING OR USE_BUNDLED_ZLIB)
        set (ZLIB_VERSION_STRING        1.2.11)
        set (ZLIB_URL_MD5               1c9f62f0778697a09d36121ead88e08e)
    endif ()

    install_external_project(ZLib)

    message(STATUS "Use bundled zlib v${ZLIB_VERSION_STRING}")
endif ()

if (NOT CURL_FOUND OR USE_BUNDLED_CURL)
    if (NOT CURL_VERSION_STRING OR USE_BUNDLED_CURL)
        set (CURL_VERSION_STRING        7.53.1)
        set (CURL_URL_MD5               9e49bb4cb275bf4464e7b69eb48613c0)
    endif ()

    install_external_project(CURL)

    message(STATUS "Use bundled curl v${CURL_VERSION_STRING}")
endif ()

if (NOT RAPIDJSON_FOUND OR USE_BUNDLED_RAPIDJSON)
    if (NOT RAPIDJSON_VERSION OR USE_BUNDLED_RAPIDJSON)
        set (RAPIDJSON_VERSION          1.1.0)
        set (RAPIDJSON_URL_MD5          badd12c511e081fec6c89c43a7027bce)
    endif ()

    install_external_project(RapidJSON)

    message(STATUS "Use bundled RapidJSON v${RAPIDJSON_VERSION}")
endif ()

if (NOT FOLLY_FOUND OR USE_BUNDLED_FOLLY)
    if (NOT FOLLY_VERSION OR USE_BUNDLED_FOLLY)
        set (FOLLY_VERSION              2017.03.13.00)
        set (FOLLY_URL_MD5              3ba9d455edcf6e930b6f43e93e9f99f7)
    endif()

    install_external_project(Folly)

    message(STATUS "Use bundled folly v${FOLLY_VERSION}")
endif ()

if (NOT LIBRDKAFKA_FOUND OR USE_BUNDLED_LIBRDKAFKA)
    if (NOT LibRDKafka_VERSION OR USE_BUNDLED_LIBRDKAFKA)
        set (LibRDKafka_VERSION         0.9.4)
        set (LibRDKafka_URL_MD5         6f40198e6068475c34ae8c9faafa6e8a)
    endif ()

    install_external_project(LibRDKafka)

    message(STATUS "Use bundled librdkafka v${LibRDKafka_VERSION}")
endif ()

if (NOT DOUBLE_CONVERSION_FOUND OR USE_BUNDLED_DOUBLE_CONVERSION)
    if (NOT DOUBLE_CONVERSION_VERSION OR USE_BUNDLED_DOUBLE_CONVERSION)
        set (DOUBLE_CONVERSION_VERSION      1.1.5)
        set (DOUBLE_CONVERSION_URL_MD5      f7c62594d7ecfbc4421da32bc341a919)
    endif ()

    install_external_project(DoubleConversion)

    message(STATUS "Use bundled double_conversion v${DOUBLE_CONVERSION_VERSION}")
endif ()

if (NOT THRIFT_FOUND OR USE_BUNDLED_THRIFT)
    if (NOT THRIFT_VERSION_STRING OR USE_BUNDLED_THRIFT)
        set (THRIFT_VERSION_STRING        0.10.0)
        set (THRIFT_URL_MD5               795c5dd192e310ffff38cfd9430d6b29)
    endif ()

    install_external_project(Thrift)

    message(STATUS "Use bundled thrift v${THRIFT_VERSION_STRING}")
endif ()

if (NOT GFLAGS_FOUND OR USE_BUNDLED_GFLAGS)
    if (NOT GFLAGS_VERSION OR USE_BUNDLED_GFLAGS)
        set (GFLAGS_VERSION     2.2.0)
        set (GFLAGS_URL_MD5     b99048d9ab82d8c56e876fb1456c285e)
    endif ()

    install_external_project(GFlags)

    message(STATUS "Use bundled gflags v${GFLAGS_VERSION}")
endif ()

if (NOT GLOG_FOUND OR USE_BUNDLED_GLOG)
    if (NOT GLOG_VERSION OR USE_BUNDLED_GLOG)
        set (GLOG_VERSION       0.3.4)
        set (GLOG_URL_MD5       df92e05c9d02504fb96674bc776a41cb)
    endif ()

    install_external_project(GLog)

    message(STATUS "Use bundled glog v${GLOG_VERSION}")
endif ()

if (NOT GTEST_FOUND OR USE_BUNDLED_GTEST)
    if (NOT GTEST_VERSION OR USE_BUNDLED_GTEST)
        set (GTEST_VERSION      1.8.0)
        set (GTEST_URL_MD5      16877098823401d1bf2ed7891d7dce36)
    endif ()

    install_external_project(GTest)

    message(STATUS "Use bundled gtest v${GTEST_VERSION}")
endif ()

if (NOT GBENCH_FOUND OR USE_BUNDLED_GBENCH)
    if (NOT GBENCH_VERSION OR USE_BUNDLED_GBENCH)
        set (GBENCH_VERSION     1.1.0)
        set (GBENCH_URL_MD5     66b2a23076cf70739525be0092fc3ae3)
    endif ()

    install_external_project(GBench)

    message(STATUS "Use bundled google-benchmark v${GBENCH_VERSION}")
endif ()